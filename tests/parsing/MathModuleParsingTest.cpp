#include <gtest/gtest.h>
#include <memory>
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"

using namespace hooc;

class MathModuleParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<HooParserWrapper>();
    }

    std::unique_ptr<HooParserWrapper> parser;
};

TEST_F(MathModuleParsingTest, ImportMathModule) {
    std::string code = R"(
        import hoo.math;
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(MathModuleParsingTest, MathModuleFunctionDeclaration) {
    std::string code = R"(
        import hoo.math;

        func:int64 abs(x: int64) { if x < 0 { return -x; } return x; }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(MathModuleParsingTest, MathModulePIConstant) {
    std::string code = R"(
        import hoo.math;

        func:double getPI() { return hoo.math.PI; }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(MathModuleParsingTest, MathModuleSqrtFunction) {
    std::string code = R"(
        import hoo.math;

        func:double squareRoot(x: double) { return hoo.math.sqrt(x); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}

TEST_F(MathModuleParsingTest, RandomClassInstantiation) {
    std::string code = R"(
        import hoo.math;

        func:int64 testRandom() { var rng = new hoo.math.Random(); return rng.nextInt(); }
    )";
    auto* parseTree = parser->parseForAST(code);
    EXPECT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}