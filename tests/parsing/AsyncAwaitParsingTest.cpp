#include <gtest/gtest.h>
#include "parsing/HooParserWrapper.h"
#include "ast/SimpleASTBuilder.h"

using namespace hooc;

TEST(AsyncAwaitParsingTest, ParseAsyncFunction) {
    std::string code = R"(
        async func:Future<string> fetchData() {
            return "data";
        }
    )";
    HooParserWrapper wrapper;
    auto ast = wrapper.parseString(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1);
    
    auto funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(funcDecl, nullptr);
    EXPECT_EQ(funcDecl->getName(), "fetchData");
    EXPECT_TRUE(funcDecl->hasModifier(ast::FunctionModifier::ASYNC));
    
    auto returnType = dynamic_cast<const ast::FutureType*>(funcDecl->getReturnType());
    ASSERT_NE(returnType, nullptr);
    
    auto elementType = dynamic_cast<const ast::PrimitiveType*>(&returnType->getElementType());
    ASSERT_NE(elementType, nullptr);
    EXPECT_EQ(elementType->getKind(), ast::PrimitiveTypeKind::STRING);
}

TEST(AsyncAwaitParsingTest, ParseAwaitExpression) {
    std::string code = R"(
        async func process() {
            var data = await(fetchData());
        }
    )";
    HooParserWrapper wrapper;
    auto ast = wrapper.parseString(code);
    ASSERT_NE(ast, nullptr);
    
    auto funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(funcDecl, nullptr);
    
    const auto& statements = funcDecl->getBody()->getStatements();
    ASSERT_EQ(statements.size(), 1);
    
    auto varDeclStmt = dynamic_cast<const ast::VariableDeclarationStatement*>(statements[0].get());
    ASSERT_NE(varDeclStmt, nullptr);
    
    auto varDecl = varDeclStmt->getDeclaration();
    ASSERT_NE(varDecl, nullptr);
    
    auto initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    
    auto primary = dynamic_cast<const ast::PrimaryExpression*>(initializer);
    ASSERT_NE(primary, nullptr);
    
    auto awaitExpr = dynamic_cast<const ast::AwaitExpression*>(&primary->getPrimary());
    ASSERT_NE(awaitExpr, nullptr);
    
    auto funcCall = dynamic_cast<const ast::FunctionCallExpression*>(&awaitExpr->getFuture());
    ASSERT_NE(funcCall, nullptr);
}
