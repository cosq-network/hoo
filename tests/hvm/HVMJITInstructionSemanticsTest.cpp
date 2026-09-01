#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/IOProvider.h"
#include "core/SymbolMangler.h"
#include "hvm/HOModule.h"
#include "hvm/HVMInstruction.h"
#include "hvm/HVMJIT.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/exception/hoo_exception.h"

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

HVMInstruction makeJ(Opcode op, OperandsJ ops) {
    HVMInstruction ins(op, ops);
    ins.setFormat(InstructionFormat::J);
    return ins;
}

std::vector<uint8_t> buildModuleBytes(
    const std::string& moduleName,
    const std::vector<HVMInstruction>& instructions,
    const std::vector<Symbol>& symbols,
    const std::vector<Section>& extraSections = {},
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

Symbol importFuncSym(const std::string& name, uint64_t pseudoTarget) {
    Symbol s = funcSym(name, pseudoTarget);
    s.type = Symbol::STT_NOTYPE;
    s.section_index = -1;
    return s;
}

ImportEntry importFrom(const std::string& symbol, const std::string& library) {
    ImportEntry i{};
    i.name = symbol;
    i.library = library;
    i.import_type = ImportEntry::IT_RUNTIME;
    i.version = 1;
    i.flags = 0;
    i.resolved_address = 0;
    return i;
}

} // namespace

class HVMJITInstructionSemanticsTest : public ::testing::Test {
protected:
    InMemoryIOProvider io;
};

