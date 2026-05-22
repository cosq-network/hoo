
// Generated from Hooc.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "HoocParser.h"


namespace hooc {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by HoocParser.
 */
class  HoocVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by HoocParser.
   */
    virtual std::any visitCompilationUnit(HoocParser::CompilationUnitContext *context) = 0;

    virtual std::any visitBasicImport(HoocParser::BasicImportContext *context) = 0;

    virtual std::any visitFromImport(HoocParser::FromImportContext *context) = 0;

    virtual std::any visitModulePath(HoocParser::ModulePathContext *context) = 0;

    virtual std::any visitQualifiedIdentifier(HoocParser::QualifiedIdentifierContext *context) = 0;

    virtual std::any visitImportItem(HoocParser::ImportItemContext *context) = 0;

    virtual std::any visitDeclaration(HoocParser::DeclarationContext *context) = 0;

    virtual std::any visitFunctionDeclaration(HoocParser::FunctionDeclarationContext *context) = 0;

    virtual std::any visitParameterList(HoocParser::ParameterListContext *context) = 0;

    virtual std::any visitParameter(HoocParser::ParameterContext *context) = 0;

    virtual std::any visitClassDeclaration(HoocParser::ClassDeclarationContext *context) = 0;

    virtual std::any visitClassModifier(HoocParser::ClassModifierContext *context) = 0;

    virtual std::any visitClassBody(HoocParser::ClassBodyContext *context) = 0;

    virtual std::any visitClassMember(HoocParser::ClassMemberContext *context) = 0;

    virtual std::any visitConstructorDeclaration(HoocParser::ConstructorDeclarationContext *context) = 0;

    virtual std::any visitFunctionModifier(HoocParser::FunctionModifierContext *context) = 0;

    virtual std::any visitVariableDeclaration(HoocParser::VariableDeclarationContext *context) = 0;

    virtual std::any visitConstantDeclaration(HoocParser::ConstantDeclarationContext *context) = 0;

    virtual std::any visitType(HoocParser::TypeContext *context) = 0;

    virtual std::any visitOptionalType(HoocParser::OptionalTypeContext *context) = 0;

    virtual std::any visitArrayType(HoocParser::ArrayTypeContext *context) = 0;

    virtual std::any visitBaseType(HoocParser::BaseTypeContext *context) = 0;

    virtual std::any visitMapType(HoocParser::MapTypeContext *context) = 0;

    virtual std::any visitMapKeyType(HoocParser::MapKeyTypeContext *context) = 0;

    virtual std::any visitPrimitiveType(HoocParser::PrimitiveTypeContext *context) = 0;

    virtual std::any visitFfiDeclaration(HoocParser::FfiDeclarationContext *context) = 0;

    virtual std::any visitFfiImportDeclaration(HoocParser::FfiImportDeclarationContext *context) = 0;

    virtual std::any visitFfiLinkDeclaration(HoocParser::FfiLinkDeclarationContext *context) = 0;

    virtual std::any visitFfiNativeFunction(HoocParser::FfiNativeFunctionContext *context) = 0;

    virtual std::any visitFfiNativeDeclaration(HoocParser::FfiNativeDeclarationContext *context) = 0;

    virtual std::any visitFfiParameterList(HoocParser::FfiParameterListContext *context) = 0;

    virtual std::any visitFfiParameter(HoocParser::FfiParameterContext *context) = 0;

    virtual std::any visitFfiType(HoocParser::FfiTypeContext *context) = 0;

    virtual std::any visitLibrarySearchPaths(HoocParser::LibrarySearchPathsContext *context) = 0;

    virtual std::any visitVersionRange(HoocParser::VersionRangeContext *context) = 0;

    virtual std::any visitStatement(HoocParser::StatementContext *context) = 0;

    virtual std::any visitTryCatchStatement(HoocParser::TryCatchStatementContext *context) = 0;

    virtual std::any visitThrowStatement(HoocParser::ThrowStatementContext *context) = 0;

    virtual std::any visitBlock(HoocParser::BlockContext *context) = 0;

    virtual std::any visitVariableDeclarationStatement(HoocParser::VariableDeclarationStatementContext *context) = 0;

    virtual std::any visitExpressionStatement(HoocParser::ExpressionStatementContext *context) = 0;

    virtual std::any visitIfStatement(HoocParser::IfStatementContext *context) = 0;

    virtual std::any visitForStatement(HoocParser::ForStatementContext *context) = 0;

    virtual std::any visitWhileStatement(HoocParser::WhileStatementContext *context) = 0;

    virtual std::any visitReturnStatement(HoocParser::ReturnStatementContext *context) = 0;

    virtual std::any visitScopeStatement(HoocParser::ScopeStatementContext *context) = 0;

    virtual std::any visitBreakStatement(HoocParser::BreakStatementContext *context) = 0;

    virtual std::any visitContinueStatement(HoocParser::ContinueStatementContext *context) = 0;

    virtual std::any visitExpression(HoocParser::ExpressionContext *context) = 0;

    virtual std::any visitAssignmentExpression(HoocParser::AssignmentExpressionContext *context) = 0;

    virtual std::any visitCompoundAssignment(HoocParser::CompoundAssignmentContext *context) = 0;

    virtual std::any visitLogicalOrExpression(HoocParser::LogicalOrExpressionContext *context) = 0;

    virtual std::any visitLogicalAndExpression(HoocParser::LogicalAndExpressionContext *context) = 0;

    virtual std::any visitRelationalExpression(HoocParser::RelationalExpressionContext *context) = 0;

    virtual std::any visitAdditiveExpression(HoocParser::AdditiveExpressionContext *context) = 0;

    virtual std::any visitMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext *context) = 0;

    virtual std::any visitUnaryExpression(HoocParser::UnaryExpressionContext *context) = 0;

    virtual std::any visitPostfixExpression(HoocParser::PostfixExpressionContext *context) = 0;

    virtual std::any visitPostfixSuffix(HoocParser::PostfixSuffixContext *context) = 0;

    virtual std::any visitAugmentedAssignment(HoocParser::AugmentedAssignmentContext *context) = 0;

    virtual std::any visitPrimary(HoocParser::PrimaryContext *context) = 0;

    virtual std::any visitNewExpression(HoocParser::NewExpressionContext *context) = 0;

    virtual std::any visitInterpolatedString(HoocParser::InterpolatedStringContext *context) = 0;

    virtual std::any visitArgumentList(HoocParser::ArgumentListContext *context) = 0;

    virtual std::any visitExpressionList(HoocParser::ExpressionListContext *context) = 0;


};

}  // namespace hooc
