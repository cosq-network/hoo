#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "hvm/HVMInstruction.h"

using namespace hvm;

class HVMInstructionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(HVMInstructionTest, DefaultConstructor) {
    HVMInstruction inst;
    EXPECT_EQ(inst.getOpcode(), Opcode::NOP);
    EXPECT_EQ(inst.getMnemonic(), "nop");
    EXPECT_EQ(inst.getFormat(), InstructionFormat::R);
    EXPECT_FALSE(inst.isExtended());
    EXPECT_EQ(inst.getSize(), 4);
}

TEST_F(HVMInstructionTest, OpcodeConstructor) {
    HVMInstruction inst(Opcode::ARITH);
    EXPECT_EQ(inst.getOpcode(), Opcode::ARITH);
    EXPECT_EQ(inst.getMnemonic(), "add"); // default for 0x10 is add (func 0)
    EXPECT_EQ(inst.getFormat(), InstructionFormat::R);
}

TEST_F(HVMInstructionTest, OpcodeWithOperands) {
    HVMInstruction inst(Opcode::ARITH, OperandsR{1, 2, 3, 0});
    EXPECT_EQ(inst.getOpcode(), Opcode::ARITH);
    EXPECT_EQ(inst.getMnemonic(), "add");
    
    ASSERT_TRUE(std::holds_alternative<OperandsR>(inst.getOperands()));
    const auto& ops = std::get<OperandsR>(inst.getOperands());
    EXPECT_EQ(ops.rd, 1);
    EXPECT_EQ(ops.rs1, 2);
    EXPECT_EQ(ops.rs2, 3);
}

TEST_F(HVMInstructionTest, EncodeDecode32) {
    HVMInstruction orig(Opcode::ARITH, OperandsR{5, 10, 15, 1}); // sub
    uint32_t encoded = orig.encode32();
    
    auto decoded = HVMInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::ARITH);
    EXPECT_EQ(decoded->getMnemonic(), "sub");
    EXPECT_EQ(decoded->getSize(), 4);
}

TEST_F(HVMInstructionTest, EncodeDecodeBytes) {
    HVMInstruction orig(Opcode::ARITH, OperandsR{5, 10, 15, 0}); // add
    auto encoded = orig.encode();
    
    ASSERT_EQ(encoded.size(), 4);
    // Bit-packed: op[31:25] | rd[24:20] | rs1[19:15] | rs2[14:10] | func[9:0]
    // ARITH (0x10) << 25 = 0x20000000
    // rd (5) << 20 = 0x00500000
    // rs1 (10) << 15 = 0x00050000
    // rs2 (15) << 10 = 0x00003C00
    // func (0) = 0
    // Total = 0x20553C00
    // LE bytes: 0x00, 0x3C, 0x55, 0x20
    EXPECT_EQ(encoded[3], 0x20);
    EXPECT_EQ(encoded[2], 0x55);
    EXPECT_EQ(encoded[1], 0x3C);
    EXPECT_EQ(encoded[0], 0x00);
    
    auto decoded = HVMInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::ARITH);
    EXPECT_EQ(decoded->getMnemonic(), "add");
    
    ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded->getOperands()));
    const auto& ops = std::get<OperandsR>(decoded->getOperands());
    EXPECT_EQ(ops.rd, 5);
    EXPECT_EQ(ops.rs1, 10);
    EXPECT_EQ(ops.rs2, 15);
}

TEST_F(HVMInstructionTest, EncodeDecodeBranch) {
    HVMInstruction orig(Opcode::BEQ, OperandsB{5, 10, -50});
    auto encoded = orig.encode();
    
    ASSERT_EQ(encoded.size(), 4);
    
    auto decoded = HVMInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::BEQ);
    ASSERT_TRUE(std::holds_alternative<OperandsB>(decoded->getOperands()));
    const auto& ops = std::get<OperandsB>(decoded->getOperands());
    EXPECT_EQ(ops.rs1, 5);
    EXPECT_EQ(ops.rs2, 10);
    EXPECT_EQ(ops.imm15, -50);
}

TEST_F(HVMInstructionTest, EncodeDecodeJump) {
    HVMInstruction orig(Opcode::JAL, OperandsJ{1, 0x12345});
    auto encoded = orig.encode();
    
    ASSERT_EQ(encoded.size(), 4);
    
    auto decoded = HVMInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::JAL);
    ASSERT_TRUE(std::holds_alternative<OperandsJ>(decoded->getOperands()));
    const auto& ops = std::get<OperandsJ>(decoded->getOperands());
    EXPECT_EQ(ops.rd, 1);
    EXPECT_EQ(ops.offset, 0x12345);
}

TEST_F(HVMInstructionTest, ToAssembly) {
    HVMInstruction inst(Opcode::ARITH, OperandsR{5, 10, 15, 0});
    EXPECT_EQ(inst.toAssembly(), "add r5, r10, r15");
    
    HVMInstruction inst2(Opcode::ARITH, OperandsR{5, 10, 15, 1});
    EXPECT_EQ(inst2.toAssembly(), "sub r5, r10, r15");
}

TEST_F(HVMInstructionTest, DecodeRejectsReservedFuncOnFuncLessRInstruction) {
    // MOV (opcode 0x01) does not use func. A word with func=5 is a reserved
    // encoding and must not decode to MOV.
    const uint32_t word = (0x01u << 25) | (1u << 20) | (1u << 15) | 5u;
    auto decoded = HVMInstruction::decode(word);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::UNKNOWN);
}

