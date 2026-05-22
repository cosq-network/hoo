#pragma once

#include "ASTNode.h"
#include "QualifiedIdentifier.h"
#include <vector>
#include <string>

namespace hooc {
namespace ast {

class Type;
class Block;
class Parameter;

// Base class for all expressions
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

// Primary expression
class PrimaryExpression : public Expression {
public:
    PrimaryExpression(std::unique_ptr<ASTNode> primary) : primary_(std::move(primary)) {}

    std::string toString() const override;

    const ASTNode& getPrimary() const { return *primary_; }

private:
    std::unique_ptr<ASTNode> primary_;
};

// Member access (obj.member)
class MemberAccess : public Expression {
public:
    MemberAccess(std::unique_ptr<Expression> object, const std::string& member)
        : object_(std::move(object)), member_(member) {}

    std::string toString() const override;

    const Expression& getObject() const { return *object_; }
    const std::string& getMember() const { return member_; }

private:
    std::unique_ptr<Expression> object_;
    std::string member_;
};

// Array access (arr[index])
class ArrayAccess : public Expression {
public:
    ArrayAccess(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index)
        : array_(std::move(array)), index_(std::move(index)) {}

    std::string toString() const override;

    const Expression& getArray() const { return *array_; }
    const Expression& getIndex() const { return *index_; }

private:
    std::unique_ptr<Expression> array_;
    std::unique_ptr<Expression> index_;
};

// Argument list
class ArgumentList : public ASTNode {
public:
    ArgumentList(std::vector<std::unique_ptr<Expression>> arguments)
        : arguments_(std::move(arguments)) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<Expression>>& getArguments() const { return arguments_; }

private:
    std::vector<std::unique_ptr<Expression>> arguments_;
};

// Function call
class FunctionCall : public Expression {
public:
    FunctionCall(std::unique_ptr<Expression> function,
                std::unique_ptr<ArgumentList> arguments)
        : function_(std::move(function)), arguments_(std::move(arguments)) {}

    std::string toString() const override;

    const Expression& getFunction() const { return *function_; }
    const ArgumentList* getArguments() const { return arguments_.get(); }

private:
    std::unique_ptr<Expression> function_;
    std::unique_ptr<ArgumentList> arguments_;
};

// New object expression
class NewObjectExpression : public Expression {
public:
    NewObjectExpression(const std::string& className,
                       std::unique_ptr<ArgumentList> arguments)
        : className_(std::make_unique<QualifiedIdentifier>(std::vector<std::string>{className})),
          arguments_(std::move(arguments)) {}

    NewObjectExpression(std::unique_ptr<QualifiedIdentifier> className,
                       std::unique_ptr<ArgumentList> arguments)
        : className_(std::move(className)), arguments_(std::move(arguments)) {}

    std::string toString() const override;

    // Get simple class name (backward compatible)
    std::string getClassName() const {
        return className_ ? className_->getName() : "";
    }

    // Get qualified class name (new API)
    const QualifiedIdentifier* getQualifiedClassName() const { return className_.get(); }

    const ArgumentList* getArguments() const { return arguments_.get(); }

private:
    std::unique_ptr<QualifiedIdentifier> className_;
    std::unique_ptr<ArgumentList> arguments_;
};

// Unary expressions
class UnaryMinus : public Expression {
public:
    UnaryMinus(std::unique_ptr<Expression> operand) : operand_(std::move(operand)) {}

    std::string toString() const override;

    const Expression& getOperand() const { return *operand_; }

private:
    std::unique_ptr<Expression> operand_;
};

class LogicalNot : public Expression {
public:
    LogicalNot(std::unique_ptr<Expression> operand) : operand_(std::move(operand)) {}

    std::string toString() const override;

    const Expression& getOperand() const { return *operand_; }

private:
    std::unique_ptr<Expression> operand_;
};

// Binary expressions
enum class BinaryOperator {
    MULTIPLY, DIVIDE, MODULO,           // Multiplicative
    PLUS, MINUS,                        // Additive
    LESS, LESS_EQUALS, GREATER,         // Relational
    GREATER_EQUALS, EQUALS, NOT_EQUALS,
    AND, OR,                           // Logical
    ASSIGN                             // Assignment
};

enum class CompoundAssignmentOperator {
    PLUS_ASSIGN, MINUS_ASSIGN, MULTIPLY_ASSIGN, DIVIDE_ASSIGN,
    MODULO_ASSIGN, LEFT_SHIFT_ASSIGN, RIGHT_SHIFT_ASSIGN
};

enum class IncrementDecrementOperator {
    INCREMENT, DECREMENT
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(std::unique_ptr<Expression> left,
                    BinaryOperator operator_,
                    std::unique_ptr<Expression> right)
        : left_(std::move(left)), operator_(operator_), right_(std::move(right)) {}

