#pragma once

#include "ASTNode.h"
#include <string>

namespace hooc {
namespace ast {

class Expression;

// Base class for primary expressions
class Primary : public ASTNode {
public:
    virtual ~Primary() = default;
};

// Identifier
class Identifier : public Primary {
public:
    Identifier(const std::string& name) : name_(name) {}

    std::string toString() const override;

    const std::string& getName() const { return name_; }

private:
    std::string name_;
};

// Integer literal
class IntegerLiteral : public Primary {
public:
    IntegerLiteral(int64_t value) : value_(value) {}

    std::string toString() const override;

    int64_t getValue() const { return value_; }

private:
    int64_t value_;
};

// Floating literal
class FloatingLiteral : public Primary {
public:
    FloatingLiteral(double value) : value_(value) {}

    std::string toString() const override;

    double getValue() const { return value_; }

private:
    double value_;
};

// String literal
class StringLiteral : public Primary {
public:
    StringLiteral(const std::string& value, bool isMultiline = false)
        : value_(value), isMultiline_(isMultiline) {}

    std::string toString() const override;

    const std::string& getValue() const { return value_; }
    bool isMultiline() const { return isMultiline_; }

private:
    std::string value_;
    bool isMultiline_;
};

// Character literal
class CharacterLiteral : public Primary {
public:
    CharacterLiteral(char value) : value_(value) {}

    std::string toString() const override;

    char getValue() const { return value_; }

private:
    char value_;
};

// Boolean literal
class BooleanLiteral : public Primary {
public:
    BooleanLiteral(bool value) : value_(value) {}

    std::string toString() const override;

    bool getValue() const { return value_; }

private:
    bool value_;
};

// Null literal
class NullLiteral : public Primary {
public:
    NullLiteral() {}

    std::string toString() const override;
};

// This literal
class ThisLiteral : public Primary {
public:
    ThisLiteral() {}

    std::string toString() const override;
};

// Interpolated string - supports "text ${expr} more text"
class InterpolatedString : public Primary {
public:
    InterpolatedString(const std::string& template_) : template_(template_) {}

    std::string toString() const override;

    const std::string& getTemplate() const { return template_; }

private:
    std::string template_;
};

// Parenthesized expression
class ParenthesizedExpression : public Primary {
public:
    ParenthesizedExpression(std::unique_ptr<Expression> expression)
        : expression_(std::move(expression)) {}

    std::string toString() const override;

    const Expression& getExpression() const { return *expression_; }

private:
    std::unique_ptr<Expression> expression_;
};

} // namespace ast
} // namespace hooc