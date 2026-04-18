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
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class ArrayGenericIntegrationTest : public ::testing::Test {
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

    bool isModuleValid(llvm::Module* module) {
        std::string errorMsg;
        raw_string_ostream errorStream(errorMsg);
        return !verifyModule(*module, &errorStream);
    }
};

// Phase 5 Test 1: Array literal initialization compiles
TEST_F(ArrayGenericIntegrationTest, ArrayLiteralInitialization) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1, 2, 3, 4, 5];
            return 5;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 2: Array literal with type inference for int64
TEST_F(ArrayGenericIntegrationTest, ArrayTypeInferenceInt64) {
    std::string code = R"(
        func:int64 main() {
            var arr = [10, 20, 30];
            return 3;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 3: Array literal with type inference for double
TEST_F(ArrayGenericIntegrationTest, ArrayTypeInferenceDouble) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1.5, 2.5, 3.5];
            return 1;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_double_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 4: Array access - read element
TEST_F(ArrayGenericIntegrationTest, ArrayAccess) {
    std::string code = R"(
        func:int64 main() {
            var arr = [10, 20, 30, 40, 50];
            var value = arr[2];
            return value;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 5: Array access at different indices
TEST_F(ArrayGenericIntegrationTest, ArrayAccessMultipleIndices) {
    std::string code = R"(
        func:int64 main() {
            var arr = [100, 200, 300, 400, 500];
            var first = arr[0];
            var last = arr[4];
            var middle = arr[2];
            return first + last + middle;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 6: Array element access in expressions
TEST_F(ArrayGenericIntegrationTest, ArrayAccessInExpressions) {
    std::string code = R"(
        func:int64 main() {
            var arr = [10, 20, 30];
            var result = arr[0] + arr[1] + arr[2];
            return result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 7: Multiple arrays with different types
TEST_F(ArrayGenericIntegrationTest, MultipleArrayTypes) {
    std::string code = R"(
        func:int64 main() {
            var ints = [1, 2, 3];
            var floats = [1.5, 2.5, 3.5];
            return 42;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
    EXPECT_TRUE(irString.find("hoo_double_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 8: Array with explicit type annotation
TEST_F(ArrayGenericIntegrationTest, ExplicitArrayTypeAnnotation) {
    std::string code = R"(
        func:int64 main() {
            var arr: int64[] = [5, 10, 15, 20];
            return arr[1];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 9: Array in function call
TEST_F(ArrayGenericIntegrationTest, ArrayAsParameter) {
    std::string code = R"(
        func:int64 process(data: int64[]) {
            return 123;
        }

        func:int64 main() {
            var arr = [1, 2, 3];
            return process(arr);
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 10: Array access in loop
TEST_F(ArrayGenericIntegrationTest, ArrayAccessInLoop) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1, 2, 3, 4, 5];
            var sum = 0;
            var i = 0;
            while (i < 5) {
                sum = sum + arr[i];
                i = i + 1;
            }
            return sum;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 11: Array with constant expressions
TEST_F(ArrayGenericIntegrationTest, ArrayWithConstantExpressions) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1 + 1, 2 + 2, 3 + 3];
            return arr[0] + arr[1] + arr[2];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 12: Empty array with explicit type
TEST_F(ArrayGenericIntegrationTest, EmptyArrayExecution) {
    std::string code = R"(
        func:int64 main() {
            var arr: int64[] = [];
            return 42;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 13: Array initialization from literal in variable
TEST_F(ArrayGenericIntegrationTest, ArrayVariableStorage) {
    std::string code = R"(
        func:int64 main() {
            var x = [42];
            return x[0];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 14: Multiple array assignments
TEST_F(ArrayGenericIntegrationTest, MultipleArrayVariables) {
    std::string code = R"(
        func:int64 main() {
            var arr1 = [10];
            var arr2 = [20];
            var arr3 = [30];
            return arr1[0] + arr2[0] + arr3[0];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 15: Array access in nested expressions
TEST_F(ArrayGenericIntegrationTest, ArrayAccessNested) {
    std::string code = R"(
        func:int64 main() {
            var arr = [2, 4, 6, 8, 10];
            return (arr[0] + arr[1]) * (arr[2] - arr[3]) + arr[4];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 16: Double array access and computation
TEST_F(ArrayGenericIntegrationTest, DoubleArrayAccess) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1.0, 2.0, 3.0];
            return 42;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_double_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 17: Array in conditional
TEST_F(ArrayGenericIntegrationTest, ArrayInConditional) {
    std::string code = R"(
        func:int64 main() {
            var arr = [5, 10, 15];
            if (arr[0] < arr[1]) {
                return 1;
            } else {
                return 0;
            }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 18: Array comparison in loop condition
TEST_F(ArrayGenericIntegrationTest, ArrayComparisonInLoop) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1, 2, 3, 4, 5];
            var count = 0;
            var i = 0;
            while (i < 5 && arr[i] < 4) {
                count = count + 1;
                i = i + 1;
            }
            return count;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 19: Array with for-in loop
TEST_F(ArrayGenericIntegrationTest, ArrayWithForInLoop) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1, 2, 3, 4, 5];
            var sum = 0;
            for item in arr {
                sum = sum + item;
            }
            return sum;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 20: Array with for-range loop
TEST_F(ArrayGenericIntegrationTest, ArrayWithForRangeLoop) {
    std::string code = R"(
        func:int64 main() {
            var arr = [10, 20, 30, 40, 50];
            var sum = 0;
            for i in 0..5 {
                sum = sum + arr[i];
            }
            return sum;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 21: Array return value from function
TEST_F(ArrayGenericIntegrationTest, ArrayReturnedFromFunction) {
    std::string code = R"(
        func:int64[] getArray() {
            return [1, 2, 3];
        }

        func:int64 main() {
            var arr = getArray();
            return arr[1];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 22: Nested function calls with arrays
TEST_F(ArrayGenericIntegrationTest, NestedFunctionCalls) {
    std::string code = R"(
        func:int64 sum_array(data: int64[]) {
            return 21;
        }

        func:int64 process() {
            return sum_array([1, 2, 3]);
        }

        func:int64 main() {
            return process();
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 23: Array access boundary check
TEST_F(ArrayGenericIntegrationTest, ArrayAccessBoundary) {
    std::string code = R"(
        func:int64 main() {
            var arr = [100, 200, 300];
            return arr[0] + arr[2];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 24: Multiple arrays in same function
TEST_F(ArrayGenericIntegrationTest, MultipleArraysInFunction) {
    std::string code = R"(
        func:int64 calculate(a: int64[], b: int64[]) {
            return a[0] + b[0];
        }

        func:int64 main() {
            var x = [5, 10, 15];
            var y = [3, 6, 9];
            return calculate(x, y);
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 25: Array element as function argument
TEST_F(ArrayGenericIntegrationTest, ArrayElementAsArgument) {
    std::string code = R"(
        func:int64 double_value(x: int64) {
            return x * 2;
        }

        func:int64 main() {
            var arr = [5, 10, 15];
            return double_value(arr[1]);
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 26: Array in complex arithmetic
TEST_F(ArrayGenericIntegrationTest, ArrayInComplexArithmetic) {
    std::string code = R"(
        func:int64 main() {
            var arr = [2, 3, 4];
            return arr[0] * arr[1] + arr[2];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 27: Array with boolean operations
TEST_F(ArrayGenericIntegrationTest, ArrayWithBooleanOps) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1, 2, 3, 4, 5];
            if (arr[0] > 0 && arr[4] < 10) {
                return 1;
            }
            return 0;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 28: Sequential array operations
TEST_F(ArrayGenericIntegrationTest, SequentialArrayOperations) {
    std::string code = R"(
        func:int64 main() {
            var arr1 = [1, 2];
            var val1 = arr1[0];
            var arr2 = [3, 4];
            var val2 = arr2[1];
            return val1 + val2;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 29: Array with variable indices
TEST_F(ArrayGenericIntegrationTest, ArrayWithVariableIndex) {
    std::string code = R"(
        func:int64 main() {
            var arr = [10, 20, 30, 40, 50];
            var idx = 2;
            return arr[idx];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 30: Array index computed from expression
TEST_F(ArrayGenericIntegrationTest, ArrayIndexComputed) {
    std::string code = R"(
        func:int64 main() {
            var arr = [10, 20, 30, 40, 50];
            var idx = 1 + 1;
            return arr[idx];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}

// Phase 5 Test 31: Large array
TEST_F(ArrayGenericIntegrationTest, LargeArray) {
    std::string code = R"(
        func:int64 main() {
            var arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                      11, 12, 13, 14, 15, 16, 17, 18, 19, 20];
            return arr[10];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("hoo_int64_array_from_buffer") != std::string::npos);
}

// Phase 5 Test 32: Array initialization and iteration
TEST_F(ArrayGenericIntegrationTest, ArrayInitAndIterate) {
    std::string code = R"(
        func:int64 main() {
            var arr = [100, 200, 300, 400, 500];
            var total = 0;
            var i = 0;
            while (i < 5) {
                total = total + arr[i];
                i = i + 1;
            }
            return total;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(isModuleValid(module.get()));
}
