#pragma once

#include "ASTNode.h"
#include "Declaration.h"
#include "ImportStatement.h"
#include "QualifiedIdentifier.h"
#include "Type.h"
#include "FunctionModifier.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace hooc {
namespace ast {

class FFIType : public ASTNode {
public:
    virtual ~FFIType() = default;
};

class FFIPrimitiveType : public FFIType {
public:
    explicit FFIPrimitiveType(PrimitiveTypeKind kind) : kind_(kind) {}
    std::string toString() const override;
    PrimitiveTypeKind getKind() const { return kind_; }

private:
    PrimitiveTypeKind kind_;
};

class FFIQualifiedType : public FFIType {
public:
    explicit FFIQualifiedType(std::unique_ptr<QualifiedIdentifier> typeName)
        : typeName_(std::move(typeName)) {}
    std::string toString() const override;
    const QualifiedIdentifier* getTypeName() const { return typeName_.get(); }

private:
    std::unique_ptr<QualifiedIdentifier> typeName_;
};

class FFIPointerType : public FFIType {
public:
    explicit FFIPointerType(std::unique_ptr<FFIType> pointee)
        : pointee_(std::move(pointee)) {}
    std::string toString() const override;
    const FFIType* getPointee() const { return pointee_.get(); }

private:
    std::unique_ptr<FFIType> pointee_;
};

class FFIArrayType : public FFIType {
public:
    FFIArrayType(int64_t size, std::unique_ptr<FFIType> elementType)
        : size_(size), elementType_(std::move(elementType)) {}
    std::string toString() const override;
    int64_t getSize() const { return size_; }
    const FFIType* getElementType() const { return elementType_.get(); }

private:
    int64_t size_;
    std::unique_ptr<FFIType> elementType_;
};

class FFIFunctionType : public FFIType {
public:
    FFIFunctionType(std::vector<std::unique_ptr<FFIType>> params,
                    std::unique_ptr<FFIType> returnType)
        : params_(std::move(params)), returnType_(std::move(returnType)) {}
    std::string toString() const override;
    const std::vector<std::unique_ptr<FFIType>>& getParams() const { return params_; }
    const FFIType* getReturnType() const { return returnType_.get(); }

private:
    std::vector<std::unique_ptr<FFIType>> params_;
    std::unique_ptr<FFIType> returnType_;
};

class FFIParameter : public ASTNode {
public:
    FFIParameter(std::string name, std::unique_ptr<FFIType> type)
        : name_(std::move(name)), type_(std::move(type)) {}
    std::string toString() const override;
    const std::string& getName() const { return name_; }
    const FFIType* getType() const { return type_.get(); }

private:
    std::string name_;
    std::unique_ptr<FFIType> type_;
};

class FFILibraryImportDeclaration : public Declaration {
public:
    FFILibraryImportDeclaration(std::string libraryPath, std::string alias)
        : libraryPath_(std::move(libraryPath)), alias_(std::move(alias)) {}
    std::string toString() const override;
    const std::string& getLibraryPath() const { return libraryPath_; }
    const std::string& getAlias() const { return alias_; }
    bool hasAlias() const { return !alias_.empty(); }

private:
    std::string libraryPath_;
    std::string alias_;
};

class FFILinkDeclaration : public Declaration {
public:
    FFILinkDeclaration(std::unique_ptr<ModulePath> modulePath,
                       std::optional<int64_t> versionMin,
                       std::optional<int64_t> versionMax,
                       std::vector<std::string> searchPaths)
        : modulePath_(std::move(modulePath)),
          versionMin_(versionMin),
          versionMax_(versionMax),
          searchPaths_(std::move(searchPaths)) {}
    std::string toString() const override;
    const ModulePath* getModulePath() const { return modulePath_.get(); }
    const std::optional<int64_t>& getVersionMin() const { return versionMin_; }
    const std::optional<int64_t>& getVersionMax() const { return versionMax_; }
    const std::vector<std::string>& getSearchPaths() const { return searchPaths_; }

private:
    std::unique_ptr<ModulePath> modulePath_;
    std::optional<int64_t> versionMin_;
    std::optional<int64_t> versionMax_;
    std::vector<std::string> searchPaths_;
};

class FFINativeFunctionDeclaration : public Declaration {
public:
    FFINativeFunctionDeclaration(bool isExtern,
                                 std::unique_ptr<FunctionDeclaration> nativeFunction,
                                 std::unique_ptr<Type> symbolType,
                                 std::string symbolName,
                                 std::vector<std::unique_ptr<FFIParameter>> ffiParameters,
                                 std::unique_ptr<Type> returnType,
                                 std::vector<FunctionModifier> modifiers)
        : isExtern_(isExtern),
          nativeFunction_(std::move(nativeFunction)),
          symbolType_(std::move(symbolType)),
          symbolName_(std::move(symbolName)),
          ffiParameters_(std::move(ffiParameters)),
          returnType_(std::move(returnType)),
          modifiers_(std::move(modifiers)) {}
    std::string toString() const override;
    bool isExtern() const { return isExtern_; }
    const FunctionDeclaration* getNativeFunction() const { return nativeFunction_.get(); }
    const Type* getSymbolType() const { return symbolType_.get(); }
    const std::string& getSymbolName() const { return symbolName_; }
    const std::vector<std::unique_ptr<FFIParameter>>& getFfiParameters() const { return ffiParameters_; }
    const Type* getReturnType() const { return returnType_.get(); }
    const std::vector<FunctionModifier>& getModifiers() const { return modifiers_; }

private:
    bool isExtern_;
    std::unique_ptr<FunctionDeclaration> nativeFunction_;
    std::unique_ptr<Type> symbolType_;
    std::string symbolName_;
    std::vector<std::unique_ptr<FFIParameter>> ffiParameters_;
    std::unique_ptr<Type> returnType_;
    std::vector<FunctionModifier> modifiers_;
};

class FFINativeVariableDeclaration : public Declaration {
public:
    FFINativeVariableDeclaration(bool isExtern,
                                 std::unique_ptr<VariableDeclaration> variable)
        : isExtern_(isExtern), variable_(std::move(variable)) {}
    std::string toString() const override;
    bool isExtern() const { return isExtern_; }
    const VariableDeclaration* getVariable() const { return variable_.get(); }

private:
    bool isExtern_;
    std::unique_ptr<VariableDeclaration> variable_;
};

} // namespace ast
} // namespace hooc