TEST_F(HVMInstructionTest, DecodeRejectsUndefinedFuncOnFuncfulRInstruction) {
    // ARITH (0x10) defines funcs 0,1,2,5,6,7. func=3 is not one of them.
    const uint32_t word = (0x10u << 25) | (1u << 20) | (2u << 15) | (3u << 10) | 3u;
    auto decoded = HVMInstruction::decode(word);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::UNKNOWN);
}

TEST_F(HVMInstructionTest, DecodeAcceptsImmediateBitsOnNonRFormats) {
    // MOVZ (0x03, I-format) carries an immediate in the low 15 bits. A
    // nonzero low-10-bit immediate is immediate, not func, and must decode.
    const uint32_t word = (0x03u << 25) | (1u << 20) | (1u << 15) | 0x1FFu;
    auto decoded = HVMInstruction::decode(word);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::MOVZ);
    ASSERT_TRUE(std::holds_alternative<OperandsI>(decoded->getOperands()));
    const auto& ops = std::get<OperandsI>(decoded->getOperands());
    EXPECT_EQ(ops.imm15, 0x1FF);
}

TEST_F(HVMInstructionTest, DecodeRejectsReservedFuncInExtendedEncoding) {
    // BREAK (0xC1, escape32) does not use func. Encode with func=7 and
    // confirm the byte-stream decoder rejects it.
    HVMInstruction orig(Opcode::BREAK, OperandsR{0, 0, 0, 7});
    auto encoded = orig.encode();
    ASSERT_EQ(encoded.size(), 8);
    size_t bytesUsed = 0;
    auto decoded = HVMInstruction::decode(encoded, bytesUsed);
    EXPECT_EQ(decoded, nullptr);
}

TEST_F(HVMInstructionTest, ToAssemblyIFormat) {
    HVMInstruction inst(Opcode::ADDI, OperandsI{3, 5, 100});
    EXPECT_EQ(inst.toAssembly(), "addi r3, r5, 100");
}

TEST_F(HVMInstructionTest, ToAssemblyBFormat) {
    HVMInstruction inst(Opcode::BEQ, OperandsB{5, 10, -50});
    EXPECT_EQ(inst.toAssembly(), "beq r5, r10, -50");
}

TEST_F(HVMInstructionTest, ToAssemblyJFormat) {
    HVMInstruction inst(Opcode::JAL, OperandsJ{1, 4096});
    EXPECT_EQ(inst.toAssembly(), "jal r1, 4096");
    
    HVMInstruction inst2(Opcode::JMP, OperandsJ{0, 4096});
    EXPECT_EQ(inst2.toAssembly(), "jmp 4096");
}

TEST_F(HVMInstructionTest, ToString) {
    HVMInstruction inst(Opcode::RET, OperandsR{0, 0, 0, 0});
    auto str = inst.toString();
    EXPECT_TRUE(str.find("ret") != std::string::npos);
}

TEST_F(HVMInstructionTest, OpcodeToString) {
    EXPECT_EQ(HVMInstruction::opcodeToString(Opcode::NOP), "nop");
    EXPECT_EQ(HVMInstruction::opcodeToString(Opcode::ARITH, 0), "add");
    EXPECT_EQ(HVMInstruction::opcodeToString(Opcode::ARITH, 1), "sub");
    EXPECT_EQ(HVMInstruction::opcodeToString(Opcode::JMP), "jmp");
    EXPECT_EQ(HVMInstruction::opcodeToString(Opcode::SYSCALL), "syscall");
}

TEST_F(HVMInstructionTest, StringToOpcode) {
    EXPECT_EQ(HVMInstruction::stringToOpcode("nop"), Opcode::NOP);
    EXPECT_EQ(HVMInstruction::stringToOpcode("add"), Opcode::ARITH);
    EXPECT_EQ(HVMInstruction::stringToOpcode("sub"), Opcode::ARITH);
    EXPECT_EQ(HVMInstruction::stringToOpcode("jmp"), Opcode::JMP);
    EXPECT_EQ(HVMInstruction::stringToOpcode("unknown_mnemonic"), Opcode::UNKNOWN);
}

TEST_F(HVMInstructionTest, GetFormatForOpcode) {
    EXPECT_EQ(HVMInstruction::getFormatForOpcode(Opcode::ARITH), InstructionFormat::R);
    EXPECT_EQ(HVMInstruction::getFormatForOpcode(Opcode::ADDI), InstructionFormat::I);
    EXPECT_EQ(HVMInstruction::getFormatForOpcode(Opcode::BEQ), InstructionFormat::B);
    EXPECT_EQ(HVMInstruction::getFormatForOpcode(Opcode::JMP), InstructionFormat::J);
}

TEST_F(HVMInstructionTest, ValidateRegister) {
    EXPECT_TRUE(HVMInstruction::validateRegister(0));
    EXPECT_TRUE(HVMInstruction::validateRegister(15));
    EXPECT_TRUE(HVMInstruction::validateRegister(31));
    EXPECT_FALSE(HVMInstruction::validateRegister(32));
    EXPECT_FALSE(HVMInstruction::validateRegister(255));
}

TEST_F(HVMInstructionTest, ValidateImmediate) {
    EXPECT_TRUE(HVMInstruction::validateImmediate(0, 16));
    EXPECT_TRUE(HVMInstruction::validateImmediate(32767, 16));
    EXPECT_TRUE(HVMInstruction::validateImmediate(-32768, 16));
    EXPECT_FALSE(HVMInstruction::validateImmediate(32768, 16));
    EXPECT_FALSE(HVMInstruction::validateImmediate(-32769, 16));
}

TEST_F(HVMInstructionTest, GetMnemonicMap) {
    const auto& map = InstructionRegistry::instance().getAllInfo();
    EXPECT_GT(map.size(), 50);
    EXPECT_EQ(map.at("add").opcode, Opcode::ARITH);
}

