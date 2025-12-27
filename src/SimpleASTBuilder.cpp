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

    // Process imports
    std::vector<std::unique_ptr<ImportStatement>> imports;
    for (auto importCtx : ctx->importStatement()) {
        auto import = buildImportStatement(importCtx);
        if (import) {
            imports.push_back(std::move(import));
        }
    }

    // Process declarations
    std::vector<std::unique_ptr<Declaration>> declarations;
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
    } else if (ctx->classDeclaration()) {
        return buildClassDeclaration(ctx->classDeclaration());
    } else if (ctx->interfaceDeclaration()) {
        return buildInterfaceDeclaration(ctx->interfaceDeclaration());
    }
    return nullptr;
}

std::unique_ptr<VariableDeclaration> SimpleASTBuilder::buildVariableDeclaration(HoocParser::VariableDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();

    if (ctx->type()) {
        // Explicit type: var x: type or var x: type = expr
        auto type = buildType(ctx->type());
        std::unique_ptr<Expression> initializer;
        if (ctx->expression()) {
            initializer = buildExpression(ctx->expression());
        }
        return std::make_unique<VariableDeclaration>(std::move(type), name, std::move(initializer));
    } else {
        // Type inference: var x = expr
        auto initializer = buildExpression(ctx->expression());
        return std::make_unique<VariableDeclaration>(name, std::move(initializer));
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
                                               std::move(returnType), std::move(body));
}

std::unique_ptr<Parameter> SimpleASTBuilder::buildParameter(HoocParser::ParameterContext* ctx) {
    // Grammar: IDENTIFIER COLON type
    std::string name = ctx->IDENTIFIER()->getText();
    auto type = buildType(ctx->type());
    return std::make_unique<Parameter>(std::move(type), name);
}

std::unique_ptr<Type> SimpleASTBuilder::buildType(HoocParser::TypeContext* ctx) {
    if (!ctx || !ctx->unionType()) {
        return std::make_unique<BaseType>("unknown");
    }

    auto unionCtx = ctx->unionType();
    auto optionalTypes = unionCtx->optionalType();

    // Check if this is actually a union (multiple types with |) or a single type
    if (optionalTypes.size() > 1) {
        // This is a union type - build and return UnionType
        return buildUnionType(unionCtx);
    } else if (optionalTypes.size() == 1) {
        // Single type - maintain backward compatibility by returning ArrayType/BaseType directly
        auto optCtx = optionalTypes[0];
        if (!optCtx->arrayType()) {
            return std::make_unique<BaseType>("unknown");
        }

        auto arrayCtx = optCtx->arrayType();

        // Check if this is actually an array (has brackets) or just a base type
        if (arrayCtx->LBRACKET().empty()) {
            // No brackets - just a base type like "int64"
            if (arrayCtx->baseType()) {
                return buildBaseType(arrayCtx->baseType());
            }
        } else {
            // Has brackets - this is an array type like "int64[]" or "int64[]?"
            return buildArrayType(arrayCtx);
        }
    }

    return std::make_unique<BaseType>("unknown");
}

std::unique_ptr<UnionType> SimpleASTBuilder::buildUnionType(HoocParser::UnionTypeContext* ctx) {
    if (!ctx) {
        return nullptr;
    }

    auto optionalTypes = ctx->optionalType();
    if (optionalTypes.empty()) {
        return nullptr;
    }

    // Build all optional types in the union
    std::vector<std::unique_ptr<OptionalType>> types;
    for (auto optTypeCtx : optionalTypes) {
        auto optType = buildOptionalType(optTypeCtx);
        if (optType) {
            types.push_back(std::move(optType));
        }
    }

    if (types.empty()) {
        return nullptr;
    }

    return std::make_unique<UnionType>(std::move(types));
}

