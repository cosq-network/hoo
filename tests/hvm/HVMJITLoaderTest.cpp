#include <gtest/gtest.h>

#include <cstdlib>
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <thread>
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

HVMInstruction makeB(Opcode op, OperandsB ops) {
    HVMInstruction ins(op, ops);
    ins.setFormat(InstructionFormat::B);
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

std::vector<uint8_t> buildModuleBytesWithSections(
    const std::string& moduleName,
    const std::vector<HVMInstruction>& instructions,
    const std::vector<Symbol>& symbols,
    const std::vector<Section>& extraSections,
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

    for (const auto& sec : extraSections) {
        module->addSection(sec);
    }
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

Symbol objSym(const std::string& name, uint64_t offset, int16_t sectionIndex) {
    Symbol s{};
    s.name = name;
    s.binding = Symbol::STB_GLOBAL;
    s.type = Symbol::STT_OBJECT;
    s.visibility = Symbol::STV_DEFAULT;
    s.value = offset;
    s.size = 8;
    s.section_index = sectionIndex;
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

ImportEntry importNative(const std::string& symbol, const std::string& library = "__process__") {
    ImportEntry i{};
    i.name = symbol;
    i.library = library;
    i.import_type = ImportEntry::IT_NATIVE;
    i.version = 1;
    i.flags = 0;
    i.resolved_address = 0;
    return i;
}

uint64_t instructionOffset(const std::vector<HVMInstruction>& instructions, size_t index) {
    uint64_t off = 0;
    for (size_t i = 0; i < index; ++i) {
        auto mod = HOModule::create("offset.calc");
        auto bytes = mod->encodeInstructions({instructions[i]});
        EXPECT_GT(bytes.size(), 0u);
        off += static_cast<uint64_t>(bytes.size());
    }
    return off;
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

TEST_F(HVMJITLoaderTest, NativeProcessImportResolvesSymbol) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 17}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    std::vector<ImportEntry> imports{
        importNative("puts"),
    };
    io.binaryFiles["native_ok.ho"] = buildModuleBytes("native.ok", ins, syms, imports);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("native_ok.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 17) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, NativeImportMissingSymbolFailsLoad) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 3}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    std::vector<ImportEntry> imports{
        importNative("hooc_missing_native_symbol_for_test"),
    };
    io.binaryFiles["native_missing.ho"] = buildModuleBytes("native.missing", ins, syms, imports);

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("native_missing.ho"));
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Native symbol not found"), std::string::npos);
    auto info = jit.getLastErrorInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->phase, HVMJIT::ErrorPhase::Resolve);
    EXPECT_EQ(info->code, HVMJIT::ErrorCode::MissingDependency);
}

TEST_F(HVMJITLoaderTest, NativeImportMissingLibraryFailsLoad) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 3}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    std::vector<ImportEntry> imports{
        importNative("some_symbol", "libhooc_missing_library_12345.dylib"),
    };
    io.binaryFiles["native_missing_lib.ho"] = buildModuleBytes("native.missing.lib", ins, syms, imports);

    HVMJIT jit(io);
    EXPECT_FALSE(jit.loadInput("native_missing_lib.ho"));
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Failed to load native library"), std::string::npos);
}

TEST_F(HVMJITLoaderTest, InboundTrampolineInvokesHvmFunction) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ADDI, OperandsI{1, 1, 1}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_cb_v", 0),
    };
    io.binaryFiles["cb.ho"] = buildModuleBytes("cb", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cb.ho")) << jit.getLastError();
    void* tramp = jit.createInboundTrampoline("cb", "_F_cb_v");
    ASSERT_NE(tramp, nullptr) << jit.getLastError();
    auto fn = reinterpret_cast<uint64_t(*)(uint64_t)>(tramp);
    EXPECT_EQ(fn(41), 42u);
}

