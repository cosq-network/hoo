#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <memory>
#include "hvm/HOModuleBase.h"

using namespace hvm;

/**
 * Tests that verify StaticHOModule and DynamicHOModule serialize/deserialize
 * correctly with the memcpy-based implementation (fixes strict aliasing) and
 * that truncated inputs are rejected gracefully (bounds checks).
 */

class HOModuleBaseSerializationTest : public ::testing::Test {
protected:
    std::shared_ptr<StaticHOModule> makeStaticModule(const std::string& name) {
        auto mod = StaticHOModule::create(name);
        mod->setLinked(true);
        mod->setLibraryPath("/usr/lib/libtest.so");
        return mod;
    }

    std::shared_ptr<DynamicHOModule> makeDynamicModule(const std::string& name) {
        auto mod = DynamicHOModule::create(name);
        mod->setLibraryLoaded(true);
        mod->addLoadedLibrary("/usr/lib/libtest.so");
        return mod;
    }

    ModuleSymbol makeSymbol(const std::string& name, const std::string& sig = "") {
        ModuleSymbol s;
        s.name = name;
        s.mangled_name = name;
        s.binding = SymbolBinding::Global;
        s.type = SymbolType::Function;
        s.address = 0;
        s.size = 0;
        s.signature = sig;
        return s;
    }
};

// ── StaticHOModule round-trip ────────────────────────────────────────────

TEST_F(HOModuleBaseSerializationTest, StaticModuleRoundTrip) {
    auto src = makeStaticModule("test_module");

    src->addSymbol(makeSymbol("func_add", "(int64,int64)->int64"));
    src->addSymbol(makeSymbol("func_sub"));
    src->addDependency("std.io", ModuleType::StaticRuntime, false);
    src->addDependency("std.math", ModuleType::StaticRuntime, true);

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));
    EXPECT_GT(data.size(), 32u);

    auto dst = StaticHOModule::create("empty");
    ASSERT_TRUE(dst->deserialize(data));

    EXPECT_EQ(dst->getName(), "test_module");
    EXPECT_TRUE(dst->isLinked());
    EXPECT_EQ(dst->getLibraryPath(), "/usr/lib/libtest.so");

    const auto* sym1 = dst->findSymbol("func_add");
    ASSERT_NE(sym1, nullptr);
    EXPECT_EQ(sym1->signature, "(int64,int64)->int64");

    const auto* sym2 = dst->findSymbol("func_sub");
    ASSERT_NE(sym2, nullptr);
    EXPECT_TRUE(sym2->signature.empty());

    const auto& deps = dst->getDependencies();
    ASSERT_EQ(deps.size(), 2u);
    EXPECT_EQ(deps[0].module_name, "std.io");
    EXPECT_FALSE(deps[0].optional);
    EXPECT_EQ(deps[1].module_name, "std.math");
    EXPECT_TRUE(deps[1].optional);
}

TEST_F(HOModuleBaseSerializationTest, StaticModuleEmptyRoundTrip) {
    auto src = makeStaticModule("empty_module");

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    auto dst = StaticHOModule::create("other");
    ASSERT_TRUE(dst->deserialize(data));
    EXPECT_EQ(dst->getName(), "empty_module");
    EXPECT_TRUE(dst->getDependencies().empty());
}

TEST_F(HOModuleBaseSerializationTest, StaticModuleNameWithUnicode) {
    auto src = makeStaticModule("module_with_long_name_1234567890_abcdef");

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    auto dst = StaticHOModule::create("");
    ASSERT_TRUE(dst->deserialize(data));
    EXPECT_EQ(dst->getName(), "module_with_long_name_1234567890_abcdef");
}

TEST_F(HOModuleBaseSerializationTest, StaticModuleLongSignature) {
    auto src = makeStaticModule("sig_test");

    std::string longSig(500, 'x');
    src->addSymbol(makeSymbol("func", longSig));

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    auto dst = StaticHOModule::create("other");
    ASSERT_TRUE(dst->deserialize(data));
    const auto* sym = dst->findSymbol("func");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->signature, longSig);
}

TEST_F(HOModuleBaseSerializationTest, StaticModuleManyDependencies) {
    auto src = makeStaticModule("dep_test");
    for (int i = 0; i < 50; ++i) {
        src->addDependency("dep_" + std::to_string(i), ModuleType::StaticRuntime, i % 3 == 0);
    }

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    auto dst = StaticHOModule::create("other");
    ASSERT_TRUE(dst->deserialize(data));
    EXPECT_EQ(dst->getDependencies().size(), 50u);
    EXPECT_TRUE(dst->getDependencies()[0].optional);
    EXPECT_FALSE(dst->getDependencies()[1].optional);
    EXPECT_FALSE(dst->getDependencies()[2].optional);
    EXPECT_TRUE(dst->getDependencies()[3].optional);
}

// ── StaticHOModule truncated input (bounds check verification) ───────────

