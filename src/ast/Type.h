#pragma once

#include "ASTNode.h"
#include "QualifiedIdentifier.h"
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
    INT8,
    BYTE,
    INT64,
    FLOAT,
    DOUBLE,
    F64,
    F8,
    BIT,
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
        : primitiveType_(std::move(primitiveType)), identifier_(nullptr) {}

    BaseType(const std::string& identifier)
        : primitiveType_(nullptr), identifier_(std::make_unique<QualifiedIdentifier>(std::vector<std::string>{identifier})) {}

    BaseType(std::unique_ptr<QualifiedIdentifier> identifier)
        : primitiveType_(nullptr), identifier_(std::move(identifier)) {}

    std::string toString() const override;

    const PrimitiveType* getPrimitiveType() const { return primitiveType_.get(); }

    // Get simple identifier name (backward compatible)
    std::string getIdentifier() const {
        return identifier_ ? identifier_->getName() : "";
    }

    // Get qualified identifier (new API)
    const QualifiedIdentifier* getQualifiedIdentifier() const { return identifier_.get(); }

    bool isPrimitive() const { return primitiveType_ != nullptr; }

private:
    std::unique_ptr<PrimitiveType> primitiveType_;
    std::unique_ptr<QualifiedIdentifier> identifier_;
};

// Array type
class ArrayType : public Type {
public:
    ArrayType(std::unique_ptr<BaseType> baseType,
             std::vector<std::unique_ptr<Expression>> dimensions)
        : baseType_(std::move(baseType)), dimensions_(std::move(dimensions)) {}

    std::string toString() const override;

    const BaseType& getBaseType() const { return *baseType_; }
    std::unique_ptr<BaseType> takeBaseType() { return std::move(baseType_); }
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
    std::unique_ptr<ArrayType> takeArrayType() { return std::move(arrayType_); }
    bool isOptional() const { return isOptional_; }

private:
    std::unique_ptr<ArrayType> arrayType_;
    bool isOptional_;
};

// Map key types (restricted to specific primitive types)
enum class MapKeyType {
    BYTE,
    INT8,
    INT64,
    CHAR,
    STRING
};

// Map type (map[K, V] where K is restricted key type)
class MapType : public Type {
public:
    MapType(MapKeyType keyType, std::unique_ptr<Type> valueType)
        : keyType_(keyType), valueType_(std::move(valueType)) {}

    std::string toString() const override;

    MapKeyType getKeyType() const { return keyType_; }
    const Type& getValueType() const { return *valueType_; }
    std::unique_ptr<Type> takeValueType() { return std::move(valueType_); }

    // Convert key type to string for runtime
    std::string keyTypeToString() const {
        switch (keyType_) {
            case MapKeyType::BYTE: return "byte";
            case MapKeyType::INT8: return "int8";
            case MapKeyType::INT64: return "int64";
            case MapKeyType::CHAR: return "char";
            case MapKeyType::STRING: return "string";
            default: return "unknown";
        }
    }

private:
    MapKeyType keyType_;
    std::unique_ptr<Type> valueType_;
};

} // namespace ast
} // namespace hooc
