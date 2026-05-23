#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/IOProvider.h"
#include "hvm/HOModule.h"
#include "hvm/HVMInstruction.h"
#include "hvm/HVMJIT.h"

using namespace hvm;
using namespace hooc;

namespace {

class InMemoryIOProvider final : public IOProvider {
public:
    std::optional<std::string> readFile(const std::string& filename) override {
        auto it = textFiles.find(filename);
        if (it == textFiles.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    bool writeFile(const std::string& filename, const std::string& content) override {
        textFiles[filename] = content;
        return true;
    }

    std::optional<std::vector<uint8_t>> readBinaryFile(const std::string& filename) override {
        auto it = binaryFiles.find(filename);
        if (it == binaryFiles.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    bool writeBinaryFile(const std::string& filename, const std::vector<uint8_t>& data) override {
        binaryFiles[filename] = data;
        return true;
    }

    std::string readStdin() override { return stdinContent; }
    void writeStdout(const std::string& output) override { stdoutContent += output; }
    void writeStderr(const std::string& output) override { stderrContent += output; }

    std::map<std::string, std::string> textFiles;
    std::map<std::string, std::vector<uint8_t>> binaryFiles;
    std::string stdinContent;
    std::string stdoutContent;
    std::string stderrContent;
};

HVMInstruction makeI(Opcode op, OperandsI ops) {
    HVMInstruction ins(op, ops);
    ins.setFormat(InstructionFormat::I);
    return ins;
}

HVMInstruction makeR(Opcode op, OperandsR ops) {
    HVMInstruction ins(op, ops);
    ins.setFormat(InstructionFormat::R);
    return ins;
}

std::vector<uint8_t> buildModuleBytes(
    const std::string& moduleName,
    const std::vector<HVMInstruction>& instructions,
    const std::vector<Symbol>& symbols,
    const std::vector<ImportEntry>& imports = {}) {
    auto module = HOModule::create(moduleName);
    module->setName(moduleName);

    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    text.data = module->encodeInstructions(instructions);
    text.virtual_size = text.data.size();
    text.alignment = 16;
    module->addSection(std::move(text));

    for (const auto& s : symbols) {
        module->addSymbol(s);
    }
    for (const auto& i : imports) {
        module->addImport(i);
    }

    std::vector<uint8_t> bytes;
    EXPECT_TRUE(module->serialize(bytes));
    return bytes;
}

Symbol funcSym(const std::string& name, uint64_t offset) {
    Symbol s{};
    s.name = name;
    s.binding = Symbol::STB_GLOBAL;
    s.type = Symbol::STT_FUNC;
    s.visibility = Symbol::STV_DEFAULT;
    s.value = offset;
    s.size = 0;
    s.section_index = 0;
    s.symbol_index = 0;
    return s;
}

ImportEntry importFrom(const std::string& symbol, const std::string& library) {
    ImportEntry i{};
    i.name = symbol;
    i.library = library;
    i.import_type = ImportEntry::IT_HOOC;
    i.version = 1;
    i.flags = 0;
    i.resolved_address = 0;
    return i;
}

} // namespace

class HVMJITLoaderTest : public ::testing::Test {
protected:
    InMemoryIOProvider io;
};

TEST_F(HVMJITLoaderTest, LoadInputFailsWhenModuleIsMissing) {
    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("missing.ho"));
    EXPECT_TRUE(jit.hasError());
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Parse);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::IoReadFailed);
}

TEST_F(HVMJITLoaderTest, LoadBytecodeAndRunSimpleMainReturnsExpectedValue) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 42}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };

    io.binaryFiles["simple.ho"] = buildModuleBytes("simple", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("simple.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 42) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, LoadInputWithModuleNameResolvesDottedPath) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 7}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };

    io.binaryFiles["app/core.ho"] = buildModuleBytes("app.core", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("app.core")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 7) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, DependenciesAreLoadedFromImports) {
    std::vector<HVMInstruction> depIns{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> depSyms{
        funcSym("_F_module_init_v", 0),
        funcSym("_F_dummy_v", 0),
    };
    io.binaryFiles["dep/mod.ho"] = buildModuleBytes("dep.mod", depIns, depSyms);

    std::vector<HVMInstruction> mainIns{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 5}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> mainSyms{
        funcSym("_F_main_v", 0),
    };
    std::vector<ImportEntry> imports{
        importFrom("_F_dummy_v", "dep.mod"),
    };
    io.binaryFiles["main.ho"] = buildModuleBytes("main", mainIns, mainSyms, imports);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("main.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 5) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, ModuleInitIsInvokedBeforeMain) {
    std::vector<HVMInstruction> ins{
        makeR(Opcode::BREAK, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_module_init_v", 0),
        funcSym("_F_main_v", 4),
    };
    io.binaryFiles["initfail.ho"] = buildModuleBytes("initfail", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("initfail.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Module init failed"), std::string::npos);
}

TEST_F(HVMJITLoaderTest, CircularDependenciesAreRejected) {
    std::vector<HVMInstruction> mainIns{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> mainSyms{
        funcSym("_F_main_v", 0),
    };

    std::vector<ImportEntry> aImports{importFrom("_F_any_v", "b.mod")};
    std::vector<ImportEntry> bImports{importFrom("_F_any_v", "a.mod")};
    io.binaryFiles["a/mod.ho"] = buildModuleBytes("a.mod", mainIns, mainSyms, aImports);
    io.binaryFiles["b/mod.ho"] = buildModuleBytes("b.mod", mainIns, mainSyms, bImports);

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("a/mod.ho"));
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Circular dependency"), std::string::npos);
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Resolve);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::CircularDependency);
}

TEST_F(HVMJITLoaderTest, LoadSourcePathUsesCompilerPipeline) {
    io.textFiles["sample.hoo"] = "func main() { return 3; }";

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sample.hoo")) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, GetSymbolAddressForMissingSymbolReturnsNull) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 9}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["sym.ho"] = buildModuleBytes("sym", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sym.ho")) << jit.getLastError();
    EXPECT_EQ(jit.getSymbolAddress("_F_missing_v"), nullptr);
    EXPECT_TRUE(jit.hasError());
}

