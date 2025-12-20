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

    // Process imports
    for (auto importCtx : ctx->importStatement()) {
        auto import = cast<ImportStatement>(visit(importCtx));
        if (import) {
            imports.push_back(std::move(import));
        }
    }

    // Process declarations
    for (auto declCtx : ctx->declaration()) {
        auto decl = cast<Declaration>(visit(declCtx));
        if (decl) {
            declarations.push_back(std::move(decl));
        }
    }

    return std::make_unique<CompilationUnit>(std::move(imports), std::move(declarations));
}

std::any ASTBuilder::visitImportStatement(HoocParser::ImportStatementContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitNamedImports(HoocParser::NamedImportsContext* ctx) {
    std::vector<std::unique_ptr<ImportItem>> items;
    
    for (auto itemCtx : ctx->importItem()) {
        auto item = cast<ImportItem>(visit(itemCtx));
        if (item) {
            items.push_back(std::move(item));
        }
    }
    
    std::string module = getStringValue(ctx->STRING_LITERAL());
    return std::unique_ptr<ImportStatement>(
        std::make_unique<NamedImports>(std::move(items), module));
}

std::any ASTBuilder::visitNamespaceImport(HoocParser::NamespaceImportContext* ctx) {
    std::string alias = ctx->IDENTIFIER()->getText();
    std::string module = getStringValue(ctx->STRING_LITERAL());
    return std::unique_ptr<ImportStatement>(
        std::make_unique<NamespaceImport>(alias, module));
}

std::any ASTBuilder::visitSideEffectImport(HoocParser::SideEffectImportContext* ctx) {
    std::string module = getStringValue(ctx->STRING_LITERAL());
    return std::unique_ptr<ImportStatement>(
        std::make_unique<SideEffectImport>(module));
}

std::any ASTBuilder::visitImportItem(HoocParser::ImportItemContext* ctx) {
    std::string name = ctx->IDENTIFIER(0)->getText();
    std::string alias = "";
    
    if (ctx->IDENTIFIER().size() > 1) {
        alias = ctx->IDENTIFIER(1)->getText();
    }
    
    return std::make_unique<ImportItem>(name, alias);
}

std::any ASTBuilder::visitDeclaration(HoocParser::DeclarationContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitFunctionDeclaration(HoocParser::FunctionDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    
    std::vector<std::unique_ptr<Parameter>> parameters;
    if (ctx->parameterList()) {
        parameters = castVector<Parameter>(visit(ctx->parameterList()));
    }
    
    std::unique_ptr<Type> returnType;
    if (ctx->type()) {
        returnType = cast<Type>(visit(ctx->type()));
    }
    
    auto body = cast<Block>(visit(ctx->block()));
    
    return std::unique_ptr<Declaration>(
        std::make_unique<FunctionDeclaration>(name, std::move(parameters), 
                                            std::move(returnType), std::move(body)));
}

std::any ASTBuilder::visitClassDeclaration(HoocParser::ClassDeclarationContext* ctx) {
    // Parse modifiers
    std::vector<ClassModifier> modifiers;
    for (auto modCtx : ctx->classModifier()) {
        std::string modText = modCtx->getText();
        modifiers.push_back(parseClassModifier(modText));
    }
    
    std::string name = ctx->IDENTIFIER(0)->getText();
    
    std::unique_ptr<PrimaryConstructor> constructor;
    if (ctx->primaryConstructor()) {
        constructor = cast<PrimaryConstructor>(visit(ctx->primaryConstructor()));
    }
    
    std::string baseClass = "";
    if (ctx->EXTENDS()) {
        baseClass = ctx->IDENTIFIER(1)->getText();
    }
    
    std::vector<std::string> interfaces;
    if (ctx->interfaceList()) {
        interfaces = std::any_cast<std::vector<std::string>>(visit(ctx->interfaceList()));
    }
    
    auto body = cast<ClassBody>(visit(ctx->classBody()));
    
    return std::unique_ptr<Declaration>(
        std::make_unique<ClassDeclaration>(modifiers, name, std::move(constructor),
                                         baseClass, interfaces, std::move(body)));
}

std::any ASTBuilder::visitInterfaceDeclaration(HoocParser::InterfaceDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    
    std::vector<std::unique_ptr<InterfaceMember>> members;
    for (auto memberCtx : ctx->interfaceMember()) {
        auto member = cast<InterfaceMember>(visit(memberCtx));
        if (member) {
            members.push_back(std::move(member));
        }
    }
    
    return std::unique_ptr<Declaration>(
        std::make_unique<InterfaceDeclaration>(name, std::move(members)));
}

std::any ASTBuilder::visitVariableDeclaration(HoocParser::VariableDeclarationContext* ctx) {
    if (ctx->VAR()) {
        // var name = expression
        std::string name = ctx->IDENTIFIER()->getText();
        auto initializer = cast<Expression>(visit(ctx->expression()));
        return std::unique_ptr<Declaration>(
            std::make_unique<VariableDeclaration>(name, std::move(initializer)));
    } else {
        // type name [= expression]
        auto type = cast<Type>(visit(ctx->type()));
        std::string name = ctx->IDENTIFIER()->getText();
        
        std::unique_ptr<Expression> initializer;
        if (ctx->expression()) {
            initializer = cast<Expression>(visit(ctx->expression()));
        }
        
        return std::unique_ptr<Declaration>(
            std::make_unique<VariableDeclaration>(std::move(type), name, std::move(initializer)));
    }
}

std::any ASTBuilder::visitParameter(HoocParser::ParameterContext* ctx) {
    auto type = cast<Type>(visit(ctx->type()));
    std::string name = ctx->IDENTIFIER()->getText();
    return std::make_unique<Parameter>(std::move(type), name);
}

std::any ASTBuilder::visitParameterList(HoocParser::ParameterListContext* ctx) {
    std::vector<std::unique_ptr<Parameter>> parameters;
    for (auto paramCtx : ctx->parameter()) {
        auto param = cast<Parameter>(visit(paramCtx));
        if (param) {
            parameters.push_back(std::move(param));
        }
    }
    return parameters;
}

std::any ASTBuilder::visitClassModifier(HoocParser::ClassModifierContext* ctx) {
    return parseClassModifier(ctx->getText());
}

std::any ASTBuilder::visitPrimaryConstructor(HoocParser::PrimaryConstructorContext* ctx) {
    std::vector<std::unique_ptr<Parameter>> parameters;
    if (ctx->parameterList()) {
        parameters = castVector<Parameter>(visit(ctx->parameterList()));
    }
    return std::make_unique<PrimaryConstructor>(std::move(parameters));
}

std::any ASTBuilder::visitClassBody(HoocParser::ClassBodyContext* ctx) {
    std::vector<std::unique_ptr<ClassMember>> members;
    for (auto memberCtx : ctx->classMember()) {
        auto member = cast<ClassMember>(visit(memberCtx));
        if (member) {
            members.push_back(std::move(member));
        }
    }
    return std::make_unique<ClassBody>(std::move(members));
}

std::any ASTBuilder::visitClassMember(HoocParser::ClassMemberContext* ctx) {
    if (ctx->eventDeclaration()) {
        auto event = cast<EventDeclaration>(visit(ctx->eventDeclaration()));
        return std::make_unique<ClassMember>(std::move(event));
    } else if (ctx->functionDeclaration()) {
        auto func = cast<FunctionDeclaration>(visit(ctx->functionDeclaration()));
        return std::make_unique<ClassMember>(std::unique_ptr<Declaration>(func.release()));
    } else if (ctx->variableDeclaration()) {
        auto var = cast<VariableDeclaration>(visit(ctx->variableDeclaration()));
        return std::make_unique<ClassMember>(std::unique_ptr<Declaration>(var.release()));
    }
    return std::unique_ptr<ClassMember>(nullptr);
}

std::any ASTBuilder::visitEventDeclaration(HoocParser::EventDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    return std::make_unique<EventDeclaration>(name);
}

std::any ASTBuilder::visitInterfaceList(HoocParser::InterfaceListContext* ctx) {
    std::vector<std::string> interfaces;
    for (auto idCtx : ctx->IDENTIFIER()) {
        interfaces.push_back(idCtx->getText());
    }
    return interfaces;
}

std::any ASTBuilder::visitInterfaceMember(HoocParser::InterfaceMemberContext* ctx) {
    auto signature = cast<FunctionSignature>(visit(ctx->functionSignature()));
    return std::make_unique<InterfaceMember>(std::move(signature));
}

std::any ASTBuilder::visitFunctionSignature(HoocParser::FunctionSignatureContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    
    std::vector<std::unique_ptr<Parameter>> parameters;
    if (ctx->parameterList()) {
        parameters = castVector<Parameter>(visit(ctx->parameterList()));
    }
    
    std::unique_ptr<Type> returnType;
    if (ctx->type()) {
        returnType = cast<Type>(visit(ctx->type()));
    }
    
    return std::make_unique<FunctionSignature>(name, std::move(parameters), std::move(returnType));
}

std::any ASTBuilder::visitType(HoocParser::TypeContext* ctx) {
    return visit(ctx->unionType());
}

std::any ASTBuilder::visitUnionType(HoocParser::UnionTypeContext* ctx) {
    std::vector<std::unique_ptr<OptionalType>> types;
    for (auto optCtx : ctx->optionalType()) {
        auto opt = cast<OptionalType>(visit(optCtx));
        if (opt) {
            types.push_back(std::move(opt));
        }
    }
    return std::unique_ptr<Type>(std::make_unique<UnionType>(std::move(types)));
}

std::any ASTBuilder::visitOptionalType(HoocParser::OptionalTypeContext* ctx) {
    auto arrayType = cast<ArrayType>(visit(ctx->arrayType()));
    bool isOptional = ctx->QUESTION() != nullptr;
    return std::make_unique<OptionalType>(std::move(arrayType), isOptional);
}

std::any ASTBuilder::visitArrayType(HoocParser::ArrayTypeContext* ctx) {
    auto baseType = cast<BaseType>(visit(ctx->baseType()));
    
    std::vector<std::unique_ptr<Expression>> dimensions;
    for (auto bracketCtx : ctx->LBRACKET()) {
        // Find corresponding expression for this bracket
        size_t index = std::distance(ctx->LBRACKET().begin(), 
                                   std::find(ctx->LBRACKET().begin(), ctx->LBRACKET().end(), bracketCtx));
        if (index < ctx->expression().size()) {
            auto expr = cast<Expression>(visit(ctx->expression(index)));
            dimensions.push_back(std::move(expr));
        } else {
            dimensions.push_back(nullptr); // Empty dimension []
        }
    }
    
    return std::make_unique<ArrayType>(std::move(baseType), std::move(dimensions));
}

std::any ASTBuilder::visitBaseType(HoocParser::BaseTypeContext* ctx) {
    if (ctx->primitiveType()) {
        auto primitive = cast<PrimitiveType>(visit(ctx->primitiveType()));
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

// Continue with statement and expression methods...
std::any ASTBuilder::visitStatement(HoocParser::StatementContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitBlock(HoocParser::BlockContext* ctx) {
    std::vector<std::unique_ptr<Statement>> statements;
    for (auto stmtCtx : ctx->statement()) {
        auto stmt = cast<Statement>(visit(stmtCtx));
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    return std::unique_ptr<Statement>(std::make_unique<Block>(std::move(statements)));
}

std::any ASTBuilder::visitExpressionStatement(HoocParser::ExpressionStatementContext* ctx) {
    auto expr = cast<Expression>(visit(ctx->expression()));
    return std::unique_ptr<Statement>(std::make_unique<ExpressionStatement>(std::move(expr)));
}

std::any ASTBuilder::visitIfStatement(HoocParser::IfStatementContext* ctx) {
    auto condition = cast<Expression>(visit(ctx->expression()));
    auto thenBlock = cast<Block>(visit(ctx->block(0)));
    
    std::unique_ptr<Block> elseBlock;
    if (ctx->block().size() > 1) {
        elseBlock = cast<Block>(visit(ctx->block(1)));
    }
    
    return std::unique_ptr<Statement>(
        std::make_unique<IfStatement>(std::move(condition), std::move(thenBlock), std::move(elseBlock)));
}

std::any ASTBuilder::visitForInStatement(HoocParser::ForInStatementContext* ctx) {
    std::string variable = ctx->IDENTIFIER()->getText();
    auto iterable = cast<Expression>(visit(ctx->expression()));
    auto body = cast<Block>(visit(ctx->block()));
    
    return std::unique_ptr<Statement>(
        std::make_unique<ForInStatement>(variable, std::move(iterable), std::move(body)));
}

std::any ASTBuilder::visitForRangeStatement(HoocParser::ForRangeStatementContext* ctx) {
    std::string variable = ctx->IDENTIFIER()->getText();
    auto start = cast<Expression>(visit(ctx->expression(0)));
    auto end = cast<Expression>(visit(ctx->expression(1)));
    auto body = cast<Block>(visit(ctx->block()));
    
    return std::unique_ptr<Statement>(
        std::make_unique<ForRangeStatement>(variable, std::move(start), std::move(end), std::move(body)));
}

std::any ASTBuilder::visitWhileStatement(HoocParser::WhileStatementContext* ctx) {
    auto condition = cast<Expression>(visit(ctx->expression()));
    auto body = cast<Block>(visit(ctx->block()));
    
    return std::unique_ptr<Statement>(
        std::make_unique<WhileStatement>(std::move(condition), std::move(body)));
}

std::any ASTBuilder::visitReturnStatement(HoocParser::ReturnStatementContext* ctx) {
    std::unique_ptr<Expression> expr;
    if (ctx->expression()) {
        expr = cast<Expression>(visit(ctx->expression()));
    }
    
    return std::unique_ptr<Statement>(std::make_unique<ReturnStatement>(std::move(expr)));
}

std::any ASTBuilder::visitScopeStatement(HoocParser::ScopeStatementContext* ctx) {
    auto body = cast<Block>(visit(ctx->block()));
    return std::unique_ptr<Statement>(std::make_unique<ScopeStatement>(std::move(body)));
}

// Expression methods (simplified - you would implement all expression types)
std::any ASTBuilder::visitExpression(HoocParser::ExpressionContext* ctx) {
    return visitChildren(ctx);
}

std::any ASTBuilder::visitPrimaryExpression(HoocParser::PrimaryExpressionContext* ctx) {
    auto primary = visit(ctx->primary());
    return std::make_unique<PrimaryExpression>(std::any_cast<std::unique_ptr<ASTNode>>(primary));
}

std::any ASTBuilder::visitPrimary(HoocParser::PrimaryContext* ctx) {
    if (ctx->IDENTIFIER()) {
        return std::unique_ptr<ASTNode>(std::make_unique<Identifier>(ctx->IDENTIFIER()->getText()));
    } else if (ctx->INTEGER_LITERAL()) {
        return std::unique_ptr<ASTNode>(std::make_unique<IntegerLiteral>(getIntegerValue(ctx->INTEGER_LITERAL())));
    } else if (ctx->FLOATING_LITERAL()) {
        return std::unique_ptr<ASTNode>(std::make_unique<FloatingLiteral>(getDoubleValue(ctx->FLOATING_LITERAL())));
    } else if (ctx->STRING_LITERAL()) {
        return std::unique_ptr<ASTNode>(std::make_unique<StringLiteral>(getStringValue(ctx->STRING_LITERAL())));
    } else if (ctx->CHAR_LITERAL()) {
        return std::unique_ptr<ASTNode>(std::make_unique<CharacterLiteral>(getCharValue(ctx->CHAR_LITERAL())));
    } else if (ctx->TRUE() || ctx->FALSE()) {
        return std::unique_ptr<ASTNode>(std::make_unique<BooleanLiteral>(ctx->TRUE() != nullptr));
    }
    
    return visitChildren(ctx);
}

// Add placeholder implementations for other expression types
std::any ASTBuilder::visitMemberAccess(HoocParser::MemberAccessContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitArrayAccess(HoocParser::ArrayAccessContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitFunctionCall(HoocParser::FunctionCallContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitNewArrayExpression(HoocParser::NewArrayExpressionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitNewObjectExpression(HoocParser::NewObjectExpressionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitUnaryMinus(HoocParser::UnaryMinusContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitLogicalNot(HoocParser::LogicalNotContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitAdditiveExpression(HoocParser::AdditiveExpressionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitRelationalExpression(HoocParser::RelationalExpressionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitLogicalAnd(HoocParser::LogicalAndContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitLogicalOr(HoocParser::LogicalOrContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitAssignmentExpression(HoocParser::AssignmentExpressionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitErrorHandlingExpression(HoocParser::ErrorHandlingExpressionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitArrayLiteral(HoocParser::ArrayLiteralContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitListComprehension(HoocParser::ListComprehensionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitLambdaExpression(HoocParser::LambdaExpressionContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitMultiParamLambda(HoocParser::MultiParamLambdaContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitInterpolatedString(HoocParser::InterpolatedStringContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitArgumentList(HoocParser::ArgumentListContext* ctx) { return visitChildren(ctx); }
std::any ASTBuilder::visitExpressionList(HoocParser::ExpressionListContext* ctx) { return visitChildren(ctx); }

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

BinaryOperator ASTBuilder::parseBinaryOperator(const std::string& op) {
    if (op == "*") return BinaryOperator::MULTIPLY;
    if (op == "/") return BinaryOperator::DIVIDE;
    if (op == "%") return BinaryOperator::MODULO;
    if (op == "+") return BinaryOperator::PLUS;
    if (op == "-") return BinaryOperator::MINUS;
    if (op == "<") return BinaryOperator::LESS;
    if (op == "<=") return BinaryOperator::LESS_EQUALS;
    if (op == ">") return BinaryOperator::GREATER;
    if (op == ">=") return BinaryOperator::GREATER_EQUALS;
    if (op == "==") return BinaryOperator::EQUALS;
    if (op == "!=") return BinaryOperator::NOT_EQUALS;
    if (op == "&&") return BinaryOperator::AND;
    if (op == "||") return BinaryOperator::OR;
    if (op == "=") return BinaryOperator::ASSIGN;
    throw std::runtime_error("Unknown binary operator: " + op);
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