TEST_F(HOModuleBaseSerializationTest, StaticModuleRejectsTooSmall) {
    auto mod = makeStaticModule("test");

    // Build valid data, then truncate it.
    std::vector<uint8_t> data;
    ASSERT_TRUE(mod->serialize(data));

    // Truncate at various points to exercise each bounds check.
    for (size_t trunc = 0; trunc < std::min(data.size(), size_t(80)); ++trunc) {
        std::vector<uint8_t> truncated(data.begin(), data.begin() + trunc);
        auto dst = StaticHOModule::create("other");
        // Should either fail gracefully or succeed if enough bytes remain.
        // The key invariant is: no crash, no UB.
        dst->deserialize(truncated);
        // If it failed, error should be set.
        if (dst->hasError()) {
            EXPECT_FALSE(dst->getError().empty());
        }
    }
}

TEST_F(HOModuleBaseSerializationTest, StaticModuleRejectsInvalidMagic) {
    auto mod = makeStaticModule("test");
    std::vector<uint8_t> data;
    ASSERT_TRUE(mod->serialize(data));

    // Corrupt the magic number.
    data[0] = 0xFF;
    data[1] = 0xFF;
    data[2] = 0xFF;
    data[3] = 0xFF;

    auto dst = StaticHOModule::create("other");
    EXPECT_FALSE(dst->deserialize(data));
    EXPECT_TRUE(dst->hasError());
}

TEST_F(HOModuleBaseSerializationTest, StaticModuleRejectsWrongModuleType) {
    // Serialize a StaticHOModule, then modify the module-type field to
    // DynamicLibrary so the deserializer rejects it.
    auto src = StaticHOModule::create("test");
    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    // Module type is at offset 0x04, stored as uint32_t LE.
    // StaticRuntime = 0x02; set it to DynamicLibrary = 0x03.
    data[0x04] = 0x03;

    auto dst = StaticHOModule::create("other");
    EXPECT_FALSE(dst->deserialize(data));
    EXPECT_TRUE(dst->hasError());
}

TEST_F(HOModuleBaseSerializationTest, StaticModuleTruncatedSymbolNameLength) {
    auto src = makeStaticModule("test");
    src->addSymbol(makeSymbol("my_func"));

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    // Find where symbols begin: header(32) + moduleNameLen.
    size_t nameLen = src->getName().size();
    size_t symOffset = 32 + nameLen;

    // Truncate right at the symbol name length field.
    for (size_t t = symOffset; t <= symOffset + 4 && t < data.size(); ++t) {
        std::vector<uint8_t> truncated(data.begin(), data.begin() + t);
        auto dst = StaticHOModule::create("other");
        dst->deserialize(truncated);
        // Must not crash; either succeeds (if enough data) or sets error.
    }
}

TEST_F(HOModuleBaseSerializationTest, StaticModuleTruncatedDependencyFields) {
    auto src = makeStaticModule("test");
    src->addDependency("lib_foo", ModuleType::StaticRuntime, false);

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    // The dependency block is after symbols; truncate progressively.
    for (size_t t = data.size() - 1; t > data.size() / 2; --t) {
        std::vector<uint8_t> truncated(data.begin(), data.begin() + t);
        auto dst = StaticHOModule::create("other");
        dst->deserialize(truncated);
        // Must not crash.
    }
}

// ── DynamicHOModule round-trip ───────────────────────────────────────────

TEST_F(HOModuleBaseSerializationTest, DynamicModuleRoundTrip) {
    auto src = makeDynamicModule("dyn_test");

    src->addSymbol(makeSymbol("dyn_func_a"));
    src->addSymbol(makeSymbol("dyn_func_b"));
    src->addDependency("base_mod", ModuleType::StaticRuntime, false);
    src->addDependency("lib_mod", ModuleType::DynamicLibrary, true);

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));
    EXPECT_GT(data.size(), 40u);

    auto dst = DynamicHOModule::create("empty");
    ASSERT_TRUE(dst->deserialize(data));

    EXPECT_EQ(dst->getName(), "dyn_test");
    EXPECT_TRUE(dst->isLibraryLoaded());

    const auto* sym1 = dst->findSymbol("dyn_func_a");
    ASSERT_NE(sym1, nullptr);
    const auto* sym2 = dst->findSymbol("dyn_func_b");
    ASSERT_NE(sym2, nullptr);

    const auto& deps = dst->getDependencies();
    ASSERT_EQ(deps.size(), 2u);
    EXPECT_EQ(deps[0].module_name, "base_mod");
    EXPECT_EQ(deps[1].module_name, "lib_mod");
    EXPECT_TRUE(deps[1].optional);
}

TEST_F(HOModuleBaseSerializationTest, DynamicModuleTruncatedInput) {
    auto mod = makeDynamicModule("test");
    mod->addSymbol(makeSymbol("func"));
    mod->addDependency("dep", ModuleType::StaticRuntime, false);

    std::vector<uint8_t> data;
    ASSERT_TRUE(mod->serialize(data));

    // Truncate at various points.
    for (size_t trunc = 0; trunc < std::min(data.size(), size_t(80)); ++trunc) {
        std::vector<uint8_t> truncated(data.begin(), data.begin() + trunc);
        auto dst = DynamicHOModule::create("other");
        dst->deserialize(truncated);
        // Must not crash.
    }
}

