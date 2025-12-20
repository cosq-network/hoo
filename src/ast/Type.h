#pragma once

#include "ASTNode.h"
#include <vector>
#include <string>

namespace hooc {
namespace ast {

class Expression;

// Base class for all types
class Type : public ASTNode {
public:
    virtual ~Type() = default;
};

// Primitive types
enum class PrimitiveTypeKind {
    BYTE,
    UINT8,
    INT64,
    FLOAT,
    DOUBLE,
    F64,
    BOOL,
    CHAR,
    STRING,
    VOID
};

class PrimitiveType : public Type {
public:
    PrimitiveType(PrimitiveTypeKind kind) : kind_(kind) {}

    std::string toString() const override;

    PrimitiveTypeKind getKind() const { return kind_; }

private:
    PrimitiveTypeKind kind_;
};

// Base type (primitive or identifier)
class BaseType : public Type {
public:
    BaseType(std::unique_ptr<PrimitiveType> primitiveType)
        : primitiveType_(std::move(primitiveType)), identifier_("") {}

    BaseType(const std::string& identifier)
        : primitiveType_(nullptr), identifier_(identifier) {}

    std::string toString() const override;

    const PrimitiveType* getPrimitiveType() const { return primitiveType_.get(); }
    const std::string& getIdentifier() const { return identifier_; }
    bool isPrimitive() const { return primitiveType_ != nullptr; }

private:
    std::unique_ptr<PrimitiveType> primitiveType_;
    std::string identifier_;
};

// Array type
class ArrayType : public Type {
public:
    ArrayType(std::unique_ptr<BaseType> baseType,
             std::vector<std::unique_ptr<Expression>> dimensions)
        : baseType_(std::move(baseType)), dimensions_(std::move(dimensions)) {}

    std::string toString() const override;

    const BaseType& getBaseType() const { return *baseType_; }
    const std::vector<std::unique_ptr<Expression>>& getDimensions() const { return dimensions_; }
    size_t getDimensionCount() const { return dimensions_.size(); }

private:
    std::unique_ptr<BaseType> baseType_;
    std::vector<std::unique_ptr<Expression>> dimensions_;
};

// Optional type (type?)
class OptionalType : public Type {
public:
    OptionalType(std::unique_ptr<ArrayType> arrayType, bool isOptional)
        : arrayType_(std::move(arrayType)), isOptional_(isOptional) {}

    std::string toString() const override;

    const ArrayType& getArrayType() const { return *arrayType_; }
    bool isOptional() const { return isOptional_; }

private:
    std::unique_ptr<ArrayType> arrayType_;
    bool isOptional_;
};

// Union type (type | type | ...)
class UnionType : public Type {
public:
    UnionType(std::vector<std::unique_ptr<OptionalType>> types)
        : types_(std::move(types)) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<OptionalType>>& getTypes() const { return types_; }
    bool isUnion() const { return types_.size() > 1; }

private:
    std::vector<std::unique_ptr<OptionalType>> types_;
};

} // namespace ast
} // namespace hooc