TEST_F(HVMJITLoaderTest, InboundTrampolineRejectsUnknownFunction) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["cb_missing.ho"] = buildModuleBytes("cb.missing", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cb_missing.ho")) << jit.getLastError();
    EXPECT_EQ(jit.createInboundTrampoline("cb.missing", "_F_not_there_v"), nullptr);
    EXPECT_TRUE(jit.hasError());
}

TEST_F(HVMJITLoaderTest, InboundTrampolineInvokesTwoArgCallback) {
    std::vector<HVMInstruction> ins{
        makeR(Opcode::ARITH, OperandsR{1, 1, 2, 0}), // r1 = r1 + r2
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_cb2_v", 0),
    };
    io.binaryFiles["cb2.ho"] = buildModuleBytes("cb2", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cb2.ho")) << jit.getLastError();
    void* tramp = jit.createInboundTrampoline("cb2", "_F_cb2_v", 2);
    ASSERT_NE(tramp, nullptr) << jit.getLastError();
    auto fn = reinterpret_cast<uint64_t(*)(uint64_t, uint64_t)>(tramp);
    EXPECT_EQ(fn(20, 22), 42u);
}

TEST_F(HVMJITLoaderTest, InboundTrampolineRejectsUnsupportedArity) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_cb_v", 0),
    };
    io.binaryFiles["cb_arity.ho"] = buildModuleBytes("cb_arity", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cb_arity.ho")) << jit.getLastError();
    EXPECT_EQ(jit.createInboundTrampoline("cb_arity", "_F_cb_v", 3), nullptr);
    EXPECT_TRUE(jit.hasError());
}

TEST_F(HVMJITLoaderTest, InspectorTraceExposesStepSnapshots) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 10}),
        makeI(Opcode::ADDI, OperandsI{1, 1, 5}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["inspector.ho"] = buildModuleBytes("inspector", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("inspector.ho")) << jit.getLastError();
    ASSERT_TRUE(jit.buildInspectorTrace("_F_main_v")) << jit.getLastError();
    auto s0 = jit.getInspectorSnapshot();
    ASSERT_TRUE(s0.has_value());
    EXPECT_EQ(s0->opcode, "movz");
    EXPECT_EQ(s0->regs[1], 0);

    ASSERT_TRUE(jit.inspectorStep());
    auto s1 = jit.getInspectorSnapshot();
    ASSERT_TRUE(s1.has_value());
    EXPECT_EQ(s1->opcode, "addi");
    EXPECT_EQ(s1->regs[1], 10);

    ASSERT_TRUE(jit.inspectorStep());
    auto s2 = jit.getInspectorSnapshot();
    ASSERT_TRUE(s2.has_value());
    EXPECT_EQ(s2->opcode, "ret");
    EXPECT_EQ(s2->regs[1], 15);
}

TEST_F(HVMJITLoaderTest, InspectorCanReadVirtualMemorySnapshot) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 140}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 0x41}),
        makeI(Opcode::ST_B, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["memsnap.ho"] = buildModuleBytes("memsnap", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("memsnap.ho")) << jit.getLastError();
    ASSERT_TRUE(jit.buildInspectorTrace("_F_main_v")) << jit.getLastError();
    while (jit.inspectorStep()) {
    }
    auto snap = jit.getInspectorSnapshot();
    ASSERT_TRUE(snap.has_value());
    EXPECT_EQ(snap->opcode, "ret");
    EXPECT_EQ(snap->regs[1], 65);

    auto bytes = jit.readVirtualMemory(140, 1);
    ASSERT_EQ(bytes.size(), 1u);
    EXPECT_EQ(bytes[0], 0x41u);
}

