#pragma once

#include "HoocBaseVisitor.h"
#include "HoocParser.h"
#include "ast/AST.h"
#include <memory>
#include <vector>
#include <string>

namespace hooc {

class ASTBuilder : public HoocBaseVisitor {
public:
    ASTBuilder() = default;
    ~ASTBuilder() = default;

    // Main entry point - returns the complete AST
    std::unique_ptr<ast::CompilationUnit> buildAST(HoocParser::CompilationUnitContext* ctx);

    // Compilation unit and imports
    std::any visitCompilationUnit(HoocParser::CompilationUnitContext* ctx) override;
    std::any visitImportStatement(HoocParser::ImportStatementContext* ctx);
    std::any visitNamedImports(HoocParser::NamedImportsContext* ctx);
    std::any visitNamespaceImport(HoocParser::NamespaceImportContext* ctx);
    std::any visitSideEffectImport(HoocParser::SideEffectImportContext* ctx);
    std::any visitImportItem(HoocParser::ImportItemContext* ctx);

    // Declarations
    std::any visitDeclaration(HoocParser::DeclarationContext* ctx) override;
    std::any visitFunctionDeclaration(HoocParser::FunctionDeclarationContext* ctx) override;
    std::any visitClassDeclaration(HoocParser::ClassDeclarationContext* ctx) override;
    std::any visitInterfaceDeclaration(HoocParser::InterfaceDeclarationContext* ctx) override;
    std::any visitVariableDeclaration(HoocParser::VariableDeclarationContext* ctx) override;
    std::any visitParameter(HoocParser::ParameterContext* ctx) override;
    std::any visitParameterList(HoocParser::ParameterListContext* ctx) override;

    // Class-related
    std::any visitClassModifier(HoocParser::ClassModifierContext* ctx) override;
    std::any visitPrimaryConstructor(HoocParser::PrimaryConstructorContext* ctx) override;
    std::any visitClassBody(HoocParser::ClassBodyContext* ctx) override;
    std::any visitClassMember(HoocParser::ClassMemberContext* ctx) override;
    std::any visitEventDeclaration(HoocParser::EventDeclarationContext* ctx) override;
    std::any visitInterfaceList(HoocParser::InterfaceListContext* ctx) override;

    // Interface-related
    std::any visitInterfaceMember(HoocParser::InterfaceMemberContext* ctx) override;
    std::any visitFunctionSignature(HoocParser::FunctionSignatureContext* ctx) override;

    // Types
    std::any visitType(HoocParser::TypeContext* ctx) override;
    std::any visitUnionType(HoocParser::UnionTypeContext* ctx) override;
    std::any visitOptionalType(HoocParser::OptionalTypeContext* ctx) override;
    std::any visitArrayType(HoocParser::ArrayTypeContext* ctx) override;
    std::any visitBaseType(HoocParser::BaseTypeContext* ctx) override;
    std::any visitPrimitiveType(HoocParser::PrimitiveTypeContext* ctx) override;

    // Statements
    std::any visitStatement(HoocParser::StatementContext* ctx) override;
    std::any visitBlock(HoocParser::BlockContext* ctx) override;
    std::any visitExpressionStatement(HoocParser::ExpressionStatementContext* ctx) override;
    std::any visitIfStatement(HoocParser::IfStatementContext* ctx) override;
    std::any visitForInStatement(HoocParser::ForInStatementContext* ctx) override;
    std::any visitForRangeStatement(HoocParser::ForRangeStatementContext* ctx) override;
    std::any visitWhileStatement(HoocParser::WhileStatementContext* ctx) override;
    std::any visitReturnStatement(HoocParser::ReturnStatementContext* ctx) override;
    std::any visitScopeStatement(HoocParser::ScopeStatementContext* ctx) override;

    // Expressions
    std::any visitExpression(HoocParser::ExpressionContext* ctx);
    std::any visitPrimaryExpression(HoocParser::PrimaryExpressionContext* ctx);
    std::any visitMemberAccess(HoocParser::MemberAccessContext* ctx);
    std::any visitArrayAccess(HoocParser::ArrayAccessContext* ctx);
    std::any visitFunctionCall(HoocParser::FunctionCallContext* ctx);
    std::any visitNewArrayExpression(HoocParser::NewArrayExpressionContext* ctx);
    std::any visitNewObjectExpression(HoocParser::NewObjectExpressionContext* ctx);
    std::any visitUnaryMinus(HoocParser::UnaryMinusContext* ctx);
    std::any visitLogicalNot(HoocParser::LogicalNotContext* ctx);
    std::any visitMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext* ctx);
    std::any visitAdditiveExpression(HoocParser::AdditiveExpressionContext* ctx);
    std::any visitRelationalExpression(HoocParser::RelationalExpressionContext* ctx);
    std::any visitLogicalAnd(HoocParser::LogicalAndContext* ctx);
    std::any visitLogicalOr(HoocParser::LogicalOrContext* ctx);
    std::any visitAssignmentExpression(HoocParser::AssignmentExpressionContext* ctx);
    std::any visitErrorHandlingExpression(HoocParser::ErrorHandlingExpressionContext* ctx);
    std::any visitArrayLiteral(HoocParser::ArrayLiteralContext* ctx);
    std::any visitListComprehension(HoocParser::ListComprehensionContext* ctx);
    std::any visitLambdaExpression(HoocParser::LambdaExpressionContext* ctx);
    std::any visitMultiParamLambda(HoocParser::MultiParamLambdaContext* ctx);

    // Primary expressions
    std::any visitPrimary(HoocParser::PrimaryContext* ctx) override;
    std::any visitInterpolatedString(HoocParser::InterpolatedStringContext* ctx) override;

    // Utility
    std::any visitArgumentList(HoocParser::ArgumentListContext* ctx) override;
    std::any visitExpressionList(HoocParser::ExpressionListContext* ctx) override;

private:
    // Helper methods
    ast::ClassModifier parseClassModifier(const std::string& modifier);
    ast::PrimitiveTypeKind parsePrimitiveType(const std::string& type);
    ast::BinaryOperator parseBinaryOperator(const std::string& op);
    std::string getStringValue(antlr4::tree::TerminalNode* node);
    int64_t getIntegerValue(antlr4::tree::TerminalNode* node);
    double getDoubleValue(antlr4::tree::TerminalNode* node);
    char getCharValue(antlr4::tree::TerminalNode* node);
    bool getBoolValue(antlr4::tree::TerminalNode* node);

    // Template helpers for casting std::any to AST types
    template<typename T>
    std::unique_ptr<T> cast(std::any value) {
        return std::any_cast<std::unique_ptr<T>>(value);
    }

    template<typename T>
    std::vector<std::unique_ptr<T>> castVector(std::any value) {
        return std::any_cast<std::vector<std::unique_ptr<T>>>(value);
    }
};

} // namespace hooc