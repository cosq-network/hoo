#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

class MapTypeParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<HooParserWrapper>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<HooParserWrapper> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    antlr4::tree::ParseTree* parseCode(const std::string& code) {
        return parser->parseForAST(code);
    }
    
    HoocParser::CompilationUnitContext* getCompilationUnit(antlr4::tree::ParseTree* tree) {
        return dynamic_cast<HoocParser::CompilationUnitContext*>(tree);
    }
};

// ============================================
// Parameter type tests
// ============================================

TEST_F(MapTypeParsingTest, ParseMapTypeWithStringKeyInt64Value) {
    std::string code = R"(
        func test(m: map<string, int64>) {
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
}

TEST_F(MapTypeParsingTest, ParseMapTypeWithInt64KeyStringValue) {
    std::string code = R"(
        func test(m: map<int64, string>) {
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
}

TEST_F(MapTypeParsingTest, ParseMapTypeWithByteKey) {
    std::string code = R"(
        func test(m: map<byte, int64>) {
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}

TEST_F(MapTypeParsingTest, ParseMapTypeWithInt8Key) {
    std::string code = R"(
        func test(m: map<int8, string>) {
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}

TEST_F(MapTypeParsingTest, ParseMapTypeWithCharKey) {
    std::string code = R"(
        func test(m: map<char, double>) {
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}

TEST_F(MapTypeParsingTest, ParseMapTypeWithClassValue) {
    std::string code = R"(
        class Person {
            var name: string;
        }
        func test(m: map<string, Person>) {
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}

// ============================================
// Return type tests (using func:returnType syntax)
// ============================================

TEST_F(MapTypeParsingTest, ParseMapReturnType) {
    // Hooc syntax: func:returnType name() { }
    std::string code = R"(
        func:map<string, int64> createMap() {
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}

TEST_F(MapTypeParsingTest, ParseMapReturnTypeWithInt64Key) {
    std::string code = R"(
        func:map<int64, string> createMap() {
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}

TEST_F(MapTypeParsingTest, ParseMapReturnTypeWithCharKey) {
    std::string code = R"(
        func:map<char, double> createMap() {
            return;
        }
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
}

// ============================================
// Variable declaration tests
// ============================================

TEST_F(MapTypeParsingTest, ParseMapVariableDeclaration) {
    std::string code = R"(
        var scores: map<string, int64>;
    )";
    auto* parseTree = parseCode(code);

    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    const auto& decls = ast->getDeclarations();
    EXPECT_EQ(decls.size(), 1);
}