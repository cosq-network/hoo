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
#include "runtime/lib/hoo_runtime.h"
#include "runtime/lib/hoo_exception.h"

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

        // Open for writing: O_RDWR|O_CREAT|O_TRUNC = 0x602
        makeI(Opcode::LDA, OperandsI{2, 30, -24}),
        makeI(Opcode::MOVZ, OperandsI{3, 0, 1538}),
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

