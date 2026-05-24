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
