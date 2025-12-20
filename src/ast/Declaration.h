#pragma once

#include "ASTNode.h"
#include <vector>
#include <string>

namespace hooc {
namespace ast {

class Type;
class Block;
class Parameter;
class Expression;

// Base class for all declarations
class Declaration : public ASTNode {
public:
    virtual ~Declaration() = default;
};

// Function declaration
class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(const std::string& name,
                       std::vector<std::unique_ptr<Parameter>> parameters,
                       std::unique_ptr<Type> returnType,
                       std::unique_ptr<Block> body)
        : name_(name), parameters_(std::move(parameters)), 
          returnType_(std::move(returnType)), body_(std::move(body)) {}

    std::string toString() const override;

    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<Parameter>>& getParameters() const { return parameters_; }
    const Type* getReturnType() const { return returnType_.get(); }
    const Block& getBody() const { return *body_; }

private:
    std::string name_;
    std::vector<std::unique_ptr<Parameter>> parameters_;
    std::unique_ptr<Type> returnType_;
    std::unique_ptr<Block> body_;
};

// Variable declaration
class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(std::unique_ptr<Type> type, const std::string& name,
                       std::unique_ptr<Expression> initializer = nullptr)
        : type_(std::move(type)), name_(name), initializer_(std::move(initializer)) {}

    // Constructor for 'var' declarations with type inference
    VariableDeclaration(const std::string& name, std::unique_ptr<Expression> initializer)
        : type_(nullptr), name_(name), initializer_(std::move(initializer)) {}

    std::string toString() const override;

    const Type* getType() const { return type_.get(); }
    const std::string& getName() const { return name_; }
    const Expression* getInitializer() const { return initializer_.get(); }
    bool hasTypeInference() const { return type_ == nullptr; }

private:
    std::unique_ptr<Type> type_;
    std::string name_;
    std::unique_ptr<Expression> initializer_;
};

// Function parameter
class Parameter : public ASTNode {
public:
    Parameter(std::unique_ptr<Type> type, const std::string& name)
        : type_(std::move(type)), name_(name) {}

    std::string toString() const override;

    const Type& getType() const { return *type_; }
    const std::string& getName() const { return name_; }

private:
    std::unique_ptr<Type> type_;
    std::string name_;
};

} // namespace ast
} // namespace hooc