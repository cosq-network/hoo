
// Generated from Hooc.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "HoocListener.h"


namespace hooc {

/**
 * This class provides an empty implementation of HoocListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  HoocBaseListener : public HoocListener {
public:

  virtual void enterCompilationUnit(HoocParser::CompilationUnitContext * /*ctx*/) override { }
  virtual void exitCompilationUnit(HoocParser::CompilationUnitContext * /*ctx*/) override { }

  virtual void enterBasicImport(HoocParser::BasicImportContext * /*ctx*/) override { }
  virtual void exitBasicImport(HoocParser::BasicImportContext * /*ctx*/) override { }

  virtual void enterFromImport(HoocParser::FromImportContext * /*ctx*/) override { }
  virtual void exitFromImport(HoocParser::FromImportContext * /*ctx*/) override { }

  virtual void enterModulePath(HoocParser::ModulePathContext * /*ctx*/) override { }
  virtual void exitModulePath(HoocParser::ModulePathContext * /*ctx*/) override { }

  virtual void enterQualifiedIdentifier(HoocParser::QualifiedIdentifierContext * /*ctx*/) override { }
  virtual void exitQualifiedIdentifier(HoocParser::QualifiedIdentifierContext * /*ctx*/) override { }

  virtual void enterImportItem(HoocParser::ImportItemContext * /*ctx*/) override { }
  virtual void exitImportItem(HoocParser::ImportItemContext * /*ctx*/) override { }

  virtual void enterDeclaration(HoocParser::DeclarationContext * /*ctx*/) override { }
  virtual void exitDeclaration(HoocParser::DeclarationContext * /*ctx*/) override { }

  virtual void enterFunctionDeclaration(HoocParser::FunctionDeclarationContext * /*ctx*/) override { }
  virtual void exitFunctionDeclaration(HoocParser::FunctionDeclarationContext * /*ctx*/) override { }

  virtual void enterParameterList(HoocParser::ParameterListContext * /*ctx*/) override { }
  virtual void exitParameterList(HoocParser::ParameterListContext * /*ctx*/) override { }

  virtual void enterParameter(HoocParser::ParameterContext * /*ctx*/) override { }
  virtual void exitParameter(HoocParser::ParameterContext * /*ctx*/) override { }

  virtual void enterClassDeclaration(HoocParser::ClassDeclarationContext * /*ctx*/) override { }
  virtual void exitClassDeclaration(HoocParser::ClassDeclarationContext * /*ctx*/) override { }

  virtual void enterClassModifier(HoocParser::ClassModifierContext * /*ctx*/) override { }
  virtual void exitClassModifier(HoocParser::ClassModifierContext * /*ctx*/) override { }

  virtual void enterClassBody(HoocParser::ClassBodyContext * /*ctx*/) override { }
  virtual void exitClassBody(HoocParser::ClassBodyContext * /*ctx*/) override { }

  virtual void enterClassMember(HoocParser::ClassMemberContext * /*ctx*/) override { }
  virtual void exitClassMember(HoocParser::ClassMemberContext * /*ctx*/) override { }

  virtual void enterConstructorDeclaration(HoocParser::ConstructorDeclarationContext * /*ctx*/) override { }
  virtual void exitConstructorDeclaration(HoocParser::ConstructorDeclarationContext * /*ctx*/) override { }

  virtual void enterFunctionModifier(HoocParser::FunctionModifierContext * /*ctx*/) override { }
  virtual void exitFunctionModifier(HoocParser::FunctionModifierContext * /*ctx*/) override { }

  virtual void enterVariableDeclaration(HoocParser::VariableDeclarationContext * /*ctx*/) override { }
  virtual void exitVariableDeclaration(HoocParser::VariableDeclarationContext * /*ctx*/) override { }

  virtual void enterConstantDeclaration(HoocParser::ConstantDeclarationContext * /*ctx*/) override { }
  virtual void exitConstantDeclaration(HoocParser::ConstantDeclarationContext * /*ctx*/) override { }

  virtual void enterType(HoocParser::TypeContext * /*ctx*/) override { }
  virtual void exitType(HoocParser::TypeContext * /*ctx*/) override { }

  virtual void enterOptionalType(HoocParser::OptionalTypeContext * /*ctx*/) override { }
  virtual void exitOptionalType(HoocParser::OptionalTypeContext * /*ctx*/) override { }

  virtual void enterArrayType(HoocParser::ArrayTypeContext * /*ctx*/) override { }
  virtual void exitArrayType(HoocParser::ArrayTypeContext * /*ctx*/) override { }

  virtual void enterBaseType(HoocParser::BaseTypeContext * /*ctx*/) override { }
  virtual void exitBaseType(HoocParser::BaseTypeContext * /*ctx*/) override { }

  virtual void enterMapType(HoocParser::MapTypeContext * /*ctx*/) override { }
  virtual void exitMapType(HoocParser::MapTypeContext * /*ctx*/) override { }

  virtual void enterMapKeyType(HoocParser::MapKeyTypeContext * /*ctx*/) override { }
  virtual void exitMapKeyType(HoocParser::MapKeyTypeContext * /*ctx*/) override { }

  virtual void enterPrimitiveType(HoocParser::PrimitiveTypeContext * /*ctx*/) override { }
  virtual void exitPrimitiveType(HoocParser::PrimitiveTypeContext * /*ctx*/) override { }

  virtual void enterFfiDeclaration(HoocParser::FfiDeclarationContext * /*ctx*/) override { }
  virtual void exitFfiDeclaration(HoocParser::FfiDeclarationContext * /*ctx*/) override { }

  virtual void enterFfiImportDeclaration(HoocParser::FfiImportDeclarationContext * /*ctx*/) override { }
  virtual void exitFfiImportDeclaration(HoocParser::FfiImportDeclarationContext * /*ctx*/) override { }

  virtual void enterFfiLinkDeclaration(HoocParser::FfiLinkDeclarationContext * /*ctx*/) override { }
  virtual void exitFfiLinkDeclaration(HoocParser::FfiLinkDeclarationContext * /*ctx*/) override { }

  virtual void enterFfiNativeFunction(HoocParser::FfiNativeFunctionContext * /*ctx*/) override { }
  virtual void exitFfiNativeFunction(HoocParser::FfiNativeFunctionContext * /*ctx*/) override { }

  virtual void enterFfiNativeDeclaration(HoocParser::FfiNativeDeclarationContext * /*ctx*/) override { }
  virtual void exitFfiNativeDeclaration(HoocParser::FfiNativeDeclarationContext * /*ctx*/) override { }

  virtual void enterFfiParameterList(HoocParser::FfiParameterListContext * /*ctx*/) override { }
  virtual void exitFfiParameterList(HoocParser::FfiParameterListContext * /*ctx*/) override { }

  virtual void enterFfiParameter(HoocParser::FfiParameterContext * /*ctx*/) override { }
  virtual void exitFfiParameter(HoocParser::FfiParameterContext * /*ctx*/) override { }

  virtual void enterFfiType(HoocParser::FfiTypeContext * /*ctx*/) override { }
  virtual void exitFfiType(HoocParser::FfiTypeContext * /*ctx*/) override { }

  virtual void enterLibrarySearchPaths(HoocParser::LibrarySearchPathsContext * /*ctx*/) override { }
  virtual void exitLibrarySearchPaths(HoocParser::LibrarySearchPathsContext * /*ctx*/) override { }

  virtual void enterVersionRange(HoocParser::VersionRangeContext * /*ctx*/) override { }
  virtual void exitVersionRange(HoocParser::VersionRangeContext * /*ctx*/) override { }

  virtual void enterStatement(HoocParser::StatementContext * /*ctx*/) override { }
  virtual void exitStatement(HoocParser::StatementContext * /*ctx*/) override { }

  virtual void enterTryCatchStatement(HoocParser::TryCatchStatementContext * /*ctx*/) override { }
  virtual void exitTryCatchStatement(HoocParser::TryCatchStatementContext * /*ctx*/) override { }

  virtual void enterThrowStatement(HoocParser::ThrowStatementContext * /*ctx*/) override { }
  virtual void exitThrowStatement(HoocParser::ThrowStatementContext * /*ctx*/) override { }

  virtual void enterBlock(HoocParser::BlockContext * /*ctx*/) override { }
  virtual void exitBlock(HoocParser::BlockContext * /*ctx*/) override { }

  virtual void enterVariableDeclarationStatement(HoocParser::VariableDeclarationStatementContext * /*ctx*/) override { }
  virtual void exitVariableDeclarationStatement(HoocParser::VariableDeclarationStatementContext * /*ctx*/) override { }

  virtual void enterExpressionStatement(HoocParser::ExpressionStatementContext * /*ctx*/) override { }
  virtual void exitExpressionStatement(HoocParser::ExpressionStatementContext * /*ctx*/) override { }

  virtual void enterIfStatement(HoocParser::IfStatementContext * /*ctx*/) override { }
  virtual void exitIfStatement(HoocParser::IfStatementContext * /*ctx*/) override { }

  virtual void enterForStatement(HoocParser::ForStatementContext * /*ctx*/) override { }
  virtual void exitForStatement(HoocParser::ForStatementContext * /*ctx*/) override { }

  virtual void enterWhileStatement(HoocParser::WhileStatementContext * /*ctx*/) override { }
  virtual void exitWhileStatement(HoocParser::WhileStatementContext * /*ctx*/) override { }

  virtual void enterReturnStatement(HoocParser::ReturnStatementContext * /*ctx*/) override { }
  virtual void exitReturnStatement(HoocParser::ReturnStatementContext * /*ctx*/) override { }

  virtual void enterScopeStatement(HoocParser::ScopeStatementContext * /*ctx*/) override { }
  virtual void exitScopeStatement(HoocParser::ScopeStatementContext * /*ctx*/) override { }

  virtual void enterBreakStatement(HoocParser::BreakStatementContext * /*ctx*/) override { }
  virtual void exitBreakStatement(HoocParser::BreakStatementContext * /*ctx*/) override { }

  virtual void enterContinueStatement(HoocParser::ContinueStatementContext * /*ctx*/) override { }
  virtual void exitContinueStatement(HoocParser::ContinueStatementContext * /*ctx*/) override { }

  virtual void enterExpression(HoocParser::ExpressionContext * /*ctx*/) override { }
  virtual void exitExpression(HoocParser::ExpressionContext * /*ctx*/) override { }

  virtual void enterAssignmentExpression(HoocParser::AssignmentExpressionContext * /*ctx*/) override { }
  virtual void exitAssignmentExpression(HoocParser::AssignmentExpressionContext * /*ctx*/) override { }

  virtual void enterCompoundAssignment(HoocParser::CompoundAssignmentContext * /*ctx*/) override { }
  virtual void exitCompoundAssignment(HoocParser::CompoundAssignmentContext * /*ctx*/) override { }

  virtual void enterLogicalOrExpression(HoocParser::LogicalOrExpressionContext * /*ctx*/) override { }
  virtual void exitLogicalOrExpression(HoocParser::LogicalOrExpressionContext * /*ctx*/) override { }

  virtual void enterLogicalAndExpression(HoocParser::LogicalAndExpressionContext * /*ctx*/) override { }
  virtual void exitLogicalAndExpression(HoocParser::LogicalAndExpressionContext * /*ctx*/) override { }

  virtual void enterRelationalExpression(HoocParser::RelationalExpressionContext * /*ctx*/) override { }
  virtual void exitRelationalExpression(HoocParser::RelationalExpressionContext * /*ctx*/) override { }

  virtual void enterAdditiveExpression(HoocParser::AdditiveExpressionContext * /*ctx*/) override { }
  virtual void exitAdditiveExpression(HoocParser::AdditiveExpressionContext * /*ctx*/) override { }

  virtual void enterMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext * /*ctx*/) override { }
  virtual void exitMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext * /*ctx*/) override { }

  virtual void enterUnaryExpression(HoocParser::UnaryExpressionContext * /*ctx*/) override { }
  virtual void exitUnaryExpression(HoocParser::UnaryExpressionContext * /*ctx*/) override { }

  virtual void enterPostfixExpression(HoocParser::PostfixExpressionContext * /*ctx*/) override { }
  virtual void exitPostfixExpression(HoocParser::PostfixExpressionContext * /*ctx*/) override { }

  virtual void enterPostfixSuffix(HoocParser::PostfixSuffixContext * /*ctx*/) override { }
  virtual void exitPostfixSuffix(HoocParser::PostfixSuffixContext * /*ctx*/) override { }

  virtual void enterAugmentedAssignment(HoocParser::AugmentedAssignmentContext * /*ctx*/) override { }
  virtual void exitAugmentedAssignment(HoocParser::AugmentedAssignmentContext * /*ctx*/) override { }

  virtual void enterPrimary(HoocParser::PrimaryContext * /*ctx*/) override { }
  virtual void exitPrimary(HoocParser::PrimaryContext * /*ctx*/) override { }

  virtual void enterNewExpression(HoocParser::NewExpressionContext * /*ctx*/) override { }
  virtual void exitNewExpression(HoocParser::NewExpressionContext * /*ctx*/) override { }

  virtual void enterInterpolatedString(HoocParser::InterpolatedStringContext * /*ctx*/) override { }
  virtual void exitInterpolatedString(HoocParser::InterpolatedStringContext * /*ctx*/) override { }

  virtual void enterArgumentList(HoocParser::ArgumentListContext * /*ctx*/) override { }
  virtual void exitArgumentList(HoocParser::ArgumentListContext * /*ctx*/) override { }

  virtual void enterExpressionList(HoocParser::ExpressionListContext * /*ctx*/) override { }
  virtual void exitExpressionList(HoocParser::ExpressionListContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

}  // namespace hooc