TEST_F(HVMJITLoaderTest, EscapeAllocaPromotionOptInDoesNotBreakExecution) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 100}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}), // alloc
        makeI(Opcode::MOVZ, OperandsI{4, 0, 0}),    // kill allocated value (non-escaping)
        makeI(Opcode::MOVZ, OperandsI{1, 0, 7}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["esc_alloca_optin.ho"] = buildModuleBytes("esc_alloca_optin", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("esc_alloca_optin.ho")) << jit.getLastError();

    setenv("HOOC_ENABLE_ESCAPE_ALLOCA", "1", 1);
    const auto rv = jit.run("_F_main_v");
    unsetenv("HOOC_ENABLE_ESCAPE_ALLOCA");

    EXPECT_EQ(rv, 7) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, EscapeAllocaPromotionDoesNotPromoteEscapingValue) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 100}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}), // alloc
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),  // pass pointer in r2
        makeI(Opcode::SYSCALL, OperandsI{5, 0, 4}), // refcount(r2) => must be 1 for managed heap obj
        makeR(Opcode::MOV, OperandsR{1, 5, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 3}), // release
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["esc_alloca_escape_guard.ho"] = buildModuleBytes("esc_alloca_escape_guard", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("esc_alloca_escape_guard.ho")) << jit.getLastError();

    setenv("HOOC_ENABLE_ESCAPE_ALLOCA", "1", 1);
    const auto rv = jit.run("_F_main_v");
    unsetenv("HOOC_ENABLE_ESCAPE_ALLOCA");

    EXPECT_EQ(rv, 1) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, DwarfDebugInfoOptInDoesNotBreakExecution) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 77}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["dwarf_optin.ho"] = buildModuleBytes("dwarf_optin", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("dwarf_optin.ho")) << jit.getLastError();

    setenv("HOOC_ENABLE_DWARF", "1", 1);
    const auto rv = jit.run("_F_main_v");
    unsetenv("HOOC_ENABLE_DWARF");

    EXPECT_EQ(rv, 77) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, DwarfDebugInfoOptInWorksWithInspectorTrace) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 5}),
        makeI(Opcode::ADDI, OperandsI{1, 1, 2}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["dwarf_inspector.ho"] = buildModuleBytes("dwarf_inspector", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("dwarf_inspector.ho")) << jit.getLastError();

    setenv("HOOC_ENABLE_DWARF", "1", 1);
    ASSERT_TRUE(jit.buildInspectorTrace("_F_main_v")) << jit.getLastError();
    while (jit.inspectorStep()) {
    }
    unsetenv("HOOC_ENABLE_DWARF");

    auto snap = jit.getInspectorSnapshot();
    ASSERT_TRUE(snap.has_value());
    EXPECT_EQ(snap->opcode, "ret");
    EXPECT_EQ(snap->regs[1], 7);
}

TEST_F(HVMJITLoaderTest, JitDebugListenerLifecycleSurvivesRepeatedInstances) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 11}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["jit_listener_lifecycle.ho"] = buildModuleBytes("jit_listener_lifecycle", ins, syms);

    for (int i = 0; i < 8; ++i) {
        HVMJIT jit(io);
        ASSERT_TRUE(jit.loadInput("jit_listener_lifecycle.ho")) << jit.getLastError();
        const auto rv = jit.run("_F_main_v");
        EXPECT_EQ(rv, 11) << "iteration=" << i << " err=" << jit.getLastError();
    }
}

TEST_F(HVMJITLoaderTest, StopExecutionConcurrentCallIsSafeDuringCompiledRun) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32767}), // outer count
        makeI(Opcode::MOVZ, OperandsI{4, 0, 32767}), // inner init template
        makeR(Opcode::MOV, OperandsR{3, 4, 0, 0}),   // inner = template
        makeI(Opcode::ADDI, OperandsI{1, 1, 1}),     // work
        makeI(Opcode::ADDI, OperandsI{3, 3, -1}),
        makeB(Opcode::BNE, OperandsB{3, 0, -2}),     // back to work
        makeI(Opcode::ADDI, OperandsI{2, 2, -1}),
        makeB(Opcode::BNE, OperandsB{2, 0, -5}),     // back to inner reset
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["stop_compiled_loop.ho"] = buildModuleBytes("stop_compiled_loop", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("stop_compiled_loop.ho")) << jit.getLastError();

    int64_t rv = 0;
    std::thread worker([&]() { rv = jit.run("_F_main_v"); });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    jit.stopExecution();
    worker.join();

    EXPECT_TRUE(rv == 1 || rv == -1);
}

