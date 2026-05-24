#pragma once

// Main header that includes all AST classes
#include "ASTNode.h"
#include "CompilationUnit.h"
#include "ImportStatement.h"
#include "Declaration.h"
#include "ClassDeclaration.h"
#include "Type.h"
#include "Statement.h"
#include "Expression.h"
#include "Primary.h"

namespace hooc {
namespace ast {

// Utility functions for AST creation
std::string classModifierToString(ClassModifier modifier);
std::string primitiveTypeToString(PrimitiveTypeKind kind);
std::string binaryOperatorToString(BinaryOperator op);

} // namespace ast
} // namespace hooc
