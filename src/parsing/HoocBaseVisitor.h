
// Generated from Hooc.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "HoocVisitor.h"


namespace hooc {

/**
 * This class provides an empty implementation of HoocVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  HoocBaseVisitor : public HoocVisitor {
public:

  virtual std::any visitCompilationUnit(HoocParser::CompilationUnitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBasicImport(HoocParser::BasicImportContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFromImport(HoocParser::FromImportContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModulePath(HoocParser::ModulePathContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQualifiedIdentifier(HoocParser::QualifiedIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportItem(HoocParser::ImportItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDeclaration(HoocParser::DeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionDeclaration(HoocParser::FunctionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParameterList(HoocParser::ParameterListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParameter(HoocParser::ParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassDeclaration(HoocParser::ClassDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassModifier(HoocParser::ClassModifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassBody(HoocParser::ClassBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassMember(HoocParser::ClassMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstructorDeclaration(HoocParser::ConstructorDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionModifier(HoocParser::FunctionModifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVariableDeclaration(HoocParser::VariableDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstantDeclaration(HoocParser::ConstantDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitType(HoocParser::TypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOptionalType(HoocParser::OptionalTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArrayType(HoocParser::ArrayTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBaseType(HoocParser::BaseTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMapType(HoocParser::MapTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMapKeyType(HoocParser::MapKeyTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimitiveType(HoocParser::PrimitiveTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFfiDeclaration(HoocParser::FfiDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFfiImportDeclaration(HoocParser::FfiImportDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFfiLinkDeclaration(HoocParser::FfiLinkDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFfiNativeFunction(HoocParser::FfiNativeFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFfiNativeDeclaration(HoocParser::FfiNativeDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFfiParameterList(HoocParser::FfiParameterListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFfiParameter(HoocParser::FfiParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFfiType(HoocParser::FfiTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLibrarySearchPaths(HoocParser::LibrarySearchPathsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVersionRange(HoocParser::VersionRangeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatement(HoocParser::StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTryCatchStatement(HoocParser::TryCatchStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitThrowStatement(HoocParser::ThrowStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlock(HoocParser::BlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVariableDeclarationStatement(HoocParser::VariableDeclarationStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionStatement(HoocParser::ExpressionStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStatement(HoocParser::IfStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitForStatement(HoocParser::ForStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhileStatement(HoocParser::WhileStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnStatement(HoocParser::ReturnStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitScopeStatement(HoocParser::ScopeStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBreakStatement(HoocParser::BreakStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitContinueStatement(HoocParser::ContinueStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpression(HoocParser::ExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignmentExpression(HoocParser::AssignmentExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCompoundAssignment(HoocParser::CompoundAssignmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicalOrExpression(HoocParser::LogicalOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicalAndExpression(HoocParser::LogicalAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelationalExpression(HoocParser::RelationalExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdditiveExpression(HoocParser::AdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpression(HoocParser::UnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPostfixExpression(HoocParser::PostfixExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPostfixSuffix(HoocParser::PostfixSuffixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAugmentedAssignment(HoocParser::AugmentedAssignmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimary(HoocParser::PrimaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNewExpression(HoocParser::NewExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInterpolatedString(HoocParser::InterpolatedStringContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArgumentList(HoocParser::ArgumentListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionList(HoocParser::ExpressionListContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace hooc
