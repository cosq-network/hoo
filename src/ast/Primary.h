#pragma once

#include "ASTNode.h"
#include <string>
#include <cassert>

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

// f8 literal. Scalar execution stores the promoted f64 value bits; the distinct
// node preserves the language-level type for type IDs and mangling.
class F8Literal : public Primary {
public:
    F8Literal(double value) : value_(value) {}

    std::string toString() const override;

    double getValue() const { return value_; }

private:
    double value_;
};

// Bit literal (0b or 1b)
class BitLiteral : public Primary {
public:
    BitLiteral(int64_t value) : value_(value ? 1 : 0) {}

    std::string toString() const override;

    int64_t getValue() const { return value_; }

private:
    int64_t value_;
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
    CharacterLiteral(int64_t value) : value_(value) { assert(value >= 0 && value <= 0x10FFFF && "Unicode codepoint out of range"); }

    std::string toString() const override;

    int64_t getValue() const { return value_; }

private:
    int64_t value_;
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
    struct Part {
        bool isExpression;
        std::string literal;
        std::unique_ptr<Expression> expression;

        Part(const std::string& lit) : isExpression(false), literal(lit) {}
        Part(std::unique_ptr<Expression> expr) : isExpression(true), expression(std::move(expr)) {}
    };

    InterpolatedString(std::vector<Part> parts) : parts_(std::move(parts)) {}

    std::string toString() const override;

    const std::vector<Part>& getParts() const { return parts_; }

private:
    std::vector<Part> parts_;
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
