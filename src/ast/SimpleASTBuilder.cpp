#include "SimpleASTBuilder.h"
#include "AST.h"
#include "parsing/HooParserWrapper.h"
#include <iostream>
#include <stdexcept>


using namespace hooc;
using namespace hooc::ast;

std::unique_ptr<CompilationUnit> SimpleASTBuilder::buildAST(HoocParser::CompilationUnitContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("CompilationUnitContext is null");
    }

    // Process imports
    std::vector<std::unique_ptr<ImportStatement>> imports;
    for (auto importCtx : ctx->importStatement()) {
        auto import = buildImportStatement(importCtx);
        if (import) {
            imports.push_back(std::move(import));
        }
    }

    std::vector<std::unique_ptr<Declaration>> declarations;
    for (auto* child : ctx->children) {
        if (auto* declCtx = dynamic_cast<HoocParser::DeclarationContext*>(child)) {
           if (auto decl = buildDeclaration(declCtx)) {
            declarations.push_back(std::move(decl));
        }
    }
    }
    
    // Group overloaded free functions
    std::vector<std::unique_ptr<Declaration>> groupedDeclarations;
    for (size_t i = 0; i < declarations.size(); ) {
        auto funcDecl = dynamic_cast<FunctionDeclaration*>(declarations[i].get());
        if (funcDecl) {
            std::string name = funcDecl->getName();
            std::vector<std::unique_ptr<FunctionDeclaration>> overloads;
            
            size_t j = i;
            while (j < declarations.size()) {
                auto nextFunc = dynamic_cast<FunctionDeclaration*>(declarations[j].get());
                if (nextFunc && nextFunc->getName() == name) {
                    nextFunc->setOverload(true);
                    // We must release from unique_ptr to move it, but it's held in declarations array.
                    // We can move it out.
                    overloads.push_back(std::unique_ptr<FunctionDeclaration>(
                        static_cast<FunctionDeclaration*>(declarations[j].release())
                    ));
                    j++;
                } else {
                    break;
                }
            }
            
            if (overloads.size() > 1) {
                groupedDeclarations.push_back(std::make_unique<OverloadList>(std::move(overloads)));
            } else {
                // Not actually overloaded if there's only 1
                overloads[0]->setOverload(false);
                groupedDeclarations.push_back(std::move(overloads[0]));
            }
            i = j;
        } else {
            groupedDeclarations.push_back(std::move(declarations[i]));
            i++;
        }
    }

    return std::make_unique<CompilationUnit>(std::move(imports), std::move(groupedDeclarations));
}



std::unique_ptr<Declaration> SimpleASTBuilder::buildDeclaration(HoocParser::DeclarationContext* ctx) {
    if (ctx->functionDeclaration()) {
        return buildFunctionDeclaration(ctx->functionDeclaration());
    } else if (ctx->variableDeclaration()) {
        auto varDecl = buildVariableDeclaration(ctx->variableDeclaration());
        if (varDecl) {
            varDecl->setGlobal(true);
        }
        return varDecl;
    } else if (ctx->constantDeclaration()) {
        auto constDecl = buildConstantDeclaration(ctx->constantDeclaration());
        if (constDecl) {
            constDecl->setGlobal(true);
            constDecl->setConstant(true);
        }
        return constDecl;
    } else if (ctx->classDeclaration()) {
        return buildClassDeclaration(ctx->classDeclaration());
    }
    throw std::runtime_error("Unknown declaration type encountered");
}

std::unique_ptr<VariableDeclaration> SimpleASTBuilder::buildVariableDeclaration(HoocParser::VariableDeclarationContext* ctx,
    std::vector<FunctionModifier> modifiers) {
    std::string name = ctx->IDENTIFIER()->getText();

    if (ctx->type()) {
        // Explicit type: var x: type or var x: type = expr
        auto type = buildType(ctx->type());
        rejectAnyTypeInPosition(type.get(), "variable declaration");
        std::unique_ptr<Expression> initializer;
        if (ctx->expression()) {
            initializer = buildExpression(ctx->expression());
        }
        return std::make_unique<VariableDeclaration>(std::move(type), name, std::move(initializer), false, false, std::move(modifiers));
    } else {
        // Type inference: var x = expr
        auto initializer = buildExpression(ctx->expression());
        return std::make_unique<VariableDeclaration>(name, std::move(initializer), false, false, std::move(modifiers));
    }
}

std::unique_ptr<VariableDeclaration> SimpleASTBuilder::buildConstantDeclaration(HoocParser::ConstantDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();

    if (!ctx->expression()) {
        throw std::runtime_error("Constant '" + name + "' must have an initializer");
    }

    if (ctx->type()) {
        auto type = buildType(ctx->type());
        rejectAnyTypeInPosition(type.get(), "constant declaration");
        auto initializer = buildExpression(ctx->expression());
        return std::make_unique<VariableDeclaration>(std::move(type), name, std::move(initializer), false, true);
    } else {
        auto initializer = buildExpression(ctx->expression());
        return std::make_unique<VariableDeclaration>(name, std::move(initializer), false, true);
    }
}

std::unique_ptr<VariableDeclarationStatement> SimpleASTBuilder::buildVariableDeclarationStatement(HoocParser::VariableDeclarationStatementContext* ctx) {
    auto varDecl = buildVariableDeclaration(ctx->variableDeclaration());
    return std::make_unique<VariableDeclarationStatement>(std::move(varDecl));
}

std::unique_ptr<FunctionDeclaration> SimpleASTBuilder::buildFunctionDeclaration(HoocParser::FunctionDeclarationContext* ctx) {
    return buildFunctionDeclaration(ctx, std::vector<FunctionModifier>{});
}

std::unique_ptr<FunctionDeclaration> SimpleASTBuilder::buildFunctionDeclaration(
    HoocParser::FunctionDeclarationContext* ctx,
    std::vector<FunctionModifier> modifiers) {
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

    // Return type is optional - defaults to void if not specified
    std::unique_ptr<Type> returnType;
    if (ctx->type()) {
        returnType = buildType(ctx->type());
    } else {
        // Create void type as default
        auto voidPrimitive = std::make_unique<PrimitiveType>(PrimitiveTypeKind::VOID);
        returnType = std::make_unique<BaseType>(std::move(voidPrimitive));
    }

    auto body = buildBlock(ctx->block());

    return std::make_unique<FunctionDeclaration>(name, std::move(parameters),
                                                std::move(returnType), std::move(body),
                                                std::move(modifiers));
}

