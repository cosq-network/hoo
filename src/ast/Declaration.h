#pragma once

#include "ASTNode.h"
#include "FunctionModifier.h"
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
                       std::unique_ptr<Block> body,
                       std::vector<FunctionModifier> modifiers = {})
        : name_(name), parameters_(std::move(parameters)),
          returnType_(std::move(returnType)), body_(std::move(body)),
          modifiers_(std::move(modifiers)) {}

    std::string toString() const override;

    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<Parameter>>& getParameters() const { return parameters_; }
    const Type* getReturnType() const { return returnType_.get(); }
    const Block& getBody() const { return *body_; }
    const std::vector<FunctionModifier>& getModifiers() const { return modifiers_; }

    bool isPublic() const {
        return std::find(modifiers_.begin(), modifiers_.end(), FunctionModifier::PUBLIC) != modifiers_.end();
    }

    bool isPrivate() const {
        return std::find(modifiers_.begin(), modifiers_.end(), FunctionModifier::PRIVATE) != modifiers_.end();
    }

    bool isAsync() const {
        return std::find(modifiers_.begin(), modifiers_.end(), FunctionModifier::ASYNC) != modifiers_.end();
    }

    bool isOverload() const { return is_overload_; }
    void setOverload(bool val) { is_overload_ = val; }

private:
    std::string name_;
    std::vector<std::unique_ptr<Parameter>> parameters_;
    std::unique_ptr<Type> returnType_;
    std::unique_ptr<Block> body_;
    std::vector<FunctionModifier> modifiers_;
    bool is_overload_ = false;
};

// Overload list
class OverloadList : public Declaration {
public:
    OverloadList(std::vector<std::unique_ptr<FunctionDeclaration>> functions)
        : functions_(std::move(functions)) {}

    std::string toString() const override {
        std::string res = "OverloadList {\n";
        for (const auto& func : functions_) {
            res += func->toString() + "\n";
        }
        res += "}";
        return res;
    }

    const std::vector<std::unique_ptr<FunctionDeclaration>>& getFunctions() const { return functions_; }

private:
    std::vector<std::unique_ptr<FunctionDeclaration>> functions_;
};

// Variable declaration
class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(std::unique_ptr<Type> type, const std::string& name,
                       std::unique_ptr<Expression> initializer = nullptr,
                       bool isGlobal = false, bool isConstant = false,
                       std::vector<FunctionModifier> modifiers = {})
        : type_(std::move(type)), name_(name), 
          initializer_(std::move(initializer)), 
          isGlobal_(isGlobal), isConstant_(isConstant),
          modifiers_(std::move(modifiers)) {}

    // Constructor for 'var' declarations with type inference
    VariableDeclaration(const std::string& name, std::unique_ptr<Expression> initializer,
                       bool isGlobal = false, bool isConstant = false,
                       std::vector<FunctionModifier> modifiers = {})
        : type_(nullptr), name_(name), 
          initializer_(std::move(initializer)), 
          isGlobal_(isGlobal), isConstant_(isConstant),
          modifiers_(std::move(modifiers)) {}

    std::string toString() const override;

    const Type* getType() const { return type_.get(); }
    const std::string& getName() const { return name_; }
    const Expression* getInitializer() const { return initializer_.get(); }
    bool hasTypeInference() const { return type_ == nullptr; }
    bool isGlobal() const { return isGlobal_; }
    void setGlobal(bool global) { isGlobal_ = global; }
    bool isConstant() const { return isConstant_; }
    void setConstant(bool constant) { isConstant_ = constant; }

    bool isPublic() const {
        return std::find(modifiers_.begin(), modifiers_.end(), FunctionModifier::PUBLIC) != modifiers_.end();
    }

    bool isPrivate() const {
        return std::find(modifiers_.begin(), modifiers_.end(), FunctionModifier::PRIVATE) != modifiers_.end();
    }

    const std::vector<FunctionModifier>& getModifiers() const { return modifiers_; }

private:
    std::unique_ptr<Type> type_;
    std::string name_;
    std::unique_ptr<Expression> initializer_;
    bool isGlobal_;
    bool isConstant_;
    std::vector<FunctionModifier> modifiers_;
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