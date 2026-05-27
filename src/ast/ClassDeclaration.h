#pragma once

#include "ASTNode.h"
#include "FunctionModifier.h"
#include <vector>
#include <memory>
#include <string>

namespace hooc {
namespace ast {

class Parameter;
class Type;
class Block;

// Class modifiers
enum class ClassModifier {
    SINGLETON,
    IMMUTABLE,
    SERVICE,
    FINAL
};

// Constructor declaration (Kotlin-style)
class ConstructorDeclaration : public ASTNode {
public:
    ConstructorDeclaration(std::vector<std::unique_ptr<Parameter>> parameters,
                          std::unique_ptr<Block> body)
        : parameters_(std::move(parameters)), body_(std::move(body)) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<Parameter>>& getParameters() const { return parameters_; }
    const Block& getBody() const { return *body_; }

private:
    std::vector<std::unique_ptr<Parameter>> parameters_;
    std::unique_ptr<Block> body_;
};

// Class member (can be constructor or function)
class ClassMember : public ASTNode {
public:
    ClassMember(std::unique_ptr<ConstructorDeclaration> constructor)
        : constructor_(std::move(constructor)) {}

    ClassMember(std::unique_ptr<Declaration> declaration)
        : declaration_(std::move(declaration)) {}

    std::string toString() const override;

    const ConstructorDeclaration* getConstructor() const { return constructor_.get(); }
    const Declaration* getDeclaration() const { return declaration_.get(); }
    bool isConstructor() const { return constructor_ != nullptr; }

private:
    std::unique_ptr<ConstructorDeclaration> constructor_;
    std::unique_ptr<Declaration> declaration_;
};

// Class body
class ClassBody : public ASTNode {
public:
    ClassBody(std::vector<std::unique_ptr<ClassMember>> members)
        : members_(std::move(members)) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<ClassMember>>& getMembers() const { return members_; }

private:
    std::vector<std::unique_ptr<ClassMember>> members_;
};

// Class declaration
class ClassDeclaration : public Declaration {
public:
    ClassDeclaration(std::vector<ClassModifier> modifiers,
                    const std::string& name,
                    const std::string& baseClass,
                    std::unique_ptr<ClassBody> body)
        : modifiers_(modifiers), name_(name),
          baseClass_(baseClass), body_(std::move(body)) {}

    std::string toString() const override;

    const std::vector<ClassModifier>& getModifiers() const { return modifiers_; }
    const std::string& getName() const { return name_; }
    const std::string& getBaseClass() const { return baseClass_; }
    const ClassBody& getBody() const { return *body_; }

    bool hasModifier(ClassModifier modifier) const;
    bool hasBaseClass() const { return !baseClass_.empty(); }

private:
    std::vector<ClassModifier> modifiers_;
    std::string name_;
    std::string baseClass_;
    std::unique_ptr<ClassBody> body_;
};

} // namespace ast
} // namespace hooc