#include <gtest/gtest.h>
#include "core/HooCompiler.h"
#include "codegen/HVMCodeGenerator.h"
#include "hvm/HoModule.h"

using namespace hooc;
using namespace hvm;

class HVMCodeGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler_ = std::make_unique<HooCompiler>();
    }

    std::unique_ptr<HooCompiler> compiler_;
};

TEST_F(HVMCodeGeneratorTest, CompileSimpleFunction) {
    std::string code = R"(
        func : int64 add(a: int64, b: int64) {
            return a + b;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->getName(), "test");

    // Check symbols
    auto sym = module->getSymbol("add");
    ASSERT_NE(sym, nullptr);
    
    // Check instructions
    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    ASSERT_GE(insts.size(), 4); // ENTER, ST_D, ST_D, LD_D, LD_D, ADD, MOV, LEAVE, RET...
    
    EXPECT_EQ(insts[0].getOpcode(), Opcode::ENTER);
    
    // Find ADD instruction
    bool foundAdd = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsR>(ops) && std::get<OperandsR>(ops).func == 0) {
                foundAdd = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundAdd);
}

TEST_F(HVMCodeGeneratorTest, CompileWithConstant) {
    std::string code = R"(
        func : int64 getAnswer() {
            return 42;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    // Find MOVZ instruction for constant 42
    bool foundConst = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::MOVZ) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsI>(ops) && std::get<OperandsI>(ops).imm15 == 42) {
                foundConst = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundConst);
}

TEST_F(HVMCodeGeneratorTest, CompileWithVariables) {
    std::string code = R"(
        func : int64 testVars(a: int64) {
            var x = a + 10;
            var y = x * 2;
            return y;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    // Check for some expected instructions
    bool foundAdd = false;
    bool foundMul = false;
    bool foundSt = false;
    bool foundLd = false;

    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsR>(ops)) {
                if (std::get<OperandsR>(ops).func == 0) foundAdd = true;
                if (std::get<OperandsR>(ops).func == 2) foundMul = true;
            }
        } else if (inst.getOpcode() == Opcode::ST_D) {
            foundSt = true;
        } else if (inst.getOpcode() == Opcode::LD_D) {
            foundLd = true;
        }
    }

    EXPECT_TRUE(foundAdd);
    EXPECT_TRUE(foundMul);
    EXPECT_TRUE(foundSt);
    EXPECT_TRUE(foundLd);
}

TEST_F(HVMCodeGeneratorTest, CompileComplexExpression) {
    std::string code = R"(
        func : int64 complex(a: int64, b: int64, c: int64) {
            return (a + b) * (c - 5);
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    int arithCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) arithCount++;
    }
    
    // a+b, c-5, and their multiplication = 3 arith ops
    EXPECT_GE(arithCount, 3);
}

TEST_F(HVMCodeGeneratorTest, CompileIfStatement) {
    std::string code = R"(
        func : int64 testIf(a: int64) {
            if (a > 0) {
                return 1;
            } else {
                return 0;
            }
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    // Should find BEQ or similar for the jump
    bool foundBranch = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::BEQ || inst.getOpcode() == Opcode::BNE) {
            foundBranch = true;
            break;
        }
    }
    EXPECT_TRUE(foundBranch);
}

TEST_F(HVMCodeGeneratorTest, CompileWhileStatement) {
    std::string code = R"(
        func : int64 testWhile(n: int64) {
            var i = 0;
            var sum = 0;
            while (i < n) {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    // Should find at least one JMP for the loop back
    bool foundJump = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::JMP) {
            foundJump = true;
            break;
        }
    }
    EXPECT_TRUE(foundJump);
}

TEST_F(HVMCodeGeneratorTest, CompileGlobalVariables) {
    std::string code = R"(
        var g_alpha = 100;
        var g_beta = 200;
        func : int64 getSum() {
            return 300;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    // Check symbols for globals
    auto sym1 = module->getSymbol("g_alpha");
    ASSERT_NE(sym1, nullptr);
    EXPECT_EQ(sym1->type, Symbol::STT_OBJECT);

    auto sym2 = module->getSymbol("g_beta");
    ASSERT_NE(sym2, nullptr);
    EXPECT_EQ(sym2->type, Symbol::STT_OBJECT);
    
    // Globals should be in .data section
    auto dataSec = module->getSection(".data");
    ASSERT_NE(dataSec, nullptr);
    EXPECT_GE(dataSec->data.size(), 16); // 2 * 8 bytes
}

TEST_F(HVMCodeGeneratorTest, CompileLogicalOperators) {
    std::string code = R"(
        func : int64 testLogic(a: int64, b: int64) {
            if (a == 1 && b == 2) {
                return 1;
            }
            if (a == 3 || b == 4) {
                return 2;
            }
            return 0;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    bool foundAnd = false;
    bool foundOr = false;

    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::LOGIC) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsR>(ops)) {
                if (std::get<OperandsR>(ops).func == 0) foundAnd = true;
                if (std::get<OperandsR>(ops).func == 1) foundOr = true;
            }
        }
    }
    EXPECT_TRUE(foundAnd);
    EXPECT_TRUE(foundOr);
}

TEST_F(HVMCodeGeneratorTest, CompileBreakContinue) {
    std::string code = R"(
        func : void testLoopControl() {
            var i = 0;
            while (true) {
                i = i + 1;
                if (i < 5) { continue; }
                if (i > 10) { break; }
            }
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    // Should find multiple JMP instructions
    int jumpCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::JMP) jumpCount++;
    }
    // 1 for while loop back, 1 for continue, 1 for break, 1 for if-skip
    EXPECT_GE(jumpCount, 3);
}

TEST_F(HVMCodeGeneratorTest, CompileNegativeConstants) {
    std::string code = R"(
        func : int64 getNegative() {
            return -123;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    bool foundNegative = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ADDI) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsI>(ops) && std::get<OperandsI>(ops).imm15 == -123) {
                foundNegative = true;
                break;
            }
        } else if (inst.getOpcode() == Opcode::ARITH) {
            auto ops = inst.getOperands();
            // SUB (func 1) with r0 (0) and some src register
            if (std::holds_alternative<OperandsR>(ops) && std::get<OperandsR>(ops).func == 1 && std::get<OperandsR>(ops).rs1 == 0) {
                foundNegative = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundNegative);
}

TEST_F(HVMCodeGeneratorTest, ErrorBreakOutsideLoop) {
    std::string code = R"(
        func : void badBreak() {
            break;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    // Should fail with error
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("break") != std::string::npos);
}

