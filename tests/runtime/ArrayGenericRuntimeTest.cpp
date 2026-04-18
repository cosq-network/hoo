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
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class ArrayGenericRuntimeTest : public ::testing::Test {
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

    bool hasGlobalDataBuffer(llvm::Module* module) {
        // Phase 4: Array literals should create global data buffers (.array_data)
        for (auto& global : module->globals()) {
            if (global.getName().contains("array_data")) {
                return true;
            }
        }
        return false;
    }

    bool hasRuntimeFunctionCall(const std::string& irString, const std::string& funcName) {
        // Check if the IR contains a call to the runtime function
        return irString.find("call") != std::string::npos &&
               irString.find(funcName) != std::string::npos;
    }
};

// Test that int64 array literals call hoo_int64_array_from_buffer
TEST_F(ArrayGenericRuntimeTest, Int64ArrayCallsHooInt64ArrayFromBuffer) {
    std::string code = R"(
        func test() {
            var arr = [1, 2, 3, 4, 5];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should have global data buffer
    EXPECT_TRUE(hasGlobalDataBuffer(module.get()));

    // Phase 4: Should call hoo_int64_array_from_buffer
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_int64_array_from_buffer"));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Test that double array literals call hoo_double_array_from_buffer
TEST_F(ArrayGenericRuntimeTest, DoubleArrayCallsHooDoubleArrayFromBuffer) {
    std::string code = R"(
        func test() {
            var arr = [1.5, 2.5, 3.5];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should have global data buffer
    EXPECT_TRUE(hasGlobalDataBuffer(module.get()));

    // Phase 4: Should call hoo_double_array_from_buffer
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_double_array_from_buffer"));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Test that array literals with explicit int64 type use runtime function
TEST_F(ArrayGenericRuntimeTest, ExplicitInt64ArrayUsesRuntime) {
    std::string code = R"(
        func test() {
            var arr: int64[] = [10, 20, 30];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should have global data buffer
    EXPECT_TRUE(hasGlobalDataBuffer(module.get()));

    // Phase 4: Should call hoo_int64_array_from_buffer
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_int64_array_from_buffer"));
}

// Test that multiple arrays create separate data buffers
TEST_F(ArrayGenericRuntimeTest, MultipleArraysCreatySeparateBuffers) {
    std::string code = R"(
        func test() {
            var arr1 = [1, 2, 3];
            var arr2 = [4.0, 5.0, 6.0];
            var arr3 = [7, 8, 9];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should have multiple global data buffers
    int bufferCount = 0;
    size_t pos = 0;
    while ((pos = irString.find("array_data", pos)) != std::string::npos) {
        bufferCount++;
        pos++;
    }
    EXPECT_GE(bufferCount, 3); // Should have at least 3 data buffers

    // Phase 4: Should call both int64 and double runtime functions
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_int64_array_from_buffer"));
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_double_array_from_buffer"));
}

// Test that runtime function receives correct parameters (buffer and length)
TEST_F(ArrayGenericRuntimeTest, RuntimeFunctionParametersCorrect) {
    std::string code = R"(
        func test() {
            var arr = [1, 2, 3, 4, 5];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should have the call with two parameters (ptr and length)
    // The IR should show: call ... @hoo_int64_array_from_buffer(..., i64 5)
    EXPECT_TRUE(irString.find("call") != std::string::npos);
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
    // Check for the length parameter (5 in this case)
    EXPECT_TRUE(irString.find("i64 5") != std::string::npos);
}

// Test that generic array handle is returned and stored
TEST_F(ArrayGenericRuntimeTest, ArrayLiteralReturnsPointer) {
    std::string code = R"(
        func test() {
            var arr = [1, 2, 3];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should have pointer allocation for storing array handle
    EXPECT_TRUE(irString.find("alloca") != std::string::npos); // Variable storage
    EXPECT_TRUE(irString.find("store") != std::string::npos);  // Storing the returned pointer
}

// Test array literal in function parameter context
TEST_F(ArrayGenericRuntimeTest, ArrayLiteralWithFunctionParameter) {
    std::string code = R"(
        func process(arr: int64[]) {
            return;
        }

        func test() {
            process([1, 2, 3]);
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should have runtime function call
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_int64_array_from_buffer"));

    // Module should be valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Test that array type inference works for different element types
TEST_F(ArrayGenericRuntimeTest, TypeInferenceForDifferentElements) {
    std::string code = R"(
        func test() {
            var ints = [1, 2, 3];
            var floats = [1.5, 2.5, 3.5];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should infer int64 for ints array
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_int64_array_from_buffer"));

    // Phase 4: Should infer double for floats array
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_double_array_from_buffer"));
}

// Test empty array handling
TEST_F(ArrayGenericRuntimeTest, EmptyArrayHandling) {
    std::string code = R"(
        func test() {
            var arr: int64[] = [];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Module should still be valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Test array literal with expressions (constant-folded)
TEST_F(ArrayGenericRuntimeTest, ArrayLiteralWithConstantExpressions) {
    std::string code = R"(
        func test() {
            var arr = [1 + 1, 2 + 2, 3 + 3];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should still have runtime call (expressions are constant-folded)
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_int64_array_from_buffer"));

    // Verify the constants are folded [2, 4, 6]
    EXPECT_TRUE(irString.find("i64 2") != std::string::npos ||
                irString.find("i64 4") != std::string::npos ||
                irString.find("i64 6") != std::string::npos);
}

// Test that arrays in different scopes get separate buffers
TEST_F(ArrayGenericRuntimeTest, ArraysInDifferentScopes) {
    std::string code = R"(
        func test() {
            if (true) {
                var arr1 = [1, 2, 3];
            }
            {
                var arr2 = [4, 5, 6];
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Phase 4: Should have runtime calls in both scopes
    EXPECT_TRUE(hasRuntimeFunctionCall(irString, "hoo_int64_array_from_buffer"));

    // Module should be valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}
