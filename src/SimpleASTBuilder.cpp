#include "SimpleASTBuilder.h"
#include "ast/AST.h"
#include <iostream>
#include <stdexcept>

using namespace hooc;
using namespace hooc::ast;

std::unique_ptr<CompilationUnit> SimpleASTBuilder::buildAST(HoocParser::CompilationUnitContext* ctx) {
    std::vector<std::unique_ptr<ImportStatement>> imports; // Empty for now
    std::vector<std::unique_ptr<Declaration>> declarations;

    // Process declarations
    for (auto declCtx : ctx->declaration()) {
        auto decl = buildDeclaration(declCtx);
        if (decl) {
            declarations.push_back(std::move(decl));
        }
    }

    return std::make_unique<CompilationUnit>(std::move(imports), std::move(declarations));
}

std::unique_ptr<Declaration> SimpleASTBuilder::buildDeclaration(HoocParser::DeclarationContext* ctx) {
    if (ctx->functionDeclaration()) {
        return buildFunctionDeclaration(ctx->functionDeclaration());
    }
    return nullptr;
}

std::unique_ptr<FunctionDeclaration> SimpleASTBuilder::buildFunctionDeclaration(HoocParser::FunctionDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    
    std::vector<std::unique_ptr<Parameter>> parameters;
    if (ctx->parameterList()) {
        for (auto paramCtx : ctx->parameterList()->parameter()) {
            auto param = buildParameter(paramCtx);
            if (param) {
                parameters.push_back(std::move(param));
            }
        }
    }
    
    std::unique_ptr<Type> returnType;
    if (ctx->type()) {
        returnType = buildType(ctx->type());
    }
    
    auto body = buildBlock(ctx->block());
    
    return std::make_unique<FunctionDeclaration>(name, std::move(parameters), 
                                               std::move(returnType), std::move(body));
}

std::unique_ptr<Parameter> SimpleASTBuilder::buildParameter(HoocParser::ParameterContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    auto type = buildType(ctx->type());
    return std::make_unique<Parameter>(std::move(type), name);
}

std::unique_ptr<Type> SimpleASTBuilder::buildType(HoocParser::TypeContext* ctx) {
    if (ctx->unionType()) {
        // For now, just take the first optional type from the union
        auto unionCtx = ctx->unionType();
        if (!unionCtx->optionalType().empty()) {
            auto optCtx = unionCtx->optionalType(0);
            if (optCtx->arrayType()) {
                if (optCtx->arrayType()->baseType()) {
                    return buildBaseType(optCtx->arrayType()->baseType());
                }
            }
        }
    }
    
    // Fallback to unknown type
    return std::make_unique<BaseType>("unknown");
}

std::unique_ptr<BaseType> SimpleASTBuilder::buildBaseType(HoocParser::BaseTypeContext* ctx) {
    if (ctx->primitiveType()) {
        auto primitive = buildPrimitiveType(ctx->primitiveType());
        return std::make_unique<BaseType>(std::move(primitive));
    } else if (ctx->IDENTIFIER()) {
        std::string identifier = ctx->IDENTIFIER()->getText();
        return std::make_unique<BaseType>(identifier);
    }
    
    return std::make_unique<BaseType>("unknown");
}

std::unique_ptr<PrimitiveType> SimpleASTBuilder::buildPrimitiveType(HoocParser::PrimitiveTypeContext* ctx) {
    std::string typeName = ctx->getText();
    PrimitiveTypeKind kind = getPrimitiveTypeKind(typeName);
    return std::make_unique<PrimitiveType>(kind);
}

std::unique_ptr<Block> SimpleASTBuilder::buildBlock(HoocParser::BlockContext* ctx) {
    std::vector<std::unique_ptr<Statement>> statements;
    
    for (auto stmtCtx : ctx->statement()) {
        auto stmt = buildStatement(stmtCtx);
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    return std::make_unique<Block>(std::move(statements));
}

std::unique_ptr<Statement> SimpleASTBuilder::buildStatement(HoocParser::StatementContext* ctx) {
    if (ctx->expressionStatement()) {
        auto expr = buildExpression(ctx->expressionStatement()->expression());
        return std::make_unique<ExpressionStatement>(std::move(expr));
    } else if (ctx->returnStatement()) {
        std::unique_ptr<Expression> expr;
        if (ctx->returnStatement()->expression()) {
            expr = buildExpression(ctx->returnStatement()->expression());
        }
        return std::make_unique<ReturnStatement>(std::move(expr));
    } else if (ctx->block()) {
        return buildBlock(ctx->block());
    }
    
    return nullptr;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildExpression(HoocParser::ExpressionContext* ctx) {
    if (ctx->primary()) {
        return buildPrimary(ctx->primary());
    }
    
    // For now, just handle primary expressions
    // TODO: Add binary operations, function calls, etc.
    return nullptr;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildPrimary(HoocParser::PrimaryContext* ctx) {
    if (ctx->IDENTIFIER()) {
        auto identifier = std::make_unique<Identifier>(ctx->IDENTIFIER()->getText());
        return std::make_unique<PrimaryExpression>(std::move(identifier));
    } else if (ctx->INTEGER_LITERAL()) {
        int value = getIntValue(ctx->INTEGER_LITERAL());
        auto intLiteral = std::make_unique<IntegerLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(intLiteral));
    }
    
    return nullptr;
}

// Helper methods
PrimitiveTypeKind SimpleASTBuilder::getPrimitiveTypeKind(const std::string& typeName) {
    if (typeName == "int" || typeName == "int64") return PrimitiveTypeKind::INT64;
    if (typeName == "double") return PrimitiveTypeKind::DOUBLE;
    if (typeName == "f64") return PrimitiveTypeKind::F64;
    if (typeName == "bool") return PrimitiveTypeKind::BOOL;
    if (typeName == "char") return PrimitiveTypeKind::CHAR;
    if (typeName == "string") return PrimitiveTypeKind::STRING;
    if (typeName == "byte") return PrimitiveTypeKind::BYTE;
    if (typeName == "uint8") return PrimitiveTypeKind::UINT8;
    return PrimitiveTypeKind::INT64; // Default fallback
}

std::string SimpleASTBuilder::getStringValue(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    if (text.length() >= 2 && text.front() == '"' && text.back() == '"') {
        return text.substr(1, text.length() - 2);
    }
    return text;
}

int SimpleASTBuilder::getIntValue(antlr4::tree::TerminalNode* node) {
    try {
        return std::stoi(node->getText());
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse integer: " << node->getText() << std::endl;
        return 0;
    }
}

double SimpleASTBuilder::getDoubleValue(antlr4::tree::TerminalNode* node) {
    try {
        return std::stod(node->getText());
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse double: " << node->getText() << std::endl;
        return 0.0;
    }
}

char SimpleASTBuilder::getCharValue(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    if (text.length() >= 3 && text.front() == '\'' && text.back() == '\'') {
        return text[1];
    }
    return '\0';
}

bool SimpleASTBuilder::getBoolValue(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    return text == "true";
}