std::unique_ptr<Parameter> SimpleASTBuilder::buildParameter(HoocParser::ParameterContext* ctx) {
    // Grammar: IDENTIFIER COLON type
    std::string name = ctx->IDENTIFIER()->getText();
    auto type = buildType(ctx->type());
    rejectAnyTypeInPosition(type.get(), "parameter");
    return std::make_unique<Parameter>(std::move(type), name);
}

std::unique_ptr<Type> SimpleASTBuilder::buildType(HoocParser::TypeContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("TypeContext is null");
    }

    if (ctx->anyType()) {
        return std::make_unique<AnyType>();
    }

    if (ctx->anyArrayType()) {
        return std::make_unique<AnyArrayType>();
    }

    if (ctx->hashMapType()) {
        return buildHashMapType(ctx->hashMapType());
    }
     if (ctx->decimalType()) {
    return buildDecimalType(ctx->decimalType());
    }
    // Handle mapType first
    if (ctx->mapType()) {
        return buildMapType(ctx->mapType());
    }

    if (ctx->tensorType()) {
        return buildTensorType(ctx->tensorType());
    }

    // Handle optionalType (array types and primitives)
    if (!ctx->optionalType()) {
        throw std::runtime_error("Malformed Type: missing supported type form");
    }

    auto optionalCtx = ctx->optionalType();
    auto arrayCtx = optionalCtx->arrayType();
    if (!arrayCtx) {
        throw std::runtime_error("Malformed Type: missing arrayType");
    }

    auto baseCtx = arrayCtx->baseType();
    if (!baseCtx) {
        throw std::runtime_error("Malformed Type: missing baseType");
    }

    auto baseType = buildBaseType(baseCtx);

    // Build dimensions
    size_t dimensionCount = arrayCtx->LBRACKET().size();

    // Check if it is optional
    bool isOptional = optionalCtx->QUESTION() != nullptr;

    if (isOptional) {
        // Always wrap in OptionalType(ArrayType(...))
        std::vector<std::unique_ptr<Expression>> dimensions;
        for (size_t i = 0; i < dimensionCount; i++) {
            dimensions.push_back(nullptr);
        }
        auto arrayType = std::make_unique<ArrayType>(std::move(baseType), std::move(dimensions));
        return std::make_unique<OptionalType>(std::move(arrayType), true);
    } else {
        // Not optional, return ArrayType or BaseType directly
        if (dimensionCount > 0) {
            std::vector<std::unique_ptr<Expression>> dimensions;
            for (size_t i = 0; i < dimensionCount; i++) {
                dimensions.push_back(nullptr);
            }
            return std::make_unique<ArrayType>(std::move(baseType), std::move(dimensions));
        } else {
            return baseType;
        }
    }
}

std::unique_ptr<OptionalType> SimpleASTBuilder::buildOptionalType(HoocParser::OptionalTypeContext* ctx) {
    if (!ctx || !ctx->arrayType()) {
        throw std::runtime_error("OptionalTypeContext or arrayType is null");
    }

    auto arrayCtx = ctx->arrayType();
    auto arrayType = buildArrayType(arrayCtx);

    if (!arrayType) {
        throw std::runtime_error("Failed to build arrayType for OptionalType");
    }

    // Check if this optional type has a QUESTION mark (making it nullable)
    bool isOptional = ctx->QUESTION() != nullptr;

    return std::make_unique<OptionalType>(std::move(arrayType), isOptional);
}

std::unique_ptr<BaseType> SimpleASTBuilder::buildBaseType(HoocParser::BaseTypeContext* ctx) {
    if (ctx->primitiveType()) {
        auto primitive = buildPrimitiveType(ctx->primitiveType());
        return std::make_unique<BaseType>(std::move(primitive));
    } else if (ctx->qualifiedIdentifier()) {
        auto qualifiedId = buildQualifiedIdentifier(ctx->qualifiedIdentifier());
        return std::make_unique<BaseType>(std::move(qualifiedId));
    }

    throw std::runtime_error("BaseType has neither primitiveType nor qualifiedIdentifier");
}

std::unique_ptr<PrimitiveType> SimpleASTBuilder::buildPrimitiveType(HoocParser::PrimitiveTypeContext* ctx) {
    std::string typeName = ctx->getText();
    PrimitiveTypeKind kind = getPrimitiveTypeKind(typeName);
    return std::make_unique<PrimitiveType>(kind);
}
std::unique_ptr<DecimalType> SimpleASTBuilder::buildDecimalType(HoocParser::DecimalTypeContext* ctx) {

    int precision =
        std::stoi(ctx->INTEGER_LITERAL(0)->getText());

    int scale =
        std::stoi(ctx->INTEGER_LITERAL(1)->getText());

    if (scale > precision) {
    throw std::runtime_error(
        "Decimal scale cannot exceed precision");
    }
    if (precision <= 0) {
    throw std::runtime_error("Decimal precision must be greater than zero");
    }
    return std::make_unique<DecimalType>(
        precision,
        scale);
}
std::unique_ptr<MapType> SimpleASTBuilder::buildMapType(HoocParser::MapTypeContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("MapTypeContext is null");
    }

    // Get key type from mapKeyType context (which is a child context)
    auto keyTypeCtx = ctx->mapKeyType();
    if (!keyTypeCtx) {
        throw std::runtime_error("MapKeyType missing in MapType");
    }

    MapKeyType keyType;
    if (keyTypeCtx->BYTE()) {
        keyType = MapKeyType::BYTE;
    } else if (keyTypeCtx->INT8()) {
        keyType = MapKeyType::INT8;
    } else if (keyTypeCtx->INT64()) {
        keyType = MapKeyType::INT64;
    } else if (keyTypeCtx->CHAR()) {
        keyType = MapKeyType::CHAR;
    } else if (keyTypeCtx->STRING()) {
        keyType = MapKeyType::STRING;
    } else {
        throw std::runtime_error("Unknown MapKeyType encountered");
    }

    // Get value type
    auto valueType = buildType(ctx->type());
    if (!valueType) {
        throw std::runtime_error("Failed to build value type for MapType");
    }

    return std::make_unique<MapType>(keyType, std::move(valueType));
}

