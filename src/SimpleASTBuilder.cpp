#include "SimpleASTBuilder.h"
#include "ast/AST.h"
#include <iostream>
#include <stdexcept>

using namespace hooc;
using namespace hooc::ast;

std::unique_ptr<CompilationUnit> SimpleASTBuilder::buildAST(HoocParser::CompilationUnitContext* ctx) {
    if (!ctx) {
        return nullptr;
    }
    
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
    } else if (ctx->variableDeclaration()) {
        return buildVariableDeclaration(ctx->variableDeclaration());
    }
    return nullptr;
}

std::unique_ptr<VariableDeclaration> SimpleASTBuilder::buildVariableDeclaration(HoocParser::VariableDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();

    if (ctx->VAR()) {
        // Type inference: var x = expr
        auto initializer = buildExpression(ctx->expression());
        return std::make_unique<VariableDeclaration>(name, std::move(initializer));
    } else {
        // Explicit type: type x or type x = expr
        auto type = buildType(ctx->type());
        std::unique_ptr<Expression> initializer;
        if (ctx->expression()) {
            initializer = buildExpression(ctx->expression());
        }
        return std::make_unique<VariableDeclaration>(std::move(type), name, std::move(initializer));
    }
}

std::unique_ptr<VariableDeclarationStatement> SimpleASTBuilder::buildVariableDeclarationStatement(HoocParser::VariableDeclarationContext* ctx) {
    auto varDecl = buildVariableDeclaration(ctx);
    return std::make_unique<VariableDeclarationStatement>(std::move(varDecl));
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
    } else if (ctx->ifStatement()) {
        return buildIfStatement(ctx->ifStatement());
    } else if (ctx->whileStatement()) {
        return buildWhileStatement(ctx->whileStatement());
    } else if (ctx->forStatement()) {
        auto forCtx = ctx->forStatement();
        if (auto forInCtx = dynamic_cast<HoocParser::ForInStatementContext*>(forCtx)) {
            return buildForInStatement(forInCtx);
        } else if (auto forRangeCtx = dynamic_cast<HoocParser::ForRangeStatementContext*>(forCtx)) {
            return buildForRangeStatement(forRangeCtx);
        }
    }

    return nullptr;
}

