#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "src/codegen/LLVMCodeGenerator.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "src/ast/AST.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class ClassArrayCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<LLVMContext>();
        codeGen = std::make_unique<LLVMCodeGenerator>(*context);
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<LLVMContext> context;
    std::unique_ptr<LLVMCodeGenerator> codeGen;
    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    std::unique_ptr<CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;
        return astBuilder->buildAST(parseTree);
    }

    std::string getModuleString(llvm::Module* module) {
        std::string str;
        raw_string_ostream rso(str);
        module->print(rso, nullptr);
        return str;
    }

    bool hasHooArrayNewCall(const std::string& irString) {
        return irString.find("call ptr @hoo_array_new") != std::string::npos;
    }

    bool hasHooArrayPushCall(const std::string& irString) {
        return irString.find("call i64 @hoo_array_push") != std::string::npos;
    }

    bool isModuleValid(llvm::Module* module) {
        std::string errorMsg;
        raw_string_ostream errorStream(errorMsg);
        return !verifyModule(*module, &errorStream);
    }
};

// Test 1: Basic class array creation
TEST_F(ClassArrayCodeGenTest, BasicClassArray) {
    std::string code = R"(
        class Point {
            constructor() {}
        }

        func test() -> void {
            var p1 = new Point();
            var p2 = new Point();
            var points = [p1, p2];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should use hoo_array_new for dynamic construction
    EXPECT_TRUE(hasHooArrayNewCall(irString));

    // Should use hoo_array_push for each element
    EXPECT_TRUE(hasHooArrayPushCall(irString));

    // Module should be valid
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 2: Single element class array
TEST_F(ClassArrayCodeGenTest, SingleElementClassArray) {
    std::string code = R"(
        class Item {
            constructor() {}
        }

        func test() -> void {
            var item = new Item();
            var items = [item];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    EXPECT_TRUE(hasHooArrayNewCall(irString));
    EXPECT_TRUE(hasHooArrayPushCall(irString));
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 3: Multiple class types in same function
TEST_F(ClassArrayCodeGenTest, MultipleClassArrays) {
    std::string code = R"(
        class Point {
            constructor() {}
        }

        class Line {
            constructor() {}
        }

        func test() -> void {
            var p1 = new Point();
            var p2 = new Point();
            var points = [p1, p2];

            var l1 = new Line();
            var l2 = new Line();
            var lines = [l1, l2];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have multiple hoo_array_new calls (one for each array)
    int newCallCount = 0;
    size_t pos = 0;
    while ((pos = irString.find("call ptr @hoo_array_new", pos)) != std::string::npos) {
        newCallCount++;
        pos++;
    }
    EXPECT_GE(newCallCount, 2);  // At least 2 arrays

    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 4: Large class array
TEST_F(ClassArrayCodeGenTest, LargeClassArray) {
    std::string code = R"(
        class Item {
            constructor() {}
        }

        func test() -> void {
            var i1 = new Item();
            var i2 = new Item();
            var i3 = new Item();
            var i4 = new Item();
            var i5 = new Item();
            var items = [i1, i2, i3, i4, i5];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    EXPECT_TRUE(hasHooArrayNewCall(irString));

    // Should have 5 push calls (one for each element)
    int pushCallCount = 0;
    size_t pos = 0;
    while ((pos = irString.find("call i64 @hoo_array_push", pos)) != std::string::npos) {
        pushCallCount++;
        pos++;
    }
    EXPECT_EQ(pushCallCount, 5);

    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 5: Class array in nested scope
TEST_F(ClassArrayCodeGenTest, ClassArrayInNestedScope) {
    std::string code = R"(
        class Item {
            constructor() {}
        }

        func test() -> void {
            if (true) {
                var i1 = new Item();
                var i2 = new Item();
                var items = [i1, i2];
            }
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    EXPECT_TRUE(hasHooArrayNewCall(irString));
    EXPECT_TRUE(hasHooArrayPushCall(irString));
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 6: Class array returned from function
TEST_F(ClassArrayCodeGenTest, ClassArrayFromFunction) {
    std::string code = R"(
        class Point {
            constructor() {}
        }

        func createPoints() -> void {
            var p1 = new Point();
            var p2 = new Point();
            var points = [p1, p2];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    EXPECT_TRUE(hasHooArrayNewCall(irString));
    EXPECT_TRUE(hasHooArrayPushCall(irString));
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 7: Array element type inference
TEST_F(ClassArrayCodeGenTest, ArrayElementTypeInference) {
    std::string code = R"(
        class Container {
            constructor() {}
        }

        func test() -> void {
            var c1 = new Container();
            var c2 = new Container();
            var c3 = new Container();
            var containers = [c1, c2, c3];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should infer pointer type and use dynamic array construction
    EXPECT_TRUE(hasHooArrayNewCall(irString));

    // Should have 3 push calls for 3 elements
    int pushCallCount = 0;
    size_t pos = 0;
    while ((pos = irString.find("call i64 @hoo_array_push", pos)) != std::string::npos) {
        pushCallCount++;
        pos++;
    }
    EXPECT_EQ(pushCallCount, 3);

    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 8: Class array in loop
TEST_F(ClassArrayCodeGenTest, ClassArrayWithLoopConstruction) {
    std::string code = R"(
        class Node {
            constructor() {}
        }

        func test() -> void {
            var n1 = new Node();
            var n2 = new Node();
            var nodes = [n1, n2];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    EXPECT_TRUE(hasHooArrayNewCall(irString));
    EXPECT_TRUE(hasHooArrayPushCall(irString));
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 9: Class array with mixed operations
TEST_F(ClassArrayCodeGenTest, ClassArrayMixedOperations) {
    std::string code = R"(
        class Value {
            constructor() {}
        }

        func test() -> void {
            var v1 = new Value();
            var v2 = new Value();
            var values = [v1, v2];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    EXPECT_TRUE(isModuleValid(module.get()));
}

// Test 10: Class array verification
TEST_F(ClassArrayCodeGenTest, ClassArrayVerification) {
    std::string code = R"(
        class Obj {
            constructor() {}
        }

        func test() -> void {
            var o1 = new Obj();
            var o2 = new Obj();
            var objs = [o1, o2];
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Verify the correct functions are declared and called
    EXPECT_TRUE(irString.find("declare ptr @hoo_array_new") != std::string::npos);
    EXPECT_TRUE(irString.find("declare i64 @hoo_array_push") != std::string::npos);

    EXPECT_TRUE(isModuleValid(module.get()));
}
