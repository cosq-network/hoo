
// Generated from Hooc.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "HoocParser.h"


namespace hooc {

/**
 * This interface defines an abstract listener for a parse tree produced by HoocParser.
 */
class  HoocListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterCompilationUnit(HoocParser::CompilationUnitContext *ctx) = 0;
  virtual void exitCompilationUnit(HoocParser::CompilationUnitContext *ctx) = 0;

  virtual void enterBasicImport(HoocParser::BasicImportContext *ctx) = 0;
  virtual void exitBasicImport(HoocParser::BasicImportContext *ctx) = 0;

  virtual void enterFromImport(HoocParser::FromImportContext *ctx) = 0;
  virtual void exitFromImport(HoocParser::FromImportContext *ctx) = 0;

  virtual void enterModulePath(HoocParser::ModulePathContext *ctx) = 0;
  virtual void exitModulePath(HoocParser::ModulePathContext *ctx) = 0;

  virtual void enterQualifiedIdentifier(HoocParser::QualifiedIdentifierContext *ctx) = 0;
  virtual void exitQualifiedIdentifier(HoocParser::QualifiedIdentifierContext *ctx) = 0;

  virtual void enterImportItem(HoocParser::ImportItemContext *ctx) = 0;
  virtual void exitImportItem(HoocParser::ImportItemContext *ctx) = 0;

  virtual void enterDeclaration(HoocParser::DeclarationContext *ctx) = 0;
  virtual void exitDeclaration(HoocParser::DeclarationContext *ctx) = 0;

  virtual void enterFunctionDeclaration(HoocParser::FunctionDeclarationContext *ctx) = 0;
  virtual void exitFunctionDeclaration(HoocParser::FunctionDeclarationContext *ctx) = 0;

  virtual void enterParameterList(HoocParser::ParameterListContext *ctx) = 0;
  virtual void exitParameterList(HoocParser::ParameterListContext *ctx) = 0;

  virtual void enterParameter(HoocParser::ParameterContext *ctx) = 0;
  virtual void exitParameter(HoocParser::ParameterContext *ctx) = 0;

  virtual void enterClassDeclaration(HoocParser::ClassDeclarationContext *ctx) = 0;
  virtual void exitClassDeclaration(HoocParser::ClassDeclarationContext *ctx) = 0;

  virtual void enterClassModifier(HoocParser::ClassModifierContext *ctx) = 0;
  virtual void exitClassModifier(HoocParser::ClassModifierContext *ctx) = 0;

  virtual void enterClassBody(HoocParser::ClassBodyContext *ctx) = 0;
  virtual void exitClassBody(HoocParser::ClassBodyContext *ctx) = 0;

  virtual void enterClassMember(HoocParser::ClassMemberContext *ctx) = 0;
  virtual void exitClassMember(HoocParser::ClassMemberContext *ctx) = 0;

  virtual void enterConstructorDeclaration(HoocParser::ConstructorDeclarationContext *ctx) = 0;
  virtual void exitConstructorDeclaration(HoocParser::ConstructorDeclarationContext *ctx) = 0;

  virtual void enterFunctionModifier(HoocParser::FunctionModifierContext *ctx) = 0;
  virtual void exitFunctionModifier(HoocParser::FunctionModifierContext *ctx) = 0;

  virtual void enterVariableDeclaration(HoocParser::VariableDeclarationContext *ctx) = 0;
  virtual void exitVariableDeclaration(HoocParser::VariableDeclarationContext *ctx) = 0;

  virtual void enterConstantDeclaration(HoocParser::ConstantDeclarationContext *ctx) = 0;
  virtual void exitConstantDeclaration(HoocParser::ConstantDeclarationContext *ctx) = 0;

  virtual void enterType(HoocParser::TypeContext *ctx) = 0;
  virtual void exitType(HoocParser::TypeContext *ctx) = 0;

  virtual void enterOptionalType(HoocParser::OptionalTypeContext *ctx) = 0;
  virtual void exitOptionalType(HoocParser::OptionalTypeContext *ctx) = 0;

  virtual void enterArrayType(HoocParser::ArrayTypeContext *ctx) = 0;
  virtual void exitArrayType(HoocParser::ArrayTypeContext *ctx) = 0;

  virtual void enterBaseType(HoocParser::BaseTypeContext *ctx) = 0;
  virtual void exitBaseType(HoocParser::BaseTypeContext *ctx) = 0;

