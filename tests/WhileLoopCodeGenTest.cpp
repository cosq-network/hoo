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

class WhileLoopCodeGenTest : public ::testing::Test {
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

// Basic while loop code generation
TEST_F(WhileLoopCodeGenTest, SimpleWhileLoop) {
    std::string code = R"(
        func test() -> void {
            while (true) {
                return;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isVoidTy());

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(WhileLoopCodeGenTest, WhileLoopWithVariableCondition) {
    std::string code = R"(
        func test() -> void {
            var x = 0;
            while (x < 10) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isVoidTy());

    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(WhileLoopCodeGenTest, WhileLoopWithComplexCondition) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            var y = 10;
            while (x < y && y > 0) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isVoidTy());

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(WhileLoopCodeGenTest, WhileLoopWithLogicalOr) {
    std::string code = R"(
        func test() -> void {
            var a = true;
            var b = false;
            while (a || b) {
                a = false;
            }
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

TEST_F(WhileLoopCodeGenTest, WhileLoopWithNegation) {
    std::string code = R"(
        func test() -> void {
            var flag = true;
            while (!flag) {
                flag = true;
            }
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

// Multiple statements in while loop
TEST_F(WhileLoopCodeGenTest, WhileLoopWithMultipleStatements) {
    std::string code = R"(
        func test() -> void {
            var x = 0;
            while (x < 5) {
                var y = x * 2;
                var z = y + 1;
                x = x + 1;
            }
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

TEST_F(WhileLoopCodeGenTest, WhileLoopWithMultipleAssignments) {
    std::string code = R"(
        func test() -> void {
            var x = 0;
            var y = 0;
            while (x < 10) {
                x = x + 1;
                y = y + 2;
                x = x + y;
            }
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

// Nested while loops
TEST_F(WhileLoopCodeGenTest, NestedWhileLoops) {
    std::string code = R"(
        func test() -> void {
            var i = 0;
            while (i < 5) {
                var j = 0;
                while (j < 3) {
                    j = j + 1;
                }
                i = i + 1;
            }
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

TEST_F(WhileLoopCodeGenTest, DeeplyNestedWhileLoops) {
    std::string code = R"(
        func test() -> void {
            var a = 0;
            while (a < 2) {
                var b = 0;
                while (b < 2) {
                    var c = 0;
                    while (c < 2) {
                        c = c + 1;
                    }
                    b = b + 1;
                }
                a = a + 1;
            }
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

// While loop with if statement
TEST_F(WhileLoopCodeGenTest, WhileLoopWithIfInside) {
    std::string code = R"(
        func test() -> void {
            var x = 0;
            while (x < 10) {
                if (x == 5) {
                    x = x + 2;
                } else {
                    x = x + 1;
                }
            }
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

TEST_F(WhileLoopCodeGenTest, IfWithWhileLoopInside) {
    std::string code = R"(
        func test() -> void {
            var flag = true;
            if (flag) {
                var x = 0;
                while (x < 5) {
                    x = x + 1;
                }
            }
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

// Type-specific tests
TEST_F(WhileLoopCodeGenTest, WhileLoopWithIntComparison) {
    std::string code = R"(
        func test() -> void {
            var count = 0;
            while (count < 100) {
                count = count + 10;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("icmp") != std::string::npos);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(WhileLoopCodeGenTest, WhileLoopWithFloatComparison) {
    std::string code = R"(
        func test() -> void {
            var value = 0.5;
            while (value < 10.0) {
                value = value + 1.5;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("fcmp") != std::string::npos);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(WhileLoopCodeGenTest, WhileLoopWithBoolVariable) {
    std::string code = R"(
        func test() -> void {
            var active = true;
            while (active) {
                active = false;
            }
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

TEST_F(WhileLoopCodeGenTest, WhileLoopWithCharComparison) {
    std::string code = R"(
        func test() -> void {
            var ch = 'a';
            while (ch != 'z') {
                ch = 'b';
            }
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

// Advanced conditions
TEST_F(WhileLoopCodeGenTest, WhileLoopWithArithmeticCondition) {
    std::string code = R"(
        func test() -> void {
            var x = 10;
            var y = 5;
            while (x - y > 0) {
                x = x - 1;
            }
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

TEST_F(WhileLoopCodeGenTest, WhileLoopWithComparisonChain) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            while (x >= 0 && x <= 10) {
                x = x - 1;
            }
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

TEST_F(WhileLoopCodeGenTest, WhileLoopWithComplexLogic) {
    std::string code = R"(
        func test() -> void {
            var a = 5;
            var b = 10;
            var c = true;
            while ((a < b) && c || (a == b)) {
                a = a + 1;
            }
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

// Control flow patterns
TEST_F(WhileLoopCodeGenTest, WhileLoopWithReturn) {
    std::string code = R"(
        func test() -> int64 {
            var x = 0;
            while (x < 10) {
                if (x == 5) {
                    return 42;
                }
                x = x + 1;
            }
            return -1;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(WhileLoopCodeGenTest, MultipleWhileLoopsInFunction) {
    std::string code = R"(
        func test() -> void {
            var x = 0;
            while (x < 5) {
                x = x + 1;
            }

            var y = 0;
            while (y < 10) {
                y = y + 1;
            }
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

TEST_F(WhileLoopCodeGenTest, WhileLoopFollowedByOtherStatements) {
    std::string code = R"(
        func test() -> int64 {
            var x = 0;
            while (x < 5) {
                x = x + 1;
            }
            var result = x + 10;
            return result;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Array operations in while loop
TEST_F(WhileLoopCodeGenTest, WhileLoopWithArrayAccess) {
    std::string code = R"(
        func test() -> void {
            var arr = [1, 2, 3, 4, 5];
            var index = 0;
            while (index < 5) {
                var value = arr[index];
                index = index + 1;
            }
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

// Function calls in while loop
TEST_F(WhileLoopCodeGenTest, WhileLoopWithFunctionCall) {
    std::string code = R"(
        func helper(x: int64) -> int64 {
            return x * 2;
        }

        func test() -> void {
            var x = 0;
            while (x < 10) {
                var y = helper(x);
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
    EXPECT_TRUE(testFunc->getReturnType()->isVoidTy());

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Empty and minimal loops
TEST_F(WhileLoopCodeGenTest, EmptyWhileLoop) {
    std::string code = R"(
        func test() -> void {
            while (false) {
            }
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

TEST_F(WhileLoopCodeGenTest, WhileLoopWithSingleStatement) {
    std::string code = R"(
        func test() -> void {
            var x = 0;
            while (x < 100) {
                x = x + 1;
            }
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

// Parameter-based conditions
TEST_F(WhileLoopCodeGenTest, WhileLoopWithParameterCondition) {
    std::string code = R"(
        func test(limit: int64) -> void {
            var x = 0;
            while (x < limit) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->arg_size(), 1);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// IR structure verification
TEST_F(WhileLoopCodeGenTest, VerifyWhileLoopIRStructure) {
    std::string code = R"(
        func test() -> void {
            var x = 0;
            while (x < 10) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Check for expected IR elements
    EXPECT_TRUE(irString.find("while.cond") != std::string::npos);
    EXPECT_TRUE(irString.find("while.body") != std::string::npos);
    EXPECT_TRUE(irString.find("while.end") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp") != std::string::npos);
    EXPECT_TRUE(irString.find("br") != std::string::npos);
}

// Complex type mixing
TEST_F(WhileLoopCodeGenTest, WhileLoopWithMixedTypes) {
    std::string code = R"(
        func test() -> void {
            var intVal = 0;
            var floatVal = 0.0;
            var boolVal = true;

            while (intVal < 10 && boolVal) {
                intVal = intVal + 1;
                floatVal = floatVal + 1.5;
            }
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

// Variable scope testing
TEST_F(WhileLoopCodeGenTest, WhileLoopVariableScope) {
    std::string code = R"(
        func test() -> int64 {
            var outer = 100;
            var x = 0;
            while (x < 5) {
                var inner = x + 1;
                outer = outer - inner;
                x = x + 1;
            }
            return outer;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Multiple loop variables
TEST_F(WhileLoopCodeGenTest, WhileLoopWithMultipleVariables) {
    std::string code = R"(
        func test() -> void {
            var x = 0;
            var y = 10;
            var product = 1;

            while (x < y) {
                product = product * x;
                x = x + 1;
            }
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

// Equality and inequality tests
TEST_F(WhileLoopCodeGenTest, WhileLoopWithEqualityCheck) {
    std::string code = R"(
        func test() -> void {
            var x = 10;
            while (x != 0) {
                x = x - 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("icmp ne") != std::string::npos);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(WhileLoopCodeGenTest, WhileLoopWithEqualsComparison) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            var target = 15;

            while (x == target) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("icmp eq") != std::string::npos);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}
