#include <gtest/gtest.h>
#include "core/HooCompiler.h"
#include "codegen/HVMCodeGenerator.h"
#include "hvm/HOModule.h"

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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, RegisterPressure) {
    std::string code = R"(
        func : int64 pressure(a: int64) {
            return (a + 1) + (a + 2) + (a + 3) + (a + 4) + (a + 5) + (a + 6) + (a + 7) + (a + 8) + (a + 9);
        }
    )";

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
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

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ConstructorWithParameters) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) {
                this.x = x;
                this.y = y;
            }
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundEnter = false;
    bool foundStore = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ENTER) foundEnter = true;
        if (inst.getOpcode() == Opcode::ST_D) foundStore = true;
    }
    EXPECT_TRUE(foundEnter);
    EXPECT_TRUE(foundStore);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ForRangeWithBody) {
    std::string code = R"(
        func : int64 sumTo(n: int64) {
            var sum = 0;
            for i in 1 .. n {
                sum = sum + i;
            }
            return sum;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    // For-range should produce: ENTER, LD, CMP, branch, ADD, ST, JMP
    int cmpCount = 0;
    int branchCount = 0;
    int arithCount = 0;
    for (const auto& inst : insts) {
        switch (inst.getOpcode()) {
            case Opcode::CMP: cmpCount++; break;
            case Opcode::BEQ:
            case Opcode::BNE:
            case Opcode::BLT:
            case Opcode::BLE: branchCount++; break;
            case Opcode::ARITH: arithCount++; break;
            default: break;
        }
    }
    EXPECT_GE(cmpCount, 1);
    EXPECT_GE(branchCount, 1);
    EXPECT_GE(arithCount, 2); // sum+i and i+step (step=1)
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ManyLocalVariablesSucceed) {
    std::string code = R"(
        func : int64 sumMany() {
            var a1 = 1;  var a2 = 2;  var a3 = 3;
            var a4 = 4;  var a5 = 5;  var a6 = 6;
            var a7 = 7;  var a8 = 8;  var a9 = 9;
            var a10 = 10; var a11 = 11; var a12 = 12;
            return a1 + a2 + a3 + a4 + a5 + a6
                 + a7 + a8 + a9 + a10 + a11 + a12;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    // Should produce ARITH instructions for the additions
    int arithCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) arithCount++;
    }
    EXPECT_GE(arithCount, 11); // 12 values combined = 11 additions
}

TEST_F(HVMCodeGeneratorComprehensiveTest, MethodCallMangledSymbol) {
    std::string code = R"(
        class Calculator {
            var value: int64;
            func : void add(x: int64) {
                this.value = this.value + x;
            }
        }
        func : void test() {
            var calc = new Calculator();
            calc.add(10);
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    // The call site uses "ptr" (p) for arguments since type inference is not yet available.
    // The symbol is registered as undefined; the mangled format includes className and methodName.
    auto* sym = module->getSymbol("_F_Calculator_add_v_p");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->type, Symbol::STT_FUNC);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ScopeNesting) {
    std::string code = R"(
        func : int64 test() {
            var x = 1;
            if (true) {
                var x = 2;
            }
            return x;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundStore = false;
    bool foundReturnMov = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ST_D) foundStore = true;
        if (inst.getOpcode() == Opcode::MOV) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsR>(ops)) {
                auto r = std::get<OperandsR>(ops);
                if (r.rd == 1) foundReturnMov = true;
            }
        }
    }
    EXPECT_TRUE(foundStore);
    EXPECT_TRUE(foundReturnMov);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ScopeIsolationError) {
    std::string code = R"(
        func : int64 test() {
            if (true) {
                var x = 42;
            }
            return x;
        }
    )";

    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Undefined variable") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ClassWithMethodAndFieldAccess) {
    std::string code = R"(
        class Counter {
            var count: int64;
            func : void increment() {
                this.count = this.count + 1;
            }
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    // Method should have field access: LD_D for this.count, ARITH for +1, ST_D for store
    bool foundLoad = false;
    bool foundArith = false;
    bool foundStore = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::LD_D) foundLoad = true;
        if (inst.getOpcode() == Opcode::ARITH) foundArith = true;
        if (inst.getOpcode() == Opcode::ST_D) foundStore = true;
    }
    EXPECT_TRUE(foundLoad);
    EXPECT_TRUE(foundArith);
    EXPECT_TRUE(foundStore);
}