  virtual void enterMapType(HoocParser::MapTypeContext *ctx) = 0;
  virtual void exitMapType(HoocParser::MapTypeContext *ctx) = 0;

  virtual void enterMapKeyType(HoocParser::MapKeyTypeContext *ctx) = 0;
  virtual void exitMapKeyType(HoocParser::MapKeyTypeContext *ctx) = 0;

  virtual void enterPrimitiveType(HoocParser::PrimitiveTypeContext *ctx) = 0;
  virtual void exitPrimitiveType(HoocParser::PrimitiveTypeContext *ctx) = 0;

  virtual void enterFfiDeclaration(HoocParser::FfiDeclarationContext *ctx) = 0;
  virtual void exitFfiDeclaration(HoocParser::FfiDeclarationContext *ctx) = 0;

  virtual void enterFfiImportDeclaration(HoocParser::FfiImportDeclarationContext *ctx) = 0;
  virtual void exitFfiImportDeclaration(HoocParser::FfiImportDeclarationContext *ctx) = 0;

  virtual void enterFfiLinkDeclaration(HoocParser::FfiLinkDeclarationContext *ctx) = 0;
  virtual void exitFfiLinkDeclaration(HoocParser::FfiLinkDeclarationContext *ctx) = 0;

  virtual void enterFfiNativeFunction(HoocParser::FfiNativeFunctionContext *ctx) = 0;
  virtual void exitFfiNativeFunction(HoocParser::FfiNativeFunctionContext *ctx) = 0;

  virtual void enterFfiNativeDeclaration(HoocParser::FfiNativeDeclarationContext *ctx) = 0;
  virtual void exitFfiNativeDeclaration(HoocParser::FfiNativeDeclarationContext *ctx) = 0;

  virtual void enterFfiParameterList(HoocParser::FfiParameterListContext *ctx) = 0;
  virtual void exitFfiParameterList(HoocParser::FfiParameterListContext *ctx) = 0;

  virtual void enterFfiParameter(HoocParser::FfiParameterContext *ctx) = 0;
  virtual void exitFfiParameter(HoocParser::FfiParameterContext *ctx) = 0;

  virtual void enterFfiType(HoocParser::FfiTypeContext *ctx) = 0;
  virtual void exitFfiType(HoocParser::FfiTypeContext *ctx) = 0;

  virtual void enterLibrarySearchPaths(HoocParser::LibrarySearchPathsContext *ctx) = 0;
  virtual void exitLibrarySearchPaths(HoocParser::LibrarySearchPathsContext *ctx) = 0;

  virtual void enterVersionRange(HoocParser::VersionRangeContext *ctx) = 0;
  virtual void exitVersionRange(HoocParser::VersionRangeContext *ctx) = 0;

  virtual void enterStatement(HoocParser::StatementContext *ctx) = 0;
  virtual void exitStatement(HoocParser::StatementContext *ctx) = 0;

  virtual void enterTryCatchStatement(HoocParser::TryCatchStatementContext *ctx) = 0;
  virtual void exitTryCatchStatement(HoocParser::TryCatchStatementContext *ctx) = 0;

  virtual void enterThrowStatement(HoocParser::ThrowStatementContext *ctx) = 0;
  virtual void exitThrowStatement(HoocParser::ThrowStatementContext *ctx) = 0;

  virtual void enterBlock(HoocParser::BlockContext *ctx) = 0;
  virtual void exitBlock(HoocParser::BlockContext *ctx) = 0;

  virtual void enterVariableDeclarationStatement(HoocParser::VariableDeclarationStatementContext *ctx) = 0;
  virtual void exitVariableDeclarationStatement(HoocParser::VariableDeclarationStatementContext *ctx) = 0;

  virtual void enterExpressionStatement(HoocParser::ExpressionStatementContext *ctx) = 0;
  virtual void exitExpressionStatement(HoocParser::ExpressionStatementContext *ctx) = 0;

  virtual void enterIfStatement(HoocParser::IfStatementContext *ctx) = 0;
  virtual void exitIfStatement(HoocParser::IfStatementContext *ctx) = 0;

  virtual void enterForStatement(HoocParser::ForStatementContext *ctx) = 0;
  virtual void exitForStatement(HoocParser::ForStatementContext *ctx) = 0;

