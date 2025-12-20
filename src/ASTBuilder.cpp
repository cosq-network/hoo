#include "ASTBuilder.h"
#include <iostream>
#include <sstream>

using namespace hooc;
using namespace hooc::ast;

std::unique_ptr<CompilationUnit> ASTBuilder::buildAST(HoocParser::CompilationUnitContext* ctx) {
    auto result = visitCompilationUnit(ctx);
    return cast<CompilationUnit>(result);
}

std::any ASTBuilder::visitCompilationUnit(HoocParser::CompilationUnitContext* ctx) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    std::vector<std::unique_ptr<Declaration>> declarations;

    // Process declarations (imports are currently disabled in simplified grammar)
    for (auto declCtx : ctx->declaration()) {
        auto declResult = visit(declCtx);
        auto decl = cast<Declaration>(declResult);
        if (decl) {
            declarations.push_back(std::move(decl));
        }
    }

    return std::make_unique<CompilationUnit>(std::move(imports), std::move(declarations));
}

std::any ASTBuilder::visitNamedImports(HoocParser::NamedImportsContext* ctx) {
    // Currently disabled in simplified grammar
    return visitChildren(ctx);
}

std::any ASTBuilder::visitNamespaceImport(HoocParser::NamespaceImportContext* ctx) {
    // Currently disabled in simplified grammar
    return visitChildren(ctx);
}

std::any ASTBuilder::visitSideEffectImport(HoocParser::SideEffectImportContext* ctx) {
    // Currently disabled in simplified grammar
    return visitChildren(ctx);
}

std::any ASTBuilder::visitImportItem(HoocParser::ImportItemContext* ctx) {
    // Currently disabled in simplified grammar
    return visitChildren(ctx);
}

