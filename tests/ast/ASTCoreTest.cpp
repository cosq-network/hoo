#include <gtest/gtest.h>
#include "ast/AST.h"
#include "ast/ASTNode.h"
#include "ast/QualifiedIdentifier.h"
#include "ast/FunctionModifier.h"
#include "ast/ImportStatement.h"
#include "ast/Declaration.h"
#include "ast/ClassDeclaration.h"
#include "ast/Type.h"
#include "ast/Primary.h"
#include "ast/Expression.h"
#include "ast/Statement.h"

using namespace hooc::ast;

class ASTCoreTest : public ::testing::Test {};

TEST_F(ASTCoreTest, QualifiedIdentifierSimple) {
    QualifiedIdentifier qi({std::string("foo")});
    EXPECT_EQ(qi.getName(), "foo");
    EXPECT_EQ(qi.getFullName(), "foo");
    EXPECT_EQ(qi.getComponentCount(), 1);
    EXPECT_FALSE(qi.isQualified());
    EXPECT_TRUE(qi.getModulePath().empty());
}

TEST_F(ASTCoreTest, QualifiedIdentifierQualified) {
    QualifiedIdentifier qi({std::string("std"), std::string("String")});
    EXPECT_EQ(qi.getName(), "String");
    EXPECT_EQ(qi.getFullName(), "std.String");
    EXPECT_EQ(qi.getComponentCount(), 2);
    EXPECT_TRUE(qi.isQualified());
    EXPECT_EQ(qi.getModulePath().size(), 1);
    EXPECT_EQ(qi.getModulePath()[0], "std");
    EXPECT_EQ(qi.getComponent(0), "std");
    EXPECT_EQ(qi.getComponent(1), "String");
}

TEST_F(ASTCoreTest, QualifiedIdentifierDeep) {
    QualifiedIdentifier qi({std::string("a"), std::string("b"), std::string("c")});
    EXPECT_EQ(qi.getName(), "c");
    EXPECT_EQ(qi.getFullName(), "a.b.c");
    EXPECT_EQ(qi.getComponentCount(), 3);
    EXPECT_TRUE(qi.isQualified());
    EXPECT_EQ(qi.getModulePath().size(), 2);
}

TEST_F(ASTCoreTest, QualifiedIdentifierToString) {
    QualifiedIdentifier qi({std::string("std"), std::string("io"), std::string("File")});
    EXPECT_EQ(qi.toString(), "std.io.File");
}

TEST_F(ASTCoreTest, ClassModifierToStringAll) {
    EXPECT_EQ(classModifierToString(ClassModifier::SINGLETON), "singleton");
    EXPECT_EQ(classModifierToString(ClassModifier::IMMUTABLE), "immutable");
    EXPECT_EQ(classModifierToString(ClassModifier::FACTORY), "factory");
    EXPECT_EQ(classModifierToString(ClassModifier::OBSERVABLE), "observable");
    EXPECT_EQ(classModifierToString(ClassModifier::SERVICE), "service");
    EXPECT_EQ(classModifierToString(ClassModifier::STRATEGY), "strategy");
    EXPECT_EQ(classModifierToString(ClassModifier::ACTOR), "actor");
    EXPECT_EQ(classModifierToString(ClassModifier::FINAL), "final");
}

TEST_F(ASTCoreTest, PrimitiveTypeToStringAll) {
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::INT8), "int8");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::BYTE), "byte");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::INT64), "int64");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::FLOAT), "float");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::DOUBLE), "double");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::F64), "f64");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::BOOL), "bool");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::CHAR), "char");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::STRING), "string");
    EXPECT_EQ(primitiveTypeToString(PrimitiveTypeKind::VOID), "void");
}

