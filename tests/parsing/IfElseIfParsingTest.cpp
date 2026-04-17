#include <gtest/gtest.h>
#include <memory>
#include "src/parsing/ProcessIsolatedParser.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/ast/AST.h"

using namespace hooc;
using namespace hooc::ast;

class IfElseIfParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    std::unique_ptr<CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;
        return astBuilder->buildAST(parseTree);
    }
};

TEST_F(IfElseIfParsingTest, SimpleIfStatement) {
    std::string code = R"(
        func test() -> void {
            if (true) {
                var x = 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, SimpleIfElseStatement) {
    std::string code = R"(
        func test() -> void {
            if (true) {
                var x = 1;
            } else {
                var y = 2;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfStatement) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            if (x == 1) {
                var result = 10;
            } else {
                if (x == 2) {
                    var result = 20;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfElseStatement) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            if (x == 1) {
                var result = 10;
            } else {
                if (x == 2) {
                    var result = 20;
                } else {
                    var result = 30;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, MultipleElseIfClauses) {
    std::string code = R"(
        func test() -> void {
            var grade = 85;
            if (grade >= 90) {
                var letter = 'A';
            } else {
                if (grade >= 80) {
                    var letter = 'B';
                } else {
                    if (grade >= 70) {
                        var letter = 'C';
                    } else {
                        if (grade >= 60) {
                            var letter = 'D';
                        } else {
                            var letter = 'F';
                        }
                    }
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithComplexConditions) {
    std::string code = R"(
        func test() -> void {
            var x = 10;
            var y = 20;
            if (x > 5 && y < 30) {
                var result = 1;
            } else {
                if (x == 10 || y == 20) {
                    var result = 2;
                } else {
                    var result = 3;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, NestedIfInElseIf) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            if (x < 0) {
                var result = -1;
            } else {
                if (x > 0) {
                    if (x < 10) {
                        var result = 1;
                    } else {
                        var result = 2;
                    }
                } else {
                    var result = 0;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithReturnStatements) {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithFunctionCalls) {
    std::string code = R"(
        func helper(x: int64) -> int64 {
            return x * 2;
        }

        func test(x: int64) -> void {
            if (x < 10) {
                var result = helper(x);
            } else {
                if (x < 20) {
                    var result = helper(x + 5);
                } else {
                    var result = helper(x + 10);
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithMultipleStatements) {
    std::string code = R"(
        func test() -> void {
            var x = 10;
            if (x < 5) {
                var a = 1;
                var b = 2;
                var c = a + b;
            } else {
                if (x < 15) {
                    var a = 10;
                    var b = 20;
                    var c = a + b;
                } else {
                    var a = 100;
                    var b = 200;
                    var c = a + b;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithArithmeticConditions) {
    std::string code = R"(
        func test() -> void {
            var x = 10;
            var y = 5;
            if (x + y > 20) {
                var result = 1;
            } else {
                if (x - y > 3) {
                    var result = 2;
                } else {
                    if (x * y > 40) {
                        var result = 3;
                    } else {
                        var result = 4;
                    }
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithBooleanVariables) {
    std::string code = R"(
        func test() -> void {
            var isActive = true;
            var isValid = false;
            if (isActive && isValid) {
                var status = 1;
            } else {
                if (isActive) {
                    var status = 2;
                } else {
                    if (isValid) {
                        var status = 3;
                    } else {
                        var status = 0;
                    }
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithComparisonChains) {
    std::string code = R"(
        func test() -> void {
            var x = 15;
            if (x < 10) {
                var range = 1;
            } else {
                if (x >= 10 && x < 20) {
                    var range = 2;
                } else {
                    if (x >= 20 && x < 30) {
                        var range = 3;
                    } else {
                        var range = 4;
                    }
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithNegatedConditions) {
    std::string code = R"(
        func test() -> void {
            var flag = true;
            if (!flag) {
                var result = 0;
            } else {
                if (flag) {
                    var result = 1;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithWhileLoop) {
    std::string code = R"(
        func test() -> void {
            var x = 10;
            if (x < 5) {
                var i = 0;
                while (i < x) {
                    i = i + 1;
                }
            } else {
                if (x < 15) {
                    var i = 0;
                    while (i < x) {
                        i = i + 1;
                    }
                } else {
                    var i = x;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithArrays) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            if (x < 3) {
                var arr = [1, 2, 3];
            } else {
                if (x < 7) {
                    var arr = [4, 5, 6];
                } else {
                    var arr = [7, 8, 9];
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, DeepElseIfChain) {
    std::string code = R"(
        func test() -> void {
            var x = 50;
            if (x == 1) {
                var r = 1;
            } else {
                if (x == 2) {
                    var r = 2;
                } else {
                    if (x == 3) {
                        var r = 3;
                    } else {
                        if (x == 4) {
                            var r = 4;
                        } else {
                            if (x == 5) {
                                var r = 5;
                            } else {
                                if (x == 6) {
                                    var r = 6;
                                } else {
                                    if (x == 7) {
                                        var r = 7;
                                    } else {
                                        if (x == 8) {
                                            var r = 8;
                                        } else {
                                            if (x == 9) {
                                                var r = 9;
                                            } else {
                                                var r = 0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithFloatComparisons) {
    std::string code = R"(
        func test() -> void {
            var temperature = 25.5;
            if (temperature < 0.0) {
                var state = 1;
            } else {
                if (temperature < 100.0) {
                    var state = 2;
                } else {
                    var state = 3;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithCharComparisons) {
    std::string code = R"(
        func test() -> void {
            var ch = 'a';
            if (ch == 'a') {
                var result = 1;
            } else {
                if (ch == 'b') {
                    var result = 2;
                } else {
                    if (ch == 'c') {
                        var result = 3;
                    } else {
                        var result = 0;
                    }
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfEmptyBlocks) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            if (x < 0) {
            } else {
                if (x == 0) {
                } else {
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithParenthesizedConditions) {
    std::string code = R"(
        func test() -> void {
            var x = 10;
            var y = 20;
            if ((x > 5) && (y < 25)) {
                var result = 1;
            } else {
                if ((x == 10) || (y == 30)) {
                    var result = 2;
                } else {
                    var result = 3;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithComplexNesting) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            var y = 10;
            if (x > 0) {
                if (y > 0) {
                    var result = 1;
                } else {
                    var result = 2;
                }
            } else {
                if (x == 0) {
                    if (y > 0) {
                        var result = 3;
                    } else {
                        var result = 4;
                    }
                } else {
                    if (y > 0) {
                        var result = 5;
                    } else {
                        var result = 6;
                    }
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithUnaryOperators) {
    std::string code = R"(
        func test() -> void {
            var x = 10;
            if (-x < 0) {
                var result = 1;
            } else {
                if (-x == 0) {
                    var result = 2;
                } else {
                    var result = 3;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithMixedTypes) {
    std::string code = R"(
        func test() -> void {
            var intVal = 5;
            var floatVal = 3.14;
            var boolVal = true;
            if (intVal > 0) {
                var r1 = 1;
            } else {
                if (floatVal > 2.0) {
                    var r2 = 2;
                } else {
                    if (boolVal) {
                        var r3 = 3;
                    } else {
                        var r4 = 0;
                    }
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfFollowedByOtherStatements) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            if (x < 3) {
                var a = 1;
            } else {
                if (x < 7) {
                    var b = 2;
                } else {
                    var c = 3;
                }
            }
            var afterIf = 100;
            var another = 200;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, MultipleIfElseIfInSameFunction) {
    std::string code = R"(
        func test() -> void {
            var x = 5;
            var y = 10;

            if (x < 3) {
                var a = 1;
            } else {
                if (x < 7) {
                    var b = 2;
                } else {
                    var c = 3;
                }
            }

            if (y < 5) {
                var d = 10;
            } else {
                if (y < 15) {
                    var e = 20;
                } else {
                    var f = 30;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(IfElseIfParsingTest, IfElseIfWithComplexExpressions) {
    std::string code = R"(
        func test() -> void {
            var a = 10;
            var b = 20;
            var c = 30;
            if (a + b > c) {
                var result = a + b - c;
            } else {
                if (a * b > c) {
                    var result = a * b / c;
                } else {
                    var result = c - a - b;
                }
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}