TEST_F(HVMJITLoaderTest, StopExecutionInterruptsCompiledInfiniteLoop) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ADDI, OperandsI{1, 1, 1}),
        makeB(Opcode::BNE, OperandsB{1, 0, -1}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
    };
    io.binaryFiles["stop_infinite_loop.ho"] = buildModuleBytes("stop_infinite_loop", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("stop_infinite_loop.ho")) << jit.getLastError();

    int64_t rv = 0;
    std::thread worker([&]() { rv = jit.run("_F_main_v"); });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    jit.stopExecution();
    worker.join();

    EXPECT_EQ(rv, -1);
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
    EXPECT_FALSE(jit.loadInput("initfail.ho"));
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Module init failed"), std::string::npos);
}

TEST_F(HVMJITLoaderTest, PostLoadInitializerRunsDependenciesInPostOrder) {
    std::vector<HVMInstruction> depIns{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 128}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeI(Opcode::ST_B, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> depSyms{
        funcSym("_F_module_init_v", 0),
        funcSym("_F_dummy_v", 16),
    };
    io.binaryFiles["dep/order.ho"] = buildModuleBytes("dep.order", depIns, depSyms);

    std::vector<HVMInstruction> mainIns{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 128}),
        makeI(Opcode::LD_BU, OperandsI{3, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),
        makeR(Opcode::CMP, OperandsR{5, 3, 4, 0}),
        makeB(Opcode::BNE, OperandsB{5, 4, 2}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::BREAK, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 9}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> mainSyms{
        funcSym("_F_module_init_v", 0),
        funcSym("_F_main_v", 28),
    };
    std::vector<ImportEntry> imports{
        importFrom("_F_dummy_v", "dep.order"),
    };
    io.binaryFiles["order_main.ho"] = buildModuleBytes("order.main", mainIns, mainSyms, imports);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("order_main.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 9) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, ModuleInitializerRunsOnlyOnceAcrossMultipleRuns) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 140}),
        makeI(Opcode::LD_BU, OperandsI{3, 2, 0}),
        makeI(Opcode::ADDI, OperandsI{3, 3, 1}),
        makeI(Opcode::ST_B, OperandsI{3, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 140}),
        makeI(Opcode::LD_BU, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_module_init_v", 0),
        funcSym("_F_main_v", 20),
    };
    io.binaryFiles["once.ho"] = buildModuleBytes("once", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("once.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, VTableInitializersRunBeforeModuleInitWithBaseFirst) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 150}),             // _F_Base_vtable_init_v
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeI(Opcode::ST_B, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 150}),             // _F_Derived_Base_vtable_init_v
        makeI(Opcode::LD_BU, OperandsI{3, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),
        makeR(Opcode::CMP, OperandsR{5, 3, 4, 0}),
        makeB(Opcode::BNE, OperandsB{5, 4, 5}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 151}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeI(Opcode::ST_B, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::BREAK, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 150}),             // _F_module_init_v
        makeI(Opcode::LD_BU, OperandsI{3, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 151}),
        makeI(Opcode::LD_BU, OperandsI{6, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),
        makeR(Opcode::CMP, OperandsR{5, 3, 4, 0}),
        makeR(Opcode::CMP, OperandsR{7, 6, 4, 0}),
        makeR(Opcode::ARITH, OperandsR{8, 5, 7, 0}),
        makeI(Opcode::MOVZ, OperandsI{9, 0, 2}),
        makeR(Opcode::CMP, OperandsR{10, 8, 9, 0}),
        makeB(Opcode::BNE, OperandsB{10, 4, 2}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::BREAK, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 5}),               // _F_main_v
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_Base_vtable_init_v", 0),
        funcSym("_F_Derived_Base_vtable_init_v", 16),
        funcSym("_F_module_init_v", 56),
        funcSym("_F_main_v", 112),
    };
    io.binaryFiles["vtable.ho"] = buildModuleBytes("vtable", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vtable.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 5) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, DataAndBssSectionsWithVirtualMemoryLoadAndRun) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 170}),             // module init writes shared marker
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeI(Opcode::ST_B, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 170}),             // main reads marker set during init
        makeI(Opcode::LD_BU, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_module_init_v", 0),
        funcSym("_F_main_v", 16),
    };
    Section dataSec;
    dataSec.name = ".data";
    dataSec.type = SectionType::SHT_DATA;
    dataSec.flags = SectionFlags::ALLOC | SectionFlags::WRITE;
    dataSec.data = std::vector<uint8_t>(16, 0);
    dataSec.data[0] = 42;
    dataSec.virtual_size = dataSec.data.size();
    dataSec.alignment = 16;

    Section bssSec;
    bssSec.name = ".bss";
    bssSec.type = SectionType::SHT_BSS;
    bssSec.flags = SectionFlags::ALLOC | SectionFlags::WRITE;
    bssSec.virtual_size = 16;
    bssSec.data = std::vector<uint8_t>(16, 0);
    bssSec.alignment = 16;

    io.binaryFiles["staticmem.ho"] = buildModuleBytesWithSections(
        "staticmem", ins, syms, {dataSec, bssSec});

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("staticmem.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, CrossModuleVTableInitResolvesBaseBeforeDerived) {
    std::vector<HVMInstruction> depIns{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 200}),             // _F_Base_vtable_init_v
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeI(Opcode::ST_B, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),             // _F_module_init_v (no-op)
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),             // _F_dummy_v
    };
    std::vector<Symbol> depSyms{
        funcSym("_F_Base_vtable_init_v", instructionOffset(depIns, 0)),
        funcSym("_F_module_init_v", instructionOffset(depIns, 4)),
        funcSym("_F_dummy_v", instructionOffset(depIns, 5)),
    };
    io.binaryFiles["dep/basevt.ho"] = buildModuleBytes("dep.basevt", depIns, depSyms);

    std::vector<HVMInstruction> mainIns{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 200}),             // _F_Derived_Base_vtable_init_v
        makeI(Opcode::LD_BU, OperandsI{3, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),
        makeR(Opcode::CMP, OperandsR{5, 3, 4, 0}),
        makeB(Opcode::BNE, OperandsB{5, 4, 5}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 201}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeI(Opcode::ST_B, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::BREAK, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 201}),             // _F_module_init_v checks derived marker
        makeI(Opcode::LD_BU, OperandsI{3, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),
        makeR(Opcode::CMP, OperandsR{5, 3, 4, 0}),
        makeB(Opcode::BNE, OperandsB{5, 4, 2}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::BREAK, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 11}),              // _F_main_v
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> mainSyms{
        funcSym("_F_Derived_Base_vtable_init_v", instructionOffset(mainIns, 0)),
        funcSym("_F_module_init_v", instructionOffset(mainIns, 10)),
        funcSym("_F_main_v", instructionOffset(mainIns, 17)),
    };
    std::vector<ImportEntry> imports{
        importFrom("_F_dummy_v", "dep.basevt"),
    };
    io.binaryFiles["crossvt_main.ho"] = buildModuleBytes("crossvt.main", mainIns, mainSyms, imports);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("crossvt_main.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 11) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, VTableInitializersRunOnceAcrossMultipleRuns) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 210}),             // _F_Base_vtable_init_v increments counter
        makeI(Opcode::LD_BU, OperandsI{3, 2, 0}),
        makeI(Opcode::ADDI, OperandsI{3, 3, 1}),
        makeI(Opcode::ST_B, OperandsI{3, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),             // _F_module_init_v
        makeI(Opcode::MOVZ, OperandsI{2, 0, 210}),             // _F_main_v reads vtable-init counter
        makeI(Opcode::LD_BU, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_Base_vtable_init_v", 0),
        funcSym("_F_module_init_v", 20),
        funcSym("_F_main_v", 24),
    };
    io.binaryFiles["vt_once.ho"] = buildModuleBytes("vt.once", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vt_once.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, VTableInitPatchesSlotAndMainReadsIt) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{3, 0, 24}),             // _F_Child_Base_vtable_init_v
        makeI(Opcode::MOVZ, OperandsI{2, 0, 216}),
        makeI(Opcode::ST_D, OperandsI{3, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),             // _F_module_init_v
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),             // _F_Child_virtual_v (placeholder)
        makeI(Opcode::MOVZ, OperandsI{2, 0, 216}),            // _F_main_v
        makeI(Opcode::LD_D, OperandsI{3, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{6, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),
        makeR(Opcode::CMP, OperandsR{5, 3, 6, 0}),
        makeB(Opcode::BNE, OperandsB{5, 4, 2}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 77}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::BREAK, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_Child_Base_vtable_init_v", instructionOffset(ins, 0)),
        funcSym("_F_module_init_v", instructionOffset(ins, 4)),
        funcSym("_F_Child_virtual_v", instructionOffset(ins, 5)),
        funcSym("_F_main_v", instructionOffset(ins, 6)),
        objSym("_H_Child_vtable_ptr", 0, 1),
    };

    Section dataSec;
    dataSec.name = ".data";
    dataSec.type = SectionType::SHT_DATA;
    dataSec.flags = SectionFlags::ALLOC | SectionFlags::WRITE;
    dataSec.data = std::vector<uint8_t>(8, 0);
    dataSec.virtual_size = dataSec.data.size();
    dataSec.alignment = 8;

    io.binaryFiles["vt_patch.ho"] = buildModuleBytesWithSections(
        "vt.patch", ins, syms, {dataSec});

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vt_patch.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 77) << jit.getLastError();
}