std::unique_ptr<IfStatement> SimpleASTBuilder::buildIfStatement(HoocParser::IfStatementContext* ctx) {
    auto condition = buildExpression(ctx->expression());
    auto thenBlock = buildBlock(ctx->block(0));
    std::unique_ptr<Block> elseBlock;
    if (ctx->block().size() > 1) {
        elseBlock = buildBlock(ctx->block(1));
    }
    return std::make_unique<IfStatement>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

std::unique_ptr<WhileStatement> SimpleASTBuilder::buildWhileStatement(HoocParser::WhileStatementContext* ctx) {
    auto condition = buildExpression(ctx->expression());
    auto body = buildBlock(ctx->block());
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<ForInStatement> SimpleASTBuilder::buildForInStatement(HoocParser::ForInStatementContext* ctx) {
    std::string variable = ctx->IDENTIFIER()->getText();
    auto iterable = buildExpression(ctx->expression());
    auto body = buildBlock(ctx->block());
    return std::make_unique<ForInStatement>(variable, std::move(iterable), std::move(body));
}

std::unique_ptr<ForRangeStatement> SimpleASTBuilder::buildForRangeStatement(HoocParser::ForRangeStatementContext* ctx) {
    std::string variable = ctx->IDENTIFIER()->getText();
    auto exprs = ctx->expression();
    auto start = buildExpression(exprs[0]);
    auto end = buildExpression(exprs[1]);
    auto body = buildBlock(ctx->block());
    return std::make_unique<ForRangeStatement>(variable, std::move(start), std::move(end), std::move(body));
}

std::unique_ptr<Expression> SimpleASTBuilder::buildExpression(HoocParser::ExpressionContext* ctx) {
    if (ctx->assignmentExpression()) {
        return buildAssignmentExpression(ctx->assignmentExpression());
    }
    return nullptr;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildAssignmentExpression(HoocParser::AssignmentExpressionContext* ctx) {
    auto left = buildLogicalOrExpression(ctx->logicalOrExpression());

    if (ctx->ASSIGN() && ctx->assignmentExpression()) {
        auto right = buildAssignmentExpression(ctx->assignmentExpression());
        return std::make_unique<AssignmentExpression>(std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildLogicalOrExpression(HoocParser::LogicalOrExpressionContext* ctx) {
    auto andExprs = ctx->logicalAndExpression();
    auto result = buildLogicalAndExpression(andExprs[0]);

    for (size_t i = 1; i < andExprs.size(); i++) {
        auto right = buildLogicalAndExpression(andExprs[i]);
        result = std::make_unique<LogicalOr>(std::move(result), std::move(right));
    }

    return result;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildLogicalAndExpression(HoocParser::LogicalAndExpressionContext* ctx) {
    auto relExprs = ctx->relationalExpression();
    auto result = buildRelationalExpression(relExprs[0]);

    for (size_t i = 1; i < relExprs.size(); i++) {
        auto right = buildRelationalExpression(relExprs[i]);
        result = std::make_unique<LogicalAnd>(std::move(result), std::move(right));
    }

    return result;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildRelationalExpression(HoocParser::RelationalExpressionContext* ctx) {
    auto addExprs = ctx->additiveExpression();
    auto result = buildAdditiveExpression(addExprs[0]);

    for (size_t i = 1; i < addExprs.size(); i++) {
        auto right = buildAdditiveExpression(addExprs[i]);

        // Determine the operator
        BinaryOperator op;
        if (ctx->EQUALS(i-1)) op = BinaryOperator::EQUALS;
        else if (ctx->NOT_EQUALS(i-1)) op = BinaryOperator::NOT_EQUALS;
        else if (ctx->LESS(i-1)) op = BinaryOperator::LESS;
        else if (ctx->LESS_EQUALS(i-1)) op = BinaryOperator::LESS_EQUALS;
        else if (ctx->GREATER(i-1)) op = BinaryOperator::GREATER;
        else if (ctx->GREATER_EQUALS(i-1)) op = BinaryOperator::GREATER_EQUALS;
        else op = BinaryOperator::EQUALS; // Default

        result = std::make_unique<RelationalExpression>(std::move(result), op, std::move(right));
    }

    return result;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildAdditiveExpression(HoocParser::AdditiveExpressionContext* ctx) {
    auto multExprs = ctx->multiplicativeExpression();
    auto result = buildMultiplicativeExpression(multExprs[0]);

    for (size_t i = 1; i < multExprs.size(); i++) {
        auto right = buildMultiplicativeExpression(multExprs[i]);

        BinaryOperator op = ctx->PLUS(i-1) ? BinaryOperator::PLUS : BinaryOperator::MINUS;
        result = std::make_unique<AdditiveExpression>(std::move(result), op, std::move(right));
    }

    return result;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildMultiplicativeExpression(HoocParser::MultiplicativeExpressionContext* ctx) {
    auto unaryExprs = ctx->unaryExpression();
    auto result = buildUnaryExpression(unaryExprs[0]);

    for (size_t i = 1; i < unaryExprs.size(); i++) {
        auto right = buildUnaryExpression(unaryExprs[i]);

        BinaryOperator op;
        if (ctx->MULTIPLY(i-1)) op = BinaryOperator::MULTIPLY;
        else if (ctx->DIVIDE(i-1)) op = BinaryOperator::DIVIDE;
        else op = BinaryOperator::MODULO;

        result = std::make_unique<MultiplicativeExpression>(std::move(result), op, std::move(right));
    }

    return result;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildUnaryExpression(HoocParser::UnaryExpressionContext* ctx) {
    auto postfix = buildPostfixExpression(ctx->postfixExpression());

    if (ctx->MINUS()) {
        return std::make_unique<UnaryMinus>(std::move(postfix));
    } else if (ctx->NOT()) {
        return std::make_unique<LogicalNot>(std::move(postfix));
    }

    return postfix;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildPostfixExpression(HoocParser::PostfixExpressionContext* ctx) {
    auto result = buildPrimary(ctx->primary());

    // Process postfix operators in order
    // We need to track which operators appear and in what order
    // For simplicity, we'll handle them based on their token positions

    // Handle member access (.member)
    for (size_t i = 0; i < ctx->DOT().size(); i++) {
        if (i < ctx->IDENTIFIER().size()) {
            std::string member = ctx->IDENTIFIER(i)->getText();
            result = std::make_unique<MemberAccess>(std::move(result), member);
        }
    }

    // Handle array access ([index])
    for (size_t i = 0; i < ctx->LBRACKET().size(); i++) {
        if (i < ctx->expression().size()) {
            auto index = buildExpression(ctx->expression(i));
            result = std::make_unique<ArrayAccess>(std::move(result), std::move(index));
        }
    }

    // Handle function calls (func(args))
    for (size_t i = 0; i < ctx->LPAREN().size(); i++) {
        auto argList = (i < ctx->argumentList().size())
            ? buildArgumentList(ctx->argumentList(i))
            : std::make_unique<ArgumentList>(std::vector<std::unique_ptr<Expression>>());
        result = std::make_unique<FunctionCall>(std::move(result), std::move(argList));
    }

    return result;
}

std::unique_ptr<ArgumentList> SimpleASTBuilder::buildArgumentList(HoocParser::ArgumentListContext* ctx) {
    std::vector<std::unique_ptr<Expression>> arguments;

    if (ctx) {
        for (auto exprCtx : ctx->expression()) {
            auto expr = buildExpression(exprCtx);
            if (expr) {
                arguments.push_back(std::move(expr));
            }
        }
    }

    return std::make_unique<ArgumentList>(std::move(arguments));
}

std::unique_ptr<Expression> SimpleASTBuilder::buildPrimary(HoocParser::PrimaryContext* ctx) {
    if (ctx->LPAREN() && ctx->expression()) {
        // Parenthesized expression
        return buildExpression(ctx->expression());
    } else if (ctx->IDENTIFIER()) {
        auto identifier = std::make_unique<Identifier>(ctx->IDENTIFIER()->getText());
        return std::make_unique<PrimaryExpression>(std::move(identifier));
    } else if (ctx->INTEGER_LITERAL()) {
        int value = getIntValue(ctx->INTEGER_LITERAL());
        auto intLiteral = std::make_unique<IntegerLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(intLiteral));
    } else if (ctx->FLOATING_LITERAL()) {
        double value = getDoubleValue(ctx->FLOATING_LITERAL());
        auto floatingLiteral = std::make_unique<FloatingLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(floatingLiteral));
    } else if (ctx->STRING_LITERAL()) {
        std::string value = getStringValue(ctx->STRING_LITERAL());
        auto stringLiteral = std::make_unique<StringLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(stringLiteral));
    } else if (ctx->CHAR_LITERAL()) {
        char value = getCharValue(ctx->CHAR_LITERAL());
        auto charLiteral = std::make_unique<CharacterLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(charLiteral));
    } else if (ctx->TRUE() || ctx->FALSE()) {
        bool value = ctx->TRUE() != nullptr;
        auto boolLiteral = std::make_unique<BooleanLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(boolLiteral));
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