    const Expression& getLeft() const { return *left_; }
    BinaryOperator getOperator() const { return operator_; }
    const Expression& getRight() const { return *right_; }

protected:
    std::unique_ptr<Expression> left_;
    BinaryOperator operator_;
    std::unique_ptr<Expression> right_;
};

// Specific binary expression types
class MultiplicativeExpression : public BinaryExpression {
public:
    MultiplicativeExpression(std::unique_ptr<Expression> left,
                           BinaryOperator operator_,
                           std::unique_ptr<Expression> right)
        : BinaryExpression(std::move(left), operator_, std::move(right)) {}

    std::string toString() const override;
};

class AdditiveExpression : public BinaryExpression {
public:
    AdditiveExpression(std::unique_ptr<Expression> left,
                      BinaryOperator operator_,
                      std::unique_ptr<Expression> right)
        : BinaryExpression(std::move(left), operator_, std::move(right)) {}

    std::string toString() const override;
};

class RelationalExpression : public BinaryExpression {
public:
    RelationalExpression(std::unique_ptr<Expression> left,
                        BinaryOperator operator_,
                        std::unique_ptr<Expression> right)
        : BinaryExpression(std::move(left), operator_, std::move(right)) {}

    std::string toString() const override;
};

class LogicalAnd : public Expression {
public:
    LogicalAnd(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : left_(std::move(left)), right_(std::move(right)) {}

    std::string toString() const override;

    const Expression& getLeft() const { return *left_; }
    const Expression& getRight() const { return *right_; }

private:
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class LogicalOr : public Expression {
public:
    LogicalOr(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : left_(std::move(left)), right_(std::move(right)) {}

    std::string toString() const override;

    const Expression& getLeft() const { return *left_; }
    const Expression& getRight() const { return *right_; }

private:
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class AssignmentExpression : public Expression {
public:
    AssignmentExpression(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : left_(std::move(left)), right_(std::move(right)) {}

    std::string toString() const override;

    const Expression& getLeft() const { return *left_; }
    const Expression& getRight() const { return *right_; }

private:
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class CompoundAssignmentExpression : public Expression {
public:
    CompoundAssignmentExpression(std::unique_ptr<Expression> left,
                              CompoundAssignmentOperator operator_,
                              std::unique_ptr<Expression> right)
        : left_(std::move(left)), operator_(operator_), right_(std::move(right)) {}

    std::string toString() const override;

    const Expression& getLeft() const { return *left_; }
    CompoundAssignmentOperator getOperator() const { return operator_; }
    const Expression& getRight() const { return *right_; }

private:
    std::unique_ptr<Expression> left_;
    CompoundAssignmentOperator operator_;
    std::unique_ptr<Expression> right_;
};

class IncrementDecrementExpression : public Expression {
public:
    IncrementDecrementExpression(std::unique_ptr<Expression> operand,
                                 IncrementDecrementOperator operator_,
                                 bool isPrefix)
        : operand_(std::move(operand)), operator_(operator_), isPrefix_(isPrefix) {}

    std::string toString() const override;

    const Expression& getOperand() const { return *operand_; }
    IncrementDecrementOperator getOperator() const { return operator_; }
    bool isPrefix() const { return isPrefix_; }

private:
    std::unique_ptr<Expression> operand_;
    IncrementDecrementOperator operator_;
    bool isPrefix_;
};

// Expression list
class ExpressionList : public ASTNode {
public:
    ExpressionList(std::vector<std::unique_ptr<Expression>> expressions)
        : expressions_(std::move(expressions)) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<Expression>>& getExpressions() const { return expressions_; }

private:
    std::vector<std::unique_ptr<Expression>> expressions_;
};

// Array literal
class ArrayLiteral : public Expression {
public:
    ArrayLiteral(std::unique_ptr<ExpressionList> elements)
        : elements_(std::move(elements)) {}

    std::string toString() const override;

    const ExpressionList* getElements() const { return elements_.get(); }

private:
    std::unique_ptr<ExpressionList> elements_;
};

} // namespace ast
} // namespace hooc