  virtual void enterWhileStatement(HoocParser::WhileStatementContext *ctx) = 0;
  virtual void exitWhileStatement(HoocParser::WhileStatementContext *ctx) = 0;

  virtual void enterReturnStatement(HoocParser::ReturnStatementContext *ctx) = 0;
  virtual void exitReturnStatement(HoocParser::ReturnStatementContext *ctx) = 0;

  virtual void enterScopeStatement(HoocParser::ScopeStatementContext *ctx) = 0;
  virtual void exitScopeStatement(HoocParser::ScopeStatementContext *ctx) = 0;

  virtual void enterBreakStatement(HoocParser::BreakStatementContext *ctx) = 0;
  virtual void exitBreakStatement(HoocParser::BreakStatementContext *ctx) = 0;

  virtual void enterContinueStatement(HoocParser::ContinueStatementContext *ctx) = 0;
  virtual void exitContinueStatement(HoocParser::ContinueStatementContext *ctx) = 0;

  virtual void enterExpression(HoocParser::ExpressionContext *ctx) = 0;
  virtual void exitExpression(HoocParser::ExpressionContext *ctx) = 0;

  virtual void enterAssignmentExpression(HoocParser::AssignmentExpressionContext *ctx) = 0;
  virtual void exitAssignmentExpression(HoocParser::AssignmentExpressionContext *ctx) = 0;

  virtual void enterCompoundAssignment(HoocParser::CompoundAssignmentContext *ctx) = 0;
  virtual void exitCompoundAssignment(HoocParser::CompoundAssignmentContext *ctx) = 0;

  virtual void enterLogicalOrExpression(HoocParser::LogicalOrExpressionContext *ctx) = 0;
  virtual void exitLogicalOrExpression(HoocParser::LogicalOrExpressionContext *ctx) = 0;

  virtual void enterLogicalAndExpression(HoocParser::LogicalAndExpressionContext *ctx) = 0;
  virtual void exitLogicalAndExpression(HoocParser::LogicalAndExpressionContext *ctx) = 0;

  virtual void enterRelationalExpression(HoocParser::RelationalExpressionContext *ctx) = 0;
  virtual void exitRelationalExpression(HoocParser::RelationalExpressionContext *ctx) = 0;

  virtual void enterAdditiveExpression(HoocParser::AdditiveExpressionContext *ctx) = 0;
  virtual void exitAdditiveExpression(HoocParser::AdditiveExpressionContext *ctx) = 0;

  virtual void enterMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext *ctx) = 0;
  virtual void exitMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext *ctx) = 0;

  virtual void enterUnaryExpression(HoocParser::UnaryExpressionContext *ctx) = 0;
  virtual void exitUnaryExpression(HoocParser::UnaryExpressionContext *ctx) = 0;

  virtual void enterPostfixExpression(HoocParser::PostfixExpressionContext *ctx) = 0;
  virtual void exitPostfixExpression(HoocParser::PostfixExpressionContext *ctx) = 0;

  virtual void enterPostfixSuffix(HoocParser::PostfixSuffixContext *ctx) = 0;
  virtual void exitPostfixSuffix(HoocParser::PostfixSuffixContext *ctx) = 0;

  virtual void enterAugmentedAssignment(HoocParser::AugmentedAssignmentContext *ctx) = 0;
  virtual void exitAugmentedAssignment(HoocParser::AugmentedAssignmentContext *ctx) = 0;

  virtual void enterPrimary(HoocParser::PrimaryContext *ctx) = 0;
  virtual void exitPrimary(HoocParser::PrimaryContext *ctx) = 0;

  virtual void enterNewExpression(HoocParser::NewExpressionContext *ctx) = 0;
  virtual void exitNewExpression(HoocParser::NewExpressionContext *ctx) = 0;

  virtual void enterInterpolatedString(HoocParser::InterpolatedStringContext *ctx) = 0;
  virtual void exitInterpolatedString(HoocParser::InterpolatedStringContext *ctx) = 0;

  virtual void enterArgumentList(HoocParser::ArgumentListContext *ctx) = 0;
  virtual void exitArgumentList(HoocParser::ArgumentListContext *ctx) = 0;

  virtual void enterExpressionList(HoocParser::ExpressionListContext *ctx) = 0;
  virtual void exitExpressionList(HoocParser::ExpressionListContext *ctx) = 0;


};

}  // namespace hooc
