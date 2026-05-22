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
    HInstruction inst(Opcode::ADD);
    EXPECT_EQ(inst.getOpcode(), Opcode::ADD);
    EXPECT_EQ(inst.getMnemonic(), "add");
    EXPECT_EQ(inst.getFormat(), InstructionFormat::R);
}

TEST_F(HInstructionTest, OpcodeWithOperands) {
    HInstruction inst(Opcode::ADD, OperandsR{1, 2, 3, 0});
    EXPECT_EQ(inst.getOpcode(), Opcode::ADD);
    EXPECT_EQ(inst.getMnemonic(), "add");
    
    ASSERT_TRUE(std::holds_alternative<OperandsR>(inst.getOperands()));
    const auto& ops = std::get<OperandsR>(inst.getOperands());
    EXPECT_EQ(ops.rd, 1);
    EXPECT_EQ(ops.rs1, 2);
    EXPECT_EQ(ops.rs2, 3);
}

TEST_F(HInstructionTest, EncodeDecode32) {
    HInstruction orig(Opcode::ADD, OperandsR{5, 10, 15, 0});
    uint32_t encoded = orig.encode32();
    
    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::ADD);
    EXPECT_EQ(decoded->getSize(), 4);
}

TEST_F(HInstructionTest, EncodeDecodeBytes) {
    HInstruction orig(Opcode::ADD, OperandsR{5, 10, 15, 0});
    auto encoded = orig.encode();
    
    ASSERT_EQ(encoded.size(), 4);
    EXPECT_EQ(encoded[0], 0x10);
    EXPECT_EQ(encoded[1], 5);
    EXPECT_EQ(encoded[2], 10);
    EXPECT_EQ(encoded[3], 15);
    
    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::ADD);
    
    ASSERT_TRUE(std::holds_alternative<OperandsR>(decoded->getOperands()));
    const auto& ops = std::get<OperandsR>(decoded->getOperands());
    EXPECT_EQ(ops.rd, 5);
    EXPECT_EQ(ops.rs1, 10);
    EXPECT_EQ(ops.rs2, 15);
}

TEST_F(HInstructionTest, EncodeDecode64) {
    HInstruction orig(Opcode::NOP, OperandsR{0, 0, 0, 0});
    orig.setExtended(true);
    
    auto encoded = orig.encode64();
    ASSERT_EQ(encoded.size(), 7);
    EXPECT_EQ(encoded[0], 0xFE);
    
    auto decoded = HInstruction::decode64(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::NOP);
    EXPECT_TRUE(decoded->isExtended());
    EXPECT_EQ(decoded->getSize(), 8);
}

TEST_F(HInstructionTest, EncodeDecodeBranch) {
    HInstruction orig(Opcode::NOP, OperandsR{0, 0, 0, 0});
    auto encoded = orig.encode();
    
    ASSERT_EQ(encoded.size(), 4);
    
    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::NOP);
}

TEST_F(HInstructionTest, EncodeDecodeJump) {
    HInstruction orig(Opcode::NOP, OperandsR{0, 0, 0, 0});
    auto encoded = orig.encode();
    
    ASSERT_EQ(encoded.size(), 4);
    
    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::NOP);
}

TEST_F(HInstructionTest, EncodeDecodeRI) {
    HInstruction orig(Opcode::NOP, OperandsR{0, 0, 0, 0});
    orig.setExtended(true);
    
    auto encoded = orig.encode64();
    ASSERT_EQ(encoded.size(), 7);
    
    auto decoded = HInstruction::decode64(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->getOpcode(), Opcode::NOP);
}

TEST_F(HInstructionTest, ToAssembly) {
    HInstruction inst(Opcode::ADD, OperandsR{5, 10, 15, 0});
    EXPECT_EQ(inst.toAssembly(), "add r5, r10, r15");
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
    HInstruction inst(Opcode::JAL, OperandsJ{1, 0x1000});
    EXPECT_EQ(inst.toAssembly(), "jal r1, 4096");
}

TEST_F(HInstructionTest, ToAssemblyRIFormat) {
    HInstruction inst(Opcode::VINSERT, OperandsRI{1, 2, 3, 100});
    EXPECT_EQ(inst.toAssembly(), "vinsert r1, r2, r3, 100");
}

TEST_F(HInstructionTest, ToString) {
    HInstruction inst(Opcode::RET, OperandsR{0, 0, 0, 0});
    auto str = inst.toString();
    EXPECT_TRUE(str.find("ret") != std::string::npos);
}

TEST_F(HInstructionTest, OpcodeToString) {
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::NOP), "nop");
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::ADD), "add");
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::JMP), "jmp");
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::VMUL), "vmul");
    EXPECT_EQ(HInstruction::opcodeToString(Opcode::TRY), "try");
}

TEST_F(HInstructionTest, StringToOpcode) {
    EXPECT_EQ(HInstruction::stringToOpcode("nop"), Opcode::NOP);
    EXPECT_EQ(HInstruction::stringToOpcode("add"), Opcode::ADD);
    EXPECT_EQ(HInstruction::stringToOpcode("jmp"), Opcode::JMP);
    EXPECT_EQ(HInstruction::stringToOpcode("vmul"), Opcode::VMUL);
    EXPECT_EQ(HInstruction::stringToOpcode("unknown_mnemonic"), Opcode::UNKNOWN);
}

TEST_F(HInstructionTest, GetFormatForOpcode) {
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::ADD), InstructionFormat::R);
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::MOVI), InstructionFormat::I);
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::BEQ), InstructionFormat::B);
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::JMP), InstructionFormat::J);
    EXPECT_EQ(HInstruction::getFormatForOpcode(Opcode::VINSERT), InstructionFormat::RI);
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
    const auto& map = HInstruction::getMnemonicMap();
    EXPECT_GT(map.size(), 100);
    EXPECT_EQ(map.at(Opcode::NOP), "nop");
    EXPECT_EQ(map.at(Opcode::ADD), "add");
    EXPECT_EQ(map.at(Opcode::VMUL), "vmul");
}

