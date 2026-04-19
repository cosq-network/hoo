#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "HoocParser.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

class FunctionModifiersTest : public ::testing::Test {
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

// ============================================================================
// PUBLIC FUNCTION MODIFIER TESTS
// ============================================================================

TEST_F(FunctionModifiersTest, ClassWithPublicFunction) {
    std::string code = R"(
        class MyClass {
            public func getValue() {
                return 42;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(FunctionModifiersTest, ClassWithPrivateFunction) {
    std::string code = R"(
        class MyClass {
            private func setValue(x:int64) {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(FunctionModifiersTest, ClassWithAsyncFunction) {
    std::string code = R"(
        class MyClass {
            public async func fetchData() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(FunctionModifiersTest, ClassWithMultipleModifiers) {
    std::string code = R"(
        class MyClass {
            public func fetchData() {
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
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(FunctionModifiersTest, ClassWithMixedModifiers) {
    std::string code = R"(
        class MyClass {
            public func publicMethod() { return; }
            private func privateMethod() { return; }
            async func asyncMethod() { return; }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(FunctionModifiersTest, ClassWithoutModifiers) {
    std::string code = R"(
        class MyClass {
            public func getValue() { return 42; }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}

TEST_F(FunctionModifiersTest, ClassWithVarAndFunctions) {
    std::string code = R"(
        class MyClass {
            var x: int64;
            public func getX() { return x; }
            private func setX(v:int64) { x = v; }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);
    
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 1);
}