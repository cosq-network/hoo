#include <gtest/gtest.h>
#include "src/codegen/LLVMCodeGenerator.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "ast/AST.h"
#include "ast/ClassDeclaration.h"
#include "HoocParser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "antlr4-runtime.h"

using namespace hooc::ast;

namespace hooc {
namespace tests {

class NewExpressionCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<llvm::LLVMContext>();
        codeGen = std::make_unique<LLVMCodeGenerator>(*context);
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ast::CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;

        auto* ctx = dynamic_cast<HoocParser::CompilationUnitContext*>(parseTree);
        if (!ctx) return nullptr;

        return astBuilder->buildAST(ctx);
    }

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<LLVMCodeGenerator> codeGen;
    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;
};

// Test 1: Simple new expression codegen
TEST_F(NewExpressionCodeGenTest, SimpleNewExpressionCodeGen) {
    std::string code = R"(
        class Widget {
            constructor() {}
        }

        func test() {
            var w = new Widget();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify functions exist
    EXPECT_NE(module->getFunction("Widget_init"), nullptr);
    EXPECT_NE(module->getFunction("test"), nullptr);

    // Verify runtime functions declared
    EXPECT_NE(module->getFunction("hoo_alloc"), nullptr);
}

// Test 2: New expression with single argument
TEST_F(NewExpressionCodeGenTest, NewExpressionSingleArgumentCodeGen) {
    std::string code = R"(
        class Counter {
            constructor(initial: int64) {}
        }

        func test() {
            var c = new Counter(42);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Counter_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Constructor should have 2 params: this + int64
    EXPECT_EQ(ctorFunc->arg_size(), 2);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
}

// Test 3: New expression with multiple arguments
TEST_F(NewExpressionCodeGenTest, NewExpressionMultipleArgumentsCodeGen) {
    std::string code = R"(
        class Point {
            constructor(x: int64, y: int64) {}
        }

        func test() {
            var p = new Point(10, 20);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Point_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Constructor should have 3 params: this + 2 ints
    EXPECT_EQ(ctorFunc->arg_size(), 3);
}

// Test 4: New expression with mixed argument types
TEST_F(NewExpressionCodeGenTest, NewExpressionMixedArgumentTypesCodeGen) {
    std::string code = R"(
        class Data {
            constructor(id: int64, value: double, flag: bool) {}
        }

        func test() {
            var d = new Data(42, 3.14, true);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Data_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Constructor should have 4 params: this + int64 + double + bool
    EXPECT_EQ(ctorFunc->arg_size(), 4);
}

// Test 5: New expression used in expression statement
TEST_F(NewExpressionCodeGenTest, NewExpressionInExpressionStatementCodeGen) {
    std::string code = R"(
        class Box {
            constructor() {}
        }

        func test() {
            var box = new Box();
            var another = new Box();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify IR contains constructor calls
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("Box_init") != std::string::npos);
}

// Test 6: Multiple new expressions
TEST_F(NewExpressionCodeGenTest, MultipleNewExpressionsCodeGen) {
    std::string code = R"(
        class Item {
            constructor() {}
        }

        func test() {
            var a = new Item();
            var b = new Item();
            var c = new Item();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify IR contains multiple hoo_alloc calls
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    // Count occurrences of hoo_alloc calls
    int allocCount = 0;
    size_t pos = 0;
    while ((pos = ir.find("call", pos)) != std::string::npos) {
        if (ir.find("hoo_alloc", pos) != std::string::npos) {
            allocCount++;
        }
        pos += 4;
    }
    EXPECT_GE(allocCount, 3);
}

// Test 7: New expression with expression arguments
TEST_F(NewExpressionCodeGenTest, NewExpressionWithExpressionArgumentsCodeGen) {
    std::string code = R"(
        class Sum {
            constructor(x: int64, y: int64) {}
        }

        func test() {
            var a = 10;
            var b = 20;
            var s = new Sum(a + b, a * 2);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify IR contains arithmetic operations
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("add") != std::string::npos);
    EXPECT_TRUE(ir.find("mul") != std::string::npos);
}

// Test 8: New expression with function call arguments
TEST_F(NewExpressionCodeGenTest, NewExpressionWithFunctionCallArgumentsCodeGen) {
    std::string code = R"(
        func:int64 getValue() {
            return 42;
        }

        class Value {
            constructor(val: int64) {}
        }

        func test() {
            var v = new Value(getValue());
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify IR contains function call to getValue
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("getValue") != std::string::npos);
}

// Test 9: New expression with variable arguments
TEST_F(NewExpressionCodeGenTest, NewExpressionWithVariableArgumentsCodeGen) {
    std::string code = R"(
        class Holder {
            constructor(x: int64, y: int64, z: int64) {}
        }

        func test() {
            var a = 1;
            var b = 2;
            var c = 3;
            var h = new Holder(a, b, c);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Holder_init");
    ASSERT_NE(ctorFunc, nullptr);
    EXPECT_EQ(ctorFunc->arg_size(), 4);
}

// Test 10: New expression in if statement
TEST_F(NewExpressionCodeGenTest, NewExpressionInIfStatementCodeGen) {
    std::string code = R"(
        class Obj {
            constructor() {}
        }

        func test() {
            if (true) {
                var o = new Obj();
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("hoo_alloc") != std::string::npos);
}

// Test 11: New expression with all primitive types
TEST_F(NewExpressionCodeGenTest, NewExpressionAllPrimitiveTypesCodeGen) {
    std::string code = R"(
        class AllTypes {
            constructor(
                i8: int8,
                i: int64,
                f: float,
                d: double,
                bo: bool,
                c: char
            ) {}
        }

        func test() {
            var a = new AllTypes(1, 2, 3.0, 4.0, true, 65);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("AllTypes_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Constructor should have 7 params: this + 6 types
    EXPECT_EQ(ctorFunc->arg_size(), 7);
}

// Test 12: New expression in nested blocks
TEST_F(NewExpressionCodeGenTest, NewExpressionInNestedBlocksCodeGen) {
    std::string code = R"(
        class Nested {
            constructor() {}
        }

        func test() {
            if (true) {
                if (true) {
                    var n = new Nested();
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
}

// Test 13: New expression with complex expression arguments
TEST_F(NewExpressionCodeGenTest, NewExpressionComplexExpressionsCodeGen) {
    std::string code = R"(
        class Calculator {
            constructor(result: int64) {}
        }

        func test() {
            var a = 10;
            var b = 20;
            var c = new Calculator((a + b) * 2 - 5);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("add") != std::string::npos);
    EXPECT_TRUE(ir.find("mul") != std::string::npos);
    EXPECT_TRUE(ir.find("sub") != std::string::npos);
}

// Test 14: New expression assigned to typed variable
TEST_F(NewExpressionCodeGenTest, NewExpressionWithTypeAnnotationCodeGen) {
    std::string code = R"(
        class Widget {
            constructor() {}
        }

        func test() {
            var w: Widget = new Widget();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
}

// Test 15: Multiple different classes
TEST_F(NewExpressionCodeGenTest, MultipleClassesNewExpressionsCodeGen) {
    std::string code = R"(
        class ClassA {
            constructor() {}
        }

        class ClassB {
            constructor(x: int64) {}
        }

        class ClassC {
            constructor(x: int64, y: int64) {}
        }

        func test() {
            var a = new ClassA();
            var b = new ClassB(42);
            var c = new ClassC(10, 20);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    EXPECT_NE(module->getFunction("ClassA_init"), nullptr);
    EXPECT_NE(module->getFunction("ClassB_init"), nullptr);
    EXPECT_NE(module->getFunction("ClassC_init"), nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
}

// Test 16: New expression with boolean arguments
TEST_F(NewExpressionCodeGenTest, NewExpressionBooleanArgumentsCodeGen) {
    std::string code = R"(
        class Config {
            constructor(enabled: bool, debug: bool) {}
        }

        func test() {
            var cfg = new Config(true, false);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Config_init");
    ASSERT_NE(ctorFunc, nullptr);
    EXPECT_EQ(ctorFunc->arg_size(), 3);
}

// Test 17: New expression calls constructor function
TEST_F(NewExpressionCodeGenTest, NewExpressionCallsConstructorCodeGen) {
    std::string code = R"(
        class Init {
            constructor(v: int64) {}
        }

        func test() {
            var obj = new Init(100);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify IR contains calls to both hoo_alloc and constructor
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("hoo_alloc") != std::string::npos);
    EXPECT_TRUE(ir.find("Init_init") != std::string::npos);
}

// Test 18: New expression with different class names
TEST_F(NewExpressionCodeGenTest, NewExpressionDifferentClassNamesCodeGen) {
    std::string code = R"(
        class ClassOne {
            constructor(x: int64) {}
        }

        class ClassTwo {
            constructor(y: int64) {}
        }

        func test() {
            var o = new ClassOne(10);
            var t = new ClassTwo(20);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify both constructors are generated
    auto* ctorOne = module->getFunction("ClassOne_init");
    auto* ctorTwo = module->getFunction("ClassTwo_init");
    ASSERT_NE(ctorOne, nullptr);
    ASSERT_NE(ctorTwo, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("ClassOne_init") != std::string::npos);
    EXPECT_TRUE(ir.find("ClassTwo_init") != std::string::npos);
}

// Test 19: New expression with no-op constructor
TEST_F(NewExpressionCodeGenTest, NewExpressionNoOpConstructorCodeGen) {
    std::string code = R"(
        class Empty {
            constructor() {}
        }

        func test() {
            var e = new Empty();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Empty_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Constructor should only have 'this' parameter
    EXPECT_EQ(ctorFunc->arg_size(), 1);
}

// Test 20: New expression storage to variable
TEST_F(NewExpressionCodeGenTest, NewExpressionStoredToVariableCodeGen) {
    std::string code = R"(
        class Storage {
            constructor() {}
        }

        func test() {
            var s = new Storage();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify IR contains alloca and store
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("alloca") != std::string::npos);
    EXPECT_TRUE(ir.find("store") != std::string::npos);
}

} // namespace tests
} // namespace hooc
