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

class LLVMCodeGeneratorComprehensiveTest : public ::testing::Test {
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

// ============================================================================
// STATEMENT TESTS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_Block) {
    std::string code = R"(
        func test() {
            {
                var x = 1;
                var y = 2;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_ReturnWithExpression) {
    std::string code = "func:int64 test() { return 42; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_ReturnVoid) {
    std::string code = "func test() { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_ExpressionStatement) {
    std::string code = R"(
        func test() {
            42;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_VariableDeclaration) {
    std::string code = R"(
        func test() {
            var x = 10;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_VariableDeclarationWithType) {
    std::string code = R"(
        func test() {
            var x: int64 = 10;
            var y: double = 3.14;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_IfStatement) {
    std::string code = R"(
        func:int64 test() {
            if (true) {
                return 1;
            }
            return 0;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_IfElseStatement) {
    std::string code = R"(
        func:int64 test() {
            if (true) {
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
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_NestedIf) {
    std::string code = R"(
        func:int64 test() {
            if (true) {
                if (false) {
                    return 1;
                }
            }
            return 0;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_WhileLoop) {
    std::string code = R"(
        func test() {
            var i = 0;
            while (i < 10) {
                i = i + 1;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_ForRangeLoop) {
    std::string code = R"(
        func test() {
            var sum = 0;
            for i in 0 .. 10 {
                sum = sum + i;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_ForInLoop) {
    std::string code = R"(
        func test() {
            var arr = [1, 2, 3];
            var sum = 0;
            for item in arr {
                sum = sum + item;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_BreakInWhile) {
    std::string code = R"(
        func test() {
            var count = 0;
            while (count < 100) {
                count = count + 1;
                if (count == 5) {
                    break;
                }
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_ContinueInWhile) {
    std::string code = R"(
        func test() {
            var count = 0;
            var sum = 0;
            while (count < 10) {
                count = count + 1;
                if (count % 2 == 0) {
                    continue;
                }
                sum = sum + count;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_BreakInForRange) {
    std::string code = R"(
        func test() {
            var sum = 0;
            for i in 0 .. 100 {
                sum = sum + i;
                if (sum > 50) {
                    break;
                }
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_NestedLoopsWithBreak) {
    std::string code = R"(
        func test() {
            var found = false;
            for i in 0 .. 10 {
                for j in 0 .. 10 {
                    if (i * j == 25) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_ScopeBlock) {
    std::string code = R"(
        func test() {
            var x = 10;
            scope {
                var y = 20;
                x = x + y;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Statement_NestedScopeBlocks) {
    std::string code = R"(
        func test() {
            scope {
                var a = 1;
                scope {
                    var b = 2;
                    scope {
                        var c = 3;
                    }
                }
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - LITERALS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_IntegerLiteral) {
    std::string code = "func:int64 test() { return 42; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_FloatingLiteral) {
    std::string code = "func:double test() { return 3.14159; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isDoubleTy());
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_FloatLiteral) {
    std::string code = "func:float test(val: float) { return val; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isFloatTy());
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BooleanLiteral_True) {
    std::string code = "func:bool test() { return true; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BooleanLiteral_False) {
    std::string code = "func:bool test() { return false; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_CharLiteral) {
    std::string code = "func:char test() { return 'a'; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_StringLiteral) {
    std::string code = R"(
        func test() {
            var s = "hello world";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_NullLiteral) {
    std::string code = R"(
        func test() {
            var ptr: int64? = null;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_InterpolatedString) {
    std::string code = R"(
        func test() {
            var name = "world";
            var s = "Hello ${name}!";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - IDENTIFIERS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_Identifier) {
    std::string code = R"(
        func:int64 test() {
            var x = 10;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - BINARY OPERATORS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_Addition) {
    std::string code = "func:int64 test() { return 5 + 3; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_Subtraction) {
    std::string code = "func:int64 test() { return 5 - 3; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_Multiplication) {
    std::string code = "func:int64 test() { return 5 * 3; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_Division) {
    std::string code = "func:int64 test() { return 15 / 3; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_Modulo) {
    std::string code = "func:int64 test() { return 17 % 5; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_Equals) {
    std::string code = "func:bool test() { return 5 == 5; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_NotEquals) {
    std::string code = "func:bool test() { return 5 != 3; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_LessThan) {
    std::string code = "func:bool test() { return 3 < 5; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_LessEquals) {
    std::string code = "func:bool test() { return 3 <= 5; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_GreaterThan) {
    std::string code = "func:bool test() { return 5 > 3; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_GreaterEquals) {
    std::string code = "func:bool test() { return 5 >= 3; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_StringConcatenation) {
    std::string code = R"(
        func test() {
            var a = "Hello";
            var b = "World";
            var c = a + b;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_BinaryOp_CompoundExpression) {
    std::string code = "func:int64 test() { return (1 + 2) * (3 - 4); }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - UNARY OPERATORS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_UnaryMinus) {
    std::string code = "func:int64 test() { return -42; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_UnaryMinusVariable) {
    std::string code = R"(
        func:int64 test() {
            var x = 10;
            return -x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_LogicalNot) {
    std::string code = "func:bool test() { return !true; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_LogicalAnd) {
    std::string code = "func:bool test() { return true && false; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_LogicalOr) {
    std::string code = "func:bool test() { return true || false; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_LogicalCompound) {
    std::string code = R"(
        func:bool test() {
            return (true && false) || (true || false);
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - ASSIGNMENT
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_Assignment) {
    std::string code = R"(
        func:int64 test() {
            var x = 10;
            x = 20;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_AssignmentCompound) {
    std::string code = R"(
        func:int64 test() {
            var x = 10;
            x = x + 5;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - FUNCTION CALLS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_FunctionCall) {
    std::string code = R"(
        func helper() { }
        func test() {
            helper();
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
    Function* helperFunc = module->getFunction("helper");
    ASSERT_NE(helperFunc, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_FunctionCallWithArgs) {
    std::string code = R"(
        func:int64 add(a: int64, b: int64) { return a + b; }
        func:int64 test() {
            return add(1, 2);
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_FunctionCallNoArgs) {
    std::string code = R"(
        func:int64 getValue() { return 42; }
        func:int64 test() {
            return getValue();
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - ARRAY LITERALS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_ArrayLiteral_Int64) {
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
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_ArrayLiteral_Double) {
    std::string code = R"(
        func test() {
            var arr = [1.1, 2.2, 3.3];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_ArrayLiteral_Empty) {
    std::string code = R"(
        func test() {
            var arr = [];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - ARRAY ACCESS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_ArrayAccess) {
    std::string code = R"(
        func:int64 test() {
            var arr = [10, 20, 30];
            return arr[1];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_ArrayAccessWithVariable) {
    std::string code = R"(
        func:int64 test() {
            var arr = [10, 20, 30];
            var i = 0;
            return arr[i];
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - NEW EXPRESSION
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_NewObject) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
        }
        func test() {
            var p = new Point();
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_NewObjectWithArgs) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) { }
        }
        func test() {
            var p = new Point(1, 2);
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - MEMBER ACCESS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_MemberAccess) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
        }
        func:int64 test() {
            var p: Point = new Point();
            return p.x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// EXPRESSION TESTS - PARENTHESIZED EXPRESSION
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Expression_Parenthesized) {
    std::string code = "func:int64 test() { return (1 + 2) * 3; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// TYPE TESTS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Primitive_Int64) {
    std::string code = "func:int64 test() { return 42; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Primitive_Double) {
    std::string code = "func:double test() { return 3.14; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isDoubleTy());
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Primitive_Bool) {
    std::string code = "func:bool test() { return true; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(1));
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Array) {
    std::string code = R"(
        func test() {
            var arr: int64[];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Optional) {
    std::string code = R"(
        func test() {
            var val: int64? = null;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// DECLARATION TESTS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Declaration_FunctionWithParameters) {
    std::string code = "func:int64 add(a: int64, b: int64) { return a + b; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("add");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->arg_size(), 2);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Declaration_Class) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor() {}
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Declaration_ClassWithConstructor) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) { }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Declaration_ClassWithExtends) {
    std::string code = R"(
        class Base {
            var value: int64;
        }
        class Derived extends Base {
            var extra: int64;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Declaration_Interface) {
    std::string code = R"(
        interface Printable {
            func print();
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Declaration_MultipleFunctions) {
    std::string code = R"(
        func:int64 first() { return 1; }
        func:int64 second() { return 2; }
        func:int64 third() { return 3; }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_NE(module->getFunction("first"), nullptr);
    EXPECT_NE(module->getFunction("second"), nullptr);
    EXPECT_NE(module->getFunction("third"), nullptr);
}

// ============================================================================
// ALL PRIMITIVE TYPE TESTS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Byte) {
    std::string code = R"(
        func:byte test() {
            var b: byte = 255;
            return b;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Uint8) {
    std::string code = R"(
        func:uint8 test() {
            var b: uint8 = 200;
            return b;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_F64) {
    std::string code = R"(
        func:f64 test() {
            var val: f64 = 3.14159;
            return val;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Char) {
    std::string code = R"(
        func:char test() {
            var c: char = 'x';
            return c;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_String) {
    std::string code = R"(
        func:string test() {
            var s: string = "hello";
            return s;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Type_Void) {
    std::string code = "func test() { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
}

// ============================================================================
// CLASS MODIFIER TESTS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, ClassModifier_Singleton) {
    std::string code = R"(
        singleton class Singleton {
            func:int64 getInstance() { return 42; }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, ClassModifier_Immutable) {
    std::string code = R"(
        immutable class ImmutableData {
            var value: int64;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, ClassModifier_Factory) {
    std::string code = R"(
        factory class FactoryClass {
            func:int64 create() { return 0; }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, ClassModifier_Observable) {
    std::string code = R"(
        observable class ObservableClass {
            event changed;
            func notify() { }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, ClassModifier_Service) {
    std::string code = R"(
        service class ServiceClass {
            func execute() { }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, ClassModifier_Strategy) {
    std::string code = R"(
        strategy class StrategyClass {
            func execute() { }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, ClassModifier_Actor) {
    std::string code = R"(
        actor class ActorClass {
            func act() { }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, ClassModifier_Final) {
    std::string code = R"(
        final class FinalClass {
            func method() { }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
}

// ============================================================================
// COMPLEX/EDGE CASE TESTS
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, Complex_DeepNesting) {
    std::string code = R"(
        func:int64 test() {
            if (true) {
                if (true) {
                    if (true) {
                        if (true) {
                            return 42;
                        }
                    }
                }
            }
            return 0;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Complex_AllOperatorsTogether) {
    std::string code = R"(
        func:int64 test() {
            var a = 1 + 2 - 3 * 4 / 5 % 6;
            var b = 1 < 2 && 3 > 4 || 5 == 6;
            return a;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Complex_RecursiveLikePattern) {
    std::string code = R"(
        func test() {
            var sum = 0;
            var i = 0;
            while (i < 10) {
                if (i == 5) {
                    i = i + 1;
                    continue;
                }
                if (i == 8) {
                    break;
                }
                sum = sum + i;
                i = i + 1;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Complex_TernaryLikeLogic) {
    std::string code = R"(
        func:int64 max(a: int64, b: int64) {
            if (a > b) {
                return a;
            } else {
                return b;
            }
        }
        func:int64 test() {
            return max(10, 20);
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Complex_FactorialLike) {
    std::string code = R"(
        func test() {
            var result = 1;
            var n = 5;
            var i = 1;
            while (i <= n) {
                result = result * i;
                i = i + 1;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, Complex_SearchPattern) {
    std::string code = R"(
        func test() {
            var found = false;
            var target = 42;
            for i in 0 .. 100 {
                if (i == target) {
                    found = true;
                    break;
                }
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// MODULE VERIFICATION
// ============================================================================

TEST_F(LLVMCodeGeneratorComprehensiveTest, ModuleVerification_ValidModule) {
    std::string code = R"(
        func:int64 test() {
            var x = 10;
            if (x > 5) {
                return x;
            }
            return 0;
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

TEST_F(LLVMCodeGeneratorComprehensiveTest, ModuleVerification_MultipleFunctions) {
    std::string code = R"(
        func:int64 first() { return 1; }
        func:int64 second() { return 2; }
        func:int64 third() { return 3; }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(LLVMCodeGeneratorComprehensiveTest, ModuleVerification_ClassWithMethods) {
    std::string code = R"(
        class Calculator {
            var result: int64;
            constructor() { }
            func:int64 add(a: int64, b: int64) {
                return a + b;
            }
            func:int64 subtract(a: int64, b: int64) {
                return a - b;
            }
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
