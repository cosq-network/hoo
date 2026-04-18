#include "AST.h"
#include <sstream>

using namespace hooc::ast;

// CompilationUnit
std::string CompilationUnit::toString() const {
    std::stringstream ss;
    ss << "CompilationUnit(imports=" << imports_.size() << ", declarations=" << declarations_.size() << ")";
    return ss.str();
}

// ImportStatement implementations
std::string ImportItem::toString() const {
    if (!alias_.empty()) {
        return name_ + " as " + alias_;
    }
    return name_;
}

std::string NamedImports::toString() const {
    return "NamedImports from " + module_;
}

std::string NamespaceImport::toString() const {
    return "NamespaceImport * as " + alias_ + " from " + module_;
}

std::string SideEffectImport::toString() const {
    return "SideEffectImport " + module_;
}

// Python-style import implementations
std::string ModulePath::toString() const {
    std::stringstream ss;
    for (size_t i = 0; i < components_.size(); i++) {
        if (i > 0) ss << ".";
        ss << components_[i];
    }
    return ss.str();
}

// Qualified identifier implementation
std::string QualifiedIdentifier::toString() const {
    std::stringstream ss;
    for (size_t i = 0; i < components_.size(); i++) {
        if (i > 0) ss << ".";
        ss << components_[i];
    }
    return ss.str();
}

std::string BasicImport::toString() const {
    std::stringstream ss;
    ss << "import " << module_->toString();
    if (!alias_.empty()) {
        ss << " as " << alias_;
    }
    return ss.str();
}

std::string FromImport::toString() const {
    std::stringstream ss;
    ss << "from " << module_->toString() << " import ";
    for (size_t i = 0; i < items_.size(); i++) {
        if (i > 0) ss << ", ";
        ss << items_[i]->toString();
    }
    return ss.str();
}

// Declaration implementations
std::string FunctionDeclaration::toString() const {
    std::stringstream ss;
    ss << "FunctionDeclaration " << name_;
    return ss.str();
}

std::string VariableDeclaration::toString() const {
    return "VariableDeclaration " + name_;
}

std::string Parameter::toString() const {
    return "Parameter " + name_;
}

// Class-related implementations
std::string ConstructorDeclaration::toString() const {
    return "ConstructorDeclaration";
}

std::string EventDeclaration::toString() const {
    return "EventDeclaration " + name_;
}

std::string ClassMember::toString() const {
    return "ClassMember";
}

std::string ClassBody::toString() const {
    return "ClassBody";
}

std::string ClassDeclaration::toString() const {
    std::stringstream ss;
    ss << "ClassDeclaration " << name_;
    return ss.str();
}

bool ClassDeclaration::hasModifier(ClassModifier modifier) const {
    for (const auto& mod : modifiers_) {
        if (mod == modifier) {
            return true;
        }
    }
    return false;
}

// Type implementations
std::string PrimitiveType::toString() const {
    return "PrimitiveType";
}

std::string BaseType::toString() const {
    std::stringstream ss;
    ss << "BaseType";
    if (isPrimitive()) {
        ss << "(primitive)";
    } else if (identifier_) {
        ss << "(" << identifier_->toString() << ")";
    }
    return ss.str();
}

std::string ArrayType::toString() const {
    return "ArrayType";
}

std::string OptionalType::toString() const {
    return "OptionalType";
}

// Statement implementations
std::string Block::toString() const {
    return "Block";
}

std::string ExpressionStatement::toString() const {
    return "ExpressionStatement";
}

std::string IfStatement::toString() const {
    return "IfStatement";
}

std::string ForInStatement::toString() const {
    return "ForInStatement " + variable_;
}

std::string ForRangeStatement::toString() const {
    return "ForRangeStatement " + variable_;
}

std::string WhileStatement::toString() const {
    return "WhileStatement";
}

std::string ReturnStatement::toString() const {
    return "ReturnStatement";
}

std::string ScopeStatement::toString() const {
    return "ScopeStatement";
}

std::string VariableDeclarationStatement::toString() const {
    return "VariableDeclarationStatement(" + declaration_->toString() + ")";
}

std::string BreakStatement::toString() const {
    return "BreakStatement";
}

std::string ContinueStatement::toString() const {
    return "ContinueStatement";
}

// Expression implementations
std::string PrimaryExpression::toString() const {
    std::stringstream ss;
    ss << "PrimaryExpression(" << primary_->toString() << ")";
    return ss.str();
}

std::string MemberAccess::toString() const {
    return "MemberAccess ." + member_;
}

std::string ArrayAccess::toString() const {
    return "ArrayAccess";
}

std::string ArgumentList::toString() const {
    return "ArgumentList";
}

