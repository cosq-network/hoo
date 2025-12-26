#pragma once

#include "ast/AST.h"
#include "../antlr4/generated/HoocBaseVisitor.h"

namespace hooc {

class SimpleASTBuilder {
public:
    std::unique_ptr<ast::CompilationUnit> buildAST(HoocParser::CompilationUnitContext* ctx);
    
private:
    std::unique_ptr<ast::Declaration> buildDeclaration(HoocParser::DeclarationContext* ctx);
    std::unique_ptr<ast::FunctionDeclaration> buildFunctionDeclaration(HoocParser::FunctionDeclarationContext* ctx);
    std::unique_ptr<ast::VariableDeclaration> buildVariableDeclaration(HoocParser::VariableDeclarationContext* ctx);
    std::unique_ptr<ast::VariableDeclarationStatement> buildVariableDeclarationStatement(HoocParser::VariableDeclarationContext* ctx);
    std::unique_ptr<ast::ClassDeclaration> buildClassDeclaration(HoocParser::ClassDeclarationContext* ctx);
    std::unique_ptr<ast::InterfaceDeclaration> buildInterfaceDeclaration(HoocParser::InterfaceDeclarationContext* ctx);
    std::unique_ptr<ast::Type> buildType(HoocParser::TypeContext* ctx);
    std::unique_ptr<ast::UnionType> buildUnionType(HoocParser::UnionTypeContext* ctx);
    std::unique_ptr<ast::OptionalType> buildOptionalType(HoocParser::OptionalTypeContext* ctx);
    std::unique_ptr<ast::ArrayType> buildArrayType(HoocParser::ArrayTypeContext* ctx);
    std::unique_ptr<ast::BaseType> buildBaseType(HoocParser::BaseTypeContext* ctx);
    std::unique_ptr<ast::PrimitiveType> buildPrimitiveType(HoocParser::PrimitiveTypeContext* ctx);
    std::unique_ptr<ast::Statement> buildStatement(HoocParser::StatementContext* ctx);
    std::unique_ptr<ast::Block> buildBlock(HoocParser::BlockContext* ctx);
    std::unique_ptr<ast::IfStatement> buildIfStatement(HoocParser::IfStatementContext* ctx);
    std::unique_ptr<ast::WhileStatement> buildWhileStatement(HoocParser::WhileStatementContext* ctx);
    std::unique_ptr<ast::ForInStatement> buildForInStatement(HoocParser::ForInStatementContext* ctx);
    std::unique_ptr<ast::ForRangeStatement> buildForRangeStatement(HoocParser::ForRangeStatementContext* ctx);
    std::unique_ptr<ast::Expression> buildExpression(HoocParser::ExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildAssignmentExpression(HoocParser::AssignmentExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildLogicalOrExpression(HoocParser::LogicalOrExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildLogicalAndExpression(HoocParser::LogicalAndExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildRelationalExpression(HoocParser::RelationalExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildAdditiveExpression(HoocParser::AdditiveExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildUnaryExpression(HoocParser::UnaryExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildPostfixExpression(HoocParser::PostfixExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildPrimary(HoocParser::PrimaryContext* ctx);
    std::unique_ptr<ast::Parameter> buildParameter(HoocParser::ParameterContext* ctx);
    std::unique_ptr<ast::ArgumentList> buildArgumentList(HoocParser::ArgumentListContext* ctx);
    std::unique_ptr<ast::ArrayLiteral> buildArrayLiteral(HoocParser::PrimaryContext* ctx);
    std::unique_ptr<ast::ExpressionList> buildExpressionList(HoocParser::ExpressionListContext* ctx);

    // Import building methods
    std::unique_ptr<ast::ImportStatement> buildImportStatement(HoocParser::ImportStatementContext* ctx);
    std::unique_ptr<ast::BasicImport> buildBasicImport(HoocParser::BasicImportContext* ctx);
    std::unique_ptr<ast::FromImport> buildFromImport(HoocParser::FromImportContext* ctx);
    std::unique_ptr<ast::ModulePath> buildModulePath(HoocParser::ModulePathContext* ctx);
    std::unique_ptr<ast::ImportItem> buildImportItem(HoocParser::ImportItemContext* ctx);

    // Class and interface building methods
    std::unique_ptr<ast::ConstructorDeclaration> buildConstructorDeclaration(HoocParser::ConstructorDeclarationContext* ctx);
    std::unique_ptr<ast::ClassBody> buildClassBody(HoocParser::ClassBodyContext* ctx);
    std::unique_ptr<ast::ClassMember> buildClassMember(HoocParser::ClassMemberContext* ctx);
    std::unique_ptr<ast::EventDeclaration> buildEventDeclaration(HoocParser::EventDeclarationContext* ctx);
    std::unique_ptr<ast::InterfaceMember> buildInterfaceMember(HoocParser::InterfaceMemberContext* ctx);
    std::unique_ptr<ast::FunctionSignature> buildFunctionSignature(HoocParser::FunctionSignatureContext* ctx);
    ast::ClassModifier getClassModifier(HoocParser::ClassModifierContext* ctx);

    // Helper methods
    ast::PrimitiveTypeKind getPrimitiveTypeKind(const std::string& typeName);
    ast::BinaryOperator getBinaryOperator(antlr4::tree::TerminalNode* node);
    std::string getStringValue(antlr4::tree::TerminalNode* node);
    int getIntValue(antlr4::tree::TerminalNode* node);
    double getDoubleValue(antlr4::tree::TerminalNode* node);
    char getCharValue(antlr4::tree::TerminalNode* node);
    bool getBoolValue(antlr4::tree::TerminalNode* node);
};

} // namespace hooc