#include <gtest/gtest.h>
#include "ast/AST.h"
#include "ast/Primary.h"
#include "ast/Expression.h"

using namespace hooc::ast;

class ASTTypeTest : public ::testing::Test {};

TEST_F(ASTTypeTest, PrimitiveTypeInt64) {
    PrimitiveType pt(PrimitiveTypeKind::INT64);
    EXPECT_EQ(pt.getKind(), PrimitiveTypeKind::INT64);
    EXPECT_EQ(pt.toString(), "PrimitiveType");
}

TEST_F(ASTTypeTest, PrimitiveTypeAllKinds) {
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::INT8).getKind(), PrimitiveTypeKind::INT8);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::BYTE).getKind(), PrimitiveTypeKind::BYTE);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::INT64).getKind(), PrimitiveTypeKind::INT64);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::FLOAT).getKind(), PrimitiveTypeKind::FLOAT);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::DOUBLE).getKind(), PrimitiveTypeKind::DOUBLE);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::F8).getKind(), PrimitiveTypeKind::F8);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::BIT).getKind(), PrimitiveTypeKind::BIT);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::BOOL).getKind(), PrimitiveTypeKind::BOOL);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::CHAR).getKind(), PrimitiveTypeKind::CHAR);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::STRING).getKind(), PrimitiveTypeKind::STRING);
    EXPECT_EQ(PrimitiveType(PrimitiveTypeKind::VOID).getKind(), PrimitiveTypeKind::VOID);
}

TEST_F(ASTTypeTest, BaseTypeFromPrimitive) {
    auto pt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    BaseType bt(std::move(pt));
    EXPECT_TRUE(bt.isPrimitive());
    EXPECT_NE(bt.getPrimitiveType(), nullptr);
    EXPECT_EQ(bt.getQualifiedIdentifier(), nullptr);
    EXPECT_TRUE(bt.getIdentifier().empty());
}

TEST_F(ASTTypeTest, BaseTypeFromIdentifier) {
    BaseType bt("MyClass");
    EXPECT_FALSE(bt.isPrimitive());
    EXPECT_EQ(bt.getPrimitiveType(), nullptr);
    EXPECT_NE(bt.getQualifiedIdentifier(), nullptr);
    EXPECT_EQ(bt.getIdentifier(), "MyClass");
}

TEST_F(ASTTypeTest, BaseTypeFromQualifiedIdentifier) {
    auto qi = std::make_unique<QualifiedIdentifier>(std::vector<std::string>{"std", "String"});
    BaseType bt(std::move(qi));
    EXPECT_FALSE(bt.isPrimitive());
    EXPECT_EQ(bt.getIdentifier(), "String");
    EXPECT_EQ(bt.getQualifiedIdentifier()->getFullName(), "std.String");
}

TEST_F(ASTTypeTest, ArrayTypeConstruction) {
    auto bt = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));
    ArrayType at(std::move(bt), {});
    EXPECT_EQ(at.getDimensionCount(), 0);
}

TEST_F(ASTTypeTest, ArrayTypeWithDimension) {
    auto bt = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));
    std::vector<std::unique_ptr<Expression>> dims;
    dims.push_back(std::make_unique<PrimaryExpression>(std::make_unique<IntegerLiteral>(10)));
    ArrayType at(std::move(bt), std::move(dims));
    EXPECT_EQ(at.getDimensionCount(), 1);
}

TEST_F(ASTTypeTest, OptionalTypeOptional) {
    auto bt = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::STRING));
    ArrayType at(std::move(bt), {});
    OptionalType ot(std::make_unique<ArrayType>(std::move(at)), true);
    EXPECT_TRUE(ot.isOptional());
}

TEST_F(ASTTypeTest, OptionalTypeNonOptional) {
    auto bt = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::STRING));
    ArrayType at(std::move(bt), {});
    OptionalType ot(std::make_unique<ArrayType>(std::move(at)), false);
    EXPECT_FALSE(ot.isOptional());
}

