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
    
    // `this` is preserved in the frame because runtime helper calls clobber r1.
    bool foundThisStore = false;
    bool foundThisLoad = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ST_D) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsI>(ops)) {
                auto i = std::get<OperandsI>(ops);
                if (i.rd == 1 && i.rs == 30) foundThisStore = true;
            }
        } else if (inst.getOpcode() == Opcode::LD_D) {
            auto ops = inst.getOperands();
            if (std::holds_alternative<OperandsI>(ops)) {
                auto i = std::get<OperandsI>(ops);
                if (i.rs == 30) foundThisLoad = true;
            }
        }
    }
    EXPECT_TRUE(foundThisStore);
    EXPECT_TRUE(foundThisLoad);
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
    // The symbol is registered as undefined; the mangled format includes modulePath, className and methodName.
    auto* sym = module->getSymbol("_F_M_test_E_Calculator_add_p_p");
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
    // Method should have field access via MOV+CALL instead of LD_D/ST_D.
    // Check that ARITH (+1) is present and that the module references the
    // field-access runtime functions in its symbol table.
    bool foundArith = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ARITH) foundArith = true;
    }
    bool foundGetField = false;
    bool foundSetField = false;
    for (const auto& sym : module->getSymbols()) {
        if (sym.name.find("_F_object_get_field_p_i8") != std::string::npos) foundGetField = true;
        if (sym.name.find("_F_object_set_field_v_p_i8_p") != std::string::npos) foundSetField = true;
    }
    EXPECT_TRUE(foundGetField);
    EXPECT_TRUE(foundArith);
    EXPECT_TRUE(foundSetField);
}

// ============================================================================
// Class Modifier Enforcement Tests
// ============================================================================

