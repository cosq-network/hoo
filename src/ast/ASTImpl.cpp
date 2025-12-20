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

// Declaration implementations
std::string FunctionDeclaration::toString() const {
    return "FunctionDeclaration " + name_;
}

std::string VariableDeclaration::toString() const {
    return "VariableDeclaration " + name_;
}

std::string Parameter::toString() const {
    return "Parameter " + name_;
}

// Class-related implementations
std::string PrimaryConstructor::toString() const {
    return "PrimaryConstructor";
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
    return "ClassDeclaration " + name_;
}

// Interface implementations
std::string FunctionSignature::toString() const {
    return "FunctionSignature " + name_;
}

std::string InterfaceMember::toString() const {
    return "InterfaceMember";
}

std::string InterfaceDeclaration::toString() const {
    return "InterfaceDeclaration " + name_;
}

// Type implementations
std::string PrimitiveType::toString() const {
    return "PrimitiveType";
}

std::string BaseType::toString() const {
    return "BaseType";
}

std::string ArrayType::toString() const {
    return "ArrayType";
}

std::string OptionalType::toString() const {
    return "OptionalType";
}

std::string UnionType::toString() const {
    return "UnionType";
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

// Expression implementations
std::string PrimaryExpression::toString() const {
    return "PrimaryExpression";
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
    return "NewObjectExpression " + className_;
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