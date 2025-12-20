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
    std::string code = "func test() -> void { return; }";
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
        func first() -> void { return; }
        func second() -> void { 42; }
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
    std::string code = "func add(int64 a, int64 b) -> int64 { return a + b; }";
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
    std::string code = "func calculate() -> void { 42; }";
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
    std::string code = "func test() -> void { var x = 10; }";
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
        func conditional() -> void { 
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
        func loop() -> void { 
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithByteParameter) {
    std::string code = "func process(byte data) -> byte { return data; }";
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithByteVariable) {
    std::string code = R"(
        func test() -> void {
            var b = 255;
            var typed: byte = 128;
            return;
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithByteArithmetic) {
    std::string code = R"(
        func calculate(byte a, byte b) -> byte {
            var result = a + b;
            return result;
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithFloatParameter) {
    std::string code = "func process(float data) -> float { return data; }";
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithFloatVariable) {
    std::string code = R"(
        func test() -> void {
            var f = 3.14;
            var typed: float = 2.71;
            return;
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithFloatArithmetic) {
    std::string code = R"(
        func calculate(float a, float b) -> float {
            var result = a + b * 2.0;
            return result;
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithBoolParameter) {
    std::string code = "func process(bool flag) -> bool { return flag; }";
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithBoolVariable) {
    std::string code = R"(
        func test() -> void {
            var flag = true;
            var explicit: bool = false;
            return;
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

TEST_F(SimpleASTBuilderTest, BuildFunctionWithBoolLogic) {
    std::string code = R"(
        func logic(bool a, bool b) -> bool {
            var and_result = a && b;
            var or_result = a || b;
            var not_result = !a;
            return and_result;
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