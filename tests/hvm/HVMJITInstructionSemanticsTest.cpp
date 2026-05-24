#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/IOProvider.h"
#include "hvm/HOModule.h"
#include "hvm/HVMInstruction.h"
#include "hvm/HVMJIT.h"
#include "runtime/lib/hoo_runtime.h"

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

TEST_F(HVMJITInstructionSemanticsTest, SimpleSupportedProgramStillFallsBackToInterpreter) {
    std::vector<HVMInstruction> ins{
        makeI(Opcode::MOVZ, OperandsI{1, 0, 11}),
        makeR(Opcode::RET, OperandsR{0, 0, 0, 0}),
    };
    std::vector<Symbol> syms{funcSym("_F_main_v", 0)};
    io.binaryFiles["jitpath.ho"] = buildModuleBytes("jitpath", ins, syms);

    HVMJIT jit(io);
    ASSERT_TRUE(jit.loadInput("jitpath.ho")) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_main_v"), 11) << jit.getLastError();
    EXPECT_FALSE(jit.lastRunUsedJIT());
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
    EXPECT_FALSE(jit.lastRunUsedJIT());
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
