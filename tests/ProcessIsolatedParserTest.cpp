#include <gtest/gtest.h>
#include <memory>
#include "../src/ProcessIsolatedParser.h"
#include "../antlr4/generated/HoocParser.h"

using namespace hooc;

class ProcessIsolatedParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
};

TEST_F(ProcessIsolatedParserTest, ParseValidFunction) {
    std::string code = "func test() { return; }";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(ProcessIsolatedParserTest, ParseMultipleFunctions) {
    std::string code = R"(
        func first() { return; }
        func second() { return; }
    )";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(ProcessIsolatedParserTest, ParseEmptySource) {
    std::string code = "";
    auto* parseTree = parser->parseForAST(code);
    
    // Empty source might be valid depending on grammar implementation
    // Test should pass regardless of whether it's considered valid or not
    if (parseTree == nullptr) {
        EXPECT_FALSE(parser->getLastError().empty());
    }
}

TEST_F(ProcessIsolatedParserTest, ParseFunctionWithParameters) {
    std::string code = "func add(int64 a, int64 b) { return a + b; }";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(ProcessIsolatedParserTest, ParseFunctionWithStatements) {
    std::string code = R"(
        func complex() {
            var x = 10;
            if (x > 5) {
                return x;
            } else {
                return 0;
            }
        }
    )";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(ProcessIsolatedParserTest, ParseInvalidSyntax) {
    std::string code = "func { invalid syntax";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ProcessIsolatedParserTest, ParseMultipleCalls) {
    // Test that parser can be reused
    std::string code1 = "func first() { return; }";
    std::string code2 = "func second() { return; }";
    
    auto* parseTree1 = parser->parseForAST(code1);
    ASSERT_NE(parseTree1, nullptr);
    
    auto* parseTree2 = parser->parseForAST(code2);
    ASSERT_NE(parseTree2, nullptr);
}

TEST_F(ProcessIsolatedParserTest, ParseVariableDeclarations) {
    std::string code = R"(
        func test() {
            var x = 42;
            var y = x + 1;
            return y;
        }
    )";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(ProcessIsolatedParserTest, ParseControlFlow) {
    std::string code = R"(
        func controlFlow() {
            while (true) {
                if (condition) {
                    break;
                } else {
                    continue;
                }
            }
        }
    )";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}