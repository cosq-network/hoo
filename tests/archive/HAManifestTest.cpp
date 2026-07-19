#include <gtest/gtest.h>
#include "archive/HAManifest.h"

using namespace hooc::archive;

// ============================================================================
// SHA-256 utility tests
// ============================================================================

TEST(SHA256Test, EmptyInput) {
    std::string hash = computeSha256(nullptr, 0);
    // SHA-256 of empty input is a well-known constant
    EXPECT_EQ(hash, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(SHA256Test, KnownInput) {
    // SHA-256 of "abc" is well-known
    const uint8_t data[] = {'a', 'b', 'c'};
    std::string hash = computeSha256(data, 3);
    EXPECT_EQ(hash, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(SHA256Test, VectorOverload) {
    std::vector<uint8_t> data = {'a', 'b', 'c'};
    std::string hash = computeSha256(data);
    EXPECT_EQ(hash, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(SHA256Test, Deterministic) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    std::string hash1 = computeSha256(data);
    std::string hash2 = computeSha256(data);
    EXPECT_EQ(hash1, hash2);
}

TEST(SHA256Test, DifferentInputsDifferentHashes) {
    std::vector<uint8_t> data1 = {0x01, 0x02};
    std::vector<uint8_t> data2 = {0x01, 0x03};
    EXPECT_NE(computeSha256(data1), computeSha256(data2));
}

// ============================================================================
// Manifest serialization round-trip tests
// ============================================================================

TEST(HAManifestTest, SerializationRoundTrip) {
    HAManifest manifest;
    
    HAModuleInfo m1;
    m1.moduleName = "a";
    m1.sourcePath = "a.hoo";
    m1.archivePath = "modules/a.ho";
    m1.imports = {"b"};
    
    HASymbolInfo s1;
    s1.name = "main";
    s1.mangled = "_F_M_a_E_main_v";
    s1.kind = "function";
    s1.returnType = "int64";
    s1.visibility = "public";
    
    m1.symbols.push_back(s1);
    
    manifest.modules.push_back(m1);
    manifest.moduleIndex["a"] = "modules/a.ho";
    manifest.symbolIndex["_F_M_a_E_main_v"] = "modules/a.ho";
    
    HAEntryPoint ep;
    ep.source = "a.hoo";
    ep.moduleName = "a";
    ep.symbol = "_F_M_a_E_main_v";
    manifest.entryPoint = ep;
    
    std::string jsonStr = manifest.toJson();
    EXPECT_FALSE(jsonStr.empty());
    
    HAManifest parsed = HAManifest::fromJson(jsonStr);
    
    EXPECT_EQ(parsed.format, "hoo-archive");
    EXPECT_EQ(parsed.formatVersion, 1);
    EXPECT_EQ(parsed.createdBy.tool, "hoo");
    EXPECT_TRUE(parsed.entryPoint.has_value());
    EXPECT_EQ(parsed.entryPoint->moduleName, "a");
    EXPECT_EQ(parsed.entryPoint->symbol, "_F_M_a_E_main_v");
    
    ASSERT_EQ(parsed.modules.size(), 1);
    EXPECT_EQ(parsed.modules[0].moduleName, "a");
    EXPECT_EQ(parsed.modules[0].sourcePath, "a.hoo");
    EXPECT_EQ(parsed.modules[0].archivePath, "modules/a.ho");
    EXPECT_EQ(parsed.modules[0].imports.size(), 1);
    EXPECT_EQ(parsed.modules[0].imports[0], "b");
    
    ASSERT_EQ(parsed.modules[0].symbols.size(), 1);
    EXPECT_EQ(parsed.modules[0].symbols[0].name, "main");
    EXPECT_EQ(parsed.modules[0].symbols[0].mangled, "_F_M_a_E_main_v");
    EXPECT_EQ(parsed.modules[0].symbols[0].kind, "function");
    
    EXPECT_EQ(parsed.moduleIndex.at("a"), "modules/a.ho");
    EXPECT_EQ(parsed.symbolIndex.at("_F_M_a_E_main_v"), "modules/a.ho");
}

TEST(HAManifestTest, Sha256RoundTrip) {
    HAManifest manifest;
    
    HAModuleInfo m1;
    m1.moduleName = "mymod";
    m1.sourcePath = "src/mymod.hoo";
    m1.archivePath = "modules/mymod.ho";
    m1.sha256 = "abc123def456";
    m1.imports = {};
    
    manifest.modules.push_back(m1);
    
    std::string jsonStr = manifest.toJson();
    HAManifest parsed = HAManifest::fromJson(jsonStr);
    
    ASSERT_EQ(parsed.modules.size(), 1);
    EXPECT_EQ(parsed.modules[0].sha256, "abc123def456");
}

TEST(HAManifestTest, EmptyManifest) {
    HAManifest manifest;
    std::string jsonStr = manifest.toJson();
    HAManifest parsed = HAManifest::fromJson(jsonStr);
    
    EXPECT_EQ(parsed.format, "hoo-archive");
    EXPECT_EQ(parsed.formatVersion, 1);
    EXPECT_TRUE(parsed.modules.empty());
    EXPECT_TRUE(parsed.entryPoint.has_value() == false);
}

TEST(HAManifestTest, InvalidFormatThrows) {
    EXPECT_THROW(HAManifest::fromJson(R"({"format":"wrong","formatVersion":1})"), std::runtime_error);
}

TEST(HAManifestTest, MalformedJsonThrows) {
    EXPECT_THROW(HAManifest::fromJson("{not json}"), std::runtime_error);
}

TEST(HAManifestTest, MultipleModules) {
    HAManifest manifest;
    
    HAModuleInfo m1;
    m1.moduleName = "a";
    m1.sourcePath = "a.hoo";
    m1.archivePath = "modules/a.ho";
    m1.sha256 = "hash_a";
    m1.imports = {"b"};
    
    HAModuleInfo m2;
    m2.moduleName = "b";
    m2.sourcePath = "b.hoo";
    m2.archivePath = "modules/b.ho";
    m2.sha256 = "hash_b";
    
    manifest.modules.push_back(m1);
    manifest.modules.push_back(m2);
    manifest.moduleIndex["a"] = "modules/a.ho";
    manifest.moduleIndex["b"] = "modules/b.ho";
    
    std::string jsonStr = manifest.toJson();
    HAManifest parsed = HAManifest::fromJson(jsonStr);
    
    ASSERT_EQ(parsed.modules.size(), 2);
    EXPECT_EQ(parsed.modules[0].sha256, "hash_a");
    EXPECT_EQ(parsed.modules[1].sha256, "hash_b");
    EXPECT_EQ(parsed.moduleIndex.size(), 2);
}

TEST(HAManifestTest, EntryPointOmittedWhenAbsent) {
    HAManifest manifest;
    manifest.entryPoint = std::nullopt;
    
    std::string jsonStr = manifest.toJson();
    HAManifest parsed = HAManifest::fromJson(jsonStr);
    
    EXPECT_FALSE(parsed.entryPoint.has_value());
}
