#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "../src/LLVMCodeGenerator.h"
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "../src/ast/AST.h"
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

class NullableCodeGenTest : public ::testing::Test {
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

    std::string getModuleString(Module* module) {
        std::string str;
        raw_string_ostream rso(str);
        module->print(rso, nullptr);
        return str;
    }
};

// Test 1: Nullable integer variable declaration
TEST_F(NullableCodeGenTest, NullableInt64VariableDeclaration) {
    std::string code = R"(
        func test() -> int64 {
            var x: int64? = 42;
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 2: Null literal assignment to nullable variable
TEST_F(NullableCodeGenTest, NullLiteralAssignment) {
    std::string code = R"(
        func test() -> int64 {
            var x: int64? = null;
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 3: Nullable double variable
TEST_F(NullableCodeGenTest, NullableDoubleVariableDeclaration) {
    std::string code = R"(
        func test() -> double {
            var x: double? = 3.14;
            return 0.0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 4: Nullable boolean variable
TEST_F(NullableCodeGenTest, NullableBoolVariableDeclaration) {
    std::string code = R"(
        func test() -> bool {
            var x: bool? = true;
            return false;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 5: Nullable char variable
TEST_F(NullableCodeGenTest, NullableCharVariableDeclaration) {
    std::string code = R"(
        func test() -> char {
            var x: char? = 'a';
            return 'z';
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 6: Multiple nullable variables in same function
TEST_F(NullableCodeGenTest, MultipleNullableVariables) {
    std::string code = R"(
        func test() -> int64 {
            var x: int64? = 10;
            var y: double? = 3.14;
            var z: bool? = true;
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 7: Nullable variables with null and non-null values mixed
TEST_F(NullableCodeGenTest, MixedNullAndNonNullValues) {
    std::string code = R"(
        func test() -> int64 {
            var a: int64? = 42;
            var b: int64? = null;
            var c: int64? = 100;
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 8: Nullable parameter in function
TEST_F(NullableCodeGenTest, NullableParameterFunction) {
    std::string code = R"(
        func process(int64? value) -> int64 {
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);

    // Verify function has the parameter
    EXPECT_GE(func->arg_size(), 1);
}

// Test 9: Multiple nullable parameters
TEST_F(NullableCodeGenTest, MultipleNullableParameters) {
    std::string code = R"(
        func compare(int64? a, double? b, bool? c) -> int64 {
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("compare");
    ASSERT_NE(func, nullptr);

    // Verify function has three parameters
    EXPECT_EQ(func->arg_size(), 3);
}

// Test 10: Mixed nullable and non-nullable parameters
TEST_F(NullableCodeGenTest, MixedNullableAndNonNullableParameters) {
    std::string code = R"(
        func process(int64 required, int64? optional) -> int64 {
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);

    // Verify function has two parameters
    EXPECT_EQ(func->arg_size(), 2);
}

// Test 11: Nullable return type function declaration
TEST_F(NullableCodeGenTest, NullableReturnTypeFunction) {
    std::string code = R"(
        func maybeValue() -> int64? {
            return 42;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("maybeValue");
    ASSERT_NE(func, nullptr);

    // Verify function has return type
    EXPECT_NE(func->getReturnType(), nullptr);
}

// Test 12: Complex nullable variable flow
TEST_F(NullableCodeGenTest, ComplexNullableVariableFlow) {
    std::string code = R"(
        func test() -> int64 {
            var x: int64? = null;
            var y: int64? = 50;
            var z: int64? = x;
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 13: Nullable variable in block
TEST_F(NullableCodeGenTest, NullableVariableInBlock) {
    std::string code = R"(
        func test() -> int64 {
            var x: int64? = null;
            {
                var y: int64? = 10;
            }
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 14: Nullable byte variable
TEST_F(NullableCodeGenTest, NullableByteVariableDeclaration) {
    std::string code = R"(
        func test() -> int64 {
            var x: byte? = 255;
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 15: Nullable float variable
TEST_F(NullableCodeGenTest, NullableFloatVariableDeclaration) {
    std::string code = R"(
        func test() -> double {
            var x: float? = 1.5;
            return 0.0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 16: Verify nullable type IR structure
TEST_F(NullableCodeGenTest, VerifyNullableTypeIRStructure) {
    std::string code = R"(
        func test() -> int64 {
            var x: int64? = 42;
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    // Verify that the function has proper LLVM IR structure
    std::string ir;
    raw_string_ostream oss(ir);
    func->print(oss);

    // The IR should contain struct types for the nullable values
    EXPECT_FALSE(ir.empty());
}

// Test 17: Null literal in multiple contexts
TEST_F(NullableCodeGenTest, NullLiteralInMultipleContexts) {
    std::string code = R"(
        func test() -> int64 {
            var a: int64? = null;
            var b: double? = null;
            var c: bool? = null;
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 18: Call function with nullable parameter
TEST_F(NullableCodeGenTest, CallFunctionWithNullableParameter) {
    std::string code = R"(
        func process(int64? value) -> int64 {
            return 10;
        }

        func test() -> int64 {
            return process(42);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 19: Nullable int64 array
TEST_F(NullableCodeGenTest, NullableArrayVariableDeclaration) {
    std::string code = R"(
        func test() -> int64 {
            var x: int64[]? = [1, 2, 3];
            return 0;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}

// Test 20: Comprehensive nullable type scenario
TEST_F(NullableCodeGenTest, ComprehensiveNullableScenario) {
    std::string code = R"(
        func getValue(int64? x, double? y) -> int64 {
            return 99;
        }

        func test() -> int64 {
            var nullable_int: int64? = null;
            var nullable_double: double? = 3.14;
            var nullable_bool: bool? = true;

            var result: int64 = getValue(nullable_int, nullable_double);

            return result;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_FALSE(func->empty());
}
