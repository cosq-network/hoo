#include <gtest/gtest.h>
#include "core/HooCompiler.h"
#include "codegen/HVMCodeGenerator.h"
#include "hvm/HoModule.h"

using namespace hooc;
using namespace hvm;

class HVMCodeGeneratorComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler_ = std::make_unique<HooCompiler>();
    }

    std::unique_ptr<HooCompiler> compiler_;
};

TEST_F(HVMCodeGeneratorComprehensiveTest, LargeConstants) {
    std::string code = R"(
        func : int64 getLarge() {
            return 123456789012345;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    if (!module) {
        std::cerr << "Compilation failed: " << compiler_->getLastError() << std::endl;
    }
    ASSERT_NE(module, nullptr);

    // Should find LD_D instruction for large constant
    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundLdD = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::LD_D) {
            foundLdD = true;
            break;
        }
    }
    EXPECT_TRUE(foundLdD);

    // Check .rodata contains the value
    auto rodata = module->getSection(".rodata");
    ASSERT_NE(rodata, nullptr);
    EXPECT_GE(rodata->data.size(), 8);
    
    int64_t val;
    memcpy(&val, rodata->data.data(), 8);
    EXPECT_EQ(val, 123456789012345);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ForRangeWithStep) {
    std::string code = R"(
        func : int64 testForStep() {
            var sum = 0;
            for i in 0 .. 10 by 2 {
                sum = sum + i;
            }
            return sum;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    if (!module) {
        std::cerr << "Compilation failed: " << compiler_->getLastError() << std::endl;
    }
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    // Should have ARITH (ADD) for the step increment
    int arithCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) arithCount++;
    }
    // sum+i and i+step
    EXPECT_GE(arithCount, 2);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, TryCatchFinally) {
    std::string code = R"(
        func : void testTryFinally() {
            try {
                var x = 1;
            } catch (e: Exception) {
                var y = 2;
            } finally {
                var z = 3;
            }
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundPushHandler = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundPushHandler = true;
            break;
        }
    }
    EXPECT_TRUE(foundPushHandler);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, MultipleCatchClauses) {
    std::string code = R"(
        func : void testMultiCatch() {
            try {
                throw new Exception("oops");
            } catch (e: Exception) {
                var a = 1;
            } catch (e: Exception) {
                var b = 2;
            }
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundCall = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundCall = true;
            break;
        }
    }
    EXPECT_TRUE(foundCall);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, Rethrow) {
    std::string code = R"(
        func : void testRethrow() {
            try {
                var x = 1;
            } catch (e: Exception) {
                rethrow;
            }
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundCall = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundCall = true;
            break;
        }
    }
    EXPECT_TRUE(foundCall);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ComparisonOperators) {
    std::string code = R"(
        func : bool testComparisons(a: int64, b: int64) {
            var r1 = a < b;
            var r2 = a <= b;
            var r3 = a > b;
            var r4 = a >= b;
            var r5 = a != b;
            var r6 = a == b;
            return r1;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    int cmpCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CMP) cmpCount++;
    }
    EXPECT_EQ(cmpCount, 6);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, DivisionOperator) {
    std::string code = R"(
        func : int64 testDiv(a: int64, b: int64) {
            return a / b;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundDiv = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsR>(ops) && std::get<OperandsR>(ops).func == 5) {
                foundDiv = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundDiv);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, Literals) {
    std::string code = R"(
        class Dummy {
            func : Dummy getThis() { return this; }
        }
        func : void testLits() {
            var b1 = true;
            var b2 = false;
            var n = null;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    
    // true -> 1, false -> 0, null -> 0
    // MOVZ instructions
    int movzCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::MOVZ) movzCount++;
    }
    EXPECT_GE(movzCount, 3);
    
    // Check for MOV rX, r1 (this)
    bool foundThis = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::MOV) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsR>(ops) && std::get<OperandsR>(ops).rs1 == 1) {
                foundThis = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundThis);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, UnaryNot) {
    std::string code = R"(
        func : bool testNot(b: bool) {
            return !b;
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, RegisterPressure) {
    std::string code = R"(
        func : int64 pressure(a: int64) {
            return (a + 1) + (a + 2) + (a + 3) + (a + 4) + (a + 5) + (a + 6) + (a + 7) + (a + 8) + (a + 9);
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    if (!module) {
        EXPECT_TRUE(compiler_->getLastError().find("Register pressure") != std::string::npos);
    }
}

TEST_F(HVMCodeGeneratorComprehensiveTest, MethodCallImplicitThis) {
    std::string code = R"(
        class Calculator {
            var value: int64;
            func : void add(x: int64) {
                this.value = this.value + x;
            }
            func : void testInternal() {
                this.add(10);
            }
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundCall = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundCall = true;
            break;
        }
    }
    EXPECT_TRUE(foundCall);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, QualifiedNew) {
    std::string code = R"(
        class MyClass { var x: int64; }
        func : void test() {
            var o = new MyClass();
        }
    )";

    auto module = compiler_->compileToHVM("test", code);
    ASSERT_NE(module, nullptr);
}