TEST_F(ASTTypeTest, MapTypeConstruction) {
    auto vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    MapType mt(MapKeyType::STRING, std::move(vt));
    EXPECT_EQ(mt.getKeyType(), MapKeyType::STRING);
    EXPECT_EQ(mt.keyTypeToString(), "string");
}

TEST_F(ASTTypeTest, MapTypeAllKeyTypes) {
    auto vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    EXPECT_EQ(MapType(MapKeyType::BYTE, std::move(vt)).keyTypeToString(), "byte");
    vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    EXPECT_EQ(MapType(MapKeyType::INT8, std::move(vt)).keyTypeToString(), "int8");
    vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    EXPECT_EQ(MapType(MapKeyType::INT64, std::move(vt)).keyTypeToString(), "int64");
    vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    EXPECT_EQ(MapType(MapKeyType::CHAR, std::move(vt)).keyTypeToString(), "char");
    vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    EXPECT_EQ(MapType(MapKeyType::STRING, std::move(vt)).keyTypeToString(), "string");
}

TEST_F(ASTTypeTest, MapTakeValueType) {
    auto vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::DOUBLE);
    MapType mt(MapKeyType::INT64, std::move(vt));
    auto taken = mt.takeValueType();
    EXPECT_NE(taken, nullptr);
}

TEST_F(ASTTypeTest, DictTypeConstruction) {
    auto vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    DictType hm(DictKeyType::INT64, std::move(vt));
    EXPECT_EQ(hm.getKeyType(), DictKeyType::INT64);
    EXPECT_EQ(hm.keyTypeToString(), "int64");
    EXPECT_EQ(hm.toString(), "DictType");
}

TEST_F(ASTTypeTest, DictTypeAllKeyTypes) {
    auto vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    EXPECT_EQ(DictType(DictKeyType::BYTE, std::move(vt)).keyTypeToString(), "byte");
    vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    EXPECT_EQ(DictType(DictKeyType::INT8, std::move(vt)).keyTypeToString(), "int8");
    vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    EXPECT_EQ(DictType(DictKeyType::INT64, std::move(vt)).keyTypeToString(), "int64");
}

TEST_F(ASTTypeTest, DictTypeWithAnyValue) {
    auto vt = std::make_unique<AnyType>();
    DictType hm(DictKeyType::INT64, std::move(vt));
    EXPECT_EQ(hm.toString(), "DictType");
    EXPECT_NE(dynamic_cast<const AnyType*>(&hm.getValueType()), nullptr);
}

TEST_F(ASTTypeTest, ListTypeConstruction) {
    ListType aat;
    EXPECT_EQ(aat.toString(), "ListType");
}

TEST_F(ASTTypeTest, AnyTypeConstruction) {
    AnyType at;
    EXPECT_EQ(at.toString(), "AnyType");
}

TEST_F(ASTTypeTest, NewDictExpressionConstruction) {
    auto vt = std::make_unique<PrimitiveType>(PrimitiveTypeKind::STRING);
    auto hmType = std::make_unique<DictType>(DictKeyType::INT64, std::move(vt));
    auto args = std::make_unique<ArgumentList>(std::vector<std::unique_ptr<Expression>>{});
    NewDictExpression expr(std::move(hmType), std::move(args));
    EXPECT_EQ(expr.toString(), "NewDictExpression");
    EXPECT_EQ(expr.getDictType().getKeyType(), DictKeyType::INT64);
    EXPECT_NE(expr.getArguments(), nullptr);
}

TEST_F(ASTTypeTest, TensorTypeConstruction) {
    auto elem = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::F8));
    std::vector<std::unique_ptr<Expression>> dims;
    dims.push_back(std::make_unique<PrimaryExpression>(std::make_unique<IntegerLiteral>(2)));
    dims.push_back(std::make_unique<PrimaryExpression>(std::make_unique<IntegerLiteral>(3)));
    TensorType tt(std::move(elem), std::move(dims));
    EXPECT_EQ(tt.toString(), "TensorType");
    EXPECT_EQ(tt.getRank(), 2u);
    EXPECT_TRUE(tt.getElementType().isPrimitive());
    EXPECT_EQ(tt.getElementType().getPrimitiveType()->getKind(), PrimitiveTypeKind::F8);
}