std::unique_ptr<HashMapType> SimpleASTBuilder::buildHashMapType(HoocParser::HashMapTypeContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("HashMapTypeContext is null");
    }

    auto keyTypeCtx = ctx->hashMapKeyType();
    if (!keyTypeCtx) {
        throw std::runtime_error("HashMapKeyType missing in HashMapType");
    }

    HashMapKeyType keyType;
    if (keyTypeCtx->BYTE()) {
        keyType = HashMapKeyType::BYTE;
    } else if (keyTypeCtx->INT8()) {
        keyType = HashMapKeyType::INT8;
    } else if (keyTypeCtx->INT64()) {
        keyType = HashMapKeyType::INT64;
    } else {
        throw std::runtime_error("Unknown HashMapKeyType encountered");
    }

    auto valueType = buildType(ctx->type());
    if (!valueType) {
        throw std::runtime_error("Failed to build value type for HashMapType");
    }

    return std::make_unique<HashMapType>(keyType, std::move(valueType));
}

std::unique_ptr<TensorType> SimpleASTBuilder::buildTensorType(HoocParser::TensorTypeContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("TensorTypeContext is null");
    }

    auto elementType = buildBaseType(ctx->baseType());
    std::vector<std::unique_ptr<Expression>> dimensions;
    for (auto intNode : ctx->INTEGER_LITERAL()) {
        dimensions.push_back(std::make_unique<PrimaryExpression>(
            std::make_unique<IntegerLiteral>(getIntValue(intNode))));
    }

    if (dimensions.empty() || dimensions.size() > 3) {
        throw std::runtime_error("tensor type requires 1, 2, or 3 dimensions");
    }

    return std::make_unique<TensorType>(std::move(elementType), std::move(dimensions));
}

std::unique_ptr<Block> SimpleASTBuilder::buildBlock(HoocParser::BlockContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("BlockContext is null");
    }
    std::vector<std::unique_ptr<Statement>> statements;
    
    for (auto stmtCtx : ctx->statement()) {
        auto stmt = buildStatement(stmtCtx);
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    return std::make_unique<Block>(std::move(statements));
}

std::unique_ptr<ArrayType> SimpleASTBuilder::buildArrayType(HoocParser::ArrayTypeContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("ArrayTypeContext is null");
    }
    auto baseType = buildBaseType(ctx->baseType());
    std::vector<std::unique_ptr<Expression>> dimensions;

    // After grammar change, ctx->expression() will always be empty
    // Count brackets to determine dimensionality
    size_t dimensionCount = ctx->LBRACKET().size();

    // All dimensions are unsized (nullptr) - only slice syntax allowed
    for (size_t i = 0; i < dimensionCount; i++) {
        dimensions.push_back(nullptr);
    }

    return std::make_unique<ArrayType>(std::move(baseType), std::move(dimensions));
}

std::unique_ptr<Statement> SimpleASTBuilder::buildStatement(HoocParser::StatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("StatementContext is null");
    }
    if (ctx->variableDeclarationStatement()) {
        return buildVariableDeclarationStatement(ctx->variableDeclarationStatement());
    } else if (ctx->expressionStatement()) {
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
    } else if (ctx->doWhileStatement()) {
        return buildDoWhileStatement(ctx->doWhileStatement());
    } else if (ctx->forStatement()) {
        auto forCtx = ctx->forStatement();
        // Check if it's a for-range loop (has RANGE ..) or for-in loop (has 1 expression)
        if (forCtx->RANGE()) {
            // for-range: for i in start..end [by step] { }
            return buildForRangeStatement(forCtx);
        } else {
            // for-in: for item in iterable { }
            return buildForInStatement(forCtx);
        }
    } else if (ctx->switchStatement()) {
        return buildSwitchStatement(ctx->switchStatement());
    } else if (ctx->breakStatement()) {
        return std::make_unique<BreakStatement>();
    } else if (ctx->continueStatement()) {
        return std::make_unique<ContinueStatement>();
    } else if (ctx->tryCatchStatement()) {
        return buildTryCatchStatement(ctx->tryCatchStatement());
    } else if (ctx->throwStatement()) {
        return buildThrowStatement(ctx->throwStatement());
    }

    throw std::runtime_error("Unknown statement type encountered: " + ctx->getText());
}

std::unique_ptr<IfStatement> SimpleASTBuilder::buildIfStatement(HoocParser::IfStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("IfStatementContext is null");
    }
    auto condition = buildExpression(ctx->expression());
    auto thenBlock = buildBlock(ctx->block(0));
    std::unique_ptr<Block> elseBlock;
    if (ctx->block().size() > 1) {
        elseBlock = buildBlock(ctx->block(1));
    }
    return std::make_unique<IfStatement>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

std::unique_ptr<WhileStatement> SimpleASTBuilder::buildWhileStatement(HoocParser::WhileStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("WhileStatementContext is null");
    }
    auto condition = buildExpression(ctx->expression());
    auto body = buildBlock(ctx->block());
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<DoWhileStatement> SimpleASTBuilder::buildDoWhileStatement(HoocParser::DoWhileStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("DoWhileStatementContext is null");
    }
    auto body = buildBlock(ctx->block());
    auto condition = buildExpression(ctx->expression());
    return std::make_unique<DoWhileStatement>(std::move(body), std::move(condition));
}

std::unique_ptr<SwitchStatement> SimpleASTBuilder::buildSwitchStatement(HoocParser::SwitchStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("SwitchStatementContext is null");
    }
    auto discriminant = buildExpression(ctx->expression());

    std::vector<SwitchStatement::CaseClause> cases;
    for (auto caseCtx : ctx->switchCase()) {
        SwitchStatement::CaseClause clause;
        clause.value = buildExpression(caseCtx->expression());
        for (auto stmtCtx : caseCtx->statement()) {
            clause.statements.push_back(buildStatement(stmtCtx));
        }
        cases.push_back(std::move(clause));
    }

    std::vector<std::unique_ptr<Statement>> defaultStatements;
    if (ctx->switchDefault()) {
        for (auto stmtCtx : ctx->switchDefault()->statement()) {
            defaultStatements.push_back(buildStatement(stmtCtx));
        }
    }

    return std::make_unique<SwitchStatement>(std::move(discriminant), std::move(cases), std::move(defaultStatements));
}

