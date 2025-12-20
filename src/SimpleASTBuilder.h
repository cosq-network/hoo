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
    std::unique_ptr<ast::Type> buildType(HoocParser::TypeContext* ctx);
    std::unique_ptr<ast::BaseType> buildBaseType(HoocParser::BaseTypeContext* ctx);
    std::unique_ptr<ast::PrimitiveType> buildPrimitiveType(HoocParser::PrimitiveTypeContext* ctx);
    std::unique_ptr<ast::Statement> buildStatement(HoocParser::StatementContext* ctx);
    std::unique_ptr<ast::Block> buildBlock(HoocParser::BlockContext* ctx);
    std::unique_ptr<ast::Expression> buildExpression(HoocParser::ExpressionContext* ctx);
    std::unique_ptr<ast::Expression> buildPrimary(HoocParser::PrimaryContext* ctx);
    std::unique_ptr<ast::Parameter> buildParameter(HoocParser::ParameterContext* ctx);
    
    // Helper methods
    ast::PrimitiveTypeKind getPrimitiveTypeKind(const std::string& typeName);
    std::string getStringValue(antlr4::tree::TerminalNode* node);
    int getIntValue(antlr4::tree::TerminalNode* node);
    double getDoubleValue(antlr4::tree::TerminalNode* node);
    char getCharValue(antlr4::tree::TerminalNode* node);
    bool getBoolValue(antlr4::tree::TerminalNode* node);
};

} // namespace hooc