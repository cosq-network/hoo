#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "hvm/HInstruction.h"

using namespace hvm;

class HInstructionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(HInstructionTest, DefaultConstructor) {
    HInstruction inst;
    EXPECT_EQ(inst.getOpcode(), Opcode::NOP);
    EXPECT_EQ(inst.getMnemonic(), "nop");
    EXPECT_EQ(inst.getFormat(), InstructionFormat::R);
    EXPECT_FALSE(inst.isExtended());
    EXPECT_EQ(inst.getSize(), 4);
}

TEST_F(HInstructionTest, OpcodeConstructor) {
    HInstruction inst(Opcode::ARITH);
    EXPECT_EQ(inst.getOpcode(), Opcode::ARITH);
    EXPECT_EQ(inst.getMnemonic(), "add"); // default for 0x10 is add (func 0)
    EXPECT_EQ(inst.getFormat(), InstructionFormat::R);
}

TEST_F(HInstructionTest, OpcodeWithOperands) {
    HInstruction inst(Opcode::ARITH, OperandsR{1, 2, 3, 0});
    EXPECT_EQ(inst.getOpcode(), Opcode::ARITH);
    EXPECT_EQ(inst.getMnemonic(), "add");
    
    ASSERT_TRUE(std::holds_alternative<OperandsR>(inst.getOperands()));
    const auto& ops = std::get<OperandsR>(inst.getOperands());
    EXPECT_EQ(ops.rd, 1);
    EXPECT_EQ(ops.rs1, 2);
    EXPECT_EQ(ops.rs2, 3);
}

TEST_F(HInstructionTest, EncodeDecode32) {
    HInstruction orig(Opcode::ARITH, OperandsR{5, 10, 15, 1}); // sub
    uint32_t encoded = orig.encode32();
    
    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::ARITH);
    EXPECT_EQ(decoded->getMnemonic(), "sub");
    EXPECT_EQ(decoded->getSize(), 4);
}

TEST_F(HInstructionTest, EncodeDecodeBytes) {
    HInstruction orig(Opcode::ARITH, OperandsR{5, 10, 15, 0}); // add
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
    
    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::ARITH);
    EXPECT_EQ(decoded->getMnemonic(), "add");
    
    ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded->getOperands()));
    const auto& ops = std::get<OperandsR>(decoded->getOperands());
    EXPECT_EQ(ops.rd, 5);
    EXPECT_EQ(ops.rs1, 10);
    EXPECT_EQ(ops.rs2, 15);
}

TEST_F(HInstructionTest, EncodeDecodeBranch) {
    HInstruction orig(Opcode::BEQ, OperandsB{5, 10, -50});
    auto encoded = orig.encode();
    
    ASSERT_EQ(encoded.size(), 4);
    
    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::BEQ);
    ASSERT_TRUE(std::holds_alternative<OperandsB>(decoded->getOperands()));
    const auto& ops = std::get<OperandsB>(decoded->getOperands());
    EXPECT_EQ(ops.rs1, 5);
    EXPECT_EQ(ops.rs2, 10);
    EXPECT_EQ(ops.imm15, -50);
}

TEST_F(HInstructionTest, EncodeDecodeJump) {
    HInstruction orig(Opcode::JAL, OperandsJ{1, 0x12345});
    auto encoded = orig.encode();
    
    ASSERT_EQ(encoded.size(), 4);
    
    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::JAL);
    ASSERT_TRUE(std::holds_alternative<OperandsJ>(decoded->getOperands()));
    const auto& ops = std::get<OperandsJ>(decoded->getOperands());
    EXPECT_EQ(ops.rd, 1);
    EXPECT_EQ(ops.offset, 0x12345);
}

TEST_F(HInstructionTest, ToAssembly) {
    HInstruction inst(Opcode::ARITH, OperandsR{5, 10, 15, 0});
    EXPECT_EQ(inst.toAssembly(), "add r5, r10, r15");
    
    HInstruction inst2(Opcode::ARITH, OperandsR{5, 10, 15, 1});
    EXPECT_EQ(inst2.toAssembly(), "sub r5, r10, r15");
}

TEST_F(HInstructionTest, ToAssemblyIFormat) {
    HInstruction inst(Opcode::ADDI, OperandsI{3, 5, 100});
    EXPECT_EQ(inst.toAssembly(), "addi r3, r5, 100");
}

TEST_F(HInstructionTest, ToAssemblyBFormat) {
    HInstruction inst(Opcode::BEQ, OperandsB{5, 10, -50});
    EXPECT_EQ(inst.toAssembly(), "beq r5, r10, -50");
}

TEST_F(HInstructionTest, ToAssemblyJFormat) {
    HInstruction inst(Opcode::JAL, OperandsJ{1, 4096});
    EXPECT_EQ(inst.toAssembly(), "jal r1, 4096");
    
    HInstruction inst2(Opcode::JMP, OperandsJ{0, 4096});
    EXPECT_EQ(inst2.toAssembly(), "jmp 4096");
}

TEST_F(HInstructionTest, ToAssemblyRIFormat) {
    HInstruction inst(Opcode::NEWA, OperandsRI{1, 2, 3, 0});
    EXPECT_EQ(inst.toAssembly(), "newa r1, r2, r3, 0");
}

TEST_F(HInstructionTest, ToString) {
    HInstruction inst(Opcode::RET, OperandsR{0, 0, 0, 0});
    auto str = inst.toString();
    EXPECT_TRUE(str.find("ret") != std::string::npos);
}