std::unique_ptr<TryCatchStatement> SimpleASTBuilder::buildTryCatchStatement(HoocParser::TryCatchStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("TryCatchStatementContext is null");
    }

    auto blocks = ctx->block();
    if (blocks.empty()) {
        throw std::runtime_error("TryCatchStatement must have at least one block");
    }

    std::unique_ptr<Block> tryBlock = buildBlock(blocks[0]);
    if (!tryBlock) {
        throw std::runtime_error("Failed to build try block for TryCatchStatement");
    }

    std::vector<TryCatchStatement::CatchClause> catchClauses;
    std::unique_ptr<Block> finallyBlock;

    if (ctx->CATCH().size() > 0) {
        for (size_t i = 0; i < ctx->CATCH().size(); i++) {
            TryCatchStatement::CatchClause clause;
            size_t blockIdx = i + 1;

            if (blockIdx < blocks.size()) {
                auto idCtx = ctx->IDENTIFIER(i);
                auto typeCtx = ctx->type(i);

                if (idCtx && typeCtx) {
                    clause.variable = idCtx->getText();
                    clause.type = buildType(typeCtx);
                    rejectAnyTypeInPosition(clause.type.get(), "catch clause");
                    clause.block = buildBlock(blocks[blockIdx]);
                    catchClauses.push_back(std::move(clause));
                }
            }
        }
    }

    size_t finallyBlockIdx = 1 + ctx->CATCH().size();
    if (finallyBlockIdx < blocks.size()) {
        finallyBlock = buildBlock(blocks[finallyBlockIdx]);
    }

    return std::make_unique<TryCatchStatement>(
        std::move(tryBlock),
        std::move(catchClauses),
        std::move(finallyBlock)
    );
}

std::unique_ptr<ThrowStatement> SimpleASTBuilder::buildThrowStatement(HoocParser::ThrowStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("ThrowStatementContext is null");
    }

    if (ctx->RETHROW()) {
        return std::make_unique<ThrowStatement>(ThrowStatement::ThrowKind::RETHROW);
    }

    std::unique_ptr<Expression> expr;
    if (ctx->expression()) {
        expr = buildExpression(ctx->expression());
    } else {
        throw std::runtime_error("ThrowStatement missing expression");
    }

    return std::make_unique<ThrowStatement>(ThrowStatement::ThrowKind::THROW, std::move(expr));
}

std::unique_ptr<ForInStatement> SimpleASTBuilder::buildForInStatement(HoocParser::ForStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("ForStatementContext is null");
    }

    if (!ctx->IDENTIFIER()) {
        throw std::runtime_error("ForInStatement missing IDENTIFIER");
    }

    std::string variable = ctx->IDENTIFIER()->getText();

    // For-in loop has exactly 1 expression (the iterable)
    auto exprs = ctx->expression();
    if (exprs.size() != 1) {
        throw std::runtime_error("ForInStatement expected 1 expression, got " + std::to_string(exprs.size()));
    }

    auto iterable = buildExpression(exprs[0]);
    if (!iterable) {
        throw std::runtime_error("Failed to build iterable expression for for-in loop");
    }

    auto body = buildBlock(ctx->block());
    if (!body) {
        throw std::runtime_error("Failed to build body block for for-in loop");
    }

    return std::make_unique<ForInStatement>(variable, std::move(iterable), std::move(body));
}

std::unique_ptr<ForRangeStatement> SimpleASTBuilder::buildForRangeStatement(HoocParser::ForStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("ForStatementContext is null");
    }

    if (!ctx->IDENTIFIER()) {
        throw std::runtime_error("ForRangeStatement missing IDENTIFIER");
    }

    std::string variable = ctx->IDENTIFIER()->getText();

    // For-range loop expressions: start .. end [by step]
    auto exprs = ctx->expression();
    
    // expression(0) is start
    auto start = buildExpression(exprs[0]);
    if (!start) {
        throw std::runtime_error("Failed to build start expression for for-range loop");
    }

    // expression(1) is end
    auto end = buildExpression(exprs[1]);
    if (!end) {
        throw std::runtime_error("Failed to build end expression for for-range loop");
    }

    // expression(2) is optional step (if BY exists)
    std::unique_ptr<Expression> step;
    if (ctx->BY()) {
        if (exprs.size() < 3) {
            throw std::runtime_error("ForRangeStatement with BY missing step expression");
        }
        step = buildExpression(exprs[2]);
    }

    auto body = buildBlock(ctx->block());
    if (!body) {
        throw std::runtime_error("Failed to build body block for for-range loop");
    }

    return std::make_unique<ForRangeStatement>(variable, std::move(start), std::move(end), std::move(step), std::move(body));
}

std::unique_ptr<Expression> SimpleASTBuilder::buildExpression(HoocParser::ExpressionContext* ctx) {
    return buildAssignmentExpression(ctx->assignmentExpression());
}

