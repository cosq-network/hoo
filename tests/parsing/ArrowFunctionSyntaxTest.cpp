#include <gtest/gtest.h>
#include <memory>
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"

using namespace hooc;

class ArrowFunctionSyntaxTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<HooParserWrapper>();
    }

    std::unique_ptr<HooParserWrapper> parser;
};

TEST_F(ArrowFunctionSyntaxTest, ArrowSyntaxParsingFails) {
    std::string code = "func test() -> int64 { return 42; }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxTest, ArrowWithVoidReturnParsingFails) {
    std::string code = "func test() -> void { return; }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxTest, ArrowWithParametersParsingFails) {
    std::string code = "func add(a: int64, b: int64) -> int64 { return a + b; }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxTest, ArrowWithDoubleReturnParsingFails) {
    std::string code = "func compute() -> double { return 3.14; }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxTest, ArrowWithStringReturnParsingFails) {
    std::string code = "func getName() -> string { return \"test\"; }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxTest, ArrowWithBoolReturnParsingFails) {
    std::string code = "func check() -> bool { return true; }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxTest, ArrowWithArrayReturnParsingFails) {
    std::string code = "func getArray() -> int64[] { return [1, 2, 3]; }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxTest, ArrowWithNullableReturnParsingFails) {
    std::string code = "func getValue() -> int64? { return null; }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxTest, ArrowMultiParameterReturnParsingFails) {
    std::string code = "func divmod(a: int64, b: int64) -> (int64, int64) { return (a / b, a % b); }";
    auto* parseTree = parser->parseForAST(code);
    
    EXPECT_EQ(parseTree, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}