TEST_F(HVMJITInstructionSemanticsTest, BranchSemanticsBEQSkipsInstructions) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 1}),
        makeB(Opcode::BEQ, OperandsB{2, 3, 2}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 7}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["branch.ho"] = buildModuleBytes("branch", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("branch.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 7) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, CallSemanticsInvokesCalleeAndReturnsValue) {
    std::vector<HVMInstruction> ins{
        makeJ(Opcode::CALL, OperandsJ{29, 3}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 33}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        funcSym("_F_helper_v", 12),
    };
    io.binaryFiles["call.ho"] = buildModuleBytes("call", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("call.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 33) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, EnterLeaveAndFrameAddressingWorkForLocals) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 55}),
        makeI(Opcode::ST_D, OperandsI{2, 30, -8}),
        makeI(Opcode::LD_D, OperandsI{1, 30, -8}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["frame.ho"] = buildModuleBytes("frame", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("frame.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 55) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, PushPopRoundTripValue) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 44}),
        makeR(Opcode::PUSH, OperandsR{2, 0, 0, 0}),
        makeR(Opcode::POP, OperandsR{1, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["stack.ho"] = buildModuleBytes("stack", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("stack.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 44) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SimpleSupportedProgramRunsViaJIT) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 11}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["jitpath.ho"] = buildModuleBytes("jitpath", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("jitpath.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 11) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HVMJITInstructionSemanticsTest, UnsupportedOpcodeFallsBackToInterpreter) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::SYSCALL, OperandsI{1, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["fallback.ho"] = buildModuleBytes("fallback", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("fallback.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HVMJITInstructionSemanticsTest, NotOpcodeInvertsBits) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),
        makeR(Opcode::NOT, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["notop.ho"] = buildModuleBytes("notop", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("notop.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -2) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, ByteHalfWordWordLoadStoreSemantics) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0x7F}),
        makeI(Opcode::ST_B, OperandsI{3, 2, 0}),
        makeI(Opcode::LD_BU, OperandsI{4, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 0x123}),
        makeI(Opcode::ST_H, OperandsI{5, 2, 2}),
        makeI(Opcode::LD_HU, OperandsI{6, 2, 2}),
        makeI(Opcode::MOVZ, OperandsI{7, 0, 0x3456}),
        makeI(Opcode::ST_W, OperandsI{7, 2, 4}),
        makeI(Opcode::LD_WU, OperandsI{8, 2, 4}),
        makeR(Opcode::ARITH, OperandsR{1, 4, 6, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 1, 8, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["ldst_widths.ho"] = buildModuleBytes("ldst_widths", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("ldst_widths.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 13816) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, RuntimeStringDataBridgeReturnsCharPointerBytes) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::LDA, OperandsI{1, 0, 0}),               // rodata "hello\0"
        makeJ(Opcode::CALL, OperandsJ{29, 9}),                // import _F_hoo_String_from_cstr_p_p at pseudo-PC 40
        makeR(Opcode::MOV, OperandsR{2, 1, 0, 0}),            // r2 = HooString
        makeI(Opcode::SYSCALL, OperandsI{3, 0, 11}),          // r3 = hoo_string_data(r2)
        makeI(Opcode::MOVZ, OperandsI{4, 0, 0}),
        makeR(Opcode::CMP, OperandsR{5, 3, 4, 1}),            // pointer != 0
        makeR(Opcode::MOV, OperandsR{1, 5, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    Section rodata;
    rodata.name = ".rodata";
    rodata.type = SectionType::SHT_RODATA;
    rodata.flags = SectionFlags::ALLOC;
    rodata.data = {'h', 'e', 'l', 'l', 'o', '\0'};
    rodata.virtual_size = rodata.data.size();
    rodata.alignment = 1;

    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        importFuncSym("_F_hoo_String_from_cstr_p_p", 40),
    };
    std::vector<ImportEntry> imports{
        importFrom("_F_hoo_String_from_cstr_p_p", "hoo"),
    };
    io.binaryFiles["string_data_bridge.ho"] = buildModuleBytes(
        "string_data_bridge", ins, syms, {rodata}, imports);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("string_data_bridge.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, TailCallTransfersControlToCallee) {
    std::vector<HVMInstruction> ins{
        makeJ(Opcode::TAILCALL, OperandsJ{0, 4}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 21}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        funcSym("_F_leaf_v", 16),
    };
    io.binaryFiles["tailcall.ho"] = buildModuleBytes("tailcall", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("tailcall.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 21) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, JalrPerformsIndirectIntraFunctionJump) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 12}),
        makeI(Opcode::JALR, OperandsI{29, 2, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 33}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["jalr.ho"] = buildModuleBytes("jalr", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("jalr.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 33) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, JalrSpeculativelyDevirtualizesFinalMethodSymbol) {
    MangledFunctionParams p{};
    p.modulePath = {"demo"};
    p.className = "Widget";
    p.baseClassName = "";
    p.classModifiers = {"FINAL"};
    p.functionName = "compute";
    p.functionModifiers = {};
    p.returnType = "int64";
    p.parameterTypes = {};
    p.isConstructor = false;
    p.isDestructor = false;
    p.isStatic = false;
    p.isVirtual = true;
    const std::string finalSym = SymbolMangler::mangleFunctionName(p);

    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),
        makeI(Opcode::JALR, OperandsI{29, 2, 0}),
        makeI(Opcode::ADDI, OperandsI{1, 1, 1}), // executes only on devirtualized call-return path
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 40}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        funcSym(finalSym, 16),
    };
    io.binaryFiles["jalr_final.ho"] = buildModuleBytes("jalr_final", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("jalr_final.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 41) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, JalrNonFinalTargetUsesNormalJumpPath) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),
        makeI(Opcode::JALR, OperandsI{29, 2, 0}),
        makeI(Opcode::ADDI, OperandsI{1, 1, 1}), // should not execute in normal jump path
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 40}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        funcSym("_F_plain_v", 16),
    };
    io.binaryFiles["jalr_plain.ho"] = buildModuleBytes("jalr_plain", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("jalr_plain.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 40) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, JalrFinalImportSymbolFallsBackToNormalJumpPath) {
    MangledFunctionParams p{};
    p.modulePath = {"demo"};
    p.className = "Widget";
    p.baseClassName = "";
    p.classModifiers = {"FINAL"};
    p.functionName = "compute";
    p.functionModifiers = {};
    p.returnType = "int64";
    p.parameterTypes = {};
    p.isConstructor = false;
    p.isDestructor = false;
    p.isStatic = false;
    p.isVirtual = true;
    const std::string finalImportSym = SymbolMangler::mangleFunctionName(p);

    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),
        makeI(Opcode::JALR, OperandsI{29, 2, 0}),
        makeI(Opcode::ADDI, OperandsI{1, 1, 1}), // should not execute on jump fallback
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 52}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        importFuncSym(finalImportSym, 16),
    };
    io.binaryFiles["jalr_final_import.ho"] = buildModuleBytes("jalr_final_import", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("jalr_final_import.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 52) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, JalrMisalignedTargetTraps) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 13}),          // r2 = 13 (not 4-byte aligned)
        makeI(Opcode::JALR, OperandsI{29, 2, 0}),          // target = 13 -> misaligned trap
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),          // must not execute
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["jalr_misaligned.ho"] = buildModuleBytes("jalr_misaligned", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("jalr_misaligned.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("not 4-byte aligned"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, FloatArithmeticAndComparisonUsingSubnormalBits) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 2}),
        makeR(Opcode::FLOAT_ARITH, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::FCMP, OperandsR{1, 2, 4, 1}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["float_subnormal.ho"] = buildModuleBytes("float_subnormal", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("float_subnormal.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, FloatingNaNResultsUseCanonicalBits) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::LUI, OperandsI{2, 0, 0x3ffc}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 1}),
        makeR(Opcode::LOGIC, OperandsR{2, 2, 3, 1}),
        makeR(Opcode::FLOAT_ARITH, OperandsR{1, 2, 0, 0}),
        makeI(Opcode::ST_D, OperandsI{1, 30, -16}),
        makeI(Opcode::LD_D, OperandsI{1, 30, -16}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["canonical_nan.ho"] = buildModuleBytes("canonical_nan", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("canonical_nan.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), static_cast<int64_t>(0x7ff8000000000000ULL))
        << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SubwordIntegerArithmeticWrapsAndUsesUnsignedRemainder) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x7F}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 1}),
        makeR(Opcode::ARITH_B, OperandsR{4, 2, 3, 0}), // 0x7f + 1 -> 0x80
        makeI(Opcode::MOVZ, OperandsI{2, 0, 250}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 7}),
        makeR(Opcode::ARITH_B, OperandsR{1, 2, 3, 8}), // 250 % 7 -> 5
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["subword_arith.ho"] = buildModuleBytes("subword_arith", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("subword_arith.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 5) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HVMJITInstructionSemanticsTest, SubwordBitLogicNormalizesBitZero) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 1}),
        makeR(Opcode::LOGIC_B, OperandsR{4, 2, 3, 0}), // XOR -> 0
        makeR(Opcode::LOGIC_B, OperandsR{1, 4, 0, 2}), // NOT -> 1
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["subword_logic.ho"] = buildModuleBytes("subword_logic", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("subword_logic.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HVMJITInstructionSemanticsTest, SubwordShiftsWrapAndPreserveSignedRightShift) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xF0}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 4}),
        makeR(Opcode::SHIFT_B, OperandsR{4, 2, 3, 0}), // 0xf0 << 4 -> 0x00
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x80}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 1}),
        makeR(Opcode::SHIFT_B, OperandsR{1, 2, 3, 2}), // -128 >> 1 -> -64
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["subword_shift.ho"] = buildModuleBytes("subword_shift", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("subword_shift.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), static_cast<int64_t>(-64)) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HVMJITInstructionSemanticsTest, SubwordFp8ArithmeticUsesCanonicalE4M3Encoding) {
    // E4M3: 1.5 = 0x3c, 2.25 = 0x41, rounded 3.75 = 0x47.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x3C}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0x41}),
        makeR(Opcode::FLOAT_ARITH_B, OperandsR{1, 2, 3, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["subword_fp8.ho"] = buildModuleBytes("subword_fp8", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("subword_fp8.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0x47) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HVMJITInstructionSemanticsTest, UnalignedDoubleWordAccessReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 10}),
        makeI(Opcode::ST_D, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["unaligned_d.ho"] = buildModuleBytes("unaligned_d", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("unaligned_d.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_NE(jit.getLastError().find("Unaligned ST.D address"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, UnalignedHalfWordAccessReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::LDA, OperandsI{2, 30, -15}),
        makeI(Opcode::LD_H, OperandsI{1, 2, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["unaligned_half.ho"] = buildModuleBytes("unaligned_half", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("unaligned_half.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_NE(jit.getLastError().find("Unaligned LD.H address"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallRuntimeIntrinsicsManageRefcount) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, HOO_TYPE_OBJECT}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}), // alloc
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{5, 0, 4}), // refcount -> 1
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 2}), // retain
        makeI(Opcode::SYSCALL, OperandsI{6, 0, 4}), // refcount -> 2
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 3}), // release
        makeI(Opcode::SYSCALL, OperandsI{7, 0, 4}), // refcount -> 1
        makeI(Opcode::SYSCALL, OperandsI{8, 0, 5}), // type_id -> HOO_TYPE_OBJECT
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 3}), // final release
        makeR(Opcode::CMP, OperandsR{9, 5, 7, 0}),  // 1 == 1
        makeR(Opcode::CMP, OperandsR{10, 6, 5, 1}), // 2 != 1
        makeR(Opcode::CMP, OperandsR{11, 8, 3, 0}), // type matches
        makeR(Opcode::ARITH, OperandsR{1, 9, 10, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 1, 11, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sysarc.ho"] = buildModuleBytes("sysarc", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sysarc.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 3) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, ArcUseDefOptInSmokeDoesNotBreakExecution) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 9}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["arc_usedef_smoke.ho"] = buildModuleBytes("arc_usedef_smoke", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("arc_usedef_smoke.ho")) << jit.getLastError();

    setenv("HOOC_ENABLE_ARC_USEDEF", "1", 1);
    auto rv = jit.run("_F_main_v");
    unsetenv("HOOC_ENABLE_ARC_USEDEF");
    EXPECT_EQ(rv, 9) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, ArcUseDefElidesAcrossBranchEdgeWithoutBarriers) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, HOO_TYPE_OBJECT}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}),    // alloc
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),     // r2 = obj
        makeI(Opcode::MOVZ, OperandsI{9, 0, 123}),     // sentinel for retain result
        makeI(Opcode::MOVZ, OperandsI{10, 0, 124}),    // sentinel for release result
        makeI(Opcode::MOVZ, OperandsI{5, 0, 1}),       // branch predicate
        makeB(Opcode::BEQ, OperandsB{5, 5, 2}),        // jump to retain block
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),      // dead path
        makeI(Opcode::SYSCALL, OperandsI{9, 0, 2}),    // retain
        makeI(Opcode::SYSCALL, OperandsI{10, 0, 3}),   // release
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 3}),    // final release to free object
        makeI(Opcode::MOVZ, OperandsI{11, 0, 123}),
        makeI(Opcode::MOVZ, OperandsI{12, 0, 124}),
        makeR(Opcode::CMP, OperandsR{13, 9, 11, 0}),   // retain syscall NOT elided in JIT => rd9 != 123
        makeR(Opcode::CMP, OperandsR{14, 10, 12, 0}),  // release syscall NOT elided in JIT => rd10 != 124
        makeR(Opcode::ARITH, OperandsR{1, 13, 14, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["arc_branch_elide.ho"] = buildModuleBytes("arc_branch_elide", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("arc_branch_elide.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, ArcUseDefDoesNotElideAcrossCallBarrier) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, HOO_TYPE_OBJECT}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}),    // alloc
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),     // r2 = obj
        makeI(Opcode::MOVZ, OperandsI{9, 0, 123}),     // sentinel for retain result
        makeI(Opcode::MOVZ, OperandsI{10, 0, 124}),    // sentinel for release result
        makeI(Opcode::SYSCALL, OperandsI{9, 0, 2}),    // retain
        makeJ(Opcode::CALL, OperandsJ{29, 9}),         // barrier call to helper
        makeI(Opcode::SYSCALL, OperandsI{10, 0, 3}),   // release
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 3}),    // final release to free object
        makeI(Opcode::MOVZ, OperandsI{11, 0, 123}),
        makeI(Opcode::MOVZ, OperandsI{12, 0, 124}),
        makeR(Opcode::CMP, OperandsR{13, 9, 11, 1}),   // retain executed => rd9 != sentinel
        makeR(Opcode::CMP, OperandsR{14, 10, 12, 1}),  // release executed => rd10 != sentinel
        makeR(Opcode::ARITH, OperandsR{1, 13, 14, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::NOP, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::NOP, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{20, 0, 1}),      // helper start
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        funcSym("_F_helper_v", 72),
    };
    io.binaryFiles["arc_call_barrier.ho"] = buildModuleBytes("arc_call_barrier", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("arc_call_barrier.ho")) << jit.getLastError();
    setenv("HOOC_ENABLE_ARC_USEDEF", "1", 1);
    const auto rv = jit.run("_F_main_v");
    unsetenv("HOOC_ENABLE_ARC_USEDEF");
    EXPECT_EQ(rv, 2) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, StDAppliesRetainStoreReleaseSequence) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{12, 0, 64}),              // slot addr
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, HOO_TYPE_OBJECT}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}),             // old obj
        makeI(Opcode::SYSCALL, OperandsI{5, 0, 1}),             // new obj
        makeI(Opcode::ST_D, OperandsI{4, 12, 0}),               // retain old => 2
        makeI(Opcode::ST_D, OperandsI{5, 12, 0}),               // retain new + release old
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{6, 0, 4}),             // old rc -> 1
        makeR(Opcode::MOV, OperandsR{2, 5, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{7, 0, 4}),             // new rc -> 2
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 3}),             // release new caller ref => 1
        makeI(Opcode::MOVZ, OperandsI{13, 0, 0}),
        makeI(Opcode::ST_D, OperandsI{13, 12, 0}),              // release slot-held new => 0/freed
        makeR(Opcode::CMP, OperandsR{8, 6, 13, 1}),             // old rc != 0
        makeR(Opcode::CMP, OperandsR{9, 7, 13, 1}),             // new rc != 0 at capture time
        makeR(Opcode::ARITH, OperandsR{1, 8, 9, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["st_arc.ho"] = buildModuleBytes("st_arc", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("st_arc.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 2) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, StDSameValueElidesRetainRelease) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{12, 0, 96}),              // slot addr
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, HOO_TYPE_OBJECT}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}),             // obj
        makeI(Opcode::ST_D, OperandsI{4, 12, 0}),               // retain on first store => rc 2
        makeI(Opcode::ST_D, OperandsI{4, 12, 0}),               // same pointer, should be elided
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{5, 0, 4}),             // rc should remain 2
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 3}),             // release caller => rc 1
        makeI(Opcode::MOVZ, OperandsI{13, 0, 0}),
        makeI(Opcode::ST_D, OperandsI{13, 12, 0}),              // release slot => free
        makeI(Opcode::MOVZ, OperandsI{6, 0, 2}),
        makeR(Opcode::CMP, OperandsR{7, 5, 6, 0}),              // rc == 2
        makeR(Opcode::MOV, OperandsR{1, 7, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["st_arc_same.ho"] = buildModuleBytes("st_arc_same", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("st_arc_same.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallCreatesRuntimeExceptionObject) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 6}), // runtime exception
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{5, 0, 4}), // refcount
        makeI(Opcode::SYSCALL, OperandsI{6, 0, 5}), // type_id
        makeI(Opcode::MOVZ, OperandsI{7, 0, 104}),  // HOO_TYPE_EXCEPTION
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 3}), // release
        makeR(Opcode::CMP, OperandsR{8, 5, 0, 0}),  // refcount == 0? false, because r0=0; use != below
        makeR(Opcode::CMP, OperandsR{9, 5, 0, 1}),  // refcount != 0
        makeR(Opcode::CMP, OperandsR{10, 6, 7, 0}), // type match
        makeR(Opcode::ARITH, OperandsR{1, 9, 10, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_exc.ho"] = buildModuleBytes("sys_exc", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_exc.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 2) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallThrowTransfersControlToRegisteredHandlerAndRestoresFrame) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{30, 0, 111}),            // fp sentinel
        makeI(Opcode::MOVZ, OperandsI{31, 0, 222}),            // sp sentinel
        makeI(Opcode::MOVZ, OperandsI{2, 0, 52}),              // handler pc (byte offset)
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 7}),            // push handler
        makeI(Opcode::MOVZ, OperandsI{30, 0, 1}),              // clobber fp
        makeI(Opcode::MOVZ, OperandsI{31, 0, 2}),              // clobber sp
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 6}),            // runtime exception
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),             // arg: exc
        makeI(Opcode::SYSCALL, OperandsI{5, 0, 9}),            // throw -> jump to handler
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),              // skipped
        makeI(Opcode::MOVZ, OperandsI{7, 0, 111}),             // handler start
        makeR(Opcode::CMP, OperandsR{8, 30, 7, 0}),            // fp restored?
        makeI(Opcode::MOVZ, OperandsI{9, 0, 222}),
        makeR(Opcode::CMP, OperandsR{10, 31, 9, 0}),           // sp restored?
        makeR(Opcode::CMP, OperandsR{11, 1, 0, 1}),            // exception in r1?
        makeR(Opcode::ARITH, OperandsR{1, 8, 10, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 1, 11, 0}),          // 3 if all true
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_throw_handler.ho"] = buildModuleBytes("sys_throw_handler", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_throw_handler.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 3) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallThrowWithoutHandlerFailsAsUnhandled) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 7}),            // push
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 8}),            // pop
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 6}),            // runtime exception
        makeR(Opcode::MOV, OperandsR{2, 4, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{5, 0, 9}),            // throw, unhandled
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_throw_unhandled.ho"] = buildModuleBytes("sys_throw_unhandled", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_throw_unhandled.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Unhandled exception trap"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, DivisionByZeroReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 2, 3, 5}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["divzero.ho"] = buildModuleBytes("divzero", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("divzero.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Division by zero"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, AddOverflowReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),
        makeI(Opcode::LUI, OperandsI{2, 2, 0x4000}),     // r2 = INT64_MIN
        makeR(Opcode::NOT, OperandsR{3, 2, 0, 0}),        // r3 = INT64_MAX
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),          // r4 = 1
        makeR(Opcode::ARITH, OperandsR{1, 3, 4, 0}),      // r1 = INT64_MAX + 1 → overflow
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["add_overflow.ho"] = buildModuleBytes("add_overflow", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("add_overflow.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Integer overflow: add"), std::string::npos);
    const auto csrs = jit.getCsrs();
    EXPECT_EQ(csrs[HVMJIT::kCsrScause], HVMJIT::kCauseArithmeticOverflow);
    EXPECT_EQ(csrs[HVMJIT::kCsrSepc], 16); // fourth instruction at byte offset 16
}

TEST_F(HVMJITInstructionSemanticsTest, DivisionByZeroRecordsScause) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 2, 3, 5}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["divzero_scause.ho"] = buildModuleBytes("divzero_scause", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("divzero_scause.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    const auto csrs = jit.getCsrs();
    EXPECT_EQ(csrs[HVMJIT::kCsrScause], HVMJIT::kCauseDivisionByZero);
    EXPECT_EQ(csrs[HVMJIT::kCsrSepc], 8); // third instruction at byte offset 8
}

TEST_F(HVMJITInstructionSemanticsTest, SubOverflowReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),
        makeI(Opcode::LUI, OperandsI{2, 2, 0x4000}),     // r2 = INT64_MIN
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),          // r4 = 1
        makeR(Opcode::ARITH, OperandsR{1, 2, 4, 1}),      // r1 = INT64_MIN - 1 → overflow
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sub_overflow.ho"] = buildModuleBytes("sub_overflow", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sub_overflow.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Integer overflow: sub"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, MulOverflowReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),
        makeI(Opcode::LUI, OperandsI{2, 2, 0x4000}),     // r2 = INT64_MIN
        makeR(Opcode::NOT, OperandsR{3, 2, 0, 0}),        // r3 = INT64_MAX
        makeI(Opcode::MOVZ, OperandsI{5, 0, 2}),          // r5 = 2
        makeR(Opcode::ARITH, OperandsR{1, 3, 5, 2}),      // r1 = INT64_MAX * 2 → overflow
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["mul_overflow.ho"] = buildModuleBytes("mul_overflow", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("mul_overflow.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Integer overflow: mul"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, DivOverflowReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),
        makeI(Opcode::LUI, OperandsI{2, 2, 0x4000}),     // r2 = INT64_MIN
        makeR(Opcode::NOT, OperandsR{7, 0, 0, 0}),        // r7 = -1
        makeR(Opcode::ARITH, OperandsR{1, 2, 7, 5}),      // r1 = INT64_MIN / -1 → overflow
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["div_overflow.ho"] = buildModuleBytes("div_overflow", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("div_overflow.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Integer overflow: div"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, ModOverflowReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),
        makeI(Opcode::LUI, OperandsI{2, 2, 0x4000}),     // r2 = INT64_MIN
        makeR(Opcode::NOT, OperandsR{7, 0, 0, 0}),        // r7 = -1
        makeR(Opcode::ARITH, OperandsR{1, 2, 7, 7}),      // r1 = INT64_MIN % -1 → overflow
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["mod_overflow.ho"] = buildModuleBytes("mod_overflow", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("mod_overflow.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Integer overflow: mod"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, InvalidStoreAddressReportsError) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),
        makeI(Opcode::LUI, OperandsI{2, 2, 1}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 5}),
        makeI(Opcode::ST_D, OperandsI{1, 2, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["badstore.ho"] = buildModuleBytes("badstore", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("badstore.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    const auto err = jit.getLastError();
    const bool invalid = err.find("Invalid ST.D address") != std::string::npos;
    const bool unaligned = err.find("Unaligned ST.D address") != std::string::npos;
    EXPECT_TRUE(invalid || unaligned);
}

TEST_F(HVMJITInstructionSemanticsTest, NestedCallReturnsCorrectly) {
    // Main calls helper which calls inner; both return correctly.
    // Instruction layout (CALL=escape32 8B, RET=MOVZ=base32 4B):
    //   PC 0:  CALL r29, 3      (8B) target =  0 + 3*4 = 12  -> helper
    //   PC 8:  RET               (4B)
    //   PC 12: CALL r29, 3      (8B) target = 12 + 3*4 = 24  -> inner
    //   PC 20: RET               (4B)
    //   PC 24: MOVZ r1,r0,42    (4B)
    //   PC 28: RET               (4B)
    std::vector<HVMInstruction> ins{
        makeJ(Opcode::CALL, OperandsJ{29, 3}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeJ(Opcode::CALL, OperandsJ{29, 3}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 42}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    // Symbols point to the helper (PC 12) and inner (PC 24) entry points
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        funcSym("_F_helper_v", 12),
        funcSym("_F_inner_v", 24),
    };
    io.binaryFiles["nested_call.ho"] = buildModuleBytes("nested_call", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("nested_call.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 42) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, CallRestoresReturnAddressAfterNestedCall) {
    // main calls helper which calls inner; both return to the correct pc.
    // Instruction layout (CALL=escape32 8B, ADDI=MOVZ=base32 4B, RET=base32 4B):
    //   PC 0:  MOVZ r1,r0,0     (4B)  r1 = 0
    //   PC 4:  CALL r29, 4      (8B)  target =  4 + 4*4 = 20  -> helper
    //   PC 12: ADDI r1,r1,10    (4B)
    //   PC 16: RET               (4B)
    //   PC 20: CALL r29, 4      (8B)  target = 20 + 4*4 = 36  -> inner
    //   PC 28: ADDI r1,r1,1     (4B)
    //   PC 32: RET               (4B)
    //   PC 36: MOVZ r1,r0,5     (4B)
    //   PC 40: RET               (4B)
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 0}),
        makeJ(Opcode::CALL, OperandsJ{29, 4}),
        makeI(Opcode::ADDI, OperandsI{1, 1, 10}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeJ(Opcode::CALL, OperandsJ{29, 4}),
        makeI(Opcode::ADDI, OperandsI{1, 1, 1}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 5}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{
        funcSym("_F_main_v", 0),
        funcSym("_F_helper_v", 20),
        funcSym("_F_inner_v", 36),
    };
    io.binaryFiles["retaddr_nested.ho"] = buildModuleBytes("retaddr_nested", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("retaddr_nested.ho")) << jit.getLastError();
    // inner returns 5, helper adds 1 => 6, main adds 10 => 16
    EXPECT_EQ(jit.run("_F_main_v"), 16) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, RunFailsWithNonexistentEntryPoint) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 42}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["missing_entry.ho"] = buildModuleBytes("missing_entry", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("missing_entry.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_nonexistent_v"), -1);
    EXPECT_TRUE(jit.hasError());
    const auto err = jit.getLastError();
    EXPECT_NE(err.find("Function symbol not found"), std::string::npos)
        << "Unexpected error: " << err;
}

TEST(HooExceptionSetCurrentTest, SetCurrentReleasesPrevious) {
    HooException exc1 = hoo_exception_create(HOO_EXCEPTION_RUNTIME, "first");
    HooException exc2 = hoo_exception_create(HOO_EXCEPTION_RUNTIME, "second");
    ASSERT_NE(exc1, nullptr);
    ASSERT_NE(exc2, nullptr);

    hoo_exception_set_current(exc1);
    EXPECT_EQ(hoo_exception_current(), exc1);

    hoo_exception_set_current(exc2);
    EXPECT_EQ(hoo_exception_current(), exc2);

    hoo_exception_clear();
    EXPECT_EQ(hoo_exception_current(), nullptr);
}

TEST(HooExceptionSetCurrentTest, SetCurrentNullClearsWithoutRelease) {
    HooException exc = hoo_exception_create(HOO_EXCEPTION_RUNTIME, "test");
    ASSERT_NE(exc, nullptr);

    hoo_exception_set_current(exc);
    ASSERT_EQ(hoo_exception_current(), exc);

    hoo_exception_set_current(nullptr);
    EXPECT_EQ(hoo_exception_current(), nullptr);
}

TEST(HooExceptionSetCurrentTest, SetCurrentSameObjectNoDoubleRelease) {
    HooException exc = hoo_exception_create(HOO_EXCEPTION_RUNTIME, "same");
    ASSERT_NE(exc, nullptr);

    hoo_exception_set_current(exc);
    EXPECT_EQ(hoo_exception_current(), exc);

    hoo_exception_set_current(exc);
    EXPECT_EQ(hoo_exception_current(), exc);

    hoo_exception_clear();
    EXPECT_EQ(hoo_exception_current(), nullptr);
}

// ---------------------------------------------------------------------------
// SYSCALL 12-23 execution tests
// ---------------------------------------------------------------------------

TEST_F(HVMJITInstructionSemanticsTest, SyscallGetTid) {
    // syscall 15 (GetTid): no args, returns thread ID (> 0)
    std::vector<HVMInstruction> ins{
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 15}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 0}),
        makeR(Opcode::CMP, OperandsR{1, 4, 5, 1}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_gettid.ho"] = buildModuleBytes("sys_gettid", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_gettid.ho")) << jit.getLastError();
    int64_t result = jit.run("_F_main_v");
    EXPECT_GT(result, 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallGetRandom) {
    // syscall 23 (GetRandom): r2=buf, r3=len -> returns bytes written
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeR(Opcode::MOV, OperandsR{2, 30, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 8}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 23}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 8}),
        makeR(Opcode::CMP, OperandsR{1, 4, 5, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_getrandom.ho"] = buildModuleBytes("sys_getrandom", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_getrandom.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallClockGetTime) {
    // syscall 22 (ClockGetTime): r2=clk_id, r3=ts_ptr -> returns 0 on success
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),
        makeR(Opcode::MOV, OperandsR{3, 30, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 22}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 0}),
        makeR(Opcode::CMP, OperandsR{1, 4, 5, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_clock.ho"] = buildModuleBytes("sys_clock", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_clock.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallFutexReturnsUnimplemented) {
    // syscall 14 (Futex): stub returns -1 (ENOSYS)
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{1, 0, 14}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_futex.ho"] = buildModuleBytes("sys_futex", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_futex.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallFileWriteToStdout) {
    // syscall 18 (Write): r2=fd(1), r3=buf, r4=count -> returns bytes written
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 0x4849}),
        makeI(Opcode::ST_H, OperandsI{5, 30, -8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),
        makeI(Opcode::LDA, OperandsI{3, 30, -8}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 2}),
        makeI(Opcode::SYSCALL, OperandsI{6, 0, 18}),
        makeI(Opcode::MOVZ, OperandsI{7, 0, 2}),
        makeR(Opcode::CMP, OperandsR{1, 6, 7, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_write.ho"] = buildModuleBytes("sys_write", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_write.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SyscallFileIOWithDiskFile) {
    std::remove("t");

    // Create "t", write "HI", close, re-open read-only, read 2 bytes, close.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 64}),

        // Build path "t\0" at r30-24 using ST.H
        makeI(Opcode::MOVZ, OperandsI{5, 0, 116}),
        makeI(Opcode::ST_H, OperandsI{5, 30, -24}),

        // Open for writing: O_RDWR|O_CREAT|O_TRUNC = 0x242 (Linux-style flags),
        // where O_RDWR=2, O_CREAT=0x40, O_TRUNC=0x200.
        makeI(Opcode::LDA, OperandsI{2, 30, -24}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 578}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 420}),
        makeI(Opcode::SYSCALL, OperandsI{6, 0, 16}),
        // Check open succeeded: fd should be > 0
        makeI(Opcode::MOVZ, OperandsI{7, 0, 0}),
        makeR(Opcode::CMP, OperandsR{8, 6, 7, 2}),   // r8 = (fd < 0) signed
        makeI(Opcode::MOVZ, OperandsI{9, 0, 0}),
        makeR(Opcode::CMP, OperandsR{9, 8, 9, 0}),   // r9 = (r8 == 0) = fd >= 0

        // Write "HI" (0x4948) from r30-16
        makeI(Opcode::MOVZ, OperandsI{10, 0, 0x4948}),
        makeI(Opcode::ST_H, OperandsI{10, 30, -16}),
        makeR(Opcode::MOV, OperandsR{2, 6, 0, 0}),
        makeI(Opcode::LDA, OperandsI{3, 30, -16}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 2}),
        makeI(Opcode::SYSCALL, OperandsI{10, 0, 18}),  // write -> r10
        makeI(Opcode::MOVZ, OperandsI{11, 0, 2}),
        makeR(Opcode::CMP, OperandsR{11, 10, 11, 0}),  // r11 = (wrote == 2)

        // Close
        makeR(Opcode::MOV, OperandsR{2, 6, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{12, 0, 19}),

        // Re-open read-only
        makeI(Opcode::LDA, OperandsI{2, 30, -24}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{6, 0, 16}),

        // Read 2 bytes into r30-32
        makeR(Opcode::MOV, OperandsR{2, 6, 0, 0}),
        makeI(Opcode::LDA, OperandsI{3, 30, -32}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 2}),
        makeI(Opcode::SYSCALL, OperandsI{13, 0, 17}),  // read -> r13
        makeI(Opcode::MOVZ, OperandsI{14, 0, 2}),
        makeR(Opcode::CMP, OperandsR{14, 13, 14, 0}),  // r14 = (read == 2)

        // Close
        makeR(Opcode::MOV, OperandsR{2, 6, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{15, 0, 19}),

        // Combine: r1 = fd_ok + wrote2 + read2 (should be 3)
        makeR(Opcode::ARITH, OperandsR{1, 9, 11, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 1, 14, 0}),

        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };

    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sys_fileio.ho"] = buildModuleBytes("sys_fileio", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sys_fileio.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 3) << jit.getLastError();
    std::remove("t");
}

TEST_F(HVMJITInstructionSemanticsTest, LdStPairRoundTrip) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 66}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 4660}),
        makeI(Opcode::LDA, OperandsI{4, 30, -16}),
        makeR(Opcode::ST_P, OperandsR{2, 3, 4, 0}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 0}),
        makeR(Opcode::LD_P, OperandsR{5, 5, 4, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 5, 0, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["ldstpair.ho"] = buildModuleBytes("ldstpair", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("ldstpair.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 66) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, LdPairMisalignedAddress) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 7}),
        makeI(Opcode::LDA, OperandsI{3, 30, 0}),
        makeR(Opcode::ARITH, OperandsR{4, 3, 2, 0}),
        makeR(Opcode::LD_P, OperandsR{5, 5, 4, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["ldp_malign.ho"] = buildModuleBytes("ldp_malign", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("ldp_malign.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, StPairMisalignedAddress) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 7}),
        makeI(Opcode::LDA, OperandsI{3, 30, 0}),
        makeR(Opcode::ARITH, OperandsR{4, 3, 2, 0}),
        makeR(Opcode::ST_P, OperandsR{2, 3, 4, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["stp_malign.ho"] = buildModuleBytes("stp_malign", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("stp_malign.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, LdPairPairOverflowRejected) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::LDA, OperandsI{2, 30, -16}),
        makeR(Opcode::LD_P, OperandsR{3, 31, 2, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["ldp_overflow.ho"] = buildModuleBytes("ldp_overflow", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("ldp_overflow.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, UnsignedCmpLessThan) {
    // CMPULT (func=4): 200 < 10 should be false (0) for unsigned bytes
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 200}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 10}),
        makeR(Opcode::CMP, OperandsR{4, 2, 3, 4}),   // r4 = (r2 <u r3) unsigned
        makeR(Opcode::ARITH, OperandsR{1, 4, 0, 0}),  // r1 = r4 + 0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["cmpult_lt.ho"] = buildModuleBytes("cmpult_lt", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cmpult_lt.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, UnsignedCmpGreater) {
    // CMPULT (func=4) swapped: 10 < 200 should be true (1) for unsigned
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 200}),
        makeR(Opcode::CMP, OperandsR{4, 2, 3, 4}),   // r4 = (r2 <u r3) unsigned
        makeR(Opcode::ARITH, OperandsR{1, 4, 0, 0}),  // r1 = r4 + 0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["cmpult_gt.ho"] = buildModuleBytes("cmpult_gt", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cmpult_gt.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, UnsignedCmpLessEqual) {
    // CMPULE (func=5): 200 <= 200 should be true (1)
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 200}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 200}),
        makeR(Opcode::CMP, OperandsR{4, 2, 3, 5}),   // r4 = (r2 <=u r3) unsigned
        makeR(Opcode::ARITH, OperandsR{1, 4, 0, 0}),  // r1 = r4 + 0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["cmpule_eq.ho"] = buildModuleBytes("cmpule_eq", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cmpule_eq.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, UnsignedCmpLessEqualFalse) {
    // CMPULE (func=5): 200 <= 10 should be false (0)
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 200}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 10}),
        makeR(Opcode::CMP, OperandsR{4, 2, 3, 5}),   // r4 = (r2 <=u r3) unsigned
        makeR(Opcode::ARITH, OperandsR{1, 4, 0, 0}),  // r1 = r4 + 0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["cmpule_false.ho"] = buildModuleBytes("cmpule_false", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cmpule_false.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, NativeByteUnsignedComparisonsUseLowEightBits) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 200}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 10}),
        makeR(Opcode::CMP_B, OperandsR{4, 2, 3, 4}), // 200 <u 10 == false
        makeR(Opcode::CMP_B, OperandsR{5, 3, 2, 4}), // 10 <u 200 == true
        makeR(Opcode::ARITH, OperandsR{1, 4, 5, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["cmp_b_unsigned.ho"] = buildModuleBytes("cmp_b_unsigned", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cmp_b_unsigned.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HVMJITInstructionSemanticsTest, ReleaseNullReturnsZero) {
    std::vector<HVMInstruction> ins{
        makeR(Opcode::RELEASE, OperandsR{1, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["release_null.ho"] = buildModuleBytes("release_null", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("release_null.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, ReleaseFinalReturnsOne) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}),
        makeR(Opcode::RELEASE, OperandsR{1, 4, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["release_final.ho"] = buildModuleBytes("release_final", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("release_final.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, ReleaseNonFinalReturnsZero) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeI(Opcode::SYSCALL, OperandsI{4, 0, 1}),
        makeR(Opcode::RETAIN, OperandsR{5, 4, 0, 0}),
        makeR(Opcode::RELEASE, OperandsR{1, 4, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["release_nonfinal.ho"] = buildModuleBytes("release_nonfinal", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("release_nonfinal.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, ReleaseNullPointerReturnsZero) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),          // r2 = 0 (null)
        makeR(Opcode::RELEASE, OperandsR{1, 2, 0, 0}),   // release(r0) -> rd = 0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["release_null.ho"] = buildModuleBytes("release_null", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("release_null.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, RetainNullPointerReturnsZero) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),          // r2 = 0 (null)
        makeR(Opcode::RETAIN, OperandsR{1, 2, 0, 0}),    // retain(r0) -> rd = 0 (no header access)
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["retain_null.ho"] = buildModuleBytes("retain_null", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("retain_null.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, LoadReserveLoadsValue) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 42}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeI(Opcode::LDA, OperandsI{4, 30, -16}),
        makeR(Opcode::ST_P, OperandsR{2, 3, 4, 0}),
        makeR(Opcode::LR_D, OperandsR{5, 4, 0, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 5, 0, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lr_load.ho"] = buildModuleBytes("lr_load", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lr_load.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 42) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, CSRRWRoundTripUsesHvmCsrWindow) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 42}),
        makeI(Opcode::CSRRW, OperandsI{1, 2, 5}),
        makeI(Opcode::CSRRW, OperandsI{1, 0, 5}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["csrrw.ho"] = buildModuleBytes("csrrw", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("csrrw.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 42) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, Feature0ReadableReportsImplementedFeatures) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::CSRRW, OperandsI{1, 0, 8}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["feature0_read.ho"] = buildModuleBytes("feature0_read", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("feature0_read.ho")) << jit.getLastError();
    // Hosted profile implements base core + extensions (bits 0..9, 11); HVM-A (bit 10) is not.
    EXPECT_EQ(jit.run("_F_main_v"), static_cast<int64_t>(HVMJIT::kHostedFeature0))
        << jit.getLastError();
    EXPECT_NE(HVMJIT::kHostedFeature0 & HVMJIT::kFeatureSiliconMvp, 0ULL);
}

TEST_F(HVMJITInstructionSemanticsTest, Feature0WriteIsIgnored) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, -1}),
        makeI(Opcode::CSRRW, OperandsI{1, 2, 8}),      // attempt write to feature0
        makeI(Opcode::CSRRW, OperandsI{1, 0, 8}),      // read feature0 back
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["feature0_wo.ho"] = buildModuleBytes("feature0_wo", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("feature0_wo.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), static_cast<int64_t>(HVMJIT::kHostedFeature0))
        << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, CsrAddressOutsideWindowTraps) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::CSRRW, OperandsI{1, 0, 12}), // 0x00C is outside 0x000..0x00B
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["csr_oor.ho"] = buildModuleBytes("csr_oor", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("csr_oor.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Invalid HVM CSR address"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, CsrResetValuesMatchSpecification) {
    // After reset, writable CSRs must read zero and satp must be Bare (=0)
    // (docs/hvm/hvm-spec.md section 9.2 reset values). feature0 is covered
    // by its own test; here we verify satp and that the scalar CSRs sum to 0.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::CSRRW, OperandsI{10, 0, 0}),  // sstatus
        makeI(Opcode::CSRRW, OperandsI{11, 0, 1}),  // stvec
        makeI(Opcode::CSRRW, OperandsI{12, 0, 2}),  // sepc
        makeI(Opcode::CSRRW, OperandsI{13, 0, 3}),  // scause
        makeI(Opcode::CSRRW, OperandsI{14, 0, 4}),  // stval
        makeI(Opcode::CSRRW, OperandsI{15, 0, 5}),  // satp    -> 0 (Bare)
        makeI(Opcode::CSRRW, OperandsI{16, 0, 6}),  // stime
        makeI(Opcode::CSRRW, OperandsI{17, 0, 7}),  // stimecmp
        makeI(Opcode::CSRRW, OperandsI{18, 0, 9}),  // bad_instruction
        makeI(Opcode::CSRRW, OperandsI{19, 0, 10}), // sip
        makeI(Opcode::CSRRW, OperandsI{20, 0, 11}), // sie
        // Fold the scalar CSRs into r1: expect 0 at reset.
        makeR(Opcode::ARITH, OperandsR{21, 10, 11, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 12, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 13, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 14, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 15, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 16, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 17, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 18, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 19, 0}),
        makeR(Opcode::ARITH, OperandsR{21, 21, 20, 0}),
        makeR(Opcode::MOV, OperandsR{1, 21, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["csr_reset.ho"] = buildModuleBytes("csr_reset", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("csr_reset.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SatpResetsBare) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::CSRRW, OperandsI{1, 0, 5}),  // satp -> 0 (Bare)
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["satp_reset.ho"] = buildModuleBytes("satp_reset", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("satp_reset.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, RdprofReturnsZeroInHostedProfile) {
    // HVM-Prof (RDPROF) is optional; the hosted profile exposes no profiling
    // counters, so the hosted interpreter and JIT both return rd = 0. This is
    // not a trap or a fault -- counters are simply unavailable. A future
    // platform profile that exposes counters may return nonzero values.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::RDPROF, OperandsI{1, 0, 0}),    // rd = r1, selector r0, imm15=0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["rdprof.ho"] = buildModuleBytes("rdprof", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("rdprof.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
    EXPECT_FALSE(jit.hasError());
}

TEST_F(HVMJITInstructionSemanticsTest, DoorbellTrapsWhenHvmANotAvailable) {
    // HVM-A is optional; the hosted profile clears feature0.Accel, so DOORBELL
    // is an unsupported instruction and traps (scause=2) without entering the
    // accelerator. The interpreter reports the precise reason (no partial effect
    // on registers or PC beyond the trap).
    std::vector<HVMInstruction> ins{
        makeR(Opcode::DOORBELL, OperandsR{0, 1, 2, 0}),      // packet mmio=1, index=2
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),             // must not execute
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["doorbell.ho"] = buildModuleBytes("doorbell", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("doorbell.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("DOORBELL"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, EcallTrapsUnhandledInHostedProfile) {
    // HVM resets into S-mode (section 9.2), so ECALL (not SYSCALL) is a legal
    // system-profile trap (scause 9) that the hosted profile cannot service:
    // there is no host-side S-mode monitor. It must surface as an unhandled
    // trap with no further instructions executed.
    std::vector<HVMInstruction> ins{
        makeR(Opcode::ECALL, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),   // must not execute
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["ecall.ho"] = buildModuleBytes("ecall", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("ecall.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("ECALL"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, BadInstructionCsrRecordsFaultingEcall) {
    HVMInstruction ecall(Opcode::ECALL, OperandsR{0, 0, 0, 0});
    ecall.setFormat(InstructionFormat::R);
    const uint64_t encoded = HVMJIT::encodeInstructionWord(ecall);

    std::vector<HVMInstruction> trapIns{
        makeR(Opcode::ECALL, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> trapSyms{funcSym("_F_main_v", 0)};
    io.binaryFiles["bad_ins_trap.ho"] = buildModuleBytes("bad_ins_trap", trapIns, trapSyms);
    HVMJIT trapJit(io);
    ASSERT_TRUE(trapJit.loadInput("bad_ins_trap.ho")) << trapJit.getLastError();
    EXPECT_EQ(trapJit.run("_F_main_v"), -1);
    EXPECT_TRUE(trapJit.hasError());
    auto csrs = trapJit.getCsrs();
    EXPECT_EQ(csrs[HVMJIT::kCsrScause], HVMJIT::kCauseEcallS);
    EXPECT_EQ(csrs[HVMJIT::kCsrBadInstruction], encoded);
    EXPECT_EQ(csrs[HVMJIT::kCsrStval], 0ULL);
}

TEST_F(HVMJITInstructionSemanticsTest, BadInstructionWriteIsIgnoredInHostedProfile) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, -1}),
        makeI(Opcode::CSRRW, OperandsI{1, 2, 9}),      // attempt write to bad_instruction
        makeI(Opcode::CSRRW, OperandsI{1, 0, 9}),      // read back
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["bad_ins_wo.ho"] = buildModuleBytes("bad_ins_wo", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("bad_ins_wo.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, SipSieRoundTrip) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 5}),       // STIP|SSIP bits 0+2? use 1|4 later
        makeI(Opcode::CSRRW, OperandsI{1, 2, 11}),     // write sie
        makeI(Opcode::CSRRW, OperandsI{1, 0, 11}),     // read sie
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sie_rw.ho"] = buildModuleBytes("sie_rw", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sie_rw.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 5) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, TrapretTrapsUnhandledInHostedProfile) {
    // TRAPRET returns from S-mode; the hosted profile has no S-mode handler,
    // so it surfaces as an unhandled system-profile trap (scause 2) instead of
    // performing a privilege transition.
    std::vector<HVMInstruction> ins{
        makeR(Opcode::TRAPRET, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),   // must not execute
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["trapret.ho"] = buildModuleBytes("trapret", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("trapret.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("TRAPRET"), std::string::npos);
}
TEST_F(HVMJITInstructionSemanticsTest, CmpUnsignedLessThan) {
    std::vector<HVMInstruction> ins{
        // r2 = -1 (0xFFFF...FFFF), r3 = 1, r4 = 0
        makeI(Opcode::MOVZ, OperandsI{2, 0, -1}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 1}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 0}),
        makeR(Opcode::CMP, OperandsR{5, 2, 3, 4}),   // cmpltu r5, r2, r3  -> 0xFFFF... < 1 = false
        makeR(Opcode::CMP, OperandsR{6, 3, 4, 4}),   // cmpltu r6, r3, r4  -> 1 < 0 = false
        makeR(Opcode::CMP, OperandsR{7, 4, 3, 4}),   // cmpltu r7, r4, r3  -> 0 < 1 = true
        // Fold all three results: r1 = r5 + r6 + r7 = 0 + 0 + 1 = 1
        makeR(Opcode::ARITH, OperandsR{8, 5, 6, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 8, 7, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["cmpltu.ho"] = buildModuleBytes("cmpltu", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cmpltu.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, CmpUnsignedLessThanEqual) {
    std::vector<HVMInstruction> ins{
        // r2 = -1 (0xFFFF...FFFF), r3 = 1, r4 = 1
        makeI(Opcode::MOVZ, OperandsI{2, 0, -1}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 1}),
        makeI(Opcode::MOVZ, OperandsI{4, 0, 1}),
        makeR(Opcode::CMP, OperandsR{5, 2, 3, 5}),   // cmpleu r5, r2, r3  -> 0xFFFF... <= 1 = false
        makeR(Opcode::CMP, OperandsR{6, 3, 4, 5}),   // cmpleu r6, r3, r4  -> 1 <= 1 = true
        makeR(Opcode::CMP, OperandsR{7, 4, 2, 5}),   // cmpleu r7, r4, r2  -> 1 <= 0xFFFF... = true
        // Fold all three results: r1 = r5 + r6 + r7 = 0 + 1 + 1 = 2
        makeR(Opcode::ARITH, OperandsR{8, 5, 6, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 8, 7, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["cmpleu.ho"] = buildModuleBytes("cmpleu", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("cmpleu.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 2) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, StoreConditionalSuccess) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 42}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeI(Opcode::LDA, OperandsI{4, 30, -16}),
        makeR(Opcode::ST_P, OperandsR{2, 3, 4, 0}),
        makeR(Opcode::LR_D, OperandsR{5, 4, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{6, 0, 77}),
        makeR(Opcode::SC_D, OperandsR{7, 4, 6, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 7, 0, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sc_success.ho"] = buildModuleBytes("sc_success", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sc_success.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, StoreConditionalFailsNoReservation) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 42}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeI(Opcode::LDA, OperandsI{4, 30, -16}),
        makeR(Opcode::ST_P, OperandsR{2, 3, 4, 0}),
        // No LR.D — reservation is UINT64_MAX, won't match r4
        makeI(Opcode::MOVZ, OperandsI{6, 0, 77}),
        makeR(Opcode::SC_D, OperandsR{7, 4, 6, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 7, 0, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sc_no_res.ho"] = buildModuleBytes("sc_no_res", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sc_no_res.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, StoreConditionalValueWritten) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::ENTER, OperandsI{0, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 42}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeI(Opcode::LDA, OperandsI{4, 30, -16}),
        makeR(Opcode::ST_P, OperandsR{2, 3, 4, 0}),
        makeR(Opcode::LR_D, OperandsR{5, 4, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{6, 0, 99}),
        makeR(Opcode::SC_D, OperandsR{7, 4, 6, 0}),
        // Now verify via LR.D that 99 was written
        makeR(Opcode::LR_D, OperandsR{8, 4, 0, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 8, 0, 0}),
        makeR(Opcode::LEAVE, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["sc_value.ho"] = buildModuleBytes("sc_value", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("sc_value.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 99) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, AllocBumpReturnsZeroNoTLAB) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeI(Opcode::ALLOC_BUMP, OperandsI{1, 2, 8}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["allocbump_zero.ho"] = buildModuleBytes("allocbump_zero", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("allocbump_zero.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, AllocBumpTLABHit) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 64}),
        makeI(Opcode::ALLOC_BUMP, OperandsI{1, 2, 8}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["allocbump_hit.ho"] = buildModuleBytes("allocbump_hit", ins, syms);

    HVMJIT jit(io);
    jit.setTLAB(4096, 8192);
    ASSERT_TRUE(jit.loadInput("allocbump_hit.ho")) << jit.getLastError();
    int64_t result = jit.run("_F_main_v");
    EXPECT_GT(result, 0) << jit.getLastError();
    EXPECT_GE(result, 4096);
    EXPECT_LT(result, 8192);
}

TEST_F(HVMJITInstructionSemanticsTest, AllocBumpAlignment) {
    // With tlabStart at 4097 (misaligned), alignment=8 should round up to 4104
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),
        makeI(Opcode::ALLOC_BUMP, OperandsI{1, 2, 8}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["allocbump_align.ho"] = buildModuleBytes("allocbump_align", ins, syms);

    HVMJIT jit(io);
    jit.setTLAB(4097, 8192);
    ASSERT_TRUE(jit.loadInput("allocbump_align.ho")) << jit.getLastError();
    int64_t result = jit.run("_F_main_v");
    EXPECT_EQ(result, 4104);  // (4097 + 7) & ~7 = 4104
}

TEST_F(HVMJITInstructionSemanticsTest, AllocBumpTLABExhausted) {
    // Set TLAB to just 8 bytes — second ALLOC.BUMP should return 0
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),
        makeI(Opcode::ALLOC_BUMP, OperandsI{3, 2, 8}),  // first alloc: fits, returns non-zero
        makeI(Opcode::ALLOC_BUMP, OperandsI{1, 2, 8}),  // second alloc: TLAB exhausted, returns 0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["allocbump_exhaust.ho"] = buildModuleBytes("allocbump_exhaust", ins, syms);

    HVMJIT jit(io);
    jit.setTLAB(4096, 4112);  // Exactly 16 bytes of TLAB (fits one 16-byte alloc)
    ASSERT_TRUE(jit.loadInput("allocbump_exhaust.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

// ── HVM-V Vector Extension Tests ─────────────────────────────────

TEST_F(HVMJITInstructionSemanticsTest, VectorSetvlReturnsMinOfAvlAndMaxvl) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    // r2 = 4 (AVL < MAXVL=8)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),    // r3 = 0 (vtype=int64)
        makeR(Opcode::VSETVL, OperandsR{1, 2, 3, 0}), // r1 = vl = min(4,8) = 4
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vsetvl_basic.ho"] = buildModuleBytes("vsetvl_basic", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vsetvl_basic.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 4) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorSetvlCapsAtMaxvl) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 128}),  // r2 = 128 (AVL > MAXVL=8)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),    // r3 = 0
        makeR(Opcode::VSETVL, OperandsR{1, 2, 3, 0}), // r1 = vl = min(128,8) = 8
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vsetvl_cap.ho"] = buildModuleBytes("vsetvl_cap", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vsetvl_cap.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 8) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorSetvlCarriesVtypeVerbatim) {
    // VSETVL returns vl = min(avl, VLMAX=8); vtype is carried verbatim from rs2
    // and must be accepted (no trap) even for float-selecting values (2, 9).
    // The hosted profile caps VLMAX at 8 regardless of vtype.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    // r2 = 3 (AVL < MAXVL)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 2}),    // r3 = 2 (float: double)
        makeR(Opcode::VSETVL, OperandsR{1, 2, 3, 0}), // r1 = vl = min(3,8) = 3
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vsetvl_vtype.ho"] = buildModuleBytes("vsetvl_vtype", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vsetvl_vtype.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 3) << jit.getLastError();
    EXPECT_FALSE(jit.hasError());
}

TEST_F(HVMJITInstructionSemanticsTest, HardwareLoopCountsAndBranches) {
    // LOOP.SET records the count; LOOP.DECBR decrements it and branches back
    // to the increment while the result remains nonzero. The loop-set
    // displacement is metadata; the LOOP.DECBR displacement (-1) is the
    // encoded branch target used by execution.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 3}),       // count = 3
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),       // accumulator = 0
        makeI(Opcode::LOOP_SET, OperandsI{0, 1, -4}),  // record loop state
        makeI(Opcode::ADDI, OperandsI{2, 2, 1}),       // accumulator++
        makeB(Opcode::LOOP_DECBR, OperandsB{0, 0, -1}), // back to ADDI
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["hardware_loop.ho"] = buildModuleBytes("hardware_loop", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("hardware_loop.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 3) << jit.getLastError();
    EXPECT_FALSE(jit.hasError());
}

TEST_F(HVMJITInstructionSemanticsTest, LoadReserveStoreConditionalUsesGranule) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 64}),       // address
        makeI(Opcode::MOVZ, OperandsI{3, 0, 7}),        // value
        makeI(Opcode::ST_D, OperandsI{3, 2, 0}),        // initialize memory
        makeR(Opcode::LR_D, OperandsR{4, 2, 0, 0}),      // reserve address 64
        makeR(Opcode::SC_D, OperandsR{1, 2, 3, 0}),     // succeeds: rd = 0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lrsc_success.ho"] = buildModuleBytes("lrsc_success", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lrsc_success.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, StoreInvalidatesLoadReservation) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 64}),       // address
        makeI(Opcode::MOVZ, OperandsI{3, 0, 7}),        // value
        makeI(Opcode::ST_D, OperandsI{3, 2, 0}),        // initialize memory
        makeR(Opcode::LR_D, OperandsR{4, 2, 0, 0}),      // reserve address 64
        makeI(Opcode::ST_D, OperandsI{3, 2, 0}),        // intervening same-value store
        makeR(Opcode::SC_D, OperandsR{1, 2, 3, 0}),     // fails: rd = 1
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lrsc_invalidate.ho"] = buildModuleBytes("lrsc_invalidate", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lrsc_invalidate.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, LoadReserveMisalignmentTraps) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 65}),       // misaligned address
        makeR(Opcode::LR_D, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lrsc_misaligned.ho"] = buildModuleBytes("lrsc_misaligned", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lrsc_misaligned.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Unaligned LR.D"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, VectorLoadStoreUnitStrideRoundtrip) {
    // Store [10,20,30,40] starting at mem[0], load into v0, store to mem[64],
    // then load scalar at mem[64] (should be 10) and mem[72] (should be 20).
    // Return 10 + 20 = 30 to verify both positions.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),
        makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),
        makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),
        makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),
        makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),     // AVL=4
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),     // vtype=int64
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}), // vld.v v0, (r0) — load from addr 0
        makeI(Opcode::MOVZ, OperandsI{2, 0, 64}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 2, 0, 1}), // vst.v v0, (r2) — store to addr 64

        makeI(Opcode::LD_D, OperandsI{3, 2, 0}),     // r3 = mem[64]
        makeI(Opcode::LD_D, OperandsI{4, 2, 8}),     // r4 = mem[72]
        makeR(Opcode::ARITH, OperandsR{1, 3, 4, 0}), // r1 = r3 + r4
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vload_store.ho"] = buildModuleBytes("vload_store", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vload_store.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 30) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorAddVV) {
    // v0 = [10,20,30,40], v1 = [1,2,3,4], v2 = v0 + v1
    // store v2 to mem[128], load element 0 (11) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),   makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    // AVL=4
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),    // vtype=int64
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),  // v0 = mem[0..31]
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),  // v1 = mem[32..63]
        makeR(Opcode::VECTOR_ARITH, OperandsR{2, 0, 1, 0}), // v2 = v0 + v1 (func=0 = vv add)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),  // store v2 to mem[128]
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = mem[128] = 10+1 = 11
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vadd_vv.ho"] = buildModuleBytes("vadd_vv", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vadd_vv.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 11) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorAddVX) {
    // v0 = [10,20,30,40], r5 = 100, v2 = v0 + r5 (vector-scalar)
    // store v2 to mem[128], load element 0 (110) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),   makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 100}),  // r5 = 100 (scalar)
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    // AVL=4
        makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),    // vtype=int64
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),  // v0 = mem[0..31]
        makeR(Opcode::VECTOR_ARITH, OperandsR{2, 0, 5, 1}), // v2 = v0 + r5 (func=1 = vx add)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),  // store v2 to mem[128]
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = mem[128] = 10+100 = 110
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vadd_vx.ho"] = buildModuleBytes("vadd_vx", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vadd_vx.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 110) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorMulVV) {
    // v0 = [5,10,15,20], v1 = [2,3,4,5], v2 = v0 * v1
    // store v2 to mem[128], load element 2 (15*4=60) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 5}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 15}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 5}),    makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::VECTOR_ARITH, OperandsR{2, 0, 1, 4}), // v2 = v0 * v1 (func=4 = vv mul)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 16}),   // r1 = mem[144] = 15*4 = 60
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vmul_vv.ho"] = buildModuleBytes("vmul_vv", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vmul_vv.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 60) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorFmaVV) {
    // v0 = [1,2,3,4] (accumulator), v1 = [5,6,7,8], v2 = [2,3,4,5]
    // v0 = v0 + v1 * v2 = [1+5*2, 2+6*3, 3+7*4, 4+8*5] = [11,20,31,44]
    // store v0 to mem[128], load element 1 (20) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 5}),    makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 6}),    makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 7}),    makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 8}),    makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 64}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 72}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 80}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 5}),    makeI(Opcode::ST_D, OperandsI{2, 0, 88}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),  // v0 = accum
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),  // v1 = multiplicand
        makeI(Opcode::MOVZ, OperandsI{2, 0, 64}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 2, 0, 0}),  // v2 = multiplier
        makeR(Opcode::VECTOR_FMA, OperandsR{0, 1, 2, 0}),  // v0 += v1 * v2 (func=0 = vv fma)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 8}),    // r1 = mem[136] = 2+6*3 = 20
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vfma_vv.ho"] = buildModuleBytes("vfma_vv", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vfma_vv.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 20) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorFmaVF) {
    // v0 = [1,2,3,4] (accumulator), v1 = [5,6,7,8], r6 = 3 (scalar multiplier)
    // v0 = v0 + v1 * 3 = [1+5*3, 2+6*3, 3+7*3, 4+8*3] = [16,20,24,28]
    // load element 2 (24) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 5}),    makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 6}),    makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 7}),    makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 8}),    makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{6, 0, 3}),    // r6 = 3 (scalar)
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::VECTOR_FMA, OperandsR{0, 1, 6, 1}),  // v0 += v1 * r6 (func=1 = vf fma)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 16}),   // r1 = mem[144] = 3+7*3 = 24
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vfma_vf.ho"] = buildModuleBytes("vfma_vf", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vfma_vf.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 24) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorReduceSum) {
    // v0 = [10,20,30,40], reduce-sum into v1[0] = 100
    // store v1 to mem[64], load element 0 into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),   makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),  // v0 = [10,20,30,40]
        makeR(Opcode::VECTOR_REDUCE, OperandsR{1, 0, 0, 0}), // v1[0] = sum(v0) (func=0)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 64}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = mem[64] = 100
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vred_sum.ho"] = buildModuleBytes("vred_sum", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vred_sum.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 100) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorReduceMin) {
    // v0 = [42,7,99,2], reduce-min into v1[0] = 2
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 42}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 7}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 99}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::VECTOR_REDUCE, OperandsR{1, 0, 0, 1}), // v1[0] = min(v0) (func=1)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 64}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = 2
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vred_min.ho"] = buildModuleBytes("vred_min", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vred_min.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 2) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorReduceMax) {
    // v0 = [42,7,99,2], reduce-max into v1[0] = 99
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 42}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 7}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 99}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::VECTOR_REDUCE, OperandsR{1, 0, 0, 2}), // v1[0] = max(v0) (func=2)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 64}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = 99
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vred_max.ho"] = buildModuleBytes("vred_max", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vred_max.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 99) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorShiftLeftVV) {
    // v0 = [1,2,3,4], v1 = [1,2,3,4] (shift amounts)
    // v2 = v0 << v1 = [2,8,24,64]
    // load element 3 (4<<4 = 64) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::VECTOR_SHIFT, OperandsR{2, 0, 1, 0}), // v2 = v0 << v1 (func=0 = vv shift)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 24}),   // r1 = mem[152] = 4<<4 = 64
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vsll_vv.ho"] = buildModuleBytes("vsll_vv", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vsll_vv.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 64) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorShiftLeftVX) {
    // v0 = [1,2,3,4], r5 = 2 (scalar shift amount)
    // v2 = v0 << r5 = [4,8,12,16]
    // load element 1 (2<<2 = 8) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 2}),    // r5 = 2 (shift amount)
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::VECTOR_SHIFT, OperandsR{2, 0, 5, 1}), // v2 = v0 << r5 (func=1 = vx shift)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 8}),    // r1 = mem[136] = 2<<2 = 8
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vsll_vx.ho"] = buildModuleBytes("vsll_vx", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vsll_vx.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 8) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorShiftRightLogicalVX) {
    // v0 = [64,128,256,512], r5 = 2 (scalar shift)
    // v2 = v0 >> r5 = [16,32,64,128]
    // load element 2 (256>>2 = 64) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 64}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 128}),  makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 256}),  makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 512}),  makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 2}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::VECTOR_SHIFT, OperandsR{2, 0, 5, 3}), // v2 = v0 >> r5 (func=3 = vx srl)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 16}),   // r1 = 256>>2 = 64
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vsrl_vx.ho"] = buildModuleBytes("vsrl_vx", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vsrl_vx.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 64) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorBitwiseXorVV) {
    // v0 = [0xFF, 0x0F, 0xAA, 0x55], v1 = [0x55, 0xF0, 0x55, 0xAA]
    // v2 = v0 ^ v1 = [0xAA, 0xFF, 0xFF, 0xFF]
    // load element 0 (0xFF ^ 0x55 = 0xAA = 170) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xFF}), makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x0F}), makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xAA}), makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x55}), makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x55}), makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xF0}), makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x55}), makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xAA}), makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::VECTOR_BITWISE, OperandsR{2, 0, 1, 2}), // v2 = v0 ^ v1 (func=2 = xor)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = 0xFF^0x55 = 0xAA = 170
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vxor_vv.ho"] = buildModuleBytes("vxor_vv", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vxor_vv.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 170) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorBitwiseAndVV) {
    // v0 = [0xFF, 0x0F, 0xAA, 0x55], v1 = [0x55, 0xF0, 0x55, 0xAA]
    // v2 = v0 & v1 = [0x55, 0x00, 0x00, 0x00]
    // load element 1 (0x0F & 0xF0 = 0) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xFF}), makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x0F}), makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xAA}), makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x55}), makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x55}), makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xF0}), makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x55}), makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xAA}), makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::VECTOR_BITWISE, OperandsR{2, 0, 1, 0}), // v2 = v0 & v1 (func=0 = and)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 8}),    // r1 = 0x0F&0xF0 = 0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vand_vv.ho"] = buildModuleBytes("vand_vv", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vand_vv.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 0) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorBitwiseOrVV) {
    // v0 = [0xF0, 0x0F, 0xAA, 0x55], v1 = [0x0F, 0xF0, 0x55, 0xAA]
    // v2 = v0 | v1 = [0xFF, 0xFF, 0xFF, 0xFF]
    // load element 3 (0x55|0xAA = 0xFF = 255) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xF0}), makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x0F}), makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xAA}), makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x55}), makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x0F}), makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xF0}), makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0x55}), makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0xAA}), makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::VECTOR_BITWISE, OperandsR{2, 0, 1, 1}), // v2 = v0 | v1 (func=1 = or)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 24}),   // r1 = 0x55|0xAA = 0xFF = 255
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vor_vv.ho"] = buildModuleBytes("vor_vv", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vor_vv.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 255) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorCompareMask) {
    // v0 = [1,2,3,4], v1 = [1,99,3,77]
    // vcomp.vv -> v2 = [1,0,1,0]
    // store v2 to mem[128], load element 1 (0) and element 2 (1), return them combined
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 99}),   makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 77}),   makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),
        makeR(Opcode::VECTOR_MASK, OperandsR{2, 0, 1, 0}), // v2 = v0 == v1 (func=0 = vcomp.vv)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{4, 3, 8}),    // r4 = mem[136] = elem 1 = 0
        makeI(Opcode::LD_D, OperandsI{5, 3, 16}),   // r5 = mem[144] = elem 2 = 1
        makeR(Opcode::ARITH, OperandsR{1, 4, 5, 0}), // r1 = 0 + 1 = 1
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vcomp_mask.ho"] = buildModuleBytes("vcomp_mask", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vcomp_mask.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorCompareVX) {
    // v0 = [10,20,30,40], r5 = 30
    // vcomp.vx -> v2 = [0,0,1,0]
    // load element 2 (1) into r1
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),   makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 30}),   // r5 = 30
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::VECTOR_MASK, OperandsR{2, 0, 5, 1}), // v2 = v0 == r5 (func=1 = vcomp.vx)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 16}),   // r1 = mem[144] = elem 2 = 1
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vcomp_vx.ho"] = buildModuleBytes("vcomp_vx", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vcomp_vx.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorMergeVvm) {
    // v0 = [1,2,3,4] (mask), v1 = [10,20,30,40] (true vals), v2 = [99,88,77,66] (false vals - stored in vd)
    // vmerge: for each i where mask[i]!=0, vd[i]=v1[i], else vd[i]=v2[i]
    // mask = [1,2,3,4], all non-zero -> result should be [10,20,30,40]
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 1}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 2}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 3}),    makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),   makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),   makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 99}),   makeI(Opcode::ST_D, OperandsI{2, 0, 64}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 88}),   makeI(Opcode::ST_D, OperandsI{2, 0, 72}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 77}),   makeI(Opcode::ST_D, OperandsI{2, 0, 80}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 66}),   makeI(Opcode::ST_D, OperandsI{2, 0, 88}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),  // v0 = mask
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),  // v1 = true vals
        makeI(Opcode::MOVZ, OperandsI{2, 0, 64}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 2, 0, 0}),  // v2 = false vals (vd)
        makeR(Opcode::VECTOR_MASK, OperandsR{2, 1, 0, 2}), // vmerge: v2 = mask(v0) ? v1 : v2
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{2, 3, 0, 1}),
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = mem[128] = 10 (mask[0]=1)
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vmerge.ho"] = buildModuleBytes("vmerge", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vmerge.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 10) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorFirstSetBit) {
    // v0 = [0,0,0,42], vfirst.m -> r1 = 3 (first non-zero element index)
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 42}),   makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::VECTOR_MASK, OperandsR{1, 0, 0, 3}), // vfirst.m r1, v0 (func=3)
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vfirst.ho"] = buildModuleBytes("vfirst", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vfirst.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 3) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorFirstSetBitAllZero) {
    // v0 = [0,0,0,0], vfirst.m -> r1 = -1 (no non-zero element)
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),
        makeR(Opcode::VECTOR_MASK, OperandsR{1, 0, 0, 3}), // vfirst.m r1, v0
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vfirst_zero.ho"] = buildModuleBytes("vfirst_zero", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vfirst_zero.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorLoadStoreStrided) {
    // Store values at addresses 0, 16, 32, 48 (stride=16) using ST_D at manual offsets
    // Load with stride=16 into v0, verify by storing unit-stride to new area and checking
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),   makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),   makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeI(Opcode::MOVZ, OperandsI{5, 0, 16}),   // r5 = 16 (stride)
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 5, 2}), // vlds.v v0, (r0), r5 (strided load)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 64}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 3, 0, 1}), // store v0 unit-stride to mem[64]
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = mem[64] = 10
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vlds.ho"] = buildModuleBytes("vlds", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vlds.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 10) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorLoadStoreIndexed) {
    // Store values [10,20,30,40] at addresses 64, 72, 80, 88 (8-byte aligned)
    // v0 = [0, 8, 16, 24] (offsets), base = 64
    // vldx.v v1, (r5), v0 loads: addr=64+0, 64+8, 64+16, 64+24 = [10,20,30,40]
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{5, 0, 64}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 5, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 5, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),   makeI(Opcode::ST_D, OperandsI{2, 5, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),   makeI(Opcode::ST_D, OperandsI{2, 5, 24}),
        // Store offset vector at mem[0]
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 8}),    makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 24}),   makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),  // v0 = offsets from mem[0]
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 5, 0, 4}),  // vldx.v v1, (r5), v0 (func=4)
        makeI(Opcode::MOVZ, OperandsI{3, 0, 128}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 3, 0, 1}),  // store v1 to mem[128]
        makeI(Opcode::LD_D, OperandsI{1, 3, 0}),    // r1 = mem[128] = first gathered = 10
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vldx.ho"] = buildModuleBytes("vldx", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vldx.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 10) << jit.getLastError();
}