std::unique_ptr<Expression> SimpleASTBuilder::buildAssignmentExpression(HoocParser::AssignmentExpressionContext* ctx) {
    auto leftVec = ctx->logicalOrExpression();
    auto left = buildLogicalOrExpression(leftVec[0]);

    if (ctx->ASSIGN() && leftVec.size() > 1) {
        auto right = buildLogicalOrExpression(leftVec[1]);
        return std::make_unique<AssignmentExpression>(std::move(left), std::move(right));
    }

    if (ctx->compoundAssignment()) {
        return buildCompoundAssignment(ctx);
    }

    return left;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildCompoundAssignment(HoocParser::AssignmentExpressionContext* ctx) {
    auto leftVec = ctx->logicalOrExpression();
    auto left = buildLogicalOrExpression(leftVec[0]);
    auto compoundCtx = ctx->compoundAssignment();

    auto& compound = compoundCtx[0];

    CompoundAssignmentOperator op;
    if (compound.COMPOUND_PLUS()) {
        op = CompoundAssignmentOperator::PLUS_ASSIGN;
    } else if (compound.COMPOUND_MINUS()) {
        op = CompoundAssignmentOperator::MINUS_ASSIGN;
    } else if (compound.COMPOUND_MULTIPLY()) {
        op = CompoundAssignmentOperator::MULTIPLY_ASSIGN;
    } else if (compound.COMPOUND_DIVIDE()) {
        op = CompoundAssignmentOperator::DIVIDE_ASSIGN;
    } else if (compound.COMPOUND_MODULO()) {
        op = CompoundAssignmentOperator::MODULO_ASSIGN;
    } else if (compound.COMPOUND_LEFT_SHIFT()) {
        op = CompoundAssignmentOperator::LEFT_SHIFT_ASSIGN;
    } else if (compound.COMPOUND_RIGHT_SHIFT()) {
        op = CompoundAssignmentOperator::RIGHT_SHIFT_ASSIGN;
    } else {
        op = CompoundAssignmentOperator::PLUS_ASSIGN;
    }

    auto rightExpr = buildCompoundAssignmentRight(&compound);
    return std::make_unique<CompoundAssignmentExpression>(std::move(left), op, std::move(rightExpr));
}

std::unique_ptr<Expression> SimpleASTBuilder::buildCompoundAssignmentRight(HoocParser::CompoundAssignmentContext* ctx) {
    auto rightVec = ctx->logicalOrExpression();
    auto& rightRef = rightVec[0];
    return buildLogicalOrExpression(&rightRef);
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
        if (ctx->ELEMENT_MULTIPLY(i-1)) op = BinaryOperator::ELEMENT_MULTIPLY;
        else if (ctx->ELEMENT_DIVIDE(i-1)) op = BinaryOperator::ELEMENT_DIVIDE;
        else if (ctx->MULTIPLY(i-1)) op = BinaryOperator::MULTIPLY;
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

    for (auto suffix : ctx->postfixSuffix()) {
        if (suffix->DOT()) {
            if (suffix->NEW()) {
                result = std::make_unique<MemberAccess>(std::move(result), "new");
            } else {
                result = std::make_unique<MemberAccess>(std::move(result), suffix->IDENTIFIER()->getText());
            }
        } else if (suffix->LBRACKET()) {
            auto index = buildExpression(suffix->expression());
            result = std::make_unique<ArrayAccess>(std::move(result), std::move(index));
        } else if (suffix->LPAREN()) {
            auto argList = suffix->argumentList()
                ? buildArgumentList(suffix->argumentList())
                : std::make_unique<ArgumentList>(std::vector<std::unique_ptr<Expression>>());

            result = std::make_unique<FunctionCall>(std::move(result), std::move(argList));
        }
    }

    for (auto augCtx : ctx->augmentedAssignment()) {
        IncrementDecrementOperator op;
        if (augCtx->INCREMENT()) {
            op = IncrementDecrementOperator::INCREMENT;
        } else {
            op = IncrementDecrementOperator::DECREMENT;
        }
        result = std::make_unique<IncrementDecrementExpression>(std::move(result), op, false);
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

std::unique_ptr<ArrayLiteral> SimpleASTBuilder::buildArrayLiteral(HoocParser::PrimaryContext* ctx) {
    std::unique_ptr<ExpressionList> elements;

    if (ctx->expressionList()) {
        elements = buildExpressionList(ctx->expressionList());
    } else {
        // Empty array []
        elements = std::make_unique<ExpressionList>(std::vector<std::unique_ptr<Expression>>());
    }

    return std::make_unique<ArrayLiteral>(std::move(elements), ctx->ANY() != nullptr);
}

std::unique_ptr<TensorLiteral> SimpleASTBuilder::buildTensorLiteral(HoocParser::PrimaryContext* ctx) {
    std::unique_ptr<ExpressionList> elements;

    if (ctx->expressionList()) {
        elements = buildExpressionList(ctx->expressionList());
    } else {
        elements = std::make_unique<ExpressionList>(std::vector<std::unique_ptr<Expression>>());
    }

    return std::make_unique<TensorLiteral>(std::move(elements));
}

std::unique_ptr<ExpressionList> SimpleASTBuilder::buildExpressionList(HoocParser::ExpressionListContext* ctx) {
    std::vector<std::unique_ptr<Expression>> expressions;
    if (ctx) {
        for (auto exprCtx : ctx->expression()) {
            auto expr = buildExpression(exprCtx);
            if (expr) {
                expressions.push_back(std::move(expr));
            }
        }
    }
    return std::make_unique<ExpressionList>(std::move(expressions));
}

std::unique_ptr<Expression> SimpleASTBuilder::buildPrimary(HoocParser::PrimaryContext* ctx) {
    if (ctx->LPAREN() && ctx->expression()) {
        auto inner = buildExpression(ctx->expression());
        auto paren = std::make_unique<ParenthesizedExpression>(std::move(inner));
        return std::make_unique<PrimaryExpression>(std::move(paren));
    } else if (ctx->LBRACKET()) {
        if (ctx->IDENTIFIER() && ctx->IDENTIFIER()->getText() == "t") {
            auto tensorLiteral = buildTensorLiteral(ctx);
            return std::make_unique<PrimaryExpression>(std::move(tensorLiteral));
        }
        auto arrayLiteral = buildArrayLiteral(ctx);
        return std::make_unique<PrimaryExpression>(std::move(arrayLiteral)); // Wrap in PrimaryExpression
    } else if (ctx->IDENTIFIER()) {
        auto identifier = std::make_unique<Identifier>(ctx->IDENTIFIER()->getText());
        return std::make_unique<PrimaryExpression>(std::move(identifier));
    } else if (ctx->THIS()) {
        auto thisLiteral = std::make_unique<ThisLiteral>();
        return std::make_unique<PrimaryExpression>(std::move(thisLiteral));
    } else if (ctx->BIT_LITERAL()) {
        int64_t value = getBitValue(ctx->BIT_LITERAL());
        auto bitLiteral = std::make_unique<BitLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(bitLiteral));
    } else if (ctx->F8_LITERAL()) {
        double value = getF8Value(ctx->F8_LITERAL());
        auto f8Literal = std::make_unique<F8Literal>(value);
        return std::make_unique<PrimaryExpression>(std::move(f8Literal));
    } else if (ctx->INTEGER_LITERAL()) {
        int64_t value = getIntValue(ctx->INTEGER_LITERAL());
        auto intLiteral = std::make_unique<IntegerLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(intLiteral));
    } else if (ctx->FLOATING_LITERAL()) {
        double value = getDoubleValue(ctx->FLOATING_LITERAL());
        auto floatingLiteral = std::make_unique<FloatingLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(floatingLiteral));
    }else if (ctx->STRING_LITERAL()) {
        std::string value = getStringValue(ctx->STRING_LITERAL());
        if (isInterpolatedString(ctx->STRING_LITERAL())) {
            auto parts = parseInterpolatedString(value);
            auto interpolatedString = std::make_unique<InterpolatedString>(std::move(parts));
            return std::make_unique<PrimaryExpression>(std::move(interpolatedString));
        }
        auto stringLiteral = std::make_unique<StringLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(stringLiteral));
    } else if (ctx->MULTILINE_STRING()) {
        std::string value = ctx->MULTILINE_STRING()->getText();
        if (value.length() >= 6 && value.substr(0, 3) == "\"\"\"" && value.substr(value.length() - 3) == "\"\"\"") {
            value = value.substr(3, value.length() - 6);
        } else {
            throw std::runtime_error("Invalid multiline string literal format: " + value);
        }
        auto stringLiteral = std::make_unique<StringLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(stringLiteral));
    } else if (ctx->CHAR_LITERAL()) {
        int64_t value = getCharValue(ctx->CHAR_LITERAL());
        auto charLiteral = std::make_unique<CharacterLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(charLiteral));
    } else if (ctx->TRUE() || ctx->FALSE()) {
        bool value = ctx->TRUE() != nullptr;
        auto boolLiteral = std::make_unique<BooleanLiteral>(value);
        return std::make_unique<PrimaryExpression>(std::move(boolLiteral));
    } else if (ctx->NULL_()) {
        auto nullLiteral = std::make_unique<NullLiteral>();
        return std::make_unique<PrimaryExpression>(std::move(nullLiteral));
    } else if (ctx->newExpression()) {
        return buildNewExpression(ctx->newExpression());
    }

    throw std::runtime_error("Unknown primary expression type encountered");
}