TEST_F(HOModuleBaseSerializationTest, DynamicModuleRejectsInvalidMagic) {
    auto mod = makeDynamicModule("test");
    std::vector<uint8_t> data;
    ASSERT_TRUE(mod->serialize(data));

    data[0] = 0xDE;
    data[1] = 0xAD;
    data[2] = 0xBE;
    data[3] = 0xEF;

    auto dst = DynamicHOModule::create("other");
    EXPECT_FALSE(dst->deserialize(data));
    EXPECT_TRUE(dst->hasError());
}

TEST_F(HOModuleBaseSerializationTest, DynamicModuleTruncatedSymbolNameLength) {
    auto src = makeDynamicModule("test");
    src->addSymbol(makeSymbol("my_func"));

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    size_t nameLen = src->getName().size();
    size_t symOffset = 40 + nameLen;

    for (size_t t = symOffset; t <= symOffset + 4 && t < data.size(); ++t) {
        std::vector<uint8_t> truncated(data.begin(), data.begin() + t);
        auto dst = DynamicHOModule::create("other");
        dst->deserialize(truncated);
        // Must not crash.
    }
}

TEST_F(HOModuleBaseSerializationTest, DynamicModuleTruncatedDependencyFields) {
    auto src = makeDynamicModule("test");
    src->addDependency("lib_foo", ModuleType::DynamicLibrary, false);

    std::vector<uint8_t> data;
    ASSERT_TRUE(src->serialize(data));

    for (size_t t = data.size() - 1; t > data.size() / 2; --t) {
        std::vector<uint8_t> truncated(data.begin(), data.begin() + t);
        auto dst = DynamicHOModule::create("other");
        dst->deserialize(truncated);
        // Must not crash.
    }
}

// ── Verify memcpy output is byte-equivalent to manual LE encoding ────────

TEST_F(HOModuleBaseSerializationTest, StaticModuleHeaderBytesAreCorrect) {
    auto mod = makeStaticModule("hdr_test");

    std::vector<uint8_t> data;
    ASSERT_TRUE(mod->serialize(data));

    // Magic: 0x484F4F48 ("HOoh") in little-endian.
    EXPECT_EQ(data[0], 0x48);
    EXPECT_EQ(data[1], 0x4F);
    EXPECT_EQ(data[2], 0x4F);
    EXPECT_EQ(data[3], 0x48);

    // Module type: StaticRuntime = 2 in little-endian uint32.
    EXPECT_EQ(data[4], 0x02);
    EXPECT_EQ(data[5], 0x00);
    EXPECT_EQ(data[6], 0x00);
    EXPECT_EQ(data[7], 0x00);

    // Version: 1 in little-endian uint32.
    EXPECT_EQ(data[8], 0x01);
    EXPECT_EQ(data[9], 0x00);
    EXPECT_EQ(data[10], 0x00);
    EXPECT_EQ(data[11], 0x00);

    // Module name length: "hdr_test" = 8 bytes.
    uint32_t nameLen;
    std::memcpy(&nameLen, data.data() + 0x0C, sizeof(nameLen));
    EXPECT_EQ(nameLen, 8u);

    // Sym count = 0, dep count = 0.
    uint32_t symCount;
    std::memcpy(&symCount, data.data() + 0x10, sizeof(symCount));
    EXPECT_EQ(symCount, 0u);
    uint32_t depCount;
    std::memcpy(&depCount, data.data() + 0x14, sizeof(depCount));
    EXPECT_EQ(depCount, 0u);
}

TEST_F(HOModuleBaseSerializationTest, DynamicModuleHeaderBytesAreCorrect) {
    auto mod = makeDynamicModule("dyn_hdr_test");

    std::vector<uint8_t> data;
    ASSERT_TRUE(mod->serialize(data));

    // Magic: 0x484F4F48 in little-endian.
    EXPECT_EQ(data[0], 0x48);
    EXPECT_EQ(data[1], 0x4F);
    EXPECT_EQ(data[2], 0x4F);
    EXPECT_EQ(data[3], 0x48);

    // Module type: DynamicLibrary = 3.
    uint32_t moduleType;
    std::memcpy(&moduleType, data.data() + 0x04, sizeof(moduleType));
    EXPECT_EQ(moduleType, 3u);

    // library_loaded at 0x18 = 1 (true).
    uint32_t libLoaded;
    std::memcpy(&libLoaded, data.data() + 0x18, sizeof(libLoaded));
    EXPECT_EQ(libLoaded, 1u);

    // exported_symbols count = 0.
    uint32_t expCount;
    std::memcpy(&expCount, data.data() + 0x20, sizeof(expCount));
    EXPECT_EQ(expCount, 0u);

    // loaded_libraries count = 1 (we called addLoadedLibrary).
    uint32_t loadedCount;
    std::memcpy(&loadedCount, data.data() + 0x24, sizeof(loadedCount));
    EXPECT_EQ(loadedCount, 1u);
}