TEST_F(ASTTypeTest, IntegerLiteralAccessors) {
    IntegerLiteral lit(42);
    EXPECT_EQ(lit.getValue(), 42);
    EXPECT_EQ(lit.toString(), "IntegerLiteral(42)");
}

TEST_F(ASTTypeTest, IntegerLiteralNegative) {
    IntegerLiteral lit(-1);
    EXPECT_EQ(lit.getValue(), -1);
}

TEST_F(ASTTypeTest, FloatingLiteralAccessors) {
    FloatingLiteral lit(3.14);
    EXPECT_DOUBLE_EQ(lit.getValue(), 3.14);
    EXPECT_EQ(lit.toString(), "FloatingLiteral(3.140000)");
}

TEST_F(ASTTypeTest, F8LiteralAccessors) {
    F8Literal lit(1.25);
    EXPECT_DOUBLE_EQ(lit.getValue(), 1.25);
    EXPECT_EQ(lit.toString(), "F8Literal");
}

TEST_F(ASTTypeTest, BitLiteralAccessors) {
    BitLiteral one(7);
    BitLiteral zero(0);
    EXPECT_EQ(one.getValue(), 1);
    EXPECT_EQ(zero.getValue(), 0);
    EXPECT_EQ(one.toString(), "BitLiteral");
}

TEST_F(ASTTypeTest, StringLiteralAccessors) {
    StringLiteral lit("hello");
    EXPECT_EQ(lit.getValue(), "hello");
    EXPECT_FALSE(lit.isMultiline());
}

TEST_F(ASTTypeTest, StringLiteralMultiline) {
    StringLiteral lit("line1\nline2", true);
    EXPECT_EQ(lit.getValue(), "line1\nline2");
    EXPECT_TRUE(lit.isMultiline());
}

TEST_F(ASTTypeTest, CharacterLiteralAccessors) {
    CharacterLiteral lit(65);
    EXPECT_EQ(lit.getValue(), 65);
    EXPECT_EQ(lit.toString(), "CharacterLiteral(65)");
}

TEST_F(ASTTypeTest, BooleanLiteralAccessors) {
    BooleanLiteral t(true);
    BooleanLiteral f(false);
    EXPECT_TRUE(t.getValue());
    EXPECT_FALSE(f.getValue());
    EXPECT_EQ(t.toString(), "BooleanLiteral(true)");
    EXPECT_EQ(f.toString(), "BooleanLiteral(false)");
}

TEST_F(ASTTypeTest, NullLiteral) {
    NullLiteral nl;
    EXPECT_EQ(nl.toString(), "NullLiteral(null)");
}

TEST_F(ASTTypeTest, ThisLiteral) {
    ThisLiteral tl;
    EXPECT_EQ(tl.toString(), "ThisLiteral(this)");
}

TEST_F(ASTTypeTest, InterpolatedString) {
    std::vector<InterpolatedString::Part> parts;
    parts.push_back(InterpolatedString::Part("Hello "));
    auto nameId = std::make_unique<Identifier>("name");
    std::unique_ptr<Expression> nameExpr = std::make_unique<PrimaryExpression>(std::move(nameId));
    parts.push_back(InterpolatedString::Part(std::move(nameExpr)));
    InterpolatedString is(std::move(parts));
    EXPECT_EQ(is.getParts().size(), 2u);
    EXPECT_EQ(is.toString(), "InterpolatedString(\"Hello \", expr(PrimaryExpression(Identifier(name))))");
}

TEST_F(ASTTypeTest, IdentifierPrimary) {
    Identifier id("myVar");
    EXPECT_EQ(id.getName(), "myVar");
    EXPECT_EQ(id.toString(), "Identifier(myVar)");
}