std::unique_ptr<Expression> SimpleASTBuilder::buildNewExpression(HoocParser::NewExpressionContext* ctx) {
    std::unique_ptr<ArgumentList> args;
    if (ctx->argumentList()) {
        args = buildArgumentList(ctx->argumentList());
    } else {
        args = std::make_unique<ArgumentList>(std::vector<std::unique_ptr<Expression>>());
    }

    if (ctx->hashMapType()) {
        return std::make_unique<NewHashMapExpression>(
            buildHashMapType(ctx->hashMapType()),
            std::move(args));
    }

    if (ctx->anyArrayType()) {
        return std::make_unique<NewObjectExpression>("AnyArray", std::move(args));
    }

    // Get the qualified class name
    auto qualifiedClassName = buildQualifiedIdentifier(ctx->qualifiedIdentifier());

    return std::make_unique<NewObjectExpression>(std::move(qualifiedClassName), std::move(args));
}

// Import building methods
std::unique_ptr<ImportStatement> SimpleASTBuilder::buildImportStatement(HoocParser::ImportStatementContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("ImportStatementContext is null");
    }
    if (auto basicCtx = dynamic_cast<HoocParser::BasicImportContext*>(ctx)) {
        return buildBasicImport(basicCtx);
    } else if (auto fromCtx = dynamic_cast<HoocParser::FromImportContext*>(ctx)) {
        return buildFromImport(fromCtx);
    }
    throw std::runtime_error("Unknown import statement type encountered");
}

std::unique_ptr<BasicImport> SimpleASTBuilder::buildBasicImport(HoocParser::BasicImportContext* ctx) {
    auto module = buildModulePath(ctx->modulePath());
    std::string alias;

    // Check if AS clause is present (IDENTIFIER after modulePath)
    if (ctx->IDENTIFIER()) {
        alias = ctx->IDENTIFIER()->getText();
    }

    return std::make_unique<BasicImport>(std::move(module), alias);
}

std::unique_ptr<FromImport> SimpleASTBuilder::buildFromImport(HoocParser::FromImportContext* ctx) {
    auto module = buildModulePath(ctx->modulePath());

    std::vector<std::unique_ptr<ImportItem>> items;
    for (auto itemCtx : ctx->importItem()) {
        auto item = buildImportItem(itemCtx);
        if (item) {
            items.push_back(std::move(item));
        }
    }

    return std::make_unique<FromImport>(std::move(module), std::move(items));
}

std::unique_ptr<ModulePath> SimpleASTBuilder::buildModulePath(HoocParser::ModulePathContext* ctx) {
    std::vector<std::string> components;
    for (auto id : ctx->IDENTIFIER()) {
        components.push_back(id->getText());
    }
    return std::make_unique<ModulePath>(std::move(components));
}

std::unique_ptr<QualifiedIdentifier> SimpleASTBuilder::buildQualifiedIdentifier(HoocParser::QualifiedIdentifierContext* ctx) {
    std::vector<std::string> components;
    for (auto id : ctx->IDENTIFIER()) {
        components.push_back(id->getText());
    }
    return std::make_unique<QualifiedIdentifier>(std::move(components));
}

std::unique_ptr<ImportItem> SimpleASTBuilder::buildImportItem(HoocParser::ImportItemContext* ctx) {
    std::string name = ctx->IDENTIFIER(0)->getText();
    std::string alias;

    // Check if AS clause is present (second IDENTIFIER)
    if (ctx->IDENTIFIER().size() > 1) {
        alias = ctx->IDENTIFIER(1)->getText();
    }

    return std::make_unique<ImportItem>(name, alias);
}

