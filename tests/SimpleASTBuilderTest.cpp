#include <gtest/gtest.h>
#include <memory>
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "../antlr4/generated/HoocParser.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

class SimpleASTBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;
    
    antlr4::tree::ParseTree* parseCode(const std::string& code) {
        return parser->parseForAST(code);
    }
    
    HoocParser::CompilationUnitContext* getCompilationUnit(antlr4::tree::ParseTree* tree) {
        return dynamic_cast<HoocParser::CompilationUnitContext*>(tree);
    }
};

TEST_F(SimpleASTBuilderTest, BuildEmptyCompilationUnit) {
    std::string code = "";
    auto* parseTree = parseCode(code);
    
    if (parseTree) {
        auto* ctx = getCompilationUnit(parseTree);
        if (ctx) {
            auto ast = astBuilder->buildAST(ctx);
            ASSERT_NE(ast, nullptr);
            EXPECT_EQ(ast->toString().find("CompilationUnit"), 0);
        }
    }
}

TEST_F(SimpleASTBuilderTest, BuildSingleFunctionDeclaration) {
    std::string code = "func test() { return; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildMultipleFunctionDeclarations) {
    std::string code = R"(
        func first() { return; }
        func second() { 42; }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=2") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithParameters) {
    std::string code = "func add(int64 a, int64 b) { return a + b; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithExpressionStatement) {
    std::string code = "func calculate() { 42; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithVariableDeclaration) {
    std::string code = "func test() { var x = 10; }";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, HandleInvalidParseTree) {
    auto ast = astBuilder->buildAST(nullptr);
    EXPECT_EQ(ast, nullptr);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithIfStatement) {
    std::string code = R"(
        func conditional() { 
            if (true) { 
                return; 
            } 
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}

TEST_F(SimpleASTBuilderTest, BuildFunctionWithWhileLoop) {
    std::string code = R"(
        func loop() { 
            while (true) { 
                break; 
            } 
        }
    )";
    auto* parseTree = parseCode(code);
    
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("CompilationUnit") != std::string::npos);
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}