TEST_F(HVMCodeGeneratorComprehensiveTest, SingletonNoConstructor) {
    std::string code = R"(
        singleton class AppConfig {
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    EXPECT_NE(module->getSymbol("_F_module_init_v"), nullptr);
    auto* dataSec = module->getSection(".data");
    ASSERT_NE(dataSec, nullptr);
    EXPECT_GE(dataSec->data.size(), 8);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, SingletonWithNoArgConstructor) {
    std::string code = R"(
        singleton class AppConfig {
            var value: int64;
            constructor() {
                this.value = 42;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    EXPECT_NE(module->getSymbol("_F_module_init_v"), nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, SingletonConstructorWithArgs) {
    std::string code = R"(
        singleton class AppConfig {
            var value: int64;
            constructor(name: int64) {
                this.value = name;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("constructor must have no parameters") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, SingletonNewExpressionReturnsInstance) {
    std::string code = R"(
        singleton class App {
            var x: int64;
            constructor() {
                this.x = 1;
            }
        }
        func : void test() {
            var a = new App();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    EXPECT_NE(module->getSymbol("_F_module_init_v"), nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, SingletonWithMethods) {
    std::string code = R"(
        singleton class Logger {
            var level: int64;
            constructor() {
                this.level = 0;
            }
            func : void setLevel(l: int64) {
                this.level = l;
            }
        }
        func : void test() {
            var log = new Logger();
            log.setLevel(1);
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    EXPECT_NE(module->getSymbol("_F_module_init_v"), nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, MultipleSingletons) {
    std::string code = R"(
        singleton class A {
            var x: int64;
            constructor() { this.x = 1; }
        }
        singleton class B {
            var y: int64;
            constructor() { this.y = 2; }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    EXPECT_NE(module->getSymbol("_F_module_init_v"), nullptr);
    auto* dataSec = module->getSection(".data");
    ASSERT_NE(dataSec, nullptr);
    EXPECT_GE(dataSec->data.size(), 16);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ImmutableAssignmentInConstructor) {
    std::string code = R"(
        immutable class Point {
            var x: int64;
            constructor(x: int64) {
                this.x = x;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ImmutableAssignmentOutsideConstructor) {
    std::string code = R"(
        immutable class Point {
            var x: int64;
            constructor(x: int64) {
                this.x = x;
            }
            func : void setX(v: int64) {
                this.x = v;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot modify field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ImmutableCompoundAssignmentOutsideConstructor) {
    std::string code = R"(
        immutable class Counter {
            var count: int64;
            constructor() {
                this.count = 0;
            }
            func : void increment() {
                this.count += 1;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot modify field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ImmutableIncrementOutsideConstructor) {
    std::string code = R"(
        immutable class Counter {
            var count: int64;
            constructor() {
                this.count = 0;
            }
            func : void increment() {
                this.count++;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot modify field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ImmutableDecrementOutsideConstructor) {
    std::string code = R"(
        immutable class Counter {
            var count: int64;
            constructor() {
                this.count = 0;
            }
            func : void decrement() {
                this.count--;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot modify field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ImmutableReadInMethod) {
    std::string code = R"(
        immutable class Point {
            var x: int64;
            constructor(x: int64) {
                this.x = x;
            }
            func : int64 getX() {
                return this.x;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, MutableClassFieldAssignmentWorks) {
    std::string code = R"(
        class Point {
            var x: int64;
            constructor(x: int64) {
                this.x = x;
            }
            func : void setX(v: int64) {
                this.x = v;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, FinalClassExtension) {
    std::string code = R"(
        final class Base {}
        class Derived extends Base {}
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot extend final class") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, NonFinalClassExtension) {
    std::string code = R"(
        class Base {}
        class Derived extends Base {}
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, FinalIndirectExtension) {
    std::string code = R"(
        final class Base {}
        class Middle extends Base {}
        class Derived extends Middle {}
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot extend final class") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceModifierAccepted) {
    std::string code = R"(
        service class MyService {
            func : void doSomething() {}
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, AllRemainingModifiersCombined) {
    std::string code = R"(
        singleton immutable final class App {
            var x: int64;
            constructor() {
                this.x = 1;
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    EXPECT_NE(module->getSymbol("_F_module_init_v"), nullptr);
}

// ============================================================================
// Service Class Modifier Tests
// ============================================================================

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceClassNoConstructor) {
    std::string code = R"(
        service class Logger {
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceClassNoArgConstructor) {
    std::string code = R"(
        service class Logger {
            constructor() {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceClassWithServiceParam) {
    std::string code = R"(
        service class Config {
            constructor() {}
        }
        service class Logger {
            constructor(cfg: Config) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceClassWithMultipleServiceParams) {
    std::string code = R"(
        service class Config {
            constructor() {}
        }
        service class Cache {
            constructor() {}
        }
        service class Logger {
            constructor(cfg: Config, cache: Cache) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceClassWithServiceParamNoArg) {
    std::string code = R"(
        service class Config {}
        service class Logger {
            constructor(cfg: Config) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceCombinedWithSingleton) {
    std::string code = R"(
        service singleton class Bad {}
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("cannot also be singleton") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceCombinedWithImmutable) {
    std::string code = R"(
        service immutable class Bad {}
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("cannot also be immutable") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceCombinedWithFinal) {
    std::string code = R"(
        service final class Bad {}
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("cannot also be final") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceCombinedWithAllConflicting) {
    std::string code = R"(
        service singleton immutable final class Bad {}
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("cannot also be") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceClassNewKeyword) {
    std::string code = R"(
        service class Logger {
            constructor() {}
        }
        func : void test() {
            var log = new Logger();
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot create instance of service class") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceConstructorPrimitiveParam) {
    std::string code = R"(
        service class Logger {
            constructor(name: int64) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("cannot be primitive type") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceConstructorStringParam) {
    std::string code = R"(
        service class Logger {
            constructor(name: string) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("cannot be primitive type") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceConstructorNonServiceClassParam) {
    std::string code = R"(
        class Config {}
        service class Logger {
            constructor(cfg: Config) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("must be a service class") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceConstructorNonServiceClassParamForward) {
    std::string code = R"(
        service class Logger {
            constructor(cfg: Config) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("must be a service class") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceConstructorMixedParams) {
    std::string code = R"(
        service class Config {
            constructor() {}
        }
        service class Logger {
            constructor(cfg: Config, name: int64) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("cannot be primitive type") != std::string::npos ||
                compiler_->getLastError().find("must be a service class") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ServiceClassMethodsNoRestriction) {
    std::string code = R"(
        service class Calculator {
            constructor() {}
            func : int64 add(a: int64, b: int64) {
                return a + b;
            }
            func : void log(msg: string) {
            }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

// ============================================================================
// Field Access Modifier Enforcement Tests
// ============================================================================

TEST_F(HVMCodeGeneratorComprehensiveTest, PublicFieldReadOutsideClass) {
    std::string code = R"(
        class Data {
            public var value: int64;
            constructor() { this.value = 42; }
        }
        func : void test() {
            var d = new Data();
            var v = d.value;
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PublicFieldWriteOutsideClass) {
    std::string code = R"(
        class Data {
            public var value: int64;
        }
        func : void test() {
            var d = new Data();
            d.value = 99;
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateFieldReadOutsideClass) {
    std::string code = R"(
        class Data {
            private var secret: int64;
        }
        func : void test() {
            var d = new Data();
            var v = d.secret;
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot access private field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateFieldWriteOutsideClass) {
    std::string code = R"(
        class Data {
            private var secret: int64;
        }
        func : void test() {
            var d = new Data();
            d.secret = 99;
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot write to field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, DefaultVarFieldReadOutsideClass) {
    std::string code = R"(
        class Data {
            var value: int64;
        }
        func : void test() {
            var d = new Data();
            var v = d.value;
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, DefaultVarFieldWriteOutsideClass) {
    std::string code = R"(
        class Data {
            var value: int64;
        }
        func : void test() {
            var d = new Data();
            d.value = 99;
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot write to field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateFieldReadFromSameClass) {
    std::string code = R"(
        class Data {
            private var secret: int64;
            constructor() { this.secret = 42; }
            public func : int64 getSecret() {
                return this.secret;
            }
        }
        func : void test() {
            var d = new Data();
            var v = d.getSecret();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateFieldWriteFromSameClass) {
    std::string code = R"(
        class Data {
            private var secret: int64;
            public func : void setSecret(v: int64) {
                this.secret = v;
            }
        }
        func : void test() {
            var d = new Data();
            d.setSecret(99);
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, DefaultVarFieldReadFromDerivedClass) {
    std::string code = R"(
        class Base {
            var value: int64;
        }
        class Derived extends Base {
            public func : int64 getValue() {
                return this.value;
            }
        }
        func : void test() {
            var d = new Derived();
            var v = d.getValue();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, DefaultVarFieldWriteFromDerivedClass) {
    std::string code = R"(
        class Base {
            var value: int64;
        }
        class Derived extends Base {
            public func : void setValue(v: int64) {
                this.value = v;
            }
        }
        func : void test() {
            var d = new Derived();
            d.setValue(99);
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateFieldReadFromDerivedClass) {
    std::string code = R"(
        class Base {
            private var secret: int64;
            constructor() { this.secret = 42; }
        }
        class Derived extends Base {
            public func : int64 getSecret() {
                return this.secret;
            }
        }
        func : void test() {
            var d = new Derived();
            var v = d.getSecret();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateFieldWriteFromDerivedClass) {
    std::string code = R"(
        class Base {
            private var secret: int64;
        }
        class Derived extends Base {
            public func : void setSecret(v: int64) {
                this.secret = v;
            }
        }
        func : void test() {
            var d = new Derived();
            d.setSecret(99);
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateFieldCompoundAssignOutsideClass) {
    std::string code = R"(
        class Data {
            private var count: int64;
        }
        func : void test() {
            var d = new Data();
            d.count += 1;
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot write to field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, DefaultVarFieldIncrementOutsideClass) {
    std::string code = R"(
        class Data {
            var count: int64;
        }
        func : void test() {
            var d = new Data();
            d.count++;
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot write to field") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, DefaultVarFieldDecrementOutsideClass) {
    std::string code = R"(
        class Data {
            var count: int64;
        }
        func : void test() {
            var d = new Data();
            d.count--;
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot write to field") != std::string::npos);
}

// ============================================================================
// Class Modifier Enforcement Tests
// ============================================================================
// Public/Private Access Modifier Enforcement Tests
// ============================================================================

TEST_F(HVMCodeGeneratorComprehensiveTest, PublicMethodAccessibleOutsideClass) {
    std::string code = R"(
        class Helper {
            public func : void doSomething() {}
        }
        func : void test() {
            var h = new Helper();
            h.doSomething();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateMethodNotAccessibleOutsideClass) {
    std::string code = R"(
        class Helper {
            private func : void helper() {}
        }
        func : void test() {
            var h = new Helper();
            h.helper();
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot access private method") != std::string::npos);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, DefaultMethodAccessibleOutsideClass) {
    std::string code = R"(
        class Helper {
            func : void doSomething() {}
        }
        func : void test() {
            var h = new Helper();
            h.doSomething();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateMethodAccessibleFromSameClass) {
    std::string code = R"(
        class Helper {
            private func : void helper() {}
            public func : void callHelper() {
                this.helper();
            }
        }
        func : void test() {
            var h = new Helper();
            h.callHelper();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateMethodAccessibleFromDerivedClass) {
    std::string code = R"(
        class Base {
            private func : void helper() {}
        }
        class Derived extends Base {
            public func : void callHelper() {
                this.helper();
            }
        }
        func : void test() {
            var d = new Derived();
            d.callHelper();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PublicMethodAccessibleFromDerivedClass) {
    std::string code = R"(
        class Base {
            public func : void helper() {}
        }
        class Derived extends Base {
            public func : void callHelper() {
                this.helper();
            }
        }
        func : void test() {
            var d = new Derived();
            d.callHelper();
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PrivateMethodNotAccessibleFromUnrelatedClass) {
    std::string code = R"(
        class Helper {
            private func : void helper() {}
        }
        class Caller {
            public func : void callHelper(h: Helper) {
                h.helper();
            }
        }
        func : void test() {
            var h = new Helper();
            var c = new Caller();
            c.callHelper(h);
        }
    )";
    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler_->getLastError().find("Cannot access private method") != std::string::npos);
}

// ============================================================================
// NEW EXPRESSION CODEGEN TESTS
// ============================================================================

TEST_F(HVMCodeGeneratorComprehensiveTest, NewExpressionEmitsAllocCall) {
    std::string code = R"(
        class Widget {
            var x: int64;
            constructor() {
                this.x = 1;
            }
        }
        func : void test() {
            var w = new Widget();
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    bool foundAlloc = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) {
            foundAlloc = true;
            break;
        }
    }
    EXPECT_TRUE(foundAlloc);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, NewExpressionWithParams) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) {
                this.x = x;
                this.y = y;
            }
        }
        func : void test() {
            var p = new Point(10, 20);
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    int movCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::MOV) {
            movCount++;
        }
    }
    EXPECT_GE(movCount, 2);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, NewExpressionMultipleFields) {
    std::string code = R"(
        class Data {
            var a: int64;
            var b: int64;
            var c: int64;
            constructor(a: int64, b: int64, c: int64) {
                this.a = a;
                this.b = b;
                this.c = c;
            }
        }
        func : void test() {
            var d = new Data(1, 2, 3);
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    int stStoreCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ST_D) stStoreCount++;
    }
    EXPECT_GE(stStoreCount, 3);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, NewExpressionChainedSameType) {
    std::string code = R"(
        class Item {
            var value: int64;
            constructor(v: int64) {
                this.value = v;
            }
        }
        func : void test() {
            var a = new Item(1);
            var b = new Item(2);
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto insts = module->decodeInstructions(module->getSection(".text")->data);
    int callCount = 0;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CALL) callCount++;
    }
    EXPECT_GE(callCount, 3);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, NewExpressionMethodCall) {
    std::string code = R"(
        class Helper {
            func:int64 getValue() { return 42; }
        }
        func : int64 test() {
            var h = new Helper();
            return h.getValue();
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    auto* sym = module->getSymbol("_F_M_test_E_test_i8");
    if (!sym) sym = module->getSymbol("_F_test_i8");
    ASSERT_NE(sym, nullptr);
}

// ---------------------------------------------------------------------------
// tp register (r4) reservation tests
// ---------------------------------------------------------------------------

TEST_F(HVMCodeGeneratorComprehensiveTest, MethodWithMaxArgsSixCompiles) {
    // Methods can take up to 6 real params (r2,r3,r5,r6,r7,r8, skipping r4= tp)
    std::string code = R"(
        class Helper {
            func :int64 sum6(a: int64, b: int64, c: int64, d: int64, e: int64, f: int64) {
                return a + b + c + d + e + f;
            }
        }
        func : int64 test() {
            var h = new Helper();
            return h.sum6(1, 2, 3, 4, 5, 6);
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, MethodWithSevenArgsFails) {
    // Methods cannot take 7 real params (only 6 arg regs after reserving r4)
    std::string code = R"(
        class Helper {
            func :int64 sum7(a: int64, b: int64, c: int64, d: int64, e: int64, f: int64, g: int64) {
                return a + b + c + d + e + f + g;
            }
        }
        func : int64 test() {
            var h = new Helper();
            return h.sum7(1, 2, 3, 4, 5, 6, 7);
        }
    )";

    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PlainFunctionWithSevenArgsCompiles) {
    // Plain functions can take 7 params (r1,r2,r3,r5,r6,r7,r8)
    std::string code = R"(
        func :int64 sum7(a: int64, b: int64, c: int64, d: int64, e: int64, f: int64, g: int64) {
            return a + b + c + d + e + f + g;
        }
        func : int64 test() {
            return sum7(1, 2, 3, 4, 5, 6, 7);
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, PlainFunctionWithEightArgsFails) {
    // Plain functions cannot take 8 params (only 7 arg regs)
    std::string code = R"(
        func :int64 sum8(a: int64, b: int64, c: int64, d: int64, e: int64, f: int64, g: int64, h: int64) {
            return a + b + c + d + e + f + g + h;
        }
        func : int64 test() {
            return sum8(1, 2, 3, 4, 5, 6, 7, 8);
        }
    )";

    auto module = compiler_->compile("test", code);
    EXPECT_EQ(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ConstructorWithSixArgsCompiles) {
    // Constructors (isMethod=true) can take up to 6 params after this (r2..r8 skipping r4)
    std::string code = R"(
        class Data {
            var a: int64;
            var b: int64;
            var c: int64;
            var d: int64;
            var e: int64;
            var f: int64;
            constructor(a: int64, b: int64, c: int64, d: int64, e: int64, f: int64) {
                this.a = a; this.b = b; this.c = c;
                this.d = d; this.e = e; this.f = f;
            }
        }
        func :int64 test() {
            var d = new Data(1, 2, 3, 4, 5, 6);
            return d.a + d.b + d.c + d.d + d.e + d.f;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}

TEST_F(HVMCodeGeneratorComprehensiveTest, ArgR4ReservedTpNotUsed) {
    // Verify that instructions in a method with 3+ args never reference r4
    // as a source register in ST.D (which would indicate arg save via r4)
    std::string code = R"(
        class Math {
            func :int64 add3(a: int64, b: int64, c: int64) {
                return a + b + c;
            }
        }
        func : int64 test() {
            var m = new Math();
            return m.add3(10, 20, 30);
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);

    // Find the method's instructions and verify no register 4 use
    auto* text = module->getSection(".text");
    ASSERT_NE(text, nullptr);
    auto insts = module->decodeInstructions(text->data);
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::ST_D) {
            const auto& ops = std::get<OperandsI>(inst.getOperands());
            EXPECT_NE(ops.rd, 4) << "ST.D should not use r4 (tp) as source";
        }
    }
}

TEST_F(HVMCodeGeneratorComprehensiveTest, LocalVariableNamedR4DoesNotConflict) {
    // A local named r4 should not conflict with tp register
    std::string code = R"(
        func :int64 test() {
            var r4: int64 = 42;
            return r4;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
}