TEST_F(ASTCoreTest, BinaryOperatorToStringAll) {
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::MULTIPLY), "*");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::DIVIDE), "/");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::MODULO), "%");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::PLUS), "+");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::MINUS), "-");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::LESS), "<");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::LESS_EQUALS), "<=");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::GREATER), ">");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::GREATER_EQUALS), ">=");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::EQUALS), "==");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::NOT_EQUALS), "!=");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::AND), "&&");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::OR), "||");
    EXPECT_EQ(binaryOperatorToString(BinaryOperator::ASSIGN), "=");
}

TEST_F(ASTCoreTest, ModulePathBasic) {
    ModulePath mp({std::string("os"), std::string("path")});
    EXPECT_EQ(mp.toString(), "os.path");
    EXPECT_EQ(mp.getComponents().size(), 2);
}

TEST_F(ASTCoreTest, ImportItemWithAlias) {
    ImportItem item("println", "print");
    EXPECT_EQ(item.getName(), "println");
    EXPECT_EQ(item.getAlias(), "print");
    EXPECT_TRUE(item.hasAlias());
}

TEST_F(ASTCoreTest, ImportItemWithoutAlias) {
    ImportItem item("println");
    EXPECT_EQ(item.getName(), "println");
    EXPECT_FALSE(item.hasAlias());
}

TEST_F(ASTCoreTest, BasicImportWithAlias) {
    auto mp = std::make_unique<ModulePath>(std::vector<std::string>{"hoo", "io"});
    BasicImport bi(std::move(mp), "io");
    EXPECT_TRUE(bi.hasAlias());
    EXPECT_EQ(bi.getAlias(), "io");
    EXPECT_NE(bi.getModule(), nullptr);
}

TEST_F(ASTCoreTest, BasicImportWithoutAlias) {
    auto mp = std::make_unique<ModulePath>(std::vector<std::string>{"math"});
    BasicImport bi(std::move(mp));
    EXPECT_FALSE(bi.hasAlias());
}

TEST_F(ASTCoreTest, FunctionDeclarationDefaultModifiers) {
    auto returnType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::VOID);
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Statement>>{});
    FunctionDeclaration fd("test", {}, std::move(returnType), std::move(body));
    EXPECT_EQ(fd.getName(), "test");
    EXPECT_FALSE(fd.isPublic());
    EXPECT_FALSE(fd.isPrivate());
    EXPECT_FALSE(fd.isAsync());
}

TEST_F(ASTCoreTest, FunctionDeclarationWithModifiers) {
    auto returnType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Statement>>{});
    FunctionDeclaration fd("add", {}, std::move(returnType), std::move(body),
                          {FunctionModifier::PUBLIC, FunctionModifier::ASYNC});
    EXPECT_TRUE(fd.isPublic());
    EXPECT_FALSE(fd.isPrivate());
    EXPECT_TRUE(fd.isAsync());
}

TEST_F(ASTCoreTest, FunctionDeclarationWithParameter) {
    auto paramType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto param = std::make_unique<Parameter>(std::move(paramType), "x");
    std::vector<std::unique_ptr<Parameter>> params;
    params.push_back(std::move(param));
    auto returnType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::VOID);
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Statement>>{});
    FunctionDeclaration fd("fn", std::move(params), std::move(returnType), std::move(body));
    EXPECT_EQ(fd.getParameters().size(), 1);
    EXPECT_EQ(fd.getParameters()[0]->getName(), "x");
}

TEST_F(ASTCoreTest, VariableDeclarationWithExplicitType) {
    auto type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    VariableDeclaration vd(std::move(type), "count");
    EXPECT_EQ(vd.getName(), "count");
    EXPECT_NE(vd.getType(), nullptr);
    EXPECT_FALSE(vd.hasTypeInference());
    EXPECT_FALSE(vd.isGlobal());
    EXPECT_FALSE(vd.isConstant());
}

TEST_F(ASTCoreTest, VariableDeclarationTypeInference) {
    auto init = std::make_unique<PrimaryExpression>(
        std::make_unique<IntegerLiteral>(42));
    VariableDeclaration vd("x", std::move(init));
    EXPECT_EQ(vd.getName(), "x");
    EXPECT_TRUE(vd.hasTypeInference());
    EXPECT_NE(vd.getInitializer(), nullptr);
}