std::unique_ptr<OptionalType> SimpleASTBuilder::buildOptionalType(HoocParser::OptionalTypeContext* ctx) {
    if (!ctx || !ctx->arrayType()) {
        return nullptr;
    }

    auto arrayCtx = ctx->arrayType();
    auto arrayType = buildArrayType(arrayCtx);

    if (!arrayType) {
        return nullptr;
    }

    // Check if this optional type has a QUESTION mark (making it nullable)
    bool isOptional = ctx->QUESTION() != nullptr;

    return std::make_unique<OptionalType>(std::move(arrayType), isOptional);
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

std::unique_ptr<ArrayType> SimpleASTBuilder::buildArrayType(HoocParser::ArrayTypeContext* ctx) {
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
    if (ctx->variableDeclaration()) {
        return buildVariableDeclarationStatement(ctx->variableDeclaration());
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
    return buildAssignmentExpression(ctx->assignmentExpression());
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

std::unique_ptr<ArrayLiteral> SimpleASTBuilder::buildArrayLiteral(HoocParser::PrimaryContext* ctx) {
    std::unique_ptr<ExpressionList> elements;

    if (ctx->expressionList()) {
        elements = buildExpressionList(ctx->expressionList());
    } else {
        // Empty array []
        elements = std::make_unique<ExpressionList>(std::vector<std::unique_ptr<Expression>>());
    }

    return std::make_unique<ArrayLiteral>(std::move(elements));
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
        // Parenthesized expression
        return buildExpression(ctx->expression());
    } else if (ctx->LBRACKET()) {
        auto arrayLiteral = buildArrayLiteral(ctx);
        return std::make_unique<PrimaryExpression>(std::move(arrayLiteral)); // Wrap in PrimaryExpression
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
    } else if (ctx->NULL_()) {
        auto nullLiteral = std::make_unique<NullLiteral>();
        return std::make_unique<PrimaryExpression>(std::move(nullLiteral));
    } else if (ctx->newExpression()) {
        return buildNewExpression(ctx->newExpression());
    }

    return nullptr;
}

std::unique_ptr<Expression> SimpleASTBuilder::buildNewExpression(HoocParser::NewExpressionContext* ctx) {
    // Get the class name
    std::string className = ctx->IDENTIFIER()->getText();

    // Build argument list if present
    std::unique_ptr<ArgumentList> args;
    if (ctx->argumentList()) {
        args = buildArgumentList(ctx->argumentList());
    } else {
        // Empty argument list
        args = std::make_unique<ArgumentList>(std::vector<std::unique_ptr<Expression>>());
    }

    return std::make_unique<NewObjectExpression>(className, std::move(args));
}

// Import building methods
std::unique_ptr<ImportStatement> SimpleASTBuilder::buildImportStatement(HoocParser::ImportStatementContext* ctx) {
    if (auto basicCtx = dynamic_cast<HoocParser::BasicImportContext*>(ctx)) {
        return buildBasicImport(basicCtx);
    } else if (auto fromCtx = dynamic_cast<HoocParser::FromImportContext*>(ctx)) {
        return buildFromImport(fromCtx);
    }
    return nullptr;
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

std::unique_ptr<ImportItem> SimpleASTBuilder::buildImportItem(HoocParser::ImportItemContext* ctx) {
    std::string name = ctx->IDENTIFIER(0)->getText();
    std::string alias;

    // Check if AS clause is present (second IDENTIFIER)
    if (ctx->IDENTIFIER().size() > 1) {
        alias = ctx->IDENTIFIER(1)->getText();
    }

    return std::make_unique<ImportItem>(name, alias);
}

// Class and interface building methods
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

    // Get interface names (optional)
    std::vector<std::string> interfaces;
    if (ctx->interfaceList()) {
        for (auto idNode : ctx->interfaceList()->IDENTIFIER()) {
            interfaces.push_back(idNode->getText());
        }
    }

    // Build class body (which may now contain a constructor)
    auto body = buildClassBody(ctx->classBody());

    return std::make_unique<ClassDeclaration>(
        std::move(modifiers),
        name,
        baseClass,
        std::move(interfaces),
        std::move(body)
    );
}

std::unique_ptr<InterfaceDeclaration> SimpleASTBuilder::buildInterfaceDeclaration(HoocParser::InterfaceDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();

    std::vector<std::unique_ptr<InterfaceMember>> members;
    for (auto memberCtx : ctx->interfaceMember()) {
        auto member = buildInterfaceMember(memberCtx);
        if (member) {
            members.push_back(std::move(member));
        }
    }

    return std::make_unique<InterfaceDeclaration>(name, std::move(members));
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
    return std::make_unique<ClassBody>(std::move(members));
}

std::unique_ptr<ClassMember> SimpleASTBuilder::buildClassMember(HoocParser::ClassMemberContext* ctx) {
    if (ctx->constructorDeclaration()) {
        auto constructor = buildConstructorDeclaration(ctx->constructorDeclaration());
        return std::make_unique<ClassMember>(std::move(constructor));
    } else if (ctx->functionDeclaration()) {
        auto decl = buildFunctionDeclaration(ctx->functionDeclaration());
        return std::make_unique<ClassMember>(std::move(decl));
    } else if (ctx->eventDeclaration()) {
        auto event = buildEventDeclaration(ctx->eventDeclaration());
        return std::make_unique<ClassMember>(std::move(event));
    }
    return nullptr;
}

std::unique_ptr<EventDeclaration> SimpleASTBuilder::buildEventDeclaration(HoocParser::EventDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    return std::make_unique<EventDeclaration>(name);
}

std::unique_ptr<InterfaceMember> SimpleASTBuilder::buildInterfaceMember(HoocParser::InterfaceMemberContext* ctx) {
    auto signature = buildFunctionSignature(ctx->functionSignature());
    return std::make_unique<InterfaceMember>(std::move(signature));
}

std::unique_ptr<FunctionSignature> SimpleASTBuilder::buildFunctionSignature(HoocParser::FunctionSignatureContext* ctx) {
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

    auto returnType = buildType(ctx->type());

    return std::make_unique<FunctionSignature>(name, std::move(parameters), std::move(returnType));
}

ClassModifier SimpleASTBuilder::getClassModifier(HoocParser::ClassModifierContext* ctx) {
    if (ctx->SINGLETON()) return ClassModifier::SINGLETON;
    if (ctx->IMMUTABLE()) return ClassModifier::IMMUTABLE;
    if (ctx->FACTORY()) return ClassModifier::FACTORY;
    if (ctx->OBSERVABLE()) return ClassModifier::OBSERVABLE;
    if (ctx->SERVICE()) return ClassModifier::SERVICE;
    if (ctx->STRATEGY()) return ClassModifier::STRATEGY;
    if (ctx->ACTOR()) return ClassModifier::ACTOR;
    if (ctx->FINAL()) return ClassModifier::FINAL;
    return ClassModifier::FINAL; // Default fallback
}

// Helper methods
PrimitiveTypeKind SimpleASTBuilder::getPrimitiveTypeKind(const std::string& typeName) {
    if (typeName == "int" || typeName == "int64") return PrimitiveTypeKind::INT64;
    if (typeName == "float") return PrimitiveTypeKind::FLOAT;
    if (typeName == "double") return PrimitiveTypeKind::DOUBLE;
    if (typeName == "f64") return PrimitiveTypeKind::F64;
    if (typeName == "bool") return PrimitiveTypeKind::BOOL;
    if (typeName == "char") return PrimitiveTypeKind::CHAR;
    if (typeName == "string") return PrimitiveTypeKind::STRING;
    if (typeName == "byte") return PrimitiveTypeKind::BYTE;
    if (typeName == "uint8") return PrimitiveTypeKind::UINT8;
    if (typeName == "void") return PrimitiveTypeKind::VOID;
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