// Class building methods
std::unique_ptr<ClassDeclaration> SimpleASTBuilder::buildClassDeclaration(HoocParser::ClassDeclarationContext* ctx) {
    // Build modifiers
    std::vector<ClassModifier> modifiers;
    for (auto modifierCtx : ctx->classModifier()) {
        modifiers.push_back(getClassModifier(modifierCtx));
    }

    // Get class name
    std::string name = ctx->IDENTIFIER(0)->getText();

    // Get base class name (optional)
    std::string baseClass;
    if (ctx->EXTENDS()) {
        // The identifier after EXTENDS is the base class
        baseClass = ctx->IDENTIFIER(1)->getText();
    }

    // Build class body (which may now contain a constructor)
    auto body = buildClassBody(ctx->classBody());

    return std::make_unique<ClassDeclaration>(
        std::move(modifiers),
        name,
        baseClass,
        std::move(body)
    );
}

std::unique_ptr<ConstructorDeclaration> SimpleASTBuilder::buildConstructorDeclaration(HoocParser::ConstructorDeclarationContext* ctx) {
    std::vector<std::unique_ptr<Parameter>> parameters;
    if (ctx->parameterList()) {
        for (auto paramCtx : ctx->parameterList()->parameter()) {
            auto param = buildParameter(paramCtx);
            if (param) {
                parameters.push_back(std::move(param));
            }
        }
    }

    auto body = buildBlock(ctx->block());

    return std::make_unique<ConstructorDeclaration>(std::move(parameters), std::move(body));
}

std::unique_ptr<ClassBody> SimpleASTBuilder::buildClassBody(HoocParser::ClassBodyContext* ctx) {
    std::vector<std::unique_ptr<ClassMember>> members;
    int constructorCount = 0;

    for (auto memberCtx : ctx->classMember()) {
        auto member = buildClassMember(memberCtx);
        if (member) {
            // Check if this member is a constructor
            if (member->isConstructor()) {
                constructorCount++;
                if (constructorCount > 1) {
                    throw std::runtime_error("Class cannot have multiple constructors. Only one constructor is allowed per class.");
                }
            }
            members.push_back(std::move(member));
        }
    }
    
    // Group overloaded methods
    std::vector<std::unique_ptr<ClassMember>> groupedMembers;
    for (size_t i = 0; i < members.size(); ) {
        auto funcDecl = dynamic_cast<FunctionDeclaration*>(members[i]->getDeclaration());
        if (funcDecl) {
            std::string name = funcDecl->getName();
            std::vector<std::unique_ptr<FunctionDeclaration>> overloads;
            
            size_t j = i;
            while (j < members.size()) {
                auto nextFunc = dynamic_cast<FunctionDeclaration*>(members[j]->getDeclaration());
                if (nextFunc && nextFunc->getName() == name) {
                    nextFunc->setOverload(true);
                    overloads.push_back(std::unique_ptr<FunctionDeclaration>(
                        static_cast<FunctionDeclaration*>(members[j]->takeDeclaration().release())
                    ));
                    j++;
                } else {
                    break;
                }
            }
            
            if (overloads.size() > 1) {
                groupedMembers.push_back(std::make_unique<ClassMember>(std::make_unique<OverloadList>(std::move(overloads))));
            } else {
                overloads[0]->setOverload(false);
                groupedMembers.push_back(std::make_unique<ClassMember>(std::move(overloads[0])));
            }
            i = j;
        } else {
            groupedMembers.push_back(std::move(members[i]));
            i++;
        }
    }
    
    return std::make_unique<ClassBody>(std::move(groupedMembers));
}

std::unique_ptr<ClassMember> SimpleASTBuilder::buildClassMember(HoocParser::ClassMemberContext* ctx) {
    if (!ctx) {
        throw std::runtime_error("ClassMemberContext is null");
    }
    if (ctx->variableDeclaration()) {
        std::vector<FunctionModifier> modifiers;
        for (auto modCtx : ctx->functionModifier()) {
            modifiers.push_back(getFunctionModifier(modCtx));
        }
        auto varDecl = buildVariableDeclaration(ctx->variableDeclaration(), std::move(modifiers));
        return std::make_unique<ClassMember>(std::move(varDecl));
    } else if (ctx->constructorDeclaration()) {
        auto constructor = buildConstructorDeclaration(ctx->constructorDeclaration());
        return std::make_unique<ClassMember>(std::move(constructor));
    } else if (ctx->functionDeclaration()) {
        std::vector<FunctionModifier> modifiers;
        for (auto modCtx : ctx->functionModifier()) {
            modifiers.push_back(getFunctionModifier(modCtx));
        }
        auto decl = buildFunctionDeclaration(ctx->functionDeclaration(), std::move(modifiers));
        return std::make_unique<ClassMember>(std::move(decl));
    }
    throw std::runtime_error("Unknown class member type encountered");
}

void SimpleASTBuilder::rejectAnyTypeInPosition(const ast::Type* type, const std::string& context) {
    if (dynamic_cast<const AnyType*>(type)) {
        throw std::runtime_error("The 'any' meta type is not allowed in " + context +
            ". It may only be used as a function return type or inside container type parameters (e.g., Map<K, any>, HashMap<K, any>).");
    }
}

ClassModifier SimpleASTBuilder::getClassModifier(HoocParser::ClassModifierContext* ctx) {
    if (ctx->SINGLETON()) return ClassModifier::SINGLETON;
    if (ctx->IMMUTABLE()) return ClassModifier::IMMUTABLE;
    if (ctx->SERVICE()) return ClassModifier::SERVICE;
    if (ctx->FINAL()) return ClassModifier::FINAL;
    if (ctx->SERIALIZABLE()) return ClassModifier::SERIALIZABLE;
    throw std::runtime_error("Unknown class modifier: " + ctx->getText());
}

FunctionModifier SimpleASTBuilder::getFunctionModifier(HoocParser::FunctionModifierContext* ctx) {
    if (ctx->PUBLIC()) return FunctionModifier::PUBLIC;
    if (ctx->PRIVATE()) return FunctionModifier::PRIVATE;
    throw std::runtime_error("Unknown function modifier: " + ctx->getText());
}

