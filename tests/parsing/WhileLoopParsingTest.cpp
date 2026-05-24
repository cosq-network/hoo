#include <gtest/gtest.h>
#include <memory>
#include "src/parsing/HooParserWrapper.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/ast/AST.h"

using namespace hooc;
using namespace hooc::ast;

class WhileLoopParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<HooParserWrapper>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<HooParserWrapper> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    std::unique_ptr<CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;
        return astBuilder->buildAST(parseTree);
    }
};

// Basic while loop tests
TEST_F(WhileLoopParsingTest, SimpleWhileLoop) {
    std::string code = R"(
        func test() {
            while (true) {
                return;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithVariableCondition) {
    std::string code = R"(
        func test() {
            var x = 0;
            while (x < 10) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithComplexCondition) {
    std::string code = R"(
        func test() {
            var x = 5;
            var y = 10;
            while (x < y && y > 0) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithLogicalOr) {
    std::string code = R"(
        func test() {
            var a = true;
            var b = false;
            while (a || b) {
                a = false;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithNegation) {
    std::string code = R"(
        func test() {
            var flag = true;
            while (!flag) {
                flag = true;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Multiple statements in while loop
TEST_F(WhileLoopParsingTest, WhileLoopWithMultipleStatements) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithMultipleAssignments) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Nested while loops
TEST_F(WhileLoopParsingTest, NestedWhileLoops) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, DeeplyNestedWhileLoops) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithIfInside) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, IfWithWhileLoopInside) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Type-specific tests
TEST_F(WhileLoopParsingTest, WhileLoopWithIntComparison) {
    std::string code = R"(
        func test() {
            var count = 0;
            while (count < 100) {
                count = count + 10;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithFloatComparison) {
    std::string code = R"(
        func test() {
            var value = 0.5;
            while (value < 10.0) {
                value = value + 1.5;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithBoolVariable) {
    std::string code = R"(
        func test() {
            var active = true;
            while (active) {
                active = false;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithCharComparison) {
    std::string code = R"(
        func test() {
            var ch = 'a';
            while (ch != 'z') {
                ch = 'b';
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Advanced conditions
TEST_F(WhileLoopParsingTest, WhileLoopWithArithmeticCondition) {
    std::string code = R"(
        func test() {
            var x = 10;
            var y = 5;
            while (x - y > 0) {
                x = x - 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithComparisonChain) {
    std::string code = R"(
        func test() {
            var x = 5;
            while (x >= 0 && x <= 10) {
                x = x - 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithComplexLogic) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Control flow patterns
TEST_F(WhileLoopParsingTest, WhileLoopWithReturn) {
    std::string code = R"(
        func:int64 test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, MultipleWhileLoopsInFunction) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopFollowedByOtherStatements) {
    std::string code = R"(
        func test() {
            var x = 0;
            while (x < 5) {
                x = x + 1;
            }
            var result = x + 10;
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Array operations in while loop
TEST_F(WhileLoopParsingTest, WhileLoopWithArrayAccess) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Function calls in while loop
TEST_F(WhileLoopParsingTest, WhileLoopWithFunctionCall) {
    std::string code = R"(
        func:int64 helper(x: int64) {
            return x * 2;
        }

        func test() {
            var x = 0;
            while (x < 10) {
                var y = helper(x);
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Empty and minimal loops
TEST_F(WhileLoopParsingTest, EmptyWhileLoop) {
    std::string code = R"(
        func test() {
            while (false) {
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithSingleStatement) {
    std::string code = R"(
        func test() {
            var x = 0;
            while (x < 100) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Parameter-based conditions
TEST_F(WhileLoopParsingTest, WhileLoopWithParameterCondition) {
    std::string code = R"(
        func test(limit: int64) {
            var x = 0;
            while (x < limit) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Complex nesting scenarios
TEST_F(WhileLoopParsingTest, WhileAndIfComplexNesting) {
    std::string code = R"(
        func test() {
            var x = 0;
            while (x < 10) {
                if (x == 3) {
                    var y = 0;
                    while (y < 2) {
                        y = y + 1;
                    }
                } else {
                    var z = x + 1;
                }
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Mixed type operations
TEST_F(WhileLoopParsingTest, WhileLoopWithMixedTypes) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Multiple loop variables
TEST_F(WhileLoopParsingTest, WhileLoopWithMultipleVariables) {
    std::string code = R"(
        func test() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

// Equality and inequality tests
TEST_F(WhileLoopParsingTest, WhileLoopWithEqualityCheck) {
    std::string code = R"(
        func test() {
            var x = 10;
            while (x != 0) {
                x = x - 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(WhileLoopParsingTest, WhileLoopWithEqualsComparison) {
    std::string code = R"(
        func test() {
            var x = 5;
            var target = 15;

            while (x == target) {
                x = x + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}