TEST_F(HInstructionTest, GetOpcodeMap) {
    const auto& map = HInstruction::getOpcodeMap();
    EXPECT_GT(map.size(), 100);
    EXPECT_EQ(map.at("nop"), Opcode::NOP);
    EXPECT_EQ(map.at("add"), Opcode::ADD);
}

TEST_F(HInstructionTest, SettersAndGetters) {
    HInstruction inst;
    inst.setOpcode(Opcode::NOP);
    EXPECT_EQ(inst.getOpcode(), Opcode::NOP);
    
    inst.setOperands(OperandsR{1, 2, 3, 0});
    EXPECT_EQ(inst.getFormat(), InstructionFormat::R);
    
    inst.setMnemonic("test");
    EXPECT_EQ(inst.getMnemonic(), "test");
    
    inst.setExtended(true);
    EXPECT_TRUE(inst.isExtended());
    EXPECT_EQ(inst.getSize(), 8);
    
    inst.setExtended(false);
    EXPECT_FALSE(inst.isExtended());
    EXPECT_EQ(inst.getSize(), 4);
}

TEST_F(HInstructionTest, InstructionRegistrySingleton) {
    InstructionRegistry& reg1 = InstructionRegistry::instance();
    InstructionRegistry& reg2 = InstructionRegistry::instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST_F(HInstructionTest, InstructionRegistryGetMnemonic) {
    InstructionRegistry& reg = InstructionRegistry::instance();
    EXPECT_EQ(reg.getMnemonic(Opcode::NOP), "nop");
    EXPECT_EQ(reg.getMnemonic(Opcode::ADD), "add");
    EXPECT_EQ(reg.getMnemonic(Opcode::JMP), "jmp");
}

TEST_F(HInstructionTest, InstructionRegistryGetOpcode) {
    InstructionRegistry& reg = InstructionRegistry::instance();
    EXPECT_EQ(reg.getOpcode("nop"), Opcode::NOP);
    EXPECT_EQ(reg.getOpcode("add"), Opcode::ADD);
    EXPECT_EQ(reg.getOpcode("jmp"), Opcode::JMP);
    EXPECT_EQ(reg.getOpcode("unknown"), Opcode::UNKNOWN);
}

TEST_F(HInstructionTest, InstructionRegistryGetFormat) {
    InstructionRegistry& reg = InstructionRegistry::instance();
    EXPECT_EQ(reg.getFormat(Opcode::ADD), InstructionFormat::R);
    EXPECT_EQ(reg.getFormat(Opcode::MOVI), InstructionFormat::I);
    EXPECT_EQ(reg.getFormat(Opcode::BEQ), InstructionFormat::B);
    EXPECT_EQ(reg.getFormat(Opcode::JMP), InstructionFormat::J);
}

TEST_F(HInstructionTest, InstructionRegistryGetInfo) {
    InstructionRegistry& reg = InstructionRegistry::instance();
    auto info = reg.getInfo(Opcode::NOP);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->mnemonic, "nop");
    EXPECT_EQ(info->format, InstructionFormat::R);
    
    auto info2 = reg.getInfo(Opcode::ADD);
    ASSERT_TRUE(info2.has_value());
    EXPECT_EQ(info2->mnemonic, "add");
}

TEST_F(HInstructionTest, InstructionRegistryGetAllInfo) {
    const auto& allInfo = InstructionRegistry::instance().getAllInfo();
    EXPECT_GT(allInfo.size(), 100);
    
    EXPECT_TRUE(allInfo.find(Opcode::NOP) != allInfo.end());
    EXPECT_TRUE(allInfo.find(Opcode::VMUL) != allInfo.end());
    EXPECT_TRUE(allInfo.find(Opcode::TRY) != allInfo.end());
}

TEST_F(HInstructionTest, DecodeInvalidSize) {
    auto result = HInstruction::decode(std::vector<uint8_t>{0x01, 0x02, 0x03});
    EXPECT_EQ(result, nullptr);
}

TEST_F(HInstructionTest, Decode64InvalidSize) {
    auto result = HInstruction::decode64(std::vector<uint8_t>{0x01, 0x02, 0x03});
    EXPECT_EQ(result, nullptr);
}

TEST_F(HInstructionTest, MultipleEncodeDecode) {
    std::vector<std::pair<Opcode, Operands>> testCases = {
        {Opcode::NOP, OperandsR{0, 0, 0, 0}},
        {Opcode::ADD, OperandsR{1, 2, 3, 0}},
        {Opcode::NOP, OperandsR{5, 10, 15, 0}},
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
    HInstruction orig(Opcode::VINSERT, OperandsRI{1, 2, 3, 0x3456});
    auto encoded = orig.encode();
    ASSERT_GE(encoded.size(), 8);
    EXPECT_EQ(encoded[0], 0xFE);

    auto decoded = HInstruction::decode(encoded);
    ASSERT_NE(decoded, nullptr);
    EXPECT_TRUE(decoded->isExtended());
    EXPECT_EQ(decoded->getOpcode(), Opcode::VINSERT);
    ASSERT_TRUE(std::holds_alternative<OperandsRI>(decoded->getOperands()));
    const auto& ops = std::get<OperandsRI>(decoded->getOperands());
    EXPECT_EQ(ops.rd, 1);
    EXPECT_EQ(ops.rd2, 2);
    EXPECT_EQ(ops.rs, 3);
    EXPECT_EQ(ops.imm, 0x3456);
}