TEST_F(HVMInstructionTest, SettersAndGetters) {
    HVMInstruction inst;
    inst.setOpcode(Opcode::NOP);
    EXPECT_EQ(inst.getOpcode(), Opcode::NOP);
    
    inst.setOperands(OperandsR{1, 2, 3, 0});
    EXPECT_EQ(inst.getFormat(), InstructionFormat::R);
    
    inst.setMnemonic("test");
    EXPECT_EQ(inst.getMnemonic(), "test");
}

TEST_F(HVMInstructionTest, InstructionRegistrySingleton) {
    InstructionRegistry& reg1 = InstructionRegistry::instance();
    InstructionRegistry& reg2 = InstructionRegistry::instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST_F(HVMInstructionTest, InstructionRegistryGetInfoByMnemonic) {
    InstructionRegistry& reg = InstructionRegistry::instance();
    auto info = reg.getInfoByMnemonic("nop");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->opcode, Opcode::NOP);
    EXPECT_EQ(info->encoding, InstructionEncoding::Base32);
    
    auto info2 = reg.getInfoByMnemonic("add");
    ASSERT_TRUE(info2.has_value());
    EXPECT_EQ(info2->opcode, Opcode::ARITH);
    EXPECT_EQ(info2->func, 0);
    EXPECT_EQ(info2->encoding, InstructionEncoding::Base32);
    
    auto info3 = reg.getInfoByMnemonic("sub");
    ASSERT_TRUE(info3.has_value());
    EXPECT_EQ(info3->opcode, Opcode::ARITH);
    EXPECT_EQ(info3->func, 1);

    auto info4 = reg.getInfoByMnemonic("syscall");
    ASSERT_TRUE(info4.has_value());
    EXPECT_EQ(info4->opcode, Opcode::SYSCALL);
    EXPECT_EQ(info4->encoding, InstructionEncoding::Escape32);
}

TEST_F(HVMInstructionTest, InstructionRegistryGetInfoByOpcode) {
    InstructionRegistry& reg = InstructionRegistry::instance();
    auto info = reg.getInfoByOpcode(Opcode::NOP);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->mnemonic, "nop");
    
    auto info2 = reg.getInfoByOpcode(Opcode::ARITH, 0);
    ASSERT_TRUE(info2.has_value());
    EXPECT_EQ(info2->mnemonic, "add");
    
    auto info3 = reg.getInfoByOpcode(Opcode::ARITH, 1);
    ASSERT_TRUE(info3.has_value());
    EXPECT_EQ(info3->mnemonic, "sub");
}

TEST_F(HVMInstructionTest, DecodeInvalidSize) {
    auto result = HVMInstruction::decode(std::vector<uint8_t>{0x01, 0x02, 0x03});
    EXPECT_EQ(result, nullptr);
}

TEST_F(HVMInstructionTest, MultipleEncodeDecode) {
    std::vector<std::pair<Opcode, Operands>> testCases = {
        {Opcode::NOP, OperandsR{0, 0, 0, 0}},
        {Opcode::ARITH, OperandsR{1, 2, 3, 0}}, // add
        {Opcode::ARITH, OperandsR{5, 10, 15, 1}}, // sub
    };
    
    for (const auto& [opcode, ops] : testCases) {
        HVMInstruction orig(opcode, ops);
        auto encoded = orig.encode();
        auto decoded = HVMInstruction::decode(encoded);
        
        ASSERT_NE(decoded, nullptr);
        EXPECT_EQ(decoded->getOpcode(), opcode);
    }
}

TEST_F(HVMInstructionTest, RIFormatDoesNotForceEscapeEncoding) {
    HVMInstruction inst(Opcode::NOP);
    inst.setFormat(InstructionFormat::RI);
    inst.setOperands(OperandsRI{1, 2, 3, 17});

    EXPECT_EQ(inst.getSize(), 4);

    auto encoded = inst.encode();
    ASSERT_EQ(encoded.size(), 4);
    EXPECT_NE(encoded[0], 0xFE);
}

TEST_F(HVMInstructionTest, ExtendedOpcodeUsesEscapedEncoding) {
    // Opcode::SYSCALL is 0xC0 (>= 0x80)
    HVMInstruction orig(Opcode::SYSCALL, OperandsI{1, 0, 100});
    auto encoded = orig.encode();
    // 1 (escape) + 2 (ULEB for 0xC0) + 1 (padding) + 4 (payload) = 8 bytes
    ASSERT_EQ(encoded.size(), 8);
    EXPECT_EQ(encoded[0], 0xFE);

    auto decoded = HVMInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_TRUE(decoded->isExtended());
    EXPECT_EQ(decoded->getOpcode(), Opcode::SYSCALL);
    ASSERT_TRUE(std::holds_alternative<OperandsI>(decoded->getOperands()));
    const auto& ops = std::get<OperandsI>(decoded->getOperands());
    EXPECT_EQ(ops.rd, 1);
    EXPECT_EQ(ops.imm15, 100);
}

TEST_F(HVMInstructionTest, RejectEscapedBaseOpcode) {
    HVMInstruction orig(Opcode::ARITH, OperandsR{1, 2, 3, 0});
    const auto payload = orig.encode32();

    std::vector<uint8_t> escaped;
    escaped.reserve(8);
    escaped.push_back(0xFE);
    escaped.push_back(static_cast<uint8_t>(Opcode::ARITH));
    escaped.push_back(0x00);
    escaped.push_back(0x00);
    escaped.push_back(static_cast<uint8_t>(payload & 0xFFU));
    escaped.push_back(static_cast<uint8_t>((payload >> 8U) & 0xFFU));
    escaped.push_back(static_cast<uint8_t>((payload >> 16U) & 0xFFU));
    escaped.push_back(static_cast<uint8_t>((payload >> 24U) & 0xFFU));

    auto decoded = HVMInstruction::decode(escaped);
    EXPECT_EQ(decoded, nullptr);
}

