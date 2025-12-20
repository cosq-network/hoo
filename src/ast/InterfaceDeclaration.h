#pragma once

#include "ASTNode.h"
#include "Declaration.h"
#include <vector>
#include <string>

namespace hooc {
namespace ast {

class Type;
class Parameter;

// Function signature (for interfaces)
class FunctionSignature : public ASTNode {
public:
    FunctionSignature(const std::string& name,
                     std::vector<std::unique_ptr<Parameter>> parameters,
                     std::unique_ptr<Type> returnType)
        : name_(name), parameters_(std::move(parameters)), returnType_(std::move(returnType)) {}

    std::string toString() const override;

    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<Parameter>>& getParameters() const { return parameters_; }
    const Type* getReturnType() const { return returnType_.get(); }

private:
    std::string name_;
    std::vector<std::unique_ptr<Parameter>> parameters_;
    std::unique_ptr<Type> returnType_;
};

// Interface member
class InterfaceMember : public ASTNode {
public:
    InterfaceMember(std::unique_ptr<FunctionSignature> signature)
        : signature_(std::move(signature)) {}

    std::string toString() const override;

    const FunctionSignature& getSignature() const { return *signature_; }

private:
    std::unique_ptr<FunctionSignature> signature_;
};

// Interface declaration
class InterfaceDeclaration : public Declaration {
public:
    InterfaceDeclaration(const std::string& name,
                        std::vector<std::unique_ptr<InterfaceMember>> members)
        : name_(name), members_(std::move(members)) {}

    std::string toString() const override;

    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<InterfaceMember>>& getMembers() const { return members_; }

private:
    std::string name_;
    std::vector<std::unique_ptr<InterfaceMember>> members_;
};

} // namespace ast
} // namespace hooc