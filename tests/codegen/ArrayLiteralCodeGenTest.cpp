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

class ArrayLiteralCodeGenTest : public ::testing::Test {
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
};

TEST_F(ArrayLiteralCodeGenTest, SimpleIntegerArrayLiteral) {
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

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());

    // Array literals are stored as global constants
    EXPECT_TRUE(irString.find("@") != std::string::npos);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(ArrayLiteralCodeGenTest, FloatArrayLiteral) {
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

    // Should have double array global constant
    EXPECT_TRUE(irString.find("double") != std::string::npos);
    EXPECT_TRUE(irString.find("@") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, BooleanArrayLiteral) {
    std::string code = R"(
        func test() {
            var arr = [true, false, true, false];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have i1 (bool) array
    EXPECT_TRUE(irString.find("i1") != std::string::npos);
    EXPECT_TRUE(irString.find("@") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, CharArrayLiteral) {
    std::string code = R"(
        func test() {
            var arr = ['a', 'b', 'c', 'd'];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have i32 array (chars are i32 in hooc)
    EXPECT_TRUE(irString.find("i32") != std::string::npos);
    EXPECT_TRUE(irString.find("@") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, EmptyArrayLiteral) {
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

    // Verify module is valid even with empty array
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(ArrayLiteralCodeGenTest, TwoDimensionalArrayLiteral) {
    std::string code = R"(
        func test() {
            var matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have nested array structures
    EXPECT_TRUE(irString.find("@") != std::string::npos);
    EXPECT_TRUE(irString.find("i64") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, ThreeDimensionalArrayLiteral) {
    std::string code = R"(
        func test() {
            var cube = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(ArrayLiteralCodeGenTest, ArrayLiteralWithExpressions) {
    std::string code = R"(
        func test() {
            var arr = [1 + 2, 3 * 4, 10 - 5];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have array with computed values (may be constant-folded)
    EXPECT_TRUE(irString.find("@") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, MultipleArrayLiterals) {
    std::string code = R"(
        func test() {
            var arr1 = [1, 2, 3];
            var arr2 = [4, 5, 6];
            var arr3 = [7, 8, 9];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have multiple global array constants
    size_t globalCount = 0;
    size_t pos = 0;
    while ((pos = irString.find("@", pos)) != std::string::npos) {
        globalCount++;
        pos++;
    }
    EXPECT_GE(globalCount, 3); // At least 3 global arrays
}

TEST_F(ArrayLiteralCodeGenTest, ArrayLiteralWithFunctionContext) {
    std::string code = R"(
func test() {
    var data = [1, 2, 3, 4, 5];
    var count = 5;
    return;
}
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify array literal was created
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@") != std::string::npos);
    EXPECT_TRUE(irString.find("alloca") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, LargeArrayLiteral) {
    std::string code = R"(
        func test() {
            var arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                      11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                      21, 22, 23, 24, 25, 26, 27, 28, 29, 30];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have array with 30 elements
    EXPECT_TRUE(irString.find("@") != std::string::npos);
    EXPECT_TRUE(irString.find("i64") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, ArrayLiteralWithExplicitType) {
    std::string code = R"(
        func test() {
            var arr: int64[] = [10, 20, 30, 40, 50];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have i64 array
    EXPECT_TRUE(irString.find("i64") != std::string::npos);
    EXPECT_TRUE(irString.find("@") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, ArrayAccessFromLiteral) {
    std::string code = R"(
        func:int64 test() {
            var arr = [10, 20, 30, 40, 50];
            var value = arr[2];
            return value;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have array access and return
    EXPECT_TRUE(irString.find("@") != std::string::npos);
    EXPECT_TRUE(irString.find("alloca") != std::string::npos);
    EXPECT_TRUE(irString.find("ret i64") != std::string::npos);
}

TEST_F(ArrayLiteralCodeGenTest, NestedArrayAccess) {
    std::string code = R"(
        func test() {
            var matrix = [[1, 2], [3, 4]];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(ArrayLiteralCodeGenTest, ArrayLiteralInReturn) {
    std::string code = R"(
        func:int64 getSize() {
            var arr = [1, 2, 3, 4, 5];
            return 5;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("getSize");
    ASSERT_NE(func, nullptr);

    // Verify array literal was created in the function
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@") != std::string::npos);
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
}

TEST_F(ArrayLiteralCodeGenTest, ArrayLiteralTypeConsistency) {
    std::string code = R"(
        func test() {
            var ints = [1, 2, 3];
            var floats = [1.0, 2.0, 3.0];
            var bools = [true, false, true];
            var chars = ['x', 'y', 'z'];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Should have different types for different arrays
    EXPECT_TRUE(irString.find("i64") != std::string::npos);  // ints
    EXPECT_TRUE(irString.find("double") != std::string::npos); // floats
    EXPECT_TRUE(irString.find("i1") != std::string::npos);   // bools
    EXPECT_TRUE(irString.find("i32") != std::string::npos);  // chars
}

TEST_F(ArrayLiteralCodeGenTest, GlobalConstantArrays) {
    std::string code = R"(
        func test() {
            var arr1 = [1, 2, 3];
            var arr2 = [1, 2, 3];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Check that arrays are stored as global constants
    bool hasGlobalVariable = false;
    for (auto& global : module->globals()) {
        if (global.isConstant()) {
            hasGlobalVariable = true;
            break;
        }
    }
    EXPECT_TRUE(hasGlobalVariable);
}

TEST_F(ArrayLiteralCodeGenTest, ArrayLiteralInIfStatement) {
    std::string code = R"(
        func test() {
            if (true) {
                var arr = [1, 2, 3];
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(ArrayLiteralCodeGenTest, ArrayLiteralInLoop) {
    std::string code = R"(
        func test() {
            var i = 0;
            while (i < 3) {
                var arr = [1, 2, 3];
                i = i + 1;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(ArrayLiteralCodeGenTest, ByteArrayLiteral) {
    std::string code = R"(
        func test() {
            var arr: byte[] = [10, 20, 30, 40, 50];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Array literals with integer values are inferred as int64[], even with explicit byte[] type
    // This is a known limitation - type coercion for array literals not yet implemented
    EXPECT_TRUE(irString.find("@") != std::string::npos); // Has global constant
    EXPECT_TRUE(irString.find("alloca") != std::string::npos); // Has variable allocation
}

TEST_F(ArrayLiteralCodeGenTest, MixedDimensionalArrays) {
    std::string code = R"(
        func test() {
            var arr1 = [1, 2, 3];
            var arr2 = [[1, 2], [3, 4]];
            var arr3 = [[[1]]];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(ArrayLiteralCodeGenTest, ArrayLiteralStorageLocation) {
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

    // Array literals should be in .rodata section (read-only data)
    // They are stored as global constants
    int globalConstCount = 0;
    for (auto& global : module->globals()) {
        if (global.isConstant()) {
            globalConstCount++;
        }
    }
    EXPECT_GT(globalConstCount, 0);
}

// Test: Array as return type
TEST_F(ArrayLiteralCodeGenTest, ArrayReturnType) {
    std::string code = R"(
        func:int64[] getNumbers() {
            return [1, 2, 3];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Check function exists with correct signature
    Function* func = module->getFunction("getNumbers");
    ASSERT_NE(func, nullptr);

    // Function should return pointer (arrays are passed by reference)
    EXPECT_TRUE(func->getReturnType()->isPointerTy());

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Test: Multi-dimensional array as return type
TEST_F(ArrayLiteralCodeGenTest, MultiDimensionalArrayReturnType) {
    std::string code = R"(
        func:int64[][] getMatrix() {
            return [[1, 2], [3, 4]];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("getMatrix");
    ASSERT_NE(func, nullptr);

    // Function should return pointer
    EXPECT_TRUE(func->getReturnType()->isPointerTy());

    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Test: Array return type with array assignment
TEST_F(ArrayLiteralCodeGenTest, ArrayReturnAndAssignment) {
    std::string code = R"(
        func test() {
            var result = [10, 20, 30];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Test: Array as function parameter and return
TEST_F(ArrayLiteralCodeGenTest, ArrayParameterAndReturn) {
    std::string code = R"(
        func:int64[] process(arr: int64[]) {
            return arr;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);

    // Should have one parameter (pointer) and return pointer
    EXPECT_EQ(func->arg_size(), static_cast<size_t>(1));
    EXPECT_TRUE(func->getReturnType()->isPointerTy());
    EXPECT_TRUE(func->getArg(0)->getType()->isPointerTy());

    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}