TEST_F(HVMInstructionTest, ExtendedOpcodeRoundTripAllEscape32) {
    struct EscapeCase {
        Opcode op;
        InstructionFormat fmt;
        Operands ops;
        std::string expectedMnemonic;
    };
    std::vector<EscapeCase> cases = {
        {Opcode::ENTER,    InstructionFormat::I, OperandsI{0, 0, 32},   "enter"},
        {Opcode::LEAVE,    InstructionFormat::R, OperandsR{0, 0, 0, 0}, "leave"},
        {Opcode::ADJSP,    InstructionFormat::I, OperandsI{0, 0, -16},  "adjsp"},
        {Opcode::FRAME,    InstructionFormat::I, OperandsI{1, 0, -8},   "frame"},
        {Opcode::CALL,     InstructionFormat::J, OperandsJ{29, 200},    "call"},
        {Opcode::TAILCALL, InstructionFormat::J, OperandsJ{0, 200},     "tailcall"},
        {Opcode::SYSCALL,  InstructionFormat::I, OperandsI{1, 0, 5},    "syscall"},
        {Opcode::SYSCALL,  InstructionFormat::I, OperandsI{2, 0, 12},   "syscall"},
        {Opcode::SYSCALL,  InstructionFormat::I, OperandsI{4, 0, 15},   "syscall"},
        {Opcode::SYSCALL,  InstructionFormat::I, OperandsI{6, 0, 23},   "syscall"},
        {Opcode::BREAK,    InstructionFormat::R, OperandsR{0, 0, 0, 0}, "break"},
    };
    for (const auto& c : cases) {
        HVMInstruction orig(c.op, c.ops);
        orig.setFormat(c.fmt);
        EXPECT_EQ(orig.getMnemonic(), c.expectedMnemonic);

        auto encoded = orig.encode();
        ASSERT_EQ(encoded.size(), 8) << "Failed for " << c.expectedMnemonic;
        EXPECT_EQ(encoded[0], 0xFE)  << "No escape byte for " << c.expectedMnemonic;

        auto decoded = HVMInstruction::decode(encoded);
        ASSERT_NE(decoded, nullptr) << "Decode failed for " << c.expectedMnemonic;
        EXPECT_TRUE(decoded->isExtended());
        EXPECT_EQ(decoded->getOpcode(), c.op);
        EXPECT_EQ(decoded->getMnemonic(), c.expectedMnemonic);
        EXPECT_EQ(decoded->getFormat(), c.fmt);
        EXPECT_EQ(decoded->getSize(), 8);
    }
}

TEST_F(HVMInstructionTest, ExtendedOpcodeRoundTripMultiple) {
    // Encode a sequence of extended instructions and decode them in order
    std::vector<uint8_t> bytes;
    auto addIns = [&](Opcode op, InstructionFormat fmt, Operands ops) {
        HVMInstruction ins(op, ops);
        ins.setFormat(fmt);
        auto enc = ins.encode();
        bytes.insert(bytes.end(), enc.begin(), enc.end());
    };
    addIns(Opcode::ENTER,    InstructionFormat::I, OperandsI{0, 0, 32});
    addIns(Opcode::CALL,     InstructionFormat::J, OperandsJ{29, 100});
    addIns(Opcode::SYSCALL,  InstructionFormat::I, OperandsI{1, 0, 1});
    addIns(Opcode::LEAVE,    InstructionFormat::R, OperandsR{0, 0, 0, 0});

    size_t offset = 0;
    for (int i = 0; i < 4; i++) {
        std::vector<uint8_t> slice(bytes.begin() + static_cast<ptrdiff_t>(offset),
                                   bytes.end());
        size_t used = 0;
        auto decoded = HVMInstruction::decode(slice, used);
        ASSERT_NE(decoded, nullptr) << "Decode failed at instruction " << i;
        EXPECT_EQ(used, 8);
        EXPECT_TRUE(decoded->isExtended());
        offset += used;
    }
    EXPECT_EQ(offset, bytes.size());
}

TEST_F(HVMInstructionTest, SYSCALLVariousIds) {
    std::vector<int16_t> ids = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    for (auto id : ids) {
        HVMInstruction orig(Opcode::SYSCALL, OperandsI{1, 0, id});
        auto encoded = orig.encode();
        ASSERT_EQ(encoded.size(), 8);
        EXPECT_EQ(encoded[0], 0xFE);

        auto decoded = HVMInstruction::decode(encoded);
        ASSERT_NE(decoded, nullptr);
        EXPECT_EQ(decoded->getOpcode(), Opcode::SYSCALL);
        ASSERT_TRUE(std::holds_alternative<OperandsI>(decoded->getOperands()));
        const auto& ops = std::get<OperandsI>(decoded->getOperands());
        EXPECT_EQ(ops.imm15, id);
        EXPECT_EQ(ops.rd, 1);
    }
}

TEST_F(HVMInstructionTest, SYSCALLNegativeId) {
    HVMInstruction orig(Opcode::SYSCALL, OperandsI{1, 0, -1});
    auto encoded = orig.encode();
    ASSERT_EQ(encoded.size(), 8);

    auto decoded = HVMInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    ASSERT_TRUE(std::holds_alternative<OperandsI>(decoded->getOperands()));
    const auto& ops = std::get<OperandsI>(decoded->getOperands());
    EXPECT_EQ(ops.imm15, -1);
}

