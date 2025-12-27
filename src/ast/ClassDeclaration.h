#pragma once

#include "ASTNode.h"
#include "Declaration.h"
#include <vector>
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
    FACTORY,
    OBSERVABLE,
    SERVICE,
    STRATEGY,
    ACTOR,
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

// Event declaration
class EventDeclaration : public ASTNode {
public:
    EventDeclaration(const std::string& name) : name_(name) {}

    std::string toString() const override;

    const std::string& getName() const { return name_; }

private:
    std::string name_;
};

// Class member (can be constructor, function, or event)
class ClassMember : public ASTNode {
public:
    ClassMember(std::unique_ptr<ConstructorDeclaration> constructor)
        : constructor_(std::move(constructor)) {}

    ClassMember(std::unique_ptr<Declaration> declaration)
        : declaration_(std::move(declaration)) {}

    ClassMember(std::unique_ptr<EventDeclaration> event)
        : event_(std::move(event)) {}

    std::string toString() const override;

    const ConstructorDeclaration* getConstructor() const { return constructor_.get(); }
    const Declaration* getDeclaration() const { return declaration_.get(); }
    const EventDeclaration* getEvent() const { return event_.get(); }
    bool isConstructor() const { return constructor_ != nullptr; }
    bool isEvent() const { return event_ != nullptr; }

private:
    std::unique_ptr<ConstructorDeclaration> constructor_;
    std::unique_ptr<Declaration> declaration_;
    std::unique_ptr<EventDeclaration> event_;
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
                    std::vector<std::string> typeParameters,
                    const std::string& baseClass,
                    std::vector<std::string> interfaces,
                    std::unique_ptr<ClassBody> body)
        : modifiers_(modifiers), name_(name),
          typeParameters_(std::move(typeParameters)),
          baseClass_(baseClass), interfaces_(interfaces), body_(std::move(body)) {}

    std::string toString() const override;

    const std::vector<ClassModifier>& getModifiers() const { return modifiers_; }
    const std::string& getName() const { return name_; }
    const std::vector<std::string>& getTypeParameters() const { return typeParameters_; }
    const std::string& getBaseClass() const { return baseClass_; }
    const std::vector<std::string>& getInterfaces() const { return interfaces_; }
    const ClassBody& getBody() const { return *body_; }

    bool hasModifier(ClassModifier modifier) const;
    bool isGeneric() const { return !typeParameters_.empty(); }
    bool hasBaseClass() const { return !baseClass_.empty(); }
    bool hasInterfaces() const { return !interfaces_.empty(); }

private:
    std::vector<ClassModifier> modifiers_;
    std::string name_;
    std::vector<std::string> typeParameters_;
    std::string baseClass_;
    std::vector<std::string> interfaces_;
    std::unique_ptr<ClassBody> body_;
};

} // namespace ast
} // namespace hooc