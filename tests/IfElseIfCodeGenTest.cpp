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

class IfElseIfCodeGenTest : public ::testing::Test {
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

// Basic if statement code generation
TEST_F(IfElseIfCodeGenTest, SimpleIfStatement) {
    std::string code = R"(
        func test() -> void {
            if (true) {
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

// Basic if-else statement code generation
TEST_F(IfElseIfCodeGenTest, SimpleIfElseStatement) {
    std::string code = R"(
        func test() -> void {
            if (true) {
                return;
            } else {
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

    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if chain (nested if in else)
TEST_F(IfElseIfCodeGenTest, IfElseIfChain) {
    std::string code = R"(
        func test(x: int64) -> void {
            if (x == 1) {
                return;
            } else {
                if (x == 2) {
                    return;
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
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with else clause
TEST_F(IfElseIfCodeGenTest, IfElseIfElseChain) {
    std::string code = R"(
        func test(x: int64) -> void {
            if (x == 1) {
                return;
            } else {
                if (x == 2) {
                    return;
                } else {
                    return;
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
    EXPECT_TRUE(func->getReturnType()->isVoidTy());

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Multiple else-if clauses
TEST_F(IfElseIfCodeGenTest, MultipleElseIfClauses) {
    std::string code = R"(
        func test(x: int64) -> int64 {
            if (x < 0) {
                return -1;
            } else {
                if (x == 0) {
                    return 0;
                } else {
                    if (x > 0) {
                        return 1;
                    } else {
                        return 2;
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 1);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with variable declarations and assignments
TEST_F(IfElseIfCodeGenTest, IfElseIfWithVariables) {
    std::string code = R"(
        func test(x: int64) -> int64 {
            var result = 0;
            if (x < 0) {
                result = -1;
            } else {
                if (x == 0) {
                    result = 0;
                } else {
                    result = 1;
                }
            }
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
    EXPECT_EQ(func->arg_size(), 1);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with complex conditions (logical AND)
TEST_F(IfElseIfCodeGenTest, IfElseIfComplexConditionAnd) {
    std::string code = R"(
        func test(x: int64, y: int64) -> int64 {
            if (x > 0 && y > 0) {
                return 1;
            } else {
                if (x < 0 && y < 0) {
                    return -1;
                } else {
                    return 0;
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 2);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with complex conditions (logical OR)
TEST_F(IfElseIfCodeGenTest, IfElseIfComplexConditionOr) {
    std::string code = R"(
        func test(x: int64, y: int64) -> int64 {
            if (x == 10 || y == 10) {
                return 10;
            } else {
                if (x == 20 || y == 20) {
                    return 20;
                } else {
                    return 0;
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 2);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with negated conditions
TEST_F(IfElseIfCodeGenTest, IfElseIfNegatedCondition) {
    std::string code = R"(
        func test(flag: bool) -> int64 {
            if (!flag) {
                return 0;
            } else {
                if (flag) {
                    return 1;
                } else {
                    return 2;
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 1);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with float comparisons
TEST_F(IfElseIfCodeGenTest, IfElseIfFloatComparison) {
    std::string code = R"(
        func test(temp: double) -> int64 {
            if (temp < 0.0) {
                return -1;
            } else {
                if (temp > 100.0) {
                    return 1;
                } else {
                    return 0;
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isDoubleTy());

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("fcmp") != std::string::npos);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with char comparisons
TEST_F(IfElseIfCodeGenTest, IfElseIfCharComparison) {
    std::string code = R"(
        func test(ch: char) -> int64 {
            if (ch == 'a') {
                return 1;
            } else {
                if (ch == 'b') {
                    return 2;
                } else {
                    if (ch == 'c') {
                        return 3;
                    } else {
                        return 0;
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(32));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with nested if statements
TEST_F(IfElseIfCodeGenTest, IfElseIfWithNestedIf) {
    std::string code = R"(
        func test(x: int64, y: int64) -> int64 {
            if (x < 0) {
                return -1;
            } else {
                if (x > 0) {
                    if (y > 0) {
                        return 1;
                    } else {
                        return 2;
                    }
                } else {
                    return 0;
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 2);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with arithmetic in conditions
TEST_F(IfElseIfCodeGenTest, IfElseIfArithmeticCondition) {
    std::string code = R"(
        func test(x: int64, y: int64) -> int64 {
            if (x + y > 20) {
                return 1;
            } else {
                if (x - y > 3) {
                    return 2;
                } else {
                    if (x * y > 40) {
                        return 3;
                    } else {
                        return 4;
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 2);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with multiple statements in blocks
TEST_F(IfElseIfCodeGenTest, IfElseIfMultipleStatements) {
    std::string code = R"(
        func test(x: int64) -> int64 {
            var a = 10;
            var b = 20;
            if (x < 5) {
                a = 1;
                b = 2;
                return a + b;
            } else {
                if (x < 15) {
                    a = 10;
                    b = 20;
                    return a + b;
                } else {
                    a = 100;
                    b = 200;
                    return a + b;
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 1);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// Deep else-if chain
TEST_F(IfElseIfCodeGenTest, DeepElseIfChain) {
    std::string code = R"(
        func test(x: int64) -> int64 {
            if (x == 1) {
                return 1;
            } else {
                if (x == 2) {
                    return 2;
                } else {
                    if (x == 3) {
                        return 3;
                    } else {
                        if (x == 4) {
                            return 4;
                        } else {
                            if (x == 5) {
                                return 5;
                            } else {
                                return 0;
                            }
                        }
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with function calls
TEST_F(IfElseIfCodeGenTest, IfElseIfWithFunctionCalls) {
    std::string code = R"(
        func helper(x: int64) -> int64 {
            return x * 2;
        }

        func test(x: int64) -> int64 {
            if (x < 10) {
                return helper(x);
            } else {
                if (x < 20) {
                    return helper(x + 5);
                } else {
                    return helper(x + 10);
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
    EXPECT_TRUE(testFunc->getReturnType()->isIntegerTy(64));

    Function* helperFunc = module->getFunction("helper");
    ASSERT_NE(helperFunc, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with array access
TEST_F(IfElseIfCodeGenTest, IfElseIfWithArrayAccess) {
    std::string code = R"(
        func test(index: int64) -> int64 {
            var arr = [10, 20, 30, 40, 50];
            if (index < 2) {
                return arr[0];
            } else {
                if (index < 4) {
                    return arr[2];
                } else {
                    return arr[4];
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with all primitive types
TEST_F(IfElseIfCodeGenTest, IfElseIfMixedTypes) {
    std::string code = R"(
        func test(intVal: int64, floatVal: double, boolVal: bool) -> int64 {
            if (intVal > 0) {
                return 1;
            } else {
                if (floatVal > 2.0) {
                    return 2;
                } else {
                    if (boolVal) {
                        return 3;
                    } else {
                        return 0;
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 3);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if followed by other statements
TEST_F(IfElseIfCodeGenTest, IfElseIfFollowedByStatements) {
    std::string code = R"(
        func test(x: int64) -> int64 {
            var result = 0;
            if (x < 3) {
                result = 1;
            } else {
                if (x < 7) {
                    result = 2;
                } else {
                    result = 3;
                }
            }
            var finalValue = result + 10;
            return finalValue;
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

// Multiple if-else-if chains in same function
TEST_F(IfElseIfCodeGenTest, MultipleIfElseIfChains) {
    std::string code = R"(
        func test(x: int64, y: int64) -> int64 {
            var result = 0;
            if (x < 3) {
                result = 1;
            } else {
                if (x < 7) {
                    result = 2;
                } else {
                    result = 3;
                }
            }

            if (y < 5) {
                result = result + 10;
            } else {
                if (y < 15) {
                    result = result + 20;
                } else {
                    result = result + 30;
                }
            }
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

// If-else-if with int64 type (typical use case)
TEST_F(IfElseIfCodeGenTest, IfElseIfInt64Type) {
    std::string code = R"(
        func test(value: int64) -> int64 {
            if (value < 10) {
                return 1;
            } else {
                if (value < 50) {
                    return 2;
                } else {
                    if (value < 100) {
                        return 3;
                    } else {
                        return 4;
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with complex nested structures
TEST_F(IfElseIfCodeGenTest, IfElseIfComplexNesting) {
    std::string code = R"(
        func test(x: int64, y: int64) -> int64 {
            if (x > 0) {
                if (y > 0) {
                    return 1;
                } else {
                    return 2;
                }
            } else {
                if (x == 0) {
                    if (y > 0) {
                        return 3;
                    } else {
                        return 4;
                    }
                } else {
                    if (y > 0) {
                        return 5;
                    } else {
                        return 6;
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with boolean variables as conditions
TEST_F(IfElseIfCodeGenTest, IfElseIfBooleanVariable) {
    std::string code = R"(
        func test(flag1: bool, flag2: bool) -> int64 {
            if (flag1 && flag2) {
                return 1;
            } else {
                if (flag1) {
                    return 2;
                } else {
                    if (flag2) {
                        return 3;
                    } else {
                        return 0;
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if with comparison chains
TEST_F(IfElseIfCodeGenTest, IfElseIfComparisonChain) {
    std::string code = R"(
        func test(x: int64) -> int64 {
            if (x < 10) {
                return 1;
            } else {
                if (x >= 10 && x < 20) {
                    return 2;
                } else {
                    if (x >= 20 && x < 30) {
                        return 3;
                    } else {
                        return 4;
                    }
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
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

// If-else-if IR verification
TEST_F(IfElseIfCodeGenTest, VerifyIfElseIfIRStructure) {
    std::string code = R"(
        func test(x: int64) -> int64 {
            if (x < 0) {
                return -1;
            } else {
                if (x == 0) {
                    return 0;
                } else {
                    return 1;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Check for expected IR elements
    EXPECT_TRUE(irString.find("define i64 @test(i64") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp") != std::string::npos);  // Comparison instruction
    EXPECT_TRUE(irString.find("br") != std::string::npos);    // Branch instruction
    EXPECT_TRUE(irString.find("ret i64") != std::string::npos);
}