TEST_F(HVMJITInstructionSemanticsTest, VectorStoreIndexed) {
    // v0 = [10,20,30,40], store via indexed store with offsets [0,8,16,24] at base=64
    // then read back each element individually
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 10}),   makeI(Opcode::ST_D, OperandsI{2, 0, 0}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),   makeI(Opcode::ST_D, OperandsI{2, 0, 8}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 30}),   makeI(Opcode::ST_D, OperandsI{2, 0, 16}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),   makeI(Opcode::ST_D, OperandsI{2, 0, 24}),
        // offset vector v1 = [0,8,16,24] at mem[32]
        makeI(Opcode::MOVZ, OperandsI{2, 0, 0}),    makeI(Opcode::ST_D, OperandsI{2, 0, 32}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 8}),    makeI(Opcode::ST_D, OperandsI{2, 0, 40}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 16}),   makeI(Opcode::ST_D, OperandsI{2, 0, 48}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 24}),   makeI(Opcode::ST_D, OperandsI{2, 0, 56}),
        makeI(Opcode::MOVZ, OperandsI{2, 0, 4}),    makeI(Opcode::MOVZ, OperandsI{3, 0, 0}),
        makeR(Opcode::VSETVL, OperandsR{4, 2, 3, 0}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 0, 0, 0}),  // v0 = [10,20,30,40]
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),
        makeR(Opcode::VECTOR_MEM, OperandsR{1, 2, 0, 0}),  // v1 = [0,8,16,24]
        makeI(Opcode::MOVZ, OperandsI{5, 0, 200}),
        makeR(Opcode::VECTOR_MEM, OperandsR{0, 5, 1, 5}),  // vstx.v v0, (r5), v1 (func=5)
        // read back: mem[200+0]=10, mem[200+8]=20, mem[200+16]=30, mem[200+24]=40
        makeI(Opcode::LD_D, OperandsI{6, 5, 0}),    // r6 = mem[200] = 10
        makeI(Opcode::LD_D, OperandsI{7, 5, 16}),   // r7 = mem[216] = 30
        makeR(Opcode::ARITH, OperandsR{1, 6, 7, 0}), // r1 = 10 + 30 = 40
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["vstx.ho"] = buildModuleBytes("vstx", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("vstx.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 40) << jit.getLastError();
}

// ---------------------------------------------------------------------------
// HVM-Cap: CHK.B bounds check
// ---------------------------------------------------------------------------

TEST_F(HVMJITInstructionSemanticsTest, ChkBBoundsPassReturnsPtr) {
    // ptr < bound: no trap, rd = ptr.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 16}),              // r1 = ptr = 16
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),              // r2 = bound = 32
        makeR(Opcode::CHK_B, OperandsR{1, 1, 2, 0}),           // rd=r1 <- r1, since 16 < 32
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["chkpass.ho"] = buildModuleBytes("chkpass", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("chkpass.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 16) << jit.getLastError();
    EXPECT_FALSE(jit.hasError());
}

TEST_F(HVMJITInstructionSemanticsTest, ChkBBoundsViolationTraps) {
    // ptr >= bound: instruction-address/bounds trap, no rd write.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 64}),              // r1 = ptr = 64
        makeI(Opcode::MOVZ, OperandsI{2, 0, 32}),              // r2 = bound = 32
        makeR(Opcode::CHK_B, OperandsR{1, 1, 2, 0}),           // 64 >= 32 -> trap
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["chktrap.ho"] = buildModuleBytes("chktrap", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("chktrap.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("bounds"), std::string::npos);
}

// ---------------------------------------------------------------------------
// HVM-NZ: LD.D.NZ null / alignment trap
// ---------------------------------------------------------------------------

TEST_F(HVMJITInstructionSemanticsTest, LdDnzNullTraps) {
    // Effective address 0 -> null-pointer throws catchable exception;
    // without a handler it fails as unhandled.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 0}),               // r1 = 0
        makeI(Opcode::LD_D_NZ, OperandsI{2, 1, 0}),            // addr = 0 -> throw
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lddnz_null.ho"] = buildModuleBytes("lddnz_null", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lddnz_null.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Unhandled exception trap"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, LdDnzMisalignedTraps) {
    // Effective address non-zero but 8-byte misaligned -> alignment trap.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 16}),              // r1 = 16 (aligned)
        makeI(Opcode::LD_D_NZ, OperandsI{2, 1, 1}),            // addr = 17 -> misaligned
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lddnz_mis.ho"] = buildModuleBytes("lddnz_mis", ins, syms);
    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lddnz_mis.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Unaligned"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, LdDnzNullWithHandlerIsCatchable) {
    // Push a handler with PC at the handler-start instruction, then
    // trigger LD_D_NZ with a null address; the null dereference must
    // dispatch through the catchable exception path and land in the
    // registered handler.  Mirror the frame-restore + exception-r1
    // verification of SyscallThrowTransfersControlToRegisteredHandler.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{30, 0, 111}),             // fp sentinel
        makeI(Opcode::MOVZ, OperandsI{31, 0, 222}),             // sp sentinel
        makeI(Opcode::MOVZ, OperandsI{2, 0, 40}),              // handler pc (byte offset)
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 7}),             // push handler
        makeI(Opcode::MOVZ, OperandsI{30, 0, 1}),               // clobber fp
        makeI(Opcode::MOVZ, OperandsI{31, 0, 2}),               // clobber sp
        makeI(Opcode::MOVZ, OperandsI{1, 0, 0}),                // r1 = 0 (null address)
        makeI(Opcode::LD_D_NZ, OperandsI{2, 1, 0}),             // addr = 0 -> throw null
        makeI(Opcode::MOVZ, OperandsI{1, 0, 99}),               // skipped
        makeI(Opcode::MOVZ, OperandsI{7, 0, 111}),              // handler start
        makeR(Opcode::CMP, OperandsR{8, 30, 7, 0}),             // fp restored?
        makeI(Opcode::MOVZ, OperandsI{9, 0, 222}),
        makeR(Opcode::CMP, OperandsR{10, 31, 9, 0}),            // sp restored?
        makeR(Opcode::CMP, OperandsR{11, 1, 0, 1}),             // exception in r1?
        makeR(Opcode::ARITH, OperandsR{1, 8, 10, 0}),
        makeR(Opcode::ARITH, OperandsR{1, 1, 11, 0}),           // 3 if all true
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lddnz_catch.ho"] = buildModuleBytes("lddnz_catch", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lddnz_catch.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 3) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HVMJITInstructionSemanticsTest, LdDnzNullWithoutHandlerFailsAsUnhandled) {
    // Pop any handler (none registered) then trigger LD_D_NZ with null;
    // the throw must fail unhandled.
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{2, 0, 20}),
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 7}),            // push
        makeI(Opcode::SYSCALL, OperandsI{0, 0, 8}),            // pop
        makeI(Opcode::MOVZ, OperandsI{1, 0, 0}),               // r1 = 0
        makeI(Opcode::LD_D_NZ, OperandsI{2, 1, 0}),            // throw, unhandled
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lddnz_nohandler.ho"] = buildModuleBytes("lddnz_nohandler", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lddnz_nohandler.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), -1);
    EXPECT_TRUE(jit.hasError());
    EXPECT_NE(jit.getLastError().find("Unhandled exception trap"), std::string::npos);
}

TEST_F(HVMJITInstructionSemanticsTest, LuiRespectsSharedShiftConstant) {
    // ISSUE-018: verify that LUI produces imm15 << kLuiImmediateShift
    // through the JIT path, so the shared constant is the single source
    // of truth for both interpreter and JIT IR.
    const uint16_t imm = 0x0ABCD;
    const int64_t expected = static_cast<int64_t>(static_cast<uint64_t>(imm) << hvm::kLuiImmediateShift);
    std::vector<HVMInstruction> ins{
        makeI(Opcode::LUI, OperandsI{1, 0, static_cast<int16_t>(imm)}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["lui_shift_pin.ho"] = buildModuleBytes("lui_shift_pin", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("lui_shift_pin.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), expected) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}