TEST_F(HVMJITLoaderTest, RuntimeBootstrapGatePassesWhenIntrinsicsExist) {
    std::vector<HVMInstruction> ins{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["runtimecheck.ho"] = buildModuleBytes("runtimecheck", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("runtimecheck.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
    EXPECT_NE(jit.getSymbolAddress("hoo_alloc"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_retain"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_release"), nullptr);
}

TEST_F(HVMJITLoaderTest, RuntimeBridgeExportsExtendedHoortSymbols) {
    std::vector<HVMInstruction> ins{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["runtimeext.ho"] = buildModuleBytes("runtimeext", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("runtimeext.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();

    EXPECT_NE(jit.getSymbolAddress("hoo_get_refcount"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_get_type_id"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_string_from_cstr"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_string_concat"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_string_length"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_array_new"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_array_push_int64"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_array_get_int64"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_map_new"), nullptr);
}

TEST_F(HVMJITLoaderTest, CreatesPerModuleJITDylibsAndLogicalSearchOrder) {
    std::vector<HVMInstruction> depIns{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> depSyms{
        funcSym("_F_module_init_v", 0),
        funcSym("_F_dummy_v", 0),
    };
    io.binaryFiles["dep/mod.ho"] = buildModuleBytes("dep.mod", depIns, depSyms);

    std::vector<HVMInstruction> mainIns{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 5}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> mainSyms{
        funcSym("_F_main_v", 0),
    };
    std::vector<ImportEntry> imports{
        importFrom("_F_dummy_v", "dep.mod"),
    };
    io.binaryFiles["main2.ho"] = buildModuleBytes("main2", mainIns, mainSyms, imports);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("main2.ho")) << jit.getLastError();
    EXPECT_TRUE(jit.hasModuleJITDylib("main2"));
    EXPECT_TRUE(jit.hasModuleJITDylib("dep.mod"));
    EXPECT_TRUE(jit.hasModuleJITDylib("hoo"));

    auto order = jit.getModuleLogicalSearchOrder("main2");
    ASSERT_FALSE(order.empty());
    EXPECT_EQ(order.front(), "main2");
    EXPECT_NE(std::find(order.begin(), order.end(), "dep.mod"), order.end());
    EXPECT_NE(std::find(order.begin(), order.end(), "hoo"), order.end());
    EXPECT_EQ(order.back(), "__process__");
}

TEST_F(HVMJITLoaderTest, FailedDependencyLoadRollsBackNewlyLoadedModules) {
    std::vector<HVMInstruction> depIns{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> depSyms{
        funcSym("_F_module_init_v", 0),
        funcSym("_F_dummy_v", 0),
    };
    io.binaryFiles["dep/good.ho"] = buildModuleBytes("dep.good", depIns, depSyms);

    std::vector<HVMInstruction> okIns{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 2}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> okSyms{
        funcSym("_F_main_v", 0),
    };
    std::vector<ImportEntry> okImports{
        importFrom("_F_dummy_v", "dep.good"),
    };
    io.binaryFiles["baseline.ho"] = buildModuleBytes("baseline", okIns, okSyms, okImports);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("baseline.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 2) << jit.getLastError();

    // This module introduces an unresolved dependency and should fail.
    std::vector<HVMInstruction> badIns{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> badSyms{
        funcSym("_F_main_v", 0),
    };
    std::vector<ImportEntry> badImports{
        importFrom("_F_unknown_v", "dep.missing"),
    };
    io.binaryFiles["badload.ho"] = buildModuleBytes("badload", badIns, badSyms, badImports);

    EXPECT_FALSE(jit.loadInput("badload.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Resolve);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::MissingDependency);

    // Baseline module should still be intact and runnable after rollback.
    EXPECT_EQ(jit.run("_F_main_v"), 2) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, SearchOrderForModuleWithoutDependenciesIncludesRuntimeAndProcess) {
    std::vector<HVMInstruction> ins{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["nodeps.ho"] = buildModuleBytes("nodeps", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("nodeps.ho")) << jit.getLastError();

    auto order = jit.getModuleLogicalSearchOrder("nodeps");
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], "nodeps");
    EXPECT_EQ(order[1], "hoo");
    EXPECT_EQ(order[2], "__process__");
}

TEST_F(HVMJITLoaderTest, ValidationRejectsInvalidPointerSize) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    auto module = HOModule::create("badptr");
    module->setPointerSize(4);
    module->setName("badptr");
    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    text.data = module->encodeInstructions(ins);
    text.virtual_size = text.data.size();
    module->addSection(std::move(text));
    module->addSymbol(funcSym("_F_main_v", 0));
    std::vector<uint8_t> bytes;
    ASSERT_TRUE(module->serialize(bytes));
    io.binaryFiles["badptr.ho"] = bytes;

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("badptr.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Validate);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::InvalidHeader);
}

TEST_F(HVMJITLoaderTest, ValidationRejectsMissingTextSection) {
    auto module = HOModule::create("notext");
    module->setName("notext");
    module->addSymbol(funcSym("_F_main_v", 0));
    std::vector<uint8_t> bytes;
    ASSERT_TRUE(module->serialize(bytes));
    io.binaryFiles["notext.ho"] = bytes;

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("notext.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Validate);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::InvalidSection);
}

TEST_F(HVMJITLoaderTest, ValidationRejectsFunctionOffsetOutOfBounds) {
    std::vector<HVMInstruction> ins{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    auto module = HOModule::create("badoffset");
    module->setName("badoffset");
    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    text.data = module->encodeInstructions(ins);
    text.virtual_size = text.data.size();
    module->addSection(std::move(text));
    module->addSymbol(funcSym("_F_main_v", 1024)); // invalid offset
    std::vector<uint8_t> bytes;
    ASSERT_TRUE(module->serialize(bytes));
    io.binaryFiles["badoffset.ho"] = bytes;

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("badoffset.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Validate);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::InvalidSymbol);
}

TEST_F(HVMJITLoaderTest, ModuleNameCollisionAcrossDifferentPathsIsRejected) {
    std::vector<HVMInstruction> ins{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["a/same.ho"] = buildModuleBytes("same.mod", ins, syms);
    io.binaryFiles["b/same.ho"] = buildModuleBytes("same.mod", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("a/same.ho")) << jit.getLastError();
    EXPECT_FALSE(jit.loadInput("b/same.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Resolve);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::InvalidMetadata);
}

TEST_F(HVMJITLoaderTest, ValidationRejectsTextSectionWithoutExecuteFlag) {
    auto module = HOModule::create("badflags");
    module->setName("badflags");
    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.flags = SectionFlags::ALLOC; // Missing EXECUTE
    text.data = {0x00, 0x00, 0x00, 0x00};
    text.virtual_size = text.data.size();
    module->addSection(std::move(text));
    module->addSymbol(funcSym("_F_main_v", 0));
    std::vector<uint8_t> bytes;
    ASSERT_TRUE(module->serialize(bytes));
    io.binaryFiles["badflags.ho"] = bytes;

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("badflags.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Validate);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::InvalidSection);
}

TEST_F(HVMJITLoaderTest, ValidationRejectsWritableRodataSection) {
    auto module = HOModule::create("badrodata");
    module->setName("badrodata");

    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    text.data = {0x00, 0x00, 0x00, 0x00};
    text.virtual_size = text.data.size();
    module->addSection(std::move(text));

    Section rodata;
    rodata.name = ".rodata";
    rodata.type = SectionType::SHT_RODATA;
    rodata.flags = SectionFlags::ALLOC | SectionFlags::WRITE; // invalid
    rodata.data = {0x41, 0x42, 0x43, 0x00};
    rodata.virtual_size = rodata.data.size();
    module->addSection(std::move(rodata));

    module->addSymbol(funcSym("_F_main_v", 0));
    std::vector<uint8_t> bytes;
    ASSERT_TRUE(module->serialize(bytes));
    io.binaryFiles["badrodata.ho"] = bytes;

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("badrodata.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Validate);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::InvalidSection);
}

TEST_F(HVMJITLoaderTest, ValidationRejectsNonWritableDataSection) {
    auto module = HOModule::create("baddata");
    module->setName("baddata");

    Section text;
    text.name = ".text";
    text.type = SectionType::SHT_TEXT;
    text.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    text.data = {0x00, 0x00, 0x00, 0x00};
    text.virtual_size = text.data.size();
    module->addSection(std::move(text));

    Section data;
    data.name = ".data";
    data.type = SectionType::SHT_DATA;
    data.flags = SectionFlags::ALLOC; // missing WRITE
    data.data = {0x00, 0x00, 0x00, 0x00};
    data.virtual_size = data.data.size();
    module->addSection(std::move(data));

    module->addSymbol(funcSym("_F_main_v", 0));
    std::vector<uint8_t> bytes;
    ASSERT_TRUE(module->serialize(bytes));
    io.binaryFiles["baddata.ho"] = bytes;

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("baddata.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Validate);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::InvalidSection);
}

TEST_F(HVMJITLoaderTest, ImportSymbolValidationRejectsMissingDependencySymbol) {
    std::vector<HVMInstruction> depIns{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> depSyms{
        funcSym("_F_module_init_v", 0),
    };
    io.binaryFiles["dep/empty.ho"] = buildModuleBytes("dep.empty", depIns, depSyms);

    std::vector<HVMInstruction> mainIns{
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> mainSyms{
        funcSym("_F_main_v", 0),
    };
    std::vector<ImportEntry> imports{
        importFrom("_F_missing_symbol_v", "dep.empty"),
    };
    io.binaryFiles["needsym.ho"] = buildModuleBytes("needsym", mainIns, mainSyms, imports);

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("needsym.ho"));
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Resolve);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::MissingDependency);
}