std::any ASTBuilder::visitDeclaration(HoocParser::DeclarationContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitFunctionDeclaration(HoocParser::FunctionDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    
    std::vector<std::unique_ptr<Parameter>> parameters;
    if (ctx->parameterList()) {
        auto paramResult = visit(ctx->parameterList());
        parameters = castVector<Parameter>(paramResult);
    }
    
    std::unique_ptr<Type> returnType;
    if (ctx->type()) {
        auto retResult = visit(ctx->type());
        returnType = cast<Type>(retResult);
    }
    
    auto bodyResult = visit(ctx->block());
    auto body = cast<Block>(bodyResult);
    
    return std::unique_ptr<Declaration>(
        std::make_unique<FunctionDeclaration>(name, std::move(parameters), 
                                            std::move(returnType), std::move(body)));
}

std::any ASTBuilder::visitParameterList(HoocParser::ParameterListContext* ctx) {
    std::vector<std::unique_ptr<Parameter>> parameters;
    for (auto paramCtx : ctx->parameter()) {
        auto paramResult = visit(paramCtx);
        auto param = cast<Parameter>(paramResult);
        if (param) {
            parameters.push_back(std::move(param));
        }
    }
    return parameters;
}

std::any ASTBuilder::visitParameter(HoocParser::ParameterContext* ctx) {
    auto typeResult = visit(ctx->type());
    auto type = cast<Type>(typeResult);
    std::string name = ctx->IDENTIFIER()->getText();
    return std::make_unique<Parameter>(std::move(type), name);
}

// Class-related methods (simplified)
std::any ASTBuilder::visitClassDeclaration(HoocParser::ClassDeclarationContext* ctx) {
    // Simplified implementation - just return basic structure
    return visitChildren(ctx);
}

std::any ASTBuilder::visitClassModifier(HoocParser::ClassModifierContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitPrimaryConstructor(HoocParser::PrimaryConstructorContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitClassBody(HoocParser::ClassBodyContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitClassMember(HoocParser::ClassMemberContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitEventDeclaration(HoocParser::EventDeclarationContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitInterfaceList(HoocParser::InterfaceListContext* ctx) {
    return visitChildren(ctx);
}

// Interface methods (simplified)
std::any ASTBuilder::visitInterfaceDeclaration(HoocParser::InterfaceDeclarationContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitInterfaceMember(HoocParser::InterfaceMemberContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitFunctionSignature(HoocParser::FunctionSignatureContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitVariableDeclaration(HoocParser::VariableDeclarationContext* ctx) {
    return visitChildren(ctx);
}

// Type methods
std::any ASTBuilder::visitType(HoocParser::TypeContext* ctx) {
    return visit(ctx->unionType());
}

std::any ASTBuilder::visitUnionType(HoocParser::UnionTypeContext* ctx) {
    std::vector<std::unique_ptr<OptionalType>> types;
    for (auto optCtx : ctx->optionalType()) {
        auto optResult = visit(optCtx);
        auto opt = cast<OptionalType>(optResult);
        if (opt) {
            types.push_back(std::move(opt));
        }
    }
    return std::unique_ptr<Type>(std::make_unique<UnionType>(std::move(types)));
}

std::any ASTBuilder::visitOptionalType(HoocParser::OptionalTypeContext* ctx) {
    auto arrayResult = visit(ctx->arrayType());
    auto arrayType = cast<ArrayType>(arrayResult);
    bool isOptional = ctx->QUESTION() != nullptr;
    return std::make_unique<OptionalType>(std::move(arrayType), isOptional);
}

std::any ASTBuilder::visitArrayType(HoocParser::ArrayTypeContext* ctx) {
    auto baseResult = visit(ctx->baseType());
    auto baseType = cast<BaseType>(baseResult);
    
    std::vector<std::unique_ptr<Expression>> dimensions;
    // Simplified: just count the brackets for now
    for (size_t i = 0; i < ctx->LBRACKET().size(); i++) {
        dimensions.push_back(nullptr); // Empty dimension []
    }
    
    return std::make_unique<ArrayType>(std::move(baseType), std::move(dimensions));
}

std::any ASTBuilder::visitBaseType(HoocParser::BaseTypeContext* ctx) {
    if (ctx->primitiveType()) {
        auto primResult = visit(ctx->primitiveType());
        auto primitive = cast<PrimitiveType>(primResult);
        return std::make_unique<BaseType>(std::move(primitive));
    } else {
        std::string identifier = ctx->IDENTIFIER()->getText();
        return std::make_unique<BaseType>(identifier);
    }
}

std::any ASTBuilder::visitPrimitiveType(HoocParser::PrimitiveTypeContext* ctx) {
    PrimitiveTypeKind kind = parsePrimitiveType(ctx->getText());
    return std::make_unique<PrimitiveType>(kind);
}

// Statement methods
std::any ASTBuilder::visitStatement(HoocParser::StatementContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitBlock(HoocParser::BlockContext* ctx) {
    std::vector<std::unique_ptr<Statement>> statements;
    for (auto stmtCtx : ctx->statement()) {
        auto stmtResult = visit(stmtCtx);
        auto stmt = cast<Statement>(stmtResult);
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    return std::unique_ptr<Statement>(std::make_unique<Block>(std::move(statements)));
}

std::any ASTBuilder::visitExpressionStatement(HoocParser::ExpressionStatementContext* ctx) {
    auto exprResult = visit(ctx->expression());
    auto expr = cast<Expression>(exprResult);
    return std::unique_ptr<Statement>(std::make_unique<ExpressionStatement>(std::move(expr)));
}

std::any ASTBuilder::visitIfStatement(HoocParser::IfStatementContext* ctx) {
    // Simplified - not implemented in current grammar
    return visitChildren(ctx);
}

std::any ASTBuilder::visitForInStatement(HoocParser::ForInStatementContext* ctx) {
    // Simplified - not implemented in current grammar
    return visitChildren(ctx);
}

std::any ASTBuilder::visitForRangeStatement(HoocParser::ForRangeStatementContext* ctx) {
    // Simplified - not implemented in current grammar
    return visitChildren(ctx);
}

std::any ASTBuilder::visitWhileStatement(HoocParser::WhileStatementContext* ctx) {
    // Simplified - not implemented in current grammar
    return visitChildren(ctx);
}

std::any ASTBuilder::visitReturnStatement(HoocParser::ReturnStatementContext* ctx) {
    std::unique_ptr<Expression> expr;
    if (ctx->expression()) {
        auto exprResult = visit(ctx->expression());
        expr = cast<Expression>(exprResult);
    }
    
    return std::unique_ptr<Statement>(std::make_unique<ReturnStatement>(std::move(expr)));
}

std::any ASTBuilder::visitScopeStatement(HoocParser::ScopeStatementContext* ctx) {
    return visitChildren(ctx);
}

// Expression methods (simplified for our ultra-basic grammar)
std::any ASTBuilder::visitExpression(HoocParser::ExpressionContext* ctx) {
    return visit(ctx->primary());
}

std::any ASTBuilder::visitPrimary(HoocParser::PrimaryContext* ctx) {
    if (ctx->IDENTIFIER()) {
        auto identifier = std::make_unique<Identifier>(ctx->IDENTIFIER()->getText());
        return std::unique_ptr<Expression>(
            std::make_unique<PrimaryExpression>(std::unique_ptr<ASTNode>(std::move(identifier))));
    } else if (ctx->INTEGER_LITERAL()) {
        auto intLiteral = std::make_unique<IntegerLiteral>(getIntegerValue(ctx->INTEGER_LITERAL()));
        return std::unique_ptr<Expression>(
            std::make_unique<PrimaryExpression>(std::unique_ptr<ASTNode>(std::move(intLiteral))));
    }
    
    return visitChildren(ctx);
}

// Utility methods
std::any ASTBuilder::visitInterpolatedString(HoocParser::InterpolatedStringContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitArgumentList(HoocParser::ArgumentListContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitExpressionList(HoocParser::ExpressionListContext* ctx) {
    return visitChildren(ctx);
}

// Helper method implementations
ClassModifier ASTBuilder::parseClassModifier(const std::string& modifier) {
    if (modifier == "singleton") return ClassModifier::SINGLETON;
    if (modifier == "immutable") return ClassModifier::IMMUTABLE;
    if (modifier == "factory") return ClassModifier::FACTORY;
    if (modifier == "observable") return ClassModifier::OBSERVABLE;
    if (modifier == "service") return ClassModifier::SERVICE;
    if (modifier == "strategy") return ClassModifier::STRATEGY;
    if (modifier == "actor") return ClassModifier::ACTOR;
    if (modifier == "final") return ClassModifier::FINAL;
    throw std::runtime_error("Unknown class modifier: " + modifier);
}

PrimitiveTypeKind ASTBuilder::parsePrimitiveType(const std::string& type) {
    if (type == "byte") return PrimitiveTypeKind::BYTE;
    if (type == "uint8") return PrimitiveTypeKind::UINT8;
    if (type == "int64") return PrimitiveTypeKind::INT64;
    if (type == "double") return PrimitiveTypeKind::DOUBLE;
    if (type == "f64") return PrimitiveTypeKind::F64;
    if (type == "bool") return PrimitiveTypeKind::BOOL;
    if (type == "char") return PrimitiveTypeKind::CHAR;
    if (type == "string") return PrimitiveTypeKind::STRING;
    throw std::runtime_error("Unknown primitive type: " + type);
}

std::string ASTBuilder::getStringValue(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    // Remove quotes and handle escape sequences
    if (text.length() >= 2 && text[0] == '"' && text.back() == '"') {
        return text.substr(1, text.length() - 2);
    }
    return text;
}

int64_t ASTBuilder::getIntegerValue(antlr4::tree::TerminalNode* node) {
    return std::stoll(node->getText());
}

double ASTBuilder::getDoubleValue(antlr4::tree::TerminalNode* node) {
    return std::stod(node->getText());
}

char ASTBuilder::getCharValue(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    if (text.length() >= 3 && text[0] == '\'' && text.back() == '\'') {
        return text[1]; // Simple char extraction
    }
    return '\0';
}

bool ASTBuilder::getBoolValue(antlr4::tree::TerminalNode* node) {
    return node->getText() == "true";
}