TEST_F(HVMInstructionTest, PUSHRegisterFieldConsistency) {
    // PUSH is R-format with operand in rd field (bits 24:20).
    // Verify encode/decode round-trip for all valid register indices.
    for (int rd = 0; rd <= 31; rd++) {
        HVMInstruction orig(Opcode::PUSH, OperandsR{static_cast<uint8_t>(rd), 0, 0, 0});
        auto encoded = orig.encode();
        ASSERT_EQ(encoded.size(), 4);

        auto decoded = HVMInstruction::decode(encoded);
        ASSERT_NE(decoded, nullptr);
        EXPECT_EQ(decoded->getOpcode(), Opcode::PUSH);
        ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded->getOperands()));
        const auto& ops = std::get<OperandsR>(decoded->getOperands());
        // The implementation stores the source register in rd (bits 24:20)
        EXPECT_EQ(ops.rd, rd) << "PUSH register mismatch for rd=" << rd;
    }
}

TEST_F(HVMInstructionTest, POPRegisterFieldConsistency) {
    for (int rd = 0; rd <= 31; rd++) {
        HVMInstruction orig(Opcode::POP, OperandsR{static_cast<uint8_t>(rd), 0, 0, 0});
        auto encoded = orig.encode();
        ASSERT_EQ(encoded.size(), 4);

        auto decoded = HVMInstruction::decode(encoded);
        ASSERT_NE(decoded, nullptr);
        EXPECT_EQ(decoded->getOpcode(), Opcode::POP);
        ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded->getOperands()));
        const auto& ops = std::get<OperandsR>(decoded->getOperands());
        EXPECT_EQ(ops.rd, rd) << "POP register mismatch for rd=" << rd;
    }
}

TEST_F(HVMInstructionTest, CALLEncodeDecodeAllRegisters) {
    // CALL is J-format with rd (typically r29) and offset
    for (int rd = 0; rd <= 31; rd++) {
        int32_t offset = 100 + rd * 10;
        HVMInstruction orig(Opcode::CALL, OperandsJ{static_cast<uint8_t>(rd), offset});
        auto encoded = orig.encode();
        ASSERT_EQ(encoded.size(), 8);
        EXPECT_EQ(encoded[0], 0xFE);

        auto decoded = HVMInstruction::decode(encoded);
        ASSERT_NE(decoded, nullptr);
        EXPECT_EQ(decoded->getOpcode(), Opcode::CALL);
        ASSERT_TRUE(std::holds_alternative<OperandsJ>(decoded->getOperands()));
        const auto& ops = std::get<OperandsJ>(decoded->getOperands());
        EXPECT_EQ(ops.rd, rd);
        EXPECT_EQ(ops.offset, offset);
    }
}

TEST_F(HVMInstructionTest, ToAssemblyJFormatEdgeCases) {
    // TAILCALL should not show rd register (offset only)
    HVMInstruction tailcall(Opcode::TAILCALL, OperandsJ{0, 200});
    tailcall.setFormat(InstructionFormat::J);
    auto asmTailcall = tailcall.toAssembly();
    EXPECT_EQ(asmTailcall, "tailcall 200")
        << "TAILCALL assembly should not include rd: got '" << asmTailcall << "'";

    // CALL should show rd and offset
    HVMInstruction call(Opcode::CALL, OperandsJ{29, 200});
    call.setFormat(InstructionFormat::J);
    auto asmCall = call.toAssembly();
    EXPECT_EQ(asmCall, "call r29, 200")
        << "CALL assembly should include rd: got '" << asmCall << "'";
}

TEST_F(HVMInstructionTest, ToAssemblySYSCALL) {
    HVMInstruction syscall(Opcode::SYSCALL, OperandsI{1, 0, 5});
    syscall.setFormat(InstructionFormat::I);
    auto asmResult = syscall.toAssembly();
    // SYSCALL should match its I-format assembly
    EXPECT_EQ(asmResult, "syscall r1, r0, 5");
}

TEST_F(HVMInstructionTest, SyscallImm15SurvivesRoundTrip) {
    // The SYSCALL imm15 carries the syscall number, which a trap handler
    // recovers from stval/bad_instruction. Verify it survives encode/decode
    // for a representative syscall number.
    HVMInstruction syscall(Opcode::SYSCALL, OperandsI{1, 2, 9}); // kSysThrowToHandler
    syscall.setFormat(InstructionFormat::I);
    auto encoded = syscall.encode();
    ASSERT_EQ(encoded.size(), 8);
    size_t used = 0;
    auto decoded = HVMInstruction::decode(encoded, used);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::SYSCALL);
    ASSERT_TRUE(std::holds_alternative<OperandsI>(decoded->getOperands()));
    EXPECT_EQ(std::get<OperandsI>(decoded->getOperands()).imm15, 9);
}

TEST_F(HVMInstructionTest, ToAssemblyRET) {
    HVMInstruction ret(Opcode::RET, OperandsR{0, 0, 0, 0});
    auto asmResult = ret.toAssembly();
    EXPECT_EQ(asmResult, "ret r0, r0, r0");
}

TEST_F(HVMInstructionTest, ToAssemblyNOP) {
    HVMInstruction nop(Opcode::NOP, OperandsR{0, 0, 0, 0});
    auto asmResult = nop.toAssembly();
    EXPECT_EQ(asmResult, "nop r0, r0, r0");
}

TEST_F(HVMInstructionTest, ToAssemblyBREAK) {
    HVMInstruction breakIns(Opcode::BREAK, OperandsR{0, 0, 0, 0});
    breakIns.setFormat(InstructionFormat::R);
    auto asmResult = breakIns.toAssembly();
    EXPECT_EQ(asmResult, "break r0, r0, r0");
}