std::string FunctionCall::toString() const {
    return "FunctionCall";
}

std::string NewArrayExpression::toString() const {
    return "NewArrayExpression";
}

std::string NewObjectExpression::toString() const {
    std::stringstream ss;
    ss << "NewObjectExpression " << className_->toString();
    return ss.str();
}

std::string UnaryMinus::toString() const {
    return "UnaryMinus";
}

std::string LogicalNot::toString() const {
    return "LogicalNot";
}

std::string MultiplicativeExpression::toString() const {
    return "MultiplicativeExpression";
}

std::string AdditiveExpression::toString() const {
    return "AdditiveExpression";
}

std::string RelationalExpression::toString() const {
    return "RelationalExpression";
}

std::string LogicalAnd::toString() const {
    return "LogicalAnd";
}

std::string LogicalOr::toString() const {
    return "LogicalOr";
}

std::string AssignmentExpression::toString() const {
    return "AssignmentExpression";
}

std::string ErrorHandlingExpression::toString() const {
    return "ErrorHandlingExpression";
}

std::string ExpressionList::toString() const {
    return "ExpressionList";
}

std::string ArrayLiteral::toString() const {
    return "ArrayLiteral";
}

std::string ListComprehension::toString() const {
    return "ListComprehension";
}

std::string LambdaExpression::toString() const {
    return "LambdaExpression";
}

std::string MultiParamLambda::toString() const {
    return "MultiParamLambda";
}

// Primary implementations
std::string Identifier::toString() const {
    return "Identifier(" + name_ + ")";
}

std::string IntegerLiteral::toString() const {
    return "IntegerLiteral(" + std::to_string(value_) + ")";
}

std::string FloatingLiteral::toString() const {
    return "FloatingLiteral(" + std::to_string(value_) + ")";
}

std::string StringLiteral::toString() const {
    return "StringLiteral(\"" + value_ + "\")";
}

std::string CharacterLiteral::toString() const {
    return "CharacterLiteral('" + std::string(1, value_) + "')";
}

std::string BooleanLiteral::toString() const {
    return "BooleanLiteral(" + std::string(value_ ? "true" : "false") + ")";
}

std::string NullLiteral::toString() const {
    return "NullLiteral(null)";
}

std::string ThisLiteral::toString() const {
    return "ThisLiteral(this)";
}

std::string InterpolatedString::toString() const {
    return "InterpolatedString";
}

std::string ParenthesizedExpression::toString() const {
    return "ParenthesizedExpression";
}

// Utility function implementations
std::string hooc::ast::classModifierToString(ClassModifier modifier) {
    switch (modifier) {
        case ClassModifier::SINGLETON: return "singleton";
        case ClassModifier::IMMUTABLE: return "immutable";
        case ClassModifier::FACTORY: return "factory";
        case ClassModifier::OBSERVABLE: return "observable";
        case ClassModifier::SERVICE: return "service";
        case ClassModifier::STRATEGY: return "strategy";
        case ClassModifier::ACTOR: return "actor";
        case ClassModifier::FINAL: return "final";
        default: return "unknown";
    }
}

std::string hooc::ast::primitiveTypeToString(PrimitiveTypeKind kind) {
    switch (kind) {
        case PrimitiveTypeKind::BYTE: return "byte";
        case PrimitiveTypeKind::UINT8: return "uint8";
        case PrimitiveTypeKind::INT64: return "int64";
        case PrimitiveTypeKind::DOUBLE: return "double";
        case PrimitiveTypeKind::F64: return "f64";
        case PrimitiveTypeKind::BOOL: return "bool";
        case PrimitiveTypeKind::CHAR: return "char";
        case PrimitiveTypeKind::STRING: return "string";
        default: return "unknown";
    }
}

std::string hooc::ast::binaryOperatorToString(BinaryOperator op) {
    switch (op) {
        case BinaryOperator::MULTIPLY: return "*";
        case BinaryOperator::DIVIDE: return "/";
        case BinaryOperator::MODULO: return "%";
        case BinaryOperator::PLUS: return "+";
        case BinaryOperator::MINUS: return "-";
        case BinaryOperator::LESS: return "<";
        case BinaryOperator::LESS_EQUALS: return "<=";
        case BinaryOperator::GREATER: return ">";
        case BinaryOperator::GREATER_EQUALS: return ">=";
        case BinaryOperator::EQUALS: return "==";
        case BinaryOperator::NOT_EQUALS: return "!=";
        case BinaryOperator::AND: return "&&";
        case BinaryOperator::OR: return "||";
        case BinaryOperator::ASSIGN: return "=";
        default: return "unknown";
    }
}