TEST_F(HVMJITLoaderTest, BssSectionStartsZeroBeforeModuleInitRuns) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 520}),             // _F_module_init_v reads memory expected zero
        makeI(Opcode::LD_BU, OperandsI{3, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 0}),
        makeR(Opcode::CMP, OperandsR{5, 3, 4, 0}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),
        makeB(Opcode::BNE, OperandsB{5, 4, 5}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 521}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 1}),
        makeI(Opcode::ST_B, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::BREAK, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 521}),             // _F_main_v reads init marker
        makeI(Opcode::LD_BU, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_module_init_v", instructionOffset(ins, 0)),
        funcSym("_F_main_v", instructionOffset(ins, 11)),
    };

    Section bssSec;
    bssSec.name = ".bss";
    bssSec.type = SectionType::SHT_BSS;
    bssSec.flags = SectionFlags::ALLOC | SectionFlags::WRITE;
    bssSec.virtual_size = 16;
    bssSec.data = std::vector<uint8_t>(16, 0);
    bssSec.alignment = 16;

    io.binaryFiles["bss_zero.ho"] = buildModuleBytesWithSections(
        "bss.zero", ins, syms, {bssSec});

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("bss_zero.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
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
    EXPECT_NE(jit.getSymbolAddress("hoo_map_set_string_int64"), nullptr);
    EXPECT_NE(jit.getSymbolAddress("hoo_map_set_string_object"), nullptr);
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

TEST_F(HVMJITLoaderTest, SameModuleNameFromDifferentPathsAccepted) {
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
    // Same module name from a different path is now allowed
    EXPECT_TRUE(jit.loadInput("b/same.ho")) << jit.getLastError();
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

TEST_F(HVMJITLoaderTest, RunFailsWhenNoModuleLoaded) {
    HVMJIT jit(io);
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("No module loaded"), std::string::npos);
}

TEST_F(HVMJITLoaderTest, GetSymbolAddressFailsWhenJitNotReady) {
    HVMJIT jit(io);
    EXPECT_EQ(jit.getSymbolAddress("_F_any_v"), nullptr);
    EXPECT_TRUE(jit.hasError());
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