TEST_F(HVMInstructionTest, ExtendedOpcodeRejectsMalformedEncoding) {
    // Missing escape byte entirely
    std::vector<uint8_t> noEscape = {0x00, 0x00, 0x00, 0x00};
    auto decoded = HVMInstruction::decode(noEscape);
    ASSERT_NE(decoded, nullptr);
    EXPECT_FALSE(decoded->isExtended());

    // Escape byte but truncated (only 5 bytes, need 8)
    std::vector<uint8_t> truncated = {0xFE, 0xC0, 0x00, 0x00, 0x00};
    size_t used = 999;
    auto dec2 = HVMInstruction::decode(truncated, used);
    EXPECT_EQ(dec2, nullptr);
    EXPECT_EQ(used, 0);

    // Escape byte with valid ULEB128 but missing payload (7 bytes, need 8)
    std::vector<uint8_t> missingPayload = {0xFE, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
    used = 999;
    auto dec3 = HVMInstruction::decode(missingPayload, used);
    EXPECT_EQ(dec3, nullptr);
    EXPECT_EQ(used, 0);
}

// CSV parity test: validates InstructionRegistry matches hvm_instruction_set.csv
struct CsvRow {
    const char* mnemonic;
    Opcode opcode;
    InstructionFormat format;
    uint16_t func;
};

static const CsvRow kCsvRows[] = {
    {"nop",   Opcode::NOP,   InstructionFormat::R, 0},
    {"mov",   Opcode::MOV,   InstructionFormat::R, 0},
    {"movz",  Opcode::MOVZ,  InstructionFormat::I, 0},
    {"lui",   Opcode::LUI,   InstructionFormat::I, 0},
    {"addi",  Opcode::ADDI,  InstructionFormat::I, 0},
    {"retain", Opcode::RETAIN, InstructionFormat::R, 0},
    {"release", Opcode::RELEASE, InstructionFormat::R, 0},
    {"icache.rng", Opcode::ICACHE_RNG, InstructionFormat::R, 0},
    {"add",   Opcode::ARITH, InstructionFormat::R, 0},
    {"sub",   Opcode::ARITH, InstructionFormat::R, 1},
    {"mul",   Opcode::ARITH, InstructionFormat::R, 2},
    {"div",   Opcode::ARITH, InstructionFormat::R, 5},
    {"divu",  Opcode::ARITH, InstructionFormat::R, 6},
    {"rem",   Opcode::ARITH, InstructionFormat::R, 7},
    {"add.b",  Opcode::ARITH_B, InstructionFormat::R, 0},
    {"sub.b",  Opcode::ARITH_B, InstructionFormat::R, 1},
    {"mul.b",  Opcode::ARITH_B, InstructionFormat::R, 2},
    {"div.b",  Opcode::ARITH_B, InstructionFormat::R, 5},
    {"divu.b", Opcode::ARITH_B, InstructionFormat::R, 6},
    {"rem.b",  Opcode::ARITH_B, InstructionFormat::R, 7},
    {"remu.b", Opcode::ARITH_B, InstructionFormat::R, 8},
    {"shl.b",  Opcode::SHIFT_B, InstructionFormat::R, 0},
    {"shr.b",  Opcode::SHIFT_B, InstructionFormat::R, 1},
    {"sar.b",  Opcode::SHIFT_B, InstructionFormat::R, 2},
    {"shl",   Opcode::SHIFT, InstructionFormat::R, 0},
    {"shr",   Opcode::SHIFT, InstructionFormat::R, 1},
    {"sar",   Opcode::SHIFT, InstructionFormat::R, 2},
    {"and",   Opcode::LOGIC, InstructionFormat::R, 0},
    {"or",    Opcode::LOGIC, InstructionFormat::R, 1},
    {"xor",   Opcode::LOGIC, InstructionFormat::R, 2},
    {"badd",  Opcode::LOGIC_B, InstructionFormat::R, 0},
    {"bmul",  Opcode::LOGIC_B, InstructionFormat::R, 1},
    {"bnot",  Opcode::LOGIC_B, InstructionFormat::R, 2},
    {"not",   Opcode::NOT,   InstructionFormat::R, 0},
    {"fadd",  Opcode::FLOAT_ARITH, InstructionFormat::R, 0},
    {"fsub",  Opcode::FLOAT_ARITH, InstructionFormat::R, 1},
    {"fmul",  Opcode::FLOAT_ARITH, InstructionFormat::R, 2},
    {"fdiv",  Opcode::FLOAT_ARITH, InstructionFormat::R, 3},
    {"fadd.b", Opcode::FLOAT_ARITH_B, InstructionFormat::R, 0},
    {"fsub.b", Opcode::FLOAT_ARITH_B, InstructionFormat::R, 1},
    {"fmul.b", Opcode::FLOAT_ARITH_B, InstructionFormat::R, 2},
    {"fdiv.b", Opcode::FLOAT_ARITH_B, InstructionFormat::R, 3},
    {"cmpeq", Opcode::CMP,   InstructionFormat::R, 0},
    {"cmpne", Opcode::CMP,   InstructionFormat::R, 1},
    {"cmplt", Opcode::CMP,   InstructionFormat::R, 2},
    {"cmple", Opcode::CMP,   InstructionFormat::R, 3},
    {"cmpult", Opcode::CMP,  InstructionFormat::R, 4},
    {"cmpule", Opcode::CMP,  InstructionFormat::R, 5},
    {"fcmpeq", Opcode::FCMP, InstructionFormat::R, 0},
    {"fcmplt", Opcode::FCMP, InstructionFormat::R, 1},
    {"fcmple", Opcode::FCMP, InstructionFormat::R, 2},
    {"beq",   Opcode::BEQ,   InstructionFormat::B, 0},
    {"bne",   Opcode::BNE,   InstructionFormat::B, 0},
    {"blt",   Opcode::BLT,   InstructionFormat::B, 0},
    {"ble",   Opcode::BLE,   InstructionFormat::B, 0},
    {"jmp",   Opcode::JMP,   InstructionFormat::J, 0},
    {"jal",   Opcode::JAL,   InstructionFormat::J, 0},
    {"jalr",  Opcode::JALR,  InstructionFormat::I, 0},
    {"ret",   Opcode::RET,   InstructionFormat::R, 0},
    {"ld.b",  Opcode::LD_B,  InstructionFormat::I, 0},
    {"ld.bu", Opcode::LD_BU, InstructionFormat::I, 0},
    {"ld.h",  Opcode::LD_H,  InstructionFormat::I, 0},
    {"ld.hu", Opcode::LD_HU, InstructionFormat::I, 0},
    {"ld.w",  Opcode::LD_W,  InstructionFormat::I, 0},
    {"ld.wu", Opcode::LD_WU, InstructionFormat::I, 0},
    {"ld.d",  Opcode::LD_D,  InstructionFormat::I, 0},
    {"ld.p",  Opcode::LD_P,  InstructionFormat::R, 0},
    {"st.b",  Opcode::ST_B,  InstructionFormat::I, 0},
    {"st.h",  Opcode::ST_H,  InstructionFormat::I, 0},
    {"st.w",  Opcode::ST_W,  InstructionFormat::I, 0},
    {"st.d",  Opcode::ST_D,  InstructionFormat::I, 0},
    {"st.p",  Opcode::ST_P,  InstructionFormat::R, 0},
    {"lda",   Opcode::LDA,   InstructionFormat::I, 0},
    {"push",  Opcode::PUSH,  InstructionFormat::R, 0},
    {"pop",   Opcode::POP,   InstructionFormat::R, 0},
    {"enter", Opcode::ENTER, InstructionFormat::I, 0},
    {"leave", Opcode::LEAVE, InstructionFormat::R, 0},
    {"adjsp", Opcode::ADJSP, InstructionFormat::I, 0},
    {"frame", Opcode::FRAME, InstructionFormat::I, 0},
    {"call",     Opcode::CALL,     InstructionFormat::J, 0},
    {"tailcall", Opcode::TAILCALL, InstructionFormat::J, 0},
    {"syscall",  Opcode::SYSCALL,  InstructionFormat::I, 0},
    {"break",    Opcode::BREAK,    InstructionFormat::R, 0},
    {"ecall",   Opcode::ECALL,   InstructionFormat::R, 0},
    {"trapret", Opcode::TRAPRET, InstructionFormat::R, 0},
    {"lr.d",    Opcode::LR_D,    InstructionFormat::R, 0},
    {"sc.d",    Opcode::SC_D,    InstructionFormat::R, 0},
    {"csrrw",   Opcode::CSRRW,   InstructionFormat::I, 0},
    {"sfence.vma", Opcode::SFENCE_VMA, InstructionFormat::R, 0},
    {"loop.set",   Opcode::LOOP_SET,   InstructionFormat::I, 0},
    {"loop.decbr", Opcode::LOOP_DECBR, InstructionFormat::B, 0},
    {"prefetch.r",   Opcode::PREFETCH_R,   InstructionFormat::I, 0},
    {"prefetch.w",   Opcode::PREFETCH_W,   InstructionFormat::I, 0},
    {"prefetch.nta", Opcode::PREFETCH_NTA, InstructionFormat::I, 0},
    {"memzero.hint", Opcode::MEMZERO_HINT, InstructionFormat::R, 0},
    {"alloc.bump", Opcode::ALLOC_BUMP, InstructionFormat::I, 0},
    {"rdprof", Opcode::RDPROF, InstructionFormat::I, 0},
    {"chk.b",  Opcode::CHK_B,  InstructionFormat::R, 0},
    {"ld.d.nz", Opcode::LD_D_NZ, InstructionFormat::I, 0},
    {"br.hint",  Opcode::BR_HINT,  InstructionFormat::B, 0},
    {"doorbell", Opcode::DOORBELL, InstructionFormat::R, 0},
    {"vsetvl",   Opcode::VSETVL,   InstructionFormat::R, 0},
    {"vld.v",  Opcode::VECTOR_MEM, InstructionFormat::R, 0},
    {"vst.v",  Opcode::VECTOR_MEM, InstructionFormat::R, 1},
    {"vlds.v", Opcode::VECTOR_MEM, InstructionFormat::R, 2},
    {"vsts.v", Opcode::VECTOR_MEM, InstructionFormat::R, 3},
    {"vldx.v", Opcode::VECTOR_MEM, InstructionFormat::R, 4},
    {"vstx.v", Opcode::VECTOR_MEM, InstructionFormat::R, 5},
    {"vadd.vv", Opcode::VECTOR_ARITH, InstructionFormat::R, 0},
    {"vadd.vx", Opcode::VECTOR_ARITH, InstructionFormat::R, 1},
    {"vsub.vv", Opcode::VECTOR_ARITH, InstructionFormat::R, 2},
    {"vsub.vx", Opcode::VECTOR_ARITH, InstructionFormat::R, 3},
    {"vmul.vv", Opcode::VECTOR_ARITH, InstructionFormat::R, 4},
    {"vmul.vx", Opcode::VECTOR_ARITH, InstructionFormat::R, 5},
    {"vdiv.vv", Opcode::VECTOR_ARITH, InstructionFormat::R, 6},
    {"vdiv.vx", Opcode::VECTOR_ARITH, InstructionFormat::R, 7},
    {"vfmacc.vv", Opcode::VECTOR_FMA, InstructionFormat::R, 0},
    {"vfmacc.vf", Opcode::VECTOR_FMA, InstructionFormat::R, 1},
    {"vcomp.vv",  Opcode::VECTOR_MASK, InstructionFormat::R, 0},
    {"vcomp.vx",  Opcode::VECTOR_MASK, InstructionFormat::R, 1},
    {"vmerge.vvm", Opcode::VECTOR_MASK, InstructionFormat::R, 2},
    {"vfirst.m",  Opcode::VECTOR_MASK, InstructionFormat::R, 3},
    {"vredadd.vs", Opcode::VECTOR_REDUCE, InstructionFormat::R, 0},
    {"vredmin.vs", Opcode::VECTOR_REDUCE, InstructionFormat::R, 1},
    {"vredmax.vs", Opcode::VECTOR_REDUCE, InstructionFormat::R, 2},
    {"vsll.vv",  Opcode::VECTOR_SHIFT,   InstructionFormat::R, 0},
    {"vsll.vx",  Opcode::VECTOR_SHIFT,   InstructionFormat::R, 1},
    {"vsrl.vv",  Opcode::VECTOR_SHIFT,   InstructionFormat::R, 2},
    {"vsrl.vx",  Opcode::VECTOR_SHIFT,   InstructionFormat::R, 3},
    {"vand.vv",  Opcode::VECTOR_BITWISE, InstructionFormat::R, 0},
    {"vor.vv",   Opcode::VECTOR_BITWISE, InstructionFormat::R, 1},
    {"vxor.vv",  Opcode::VECTOR_BITWISE, InstructionFormat::R, 2},
};

TEST_F(HVMInstructionTest, CsvParity_AllRowsRegistered) {
    const auto& reg = InstructionRegistry::instance();
    int missingCount = 0;
    int mismatchCount = 0;
    int totalRows = sizeof(kCsvRows) / sizeof(kCsvRows[0]);

    for (int i = 0; i < totalRows; ++i) {
        const auto& row = kCsvRows[i];
        auto info = reg.getInfoByMnemonic(row.mnemonic);
        if (!info) {
            ADD_FAILURE() << "CSV row #" << i << ": mnemonic '" << row.mnemonic << "' not registered";
            ++missingCount;
            continue;
        }
        if (info->opcode != row.opcode) {
            ADD_FAILURE() << "CSV row #" << i << ": '" << row.mnemonic
                          << "' opcode mismatch: registry=" << static_cast<int>(info->opcode)
                          << " csv=" << static_cast<int>(row.opcode);
            ++mismatchCount;
        }
        if (info->format != row.format) {
            ADD_FAILURE() << "CSV row #" << i << ": '" << row.mnemonic
                          << "' format mismatch: registry=" << static_cast<int>(info->format)
                          << " csv=" << static_cast<int>(row.format);
            ++mismatchCount;
        }
        if (info->func != row.func) {
            ADD_FAILURE() << "CSV row #" << i << ": '" << row.mnemonic
                          << "' func mismatch: registry=" << info->func
                          << " csv=" << row.func;
            ++mismatchCount;
        }
    }

    EXPECT_EQ(missingCount, 0) << missingCount << " CSV mnemonics missing from registry";
    EXPECT_EQ(mismatchCount, 0) << mismatchCount << " registry entries differ from CSV";
    EXPECT_EQ(totalRows, 132) << "CSV parity table has wrong row count";
}

TEST_F(HVMInstructionTest, CsvParity_StringToOpcodeResolvesAll) {
    const auto& reg = InstructionRegistry::instance();
    int failedCount = 0;
    int totalRows = sizeof(kCsvRows) / sizeof(kCsvRows[0]);

    for (int i = 0; i < totalRows; ++i) {
        const auto& row = kCsvRows[i];
        Opcode resolved = HVMInstruction::stringToOpcode(row.mnemonic);
        if (resolved == Opcode::UNKNOWN) {
            ADD_FAILURE() << "CSV row #" << i << ": stringToOpcode(\"" << row.mnemonic << "\") returned UNKNOWN";
            ++failedCount;
        }
    }

    EXPECT_EQ(failedCount, 0) << failedCount << " mnemonics not resolved by stringToOpcode";
}

TEST_F(HVMInstructionTest, CsvParity_EncodeDecodeRoundTrip) {
    int failedCount = 0;
    int totalRows = sizeof(kCsvRows) / sizeof(kCsvRows[0]);

    for (int i = 0; i < totalRows; ++i) {
        const auto& row = kCsvRows[i];
        HVMInstruction orig(row.opcode);
        if (row.func != 0 && std::holds_alternative<OperandsR>(orig.getOperands())) {
            orig = HVMInstruction(row.opcode, OperandsR{1, 1, 1, row.func});
        }
        orig.setMnemonic(row.mnemonic);

        auto encoded = orig.encode();
        size_t used = 0;
        auto decoded = HVMInstruction::decode(encoded, used);

        if (!decoded) {
            ADD_FAILURE() << "CSV row #" << i << ": '" << row.mnemonic
                          << "' encode/decode round-trip failed";
            ++failedCount;
        } else if (decoded->getOpcode() != row.opcode) {
            ADD_FAILURE() << "CSV row #" << i << ": '" << row.mnemonic
                          << "' decode opcode=" << static_cast<int>(decoded->getOpcode());
            ++failedCount;
        }
    }

    EXPECT_EQ(failedCount, 0) << failedCount << " mnemonics failed encode/decode round-trip";
}
