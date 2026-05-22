#pragma once

#include "ASTNode.h"
#include <stdexcept>
#include <vector>

namespace hooc {
namespace ast {

class Expression;
class VariableDeclaration;

// Base class for all statements
class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

// Block statement
class Block : public Statement {
public:
    Block(std::vector<std::unique_ptr<Statement>> statements)
        : statements_(std::move(statements)) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<Statement>>& getStatements() const { return statements_; }

private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

// Expression statement
class ExpressionStatement : public Statement {
public:
    ExpressionStatement(std::unique_ptr<Expression> expression)
        : expression_(std::move(expression)) {}

    std::string toString() const override;

    const Expression& getExpression() const { return *expression_; }

private:
    std::unique_ptr<Expression> expression_;
};

// If statement
class IfStatement : public Statement {
public:
    IfStatement(std::unique_ptr<Expression> condition,
               std::unique_ptr<Block> thenBlock,
               std::unique_ptr<Block> elseBlock = nullptr)
        : condition_(std::move(condition)), thenBlock_(std::move(thenBlock)),
          elseBlock_(std::move(elseBlock)) {}

    std::string toString() const override;

    const Expression& getCondition() const { return *condition_; }
    const Block& getThenBlock() const { return *thenBlock_; }
    const Block* getElseBlock() const { return elseBlock_.get(); }
    bool hasElse() const { return elseBlock_ != nullptr; }

private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Block> thenBlock_;
    std::unique_ptr<Block> elseBlock_;
};

// For-in statement
class ForInStatement : public Statement {
public:
    ForInStatement(const std::string& variable,
                  std::unique_ptr<Expression> iterable,
                  std::unique_ptr<Block> body)
        : variable_(variable), iterable_(std::move(iterable)), body_(std::move(body)) {}

    std::string toString() const override;

    const std::string& getVariable() const { return variable_; }
    const Expression& getIterable() const { return *iterable_; }
    const Block& getBody() const { return *body_; }

private:
    std::string variable_;
    std::unique_ptr<Expression> iterable_;
    std::unique_ptr<Block> body_;
};

// For-range statement
class ForRangeStatement : public Statement {
public:
    ForRangeStatement(const std::string& variable,
                     std::unique_ptr<Expression> start,
                     std::unique_ptr<Expression> end,
                     std::unique_ptr<Expression> step,
                     std::unique_ptr<Block> body)
        : variable_(variable), start_(std::move(start)), 
          end_(std::move(end)), step_(std::move(step)), body_(std::move(body)) {}

    std::string toString() const override;

    const std::string& getVariable() const { return variable_; }
    const Expression& getStart() const { return *start_; }
    const Expression& getEnd() const { return *end_; }
    const Expression* getStep() const { return step_.get(); }
    bool hasStep() const { return step_ != nullptr; }
    const Block& getBody() const { return *body_; }

private:
    std::string variable_;
    std::unique_ptr<Expression> start_;
    std::unique_ptr<Expression> end_;
    std::unique_ptr<Expression> step_;
    std::unique_ptr<Block> body_;
};

// While statement
class WhileStatement : public Statement {
public:
    WhileStatement(std::unique_ptr<Expression> condition,
                  std::unique_ptr<Block> body)
        : condition_(std::move(condition)), body_(std::move(body)) {}

    std::string toString() const override;

    const Expression& getCondition() const { return *condition_; }
    const Block& getBody() const { return *body_; }

private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Block> body_;
};

// Return statement
class ReturnStatement : public Statement {
public:
    ReturnStatement(std::unique_ptr<Expression> expression = nullptr)
        : expression_(std::move(expression)) {}

    std::string toString() const override;

    const Expression* getExpression() const { return expression_.get(); }
    bool hasExpression() const { return expression_ != nullptr; }

private:
    std::unique_ptr<Expression> expression_;
};

// Scope statement
class ScopeStatement : public Statement {
public:
    ScopeStatement(std::unique_ptr<Block> body) : body_(std::move(body)) {}

    std::string toString() const override;

    const Block& getBody() const { return *body_; }

private:
    std::unique_ptr<Block> body_;
};

// Variable declaration statement
class VariableDeclarationStatement : public Statement {
public:
    VariableDeclarationStatement(std::unique_ptr<VariableDeclaration> declaration)
        : declaration_(std::move(declaration)) {}

    std::string toString() const override;

    const VariableDeclaration& getDeclaration() const { return *declaration_; }

private:
    std::unique_ptr<VariableDeclaration> declaration_;
};

// Break statement (exit loop)
class BreakStatement : public Statement {
public:
    BreakStatement() {}

    std::string toString() const override;
};

// Continue statement (skip to next iteration)
class ContinueStatement : public Statement {
public:
    ContinueStatement() {}

    std::string toString() const override;
};

// Try-catch-finally statement
class TryCatchStatement : public Statement {
public:
    struct CatchClause {
        std::string variable;
        std::unique_ptr<Type> type;
        std::unique_ptr<Block> block;
    };

    TryCatchStatement(std::unique_ptr<Block> tryBlock,
                     std::vector<CatchClause> catchClauses,
                     std::unique_ptr<Block> finallyBlock)
        : tryBlock_(std::move(tryBlock)),
          catchClauses_(std::move(catchClauses)),
          finallyBlock_(std::move(finallyBlock)) {}

    std::string toString() const override;

    const Block& getTryBlock() const { return *tryBlock_; }
    const std::vector<CatchClause>& getCatchClauses() const { return catchClauses_; }
    bool hasCatch() const { return !catchClauses_.empty(); }
    const Block* getFinallyBlock() const { return finallyBlock_.get(); }
    bool hasFinally() const { return finallyBlock_ != nullptr; }

private:
    std::unique_ptr<Block> tryBlock_;
    std::vector<CatchClause> catchClauses_;
    std::unique_ptr<Block> finallyBlock_;
};

// Throw statement (throw expression;)
class ThrowStatement : public Statement {
public:
    enum class ThrowKind {
        THROW,
        RETHROW
    };

    ThrowStatement(std::unique_ptr<Expression> expression)
        : kind_(ThrowKind::THROW), expression_(std::move(expression)) {
        if (!expression_) {
            throw std::invalid_argument("ThrowStatement(THROW) requires a non-null expression");
        }
    }

    ThrowStatement(ThrowKind kind, std::unique_ptr<Expression> expression = nullptr)
        : kind_(kind), expression_(std::move(expression)) {
        if (kind_ == ThrowKind::THROW && !expression_) {
            throw std::invalid_argument("ThrowStatement(THROW) requires a non-null expression");
        }
        if (kind_ == ThrowKind::RETHROW && expression_) {
            throw std::invalid_argument("ThrowStatement(RETHROW) must not carry an expression");
        }
    }

    std::string toString() const override;

    ThrowKind getKind() const { return kind_; }
    const Expression* getExpression() const { return expression_.get(); }
    bool isRethrow() const { return kind_ == ThrowKind::RETHROW; }

private:
    ThrowKind kind_;
    std::unique_ptr<Expression> expression_;
};

} // namespace ast
} // namespace hooc
