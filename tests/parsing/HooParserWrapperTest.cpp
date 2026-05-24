#include <gtest/gtest.h>
#include <memory>
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"

using namespace hooc;

class HooParserWrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<HooParserWrapper>();
    }

    std::unique_ptr<HooParserWrapper> parser;
};

TEST_F(HooParserWrapperTest, ParseValidFunction) {
    std::string code = "func test() { return; }";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(HooParserWrapperTest, ParseMultipleFunctions) {
    std::string code = R"(
        func first() { return; }
        func second() { return; }
    )";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(HooParserWrapperTest, ParseEmptySource) {
    std::string code = "";
    auto* parseTree = parser->parseForAST(code);
    
    // Empty source might be valid depending on grammar implementation
    // Test should pass regardless of whether it's considered valid or not
    if (parseTree == nullptr) {
        EXPECT_FALSE(parser->getLastError().empty());
    }
}

TEST_F(HooParserWrapperTest, ParseFunctionWithParameters) {
    std::string code = "func:int64 add(a: int64, b: int64) { return a + b; }";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(HooParserWrapperTest, ParseFunctionWithStatements) {
    std::string code = R"(
        func:int64 complex() {
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

TEST_F(HooParserWrapperTest, ParseInvalidSyntax) {
    std::string code = "func { invalid syntax";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(HooParserWrapperTest, ParseMultipleCalls) {
    // Test that parser can be reused
    std::string code1 = "func first() { return; }";
    std::string code2 = "func second() { return; }";
    
    auto* parseTree1 = parser->parseForAST(code1);
    ASSERT_NE(parseTree1, nullptr);
    
    auto* parseTree2 = parser->parseForAST(code2);
    ASSERT_NE(parseTree2, nullptr);
}

TEST_F(HooParserWrapperTest, ParseVariableDeclarations) {
    std::string code = R"(
        func:int64 test() {
            var x = 42;
            var y = x + 1;
            return y;
        }
    )";
    auto* parseTree = parser->parseForAST(code);
    
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(HooParserWrapperTest, ParseControlFlow) {
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