TEST_F(ASTCoreTest, VariableDeclarationGlobalConstant) {
    auto type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::STRING);
    VariableDeclaration vd(std::move(type), "MAX_SIZE", nullptr, true, true);
    EXPECT_TRUE(vd.isGlobal());
    EXPECT_TRUE(vd.isConstant());
}

TEST_F(ASTCoreTest, ParameterConstruction) {
    auto type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::DOUBLE);
    Parameter p(std::move(type), "value");
    EXPECT_EQ(p.getName(), "value");
    EXPECT_NE(&p.getType(), nullptr);
    EXPECT_EQ(p.toString(), "Parameter value");
}

TEST_F(ASTCoreTest, ClassDeclaration) {
    auto body = std::make_unique<ClassBody>(std::vector<std::unique_ptr<ClassMember>>{});
    ClassDeclaration decl({}, "Point", "", std::move(body));
    EXPECT_EQ(decl.getName(), "Point");
    EXPECT_FALSE(decl.hasBaseClass());
    EXPECT_FALSE(decl.hasModifier(ClassModifier::FINAL));
}

TEST_F(ASTCoreTest, ClassDeclarationWithModifierAndBase) {
    auto body = std::make_unique<ClassBody>(std::vector<std::unique_ptr<ClassMember>>{});
    ClassDeclaration decl({ClassModifier::FINAL}, "Point", "Shape", std::move(body));
    EXPECT_TRUE(decl.hasBaseClass());
    EXPECT_EQ(decl.getBaseClass(), "Shape");
    EXPECT_TRUE(decl.hasModifier(ClassModifier::FINAL));
}

TEST_F(ASTCoreTest, ConstructorDeclaration) {
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Statement>>{});
    ConstructorDeclaration cd({}, std::move(body));
    EXPECT_TRUE(cd.getParameters().empty());
    EXPECT_EQ(cd.toString(), "ConstructorDeclaration");
}

TEST_F(ASTCoreTest, ClassMemberConstructor) {
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Statement>>{});
    auto cd = std::make_unique<ConstructorDeclaration>(std::vector<std::unique_ptr<Parameter>>{}, std::move(body));
    ClassMember member(std::move(cd));
    EXPECT_TRUE(member.isConstructor());
    EXPECT_NE(member.getConstructor(), nullptr);
    EXPECT_EQ(member.getDeclaration(), nullptr);
}

TEST_F(ASTCoreTest, ClassMemberDeclaration) {
    auto returnType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::VOID);
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Statement>>{});
    auto fd = std::make_unique<FunctionDeclaration>("method", std::vector<std::unique_ptr<Parameter>>{},
        std::move(returnType), std::move(body));
    ClassMember member(std::move(fd));
    EXPECT_FALSE(member.isConstructor());
    EXPECT_EQ(member.getConstructor(), nullptr);
    EXPECT_NE(member.getDeclaration(), nullptr);
}

TEST_F(ASTCoreTest, FromImport) {
    auto mp = std::make_unique<ModulePath>(std::vector<std::string>{"hoo", "io"});
    std::vector<std::unique_ptr<ImportItem>> items;
    items.push_back(std::make_unique<ImportItem>("println"));
    items.push_back(std::make_unique<ImportItem>("readln", "read"));
    FromImport fi(std::move(mp), std::move(items));
    EXPECT_EQ(fi.getItems().size(), 2);
    EXPECT_TRUE(fi.getItems()[1]->hasAlias());
}

TEST_F(ASTCoreTest, ParenthesizedExpression) {
    auto inner = std::make_unique<PrimaryExpression>(
        std::make_unique<IntegerLiteral>(42));
    ParenthesizedExpression pe(std::move(inner));
    EXPECT_EQ(pe.toString(), "ParenthesizedExpression");
}