TEST_F(HInstructionTest, OpcodeToString) {
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::NOP), "nop");
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::ARITH, 0), "add");
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::ARITH, 1), "sub");
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::JMP), "jmp");
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::TRY), "try");
}

TEST_F(HInstructionTest, StringToOpcode) {
    EXPECT_EQ(HInstruction::stringToOpcode("nop"), Opcode::NOP);
    EXPECT_EQ(HInstruction::stringToOpcode("add"), Opcode::ARITH);
    EXPECT_EQ(HInstruction::stringToOpcode("sub"), Opcode::ARITH);
    EXPECT_EQ(HInstruction::stringToOpcode("jmp"), Opcode::JMP);
    EXPECT_EQ(HInstruction::stringToOpcode("unknown_mnemonic"), Opcode::UNKNOWN);
}

TEST_F(HInstructionTest, GetFormatForOpcode) {
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::ARITH), InstructionFormat::R);
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::ADDI), InstructionFormat::I);
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::BEQ), InstructionFormat::B);
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::JMP), InstructionFormat::J);
}

TEST_F(HInstructionTest, ValidateRegister) {
    EXPECT_TRUE(HInstruction::validateRegister(0));
    EXPECT_TRUE(HInstruction::validateRegister(15));
    EXPECT_TRUE(HInstruction::validateRegister(31));
    EXPECT_FALSE(HInstruction::validateRegister(32));
    EXPECT_FALSE(HInstruction::validateRegister(255));
}

TEST_F(HInstructionTest, ValidateImmediate) {
    EXPECT_TRUE(HInstruction::validateImmediate(0, 16));
    EXPECT_TRUE(HInstruction::validateImmediate(32767, 16));
    EXPECT_TRUE(HInstruction::validateImmediate(-32768, 16));
    EXPECT_FALSE(HInstruction::validateImmediate(32768, 16));
    EXPECT_FALSE(HInstruction::validateImmediate(-32769, 16));
}

TEST_F(HInstructionTest, GetMnemonicMap) {
    const auto& map = InstructionRegistry::instance().getAllInfo();
    EXPECT_GT(map.size(), 50);
    EXPECT_EQ(map.at("add").opcode, Opcode::ARITH);
}

TEST_F(HInstructionTest, SettersAndGetters) {
    HInstruction inst;
    inst.setOpcode(Opcode::NOP);
    EXPECT_EQ(inst.getOpcode(), Opcode::NOP);
    
    inst.setOperands(OperandsR{1, 2, 3, 0});
    EXPECT_EQ(inst.getFormat(), InstructionFormat::R);
    
    inst.setMnemonic("test");
    EXPECT_EQ(inst.getMnemonic(), "test");
}

TEST_F(HInstructionTest, InstructionRegistrySingleton) {
    InstructionRegistry& reg1 = InstructionRegistry::instance();
    InstructionRegistry& reg2 = InstructionRegistry::instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST_F(HInstructionTest, InstructionRegistryGetInfoByMnemonic) {
    InstructionRegistry& reg = InstructionRegistry::instance();
    auto info = reg.getInfoByMnemonic("nop");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->opcode, Opcode::NOP);
    
    auto info2 = reg.getInfoByMnemonic("add");
    ASSERT_TRUE(info2.has_value());
    EXPECT_EQ(info2->opcode, Opcode::ARITH);
    EXPECT_EQ(info2->func, 0);
    
    auto info3 = reg.getInfoByMnemonic("sub");
    ASSERT_TRUE(info3.has_value());
    EXPECT_EQ(info3->opcode, Opcode::ARITH);
    EXPECT_EQ(info3->func, 1);
}

TEST_F(HInstructionTest, InstructionRegistryGetInfoByOpcode) {
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

TEST_F(HInstructionTest, DecodeInvalidSize) {
    auto result = HInstruction::decode(std::vector<uint8_t>{0x01, 0x02, 0x03});
    EXPECT_EQ(result, nullptr);
}

TEST_F(HInstructionTest, MultipleEncodeDecode) {
    std::vector<std::pair<Opcode, Operands>> testCases = {
        {Opcode::NOP, OperandsR{0, 0, 0, 0}},
        {Opcode::ARITH, OperandsR{1, 2, 3, 0}}, // add
        {Opcode::ARITH, OperandsR{5, 10, 15, 1}}, // sub
    };
    
    for (const auto& [opcode, ops] : testCases) {
        HInstruction orig(opcode, ops);
        auto encoded = orig.encode();
        auto decoded = HInstruction::decode(encoded);
        
        ASSERT_NE(decoded, nullptr);
        EXPECT_EQ(decoded->getOpcode(), opcode);
    }
}

TEST_F(HInstructionTest, ExtendedOpcodeUsesEscapedEncoding) {
    // Opcode::TRY is 0x110 (>= 0x80)
    HInstruction orig(Opcode::TRY, OperandsI{1, 0, 100});
    auto encoded = orig.encode();
    // 1 (escape) + 2 (ULEB for 0x110) + 1 (padding) + 4 (payload) = 8 bytes
    ASSERT_EQ(encoded.size(), 8);
    EXPECT_EQ(encoded[0], 0xFE);

    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_TRUE(decoded->isExtended());
    EXPECT_EQ(decoded->getOpcode(), Opcode::TRY);
    ASSERT_TRUE(std::holds_alternative<OperandsI>(decoded->getOperands()));
    const auto& ops = std::get<OperandsI>(decoded->getOperands());
    EXPECT_EQ(ops.rd, 1);
    EXPECT_EQ(ops.imm15, 100);
}
