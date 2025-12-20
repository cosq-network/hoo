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
    std::any visitNamedImports(HoocParser::NamedImportsContext* ctx) override;
    std::any visitNamespaceImport(HoocParser::NamespaceImportContext* ctx) override;
    std::any visitSideEffectImport(HoocParser::SideEffectImportContext* ctx) override;
    std::any visitImportItem(HoocParser::ImportItemContext* ctx) override;

    // Declarations
    std::any visitDeclaration(HoocParser::DeclarationContext* ctx) override;
    std::any visitFunctionDeclaration(HoocParser::FunctionDeclarationContext* ctx) override;
    std::any visitParameterList(HoocParser::ParameterListContext* ctx) override;
    std::any visitParameter(HoocParser::ParameterContext* ctx) override;

    // Class-related (simplified - only what exists in grammar)
    std::any visitClassDeclaration(HoocParser::ClassDeclarationContext* ctx) override;
    std::any visitClassModifier(HoocParser::ClassModifierContext* ctx) override;
    std::any visitPrimaryConstructor(HoocParser::PrimaryConstructorContext* ctx) override;
    std::any visitClassBody(HoocParser::ClassBodyContext* ctx) override;
    std::any visitClassMember(HoocParser::ClassMemberContext* ctx) override;
    std::any visitEventDeclaration(HoocParser::EventDeclarationContext* ctx) override;
    std::any visitInterfaceList(HoocParser::InterfaceListContext* ctx) override;

    // Interface-related
    std::any visitInterfaceDeclaration(HoocParser::InterfaceDeclarationContext* ctx) override;
    std::any visitInterfaceMember(HoocParser::InterfaceMemberContext* ctx) override;
    std::any visitFunctionSignature(HoocParser::FunctionSignatureContext* ctx) override;

    // Variable declarations
    std::any visitVariableDeclaration(HoocParser::VariableDeclarationContext* ctx) override;

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

    // Expressions (only what exists in simplified grammar)
    std::any visitExpression(HoocParser::ExpressionContext* ctx) override;
    std::any visitPrimary(HoocParser::PrimaryContext* ctx) override;

    // Utility
    std::any visitInterpolatedString(HoocParser::InterpolatedStringContext* ctx) override;
    std::any visitArgumentList(HoocParser::ArgumentListContext* ctx) override;
    std::any visitExpressionList(HoocParser::ExpressionListContext* ctx) override;

private:
    // Helper methods
    ast::ClassModifier parseClassModifier(const std::string& modifier);
    ast::PrimitiveTypeKind parsePrimitiveType(const std::string& type);
    std::string getStringValue(antlr4::tree::TerminalNode* node);
    int64_t getIntegerValue(antlr4::tree::TerminalNode* node);
    double getDoubleValue(antlr4::tree::TerminalNode* node);
    char getCharValue(antlr4::tree::TerminalNode* node);
    bool getBoolValue(antlr4::tree::TerminalNode* node);

    // Template helpers for casting std::any to AST types
    template<typename T>
    std::unique_ptr<T> cast(std::any& value) {
        try {
            return std::move(std::any_cast<std::unique_ptr<T>&>(value));
        } catch (const std::bad_any_cast& e) {
            std::cerr << "Failed to cast AST node: " << e.what() << std::endl;
            return nullptr;
        }
    }

    template<typename T>
    std::vector<std::unique_ptr<T>> castVector(std::any& value) {
        try {
            return std::move(std::any_cast<std::vector<std::unique_ptr<T>>&>(value));
        } catch (const std::bad_any_cast& e) {
            std::cerr << "Failed to cast AST vector: " << e.what() << std::endl;
            return std::vector<std::unique_ptr<T>>();
        }
    }
};

} // namespace hooc