// Helper methods
PrimitiveTypeKind SimpleASTBuilder::getPrimitiveTypeKind(const std::string& typeName) {
    if (typeName == "int" || typeName == "int64") return PrimitiveTypeKind::INT64;
    if (typeName == "float") return PrimitiveTypeKind::FLOAT;
    if (typeName == "double") return PrimitiveTypeKind::DOUBLE;
    if (typeName == "f64") return PrimitiveTypeKind::F64;
    if (typeName == "f8") return PrimitiveTypeKind::F8;
    if (typeName == "bit") return PrimitiveTypeKind::BIT;
    if (typeName == "bool") return PrimitiveTypeKind::BOOL;
    if (typeName == "char") return PrimitiveTypeKind::CHAR;
    if (typeName == "string") return PrimitiveTypeKind::STRING;
    if (typeName == "buffer") return PrimitiveTypeKind::BUFFER;
    if (typeName == "int8") return PrimitiveTypeKind::INT8;
    if (typeName == "byte") return PrimitiveTypeKind::BYTE;
    if (typeName == "void") return PrimitiveTypeKind::VOID;
    if (typeName.rfind("Decimal<", 0) == 0) return PrimitiveTypeKind::DECIMAL;
    throw std::runtime_error("Unknown primitive type: " + typeName);
}

std::string SimpleASTBuilder::getStringValue(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    if (text.length() >= 2 && text.front() == '"' && text.back() == '"') {
        std::string inner = text.substr(1, text.length() - 2);
        std::string result;
        result.reserve(inner.size());
        for (size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '\\' && i + 1 < inner.size()) {
                ++i;
                switch (inner[i]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'u': {
                        if (i + 4 >= inner.size()) {
                            result += inner[i];
                            break;
                        }
                        std::string hex = inner.substr(i + 1, 4);
                        char32_t cp = static_cast<char32_t>(std::stoul(hex, nullptr, 16));
                        if (cp < 0x80) {
                            result += static_cast<char>(cp);
                        } else if (cp < 0x800) {
                            result += static_cast<char>(0xC0 | (cp >> 6));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else if (cp < 0x10000) {
                            result += static_cast<char>(0xE0 | (cp >> 12));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else {
                            result += static_cast<char>(0xF0 | (cp >> 18));
                            result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        i += 4;
                        break;
                    }
                    case 'x': {
                        if (i + 2 >= inner.size()) {
                            result += inner[i];
                            break;
                        }
                        std::string hex = inner.substr(i + 1, 2);
                        char byte = static_cast<char>(std::stoul(hex, nullptr, 16));
                        result += byte;
                        i += 2;
                        break;
                    }
                    case '0': result += '\0'; break;
                    case 'v': result += '\v'; break;
                    default: result += inner[i]; break;
                }
            } else {
                result += inner[i];
            }
        }
        return result;
    }
    throw std::runtime_error("Invalid string literal format: " + text);
}

int64_t SimpleASTBuilder::getIntValue(antlr4::tree::TerminalNode* node) {
    try {
        return std::stoll(node->getText());
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse integer: " + node->getText());
    }
}

double SimpleASTBuilder::getDoubleValue(antlr4::tree::TerminalNode* node) {
    try {
        return std::stod(node->getText());
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse double: " + node->getText());
    }
}

double SimpleASTBuilder::getF8Value(antlr4::tree::TerminalNode* node) {
    try {
        std::string text = node->getText();
        if (text.size() >= 2 && text.substr(text.size() - 2) == "f8") {
            text.resize(text.size() - 2);
        }
        return std::stod(text);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse f8: " + node->getText());
    }
}

int64_t SimpleASTBuilder::getBitValue(antlr4::tree::TerminalNode* node) {
    const std::string text = node->getText();
    if (text == "0b") return 0;
    if (text == "1b") return 1;
    throw std::runtime_error("Invalid bit literal: " + text);
}

int64_t SimpleASTBuilder::getCharValue(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    if (text.length() >= 3 && text.front() == '\'' && text.back() == '\'') {
        std::string inner = text.substr(1, text.length() - 2);
        if (inner.empty()) throw std::runtime_error("Empty character literal");

        if (inner[0] == '\\' && inner.length() > 1) {
            switch (inner[1]) {
                case 'n': return '\n';
                case 'r': return '\r';
                case 't': return '\t';
                case 'b': return '\b';
                case 'f': return '\f';
                case '\\': return '\\';
                case '\'': return '\'';
                case '"': return '\"';
                case '0': return '\0';
                default: return inner[1];
            }
        }

        const unsigned char* data = (const unsigned char*)inner.c_str();
        size_t len = inner.length();
        if (len == 1) return data[0];
        if (len == 2) return ((data[0] & 0x1F) << 6) | (data[1] & 0x3F);
        if (len == 3) return ((data[0] & 0x0F) << 12) | ((data[1] & 0x3F) << 6) | (data[2] & 0x3F);
        if (len == 4) return ((data[0] & 0x07) << 18) | ((data[1] & 0x3F) << 12) | ((data[2] & 0x3F) << 6) | (data[3] & 0x3F);
        
        return data[0]; // Fallback
    }
    throw std::runtime_error("Invalid character literal format: " + text);
}

bool SimpleASTBuilder::getBoolValue(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    return text == "true";
}

bool SimpleASTBuilder::isInterpolatedString(antlr4::tree::TerminalNode* node) {
    std::string text = node->getText();
    return text.find("${") != std::string::npos;
}

std::vector<InterpolatedString::Part> SimpleASTBuilder::parseInterpolatedString(const std::string& tpl) {
    std::vector<InterpolatedString::Part> parts;
    HooParserWrapper parser;
    
    size_t i = 0;
    while (i < tpl.length()) {
        size_t start = tpl.find("${", i);
        if (start == std::string::npos) {
            parts.push_back(InterpolatedString::Part(tpl.substr(i)));
            break;
        }

        if (start > i) {
            parts.push_back(InterpolatedString::Part(tpl.substr(i, start - i)));
        }

        size_t end = tpl.find("}", start);
        if (end == std::string::npos) {
            parts.push_back(InterpolatedString::Part(tpl.substr(start)));
            break;
        }

        std::string exprText = tpl.substr(start + 2, end - start - 2);
        auto* exprCtx = parser.parseExpression(exprText);
        if (exprCtx) {
            parts.push_back(InterpolatedString::Part(buildExpression(exprCtx)));
        } else {
            // Fallback: treat failed expression as literal for now or throw error
            parts.push_back(InterpolatedString::Part("${" + exprText + "}"));
        }
        
        i = end + 1;
    }
    return parts;
}
