# Abstract Syntax Tree (`src/ast`)

This directory contains the AST node classes that represent the parsed Hooc source code. The AST is an intermediate representation between the parser (ANTLR4) and the code generator (LLVM IR).

## Overview

```
Source Code (.hoo)
       │
       ▼
ANTLR4 Parse Tree
       │
       ▼
SimpleASTBuilder (Parse Tree → AST)
       │
       ▼
   AST Nodes
       │
       ▼
LLVMCodeGenerator (AST → LLVM IR)
```

## Class Hierarchy

```
ASTNode (base)
├── Declaration
│   ├── FunctionDeclaration
│   ├── VariableDeclaration
│   ├── Parameter
│   ├── ClassDeclaration
│   │   ├── ConstructorDeclaration
│   │   ├── EventDeclaration
│   │   ├── ClassMember
│   │   └── ClassBody
├── Statement
│   ├── Block
│   ├── ExpressionStatement
│   ├── IfStatement
│   ├── ForInStatement
│   ├── ForRangeStatement
│   ├── WhileStatement
│   ├── ReturnStatement
│   ├── ScopeStatement
│   └── VariableDeclarationStatement
├── Expression
│   ├── PrimaryExpression
│   │   └── Primary
│   │       ├── Identifier
│   │       ├── IntegerLiteral
│   │       ├── FloatingLiteral
│   │       ├── StringLiteral
│   │       ├── CharacterLiteral
│   │       ├── BooleanLiteral
│   │       ├── NullLiteral
│   │       ├── InterpolatedString
│   │       └── ParenthesizedExpression
│   ├── MemberAccess
│   ├── ArrayAccess
│   ├── ArgumentList
│   ├── FunctionCall
│   ├── NewArrayExpression
│   ├── NewObjectExpression
│   ├── UnaryMinus
│   ├── LogicalNot
│   ├── BinaryExpression
│   │   ├── MultiplicativeExpression
│   │   ├── AdditiveExpression
│   │   ├── RelationalExpression
│   │   ├── LogicalAnd
│   │   ├── LogicalOr
│   │   └── AssignmentExpression
│   ├── ErrorHandlingExpression
│   ├── ExpressionList
│   ├── ArrayLiteral
│   ├── ListComprehension
│   ├── LambdaExpression
│   └── MultiParamLambda
├── Type
│   ├── PrimitiveType
│   ├── BaseType
│   ├── ArrayType
│   └── OptionalType
├── ImportStatement
│   ├── BasicImport
│   ├── FromImport
│   ├── NamedImports
│   ├── NamespaceImport
│   ├── SideEffectImport
│   ├── ModulePath
│   └── ImportItem
├── CompilationUnit
└── QualifiedIdentifier
```

## Contents

| File | Description |
|------|-------------|
| `AST.h` | Main header that includes all AST classes |
| `ASTNode.h` | Base class for all AST nodes |
| `Declaration.h` | Function, variable, and parameter declarations |
| `Expression.h` | All expression types |
| `Statement.h` | All statement types |
| `Type.h` | Type system representation |
| `ClassDeclaration.h` | Class-related AST nodes |
| `ImportStatement.h` | Import statement types |
| `CompilationUnit.h` | Root AST node |
| `Primary.h` | Primary expression literals |
| `QualifiedIdentifier.h` | Module-qualified names |
| `ASTImpl.cpp` | `toString()` implementations |

## Base Classes

### ASTNode

```cpp
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

template<typename T, typename... Args>
std::unique_ptr<T> makeAST(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}
```

### Declaration

Base class for declarations (functions, variables, parameters, classes).

### Statement

Base class for statements (blocks, conditionals, loops, returns).

### Expression

Base class for expressions (literals, operations, function calls).

### Type

Base class for type representations.

## Declaration Classes

### FunctionDeclaration

```cpp
class FunctionDeclaration : public Declaration {
    const std::string& getName() const;
    const std::vector<std::unique_ptr<Parameter>>& getParameters() const;
    const Type* getReturnType() const;
    const Block& getBody() const;
};
```

### VariableDeclaration

```cpp
class VariableDeclaration : public Declaration {
    const Type* getType() const;           // Can be nullptr for type inference
    const std::string& getName() const;
    const Expression* getInitializer() const;
    bool hasTypeInference() const;
};
```

### Parameter

```cpp
class Parameter : public ASTNode {
    const Type& getType() const;
    const std::string& getName() const;
};
```

## Type Classes

### PrimitiveType

Represents built-in types:

```cpp
enum class PrimitiveTypeKind {
    INT8, BYTE, INT64, FLOAT, DOUBLE, F64,
    BOOL, CHAR, STRING, VOID
};
```

### BaseType

Either a primitive type or a named type:

```cpp
class BaseType : public Type {
    const PrimitiveType* getPrimitiveType() const;
    const QualifiedIdentifier* getQualifiedIdentifier() const;
    bool isPrimitive() const;
};
```

### ArrayType

```cpp
class ArrayType : public Type {
    const BaseType& getBaseType() const;
    const std::vector<std::unique_ptr<Expression>>& getDimensions() const;
    size_t getDimensionCount() const;
};
```

### OptionalType

Nullable types (e.g., `int64?`):

```cpp
class OptionalType : public Type {
    const ArrayType& getArrayType() const;
    bool isOptional() const;
};
```

## Expression Classes

### Primary Literals

```cpp
Identifier(name)           // Variable references
IntegerLiteral(value)     // 42
FloatingLiteral(value)    // 3.14
StringLiteral(value)      // "hello"
CharacterLiteral(value)   // 'x'
BooleanLiteral(value)     // true/false
NullLiteral()             // null
```

### MemberAccess

Object member access: `obj.member`

```cpp
class MemberAccess : public Expression {
    const Expression& getObject() const;
    const std::string& getMember() const;
};
```

### ArrayAccess

Array indexing: `arr[index]`

```cpp
class ArrayAccess : public Expression {
    const Expression& getArray() const;
    const Expression& getIndex() const;
};
```

### FunctionCall

```cpp
class FunctionCall : public Expression {
    const Expression& getFunction() const;
    const ArgumentList* getArguments() const;
};
```

### NewObjectExpression

Constructor calls: `new ClassName(args)`

```cpp
class NewObjectExpression : public Expression {
    std::string getClassName() const;                    // Simple name
    const QualifiedIdentifier* getQualifiedClassName() const;  // Full path
    const ArgumentList* getArguments() const;
};
```

### BinaryExpression

```cpp
enum class BinaryOperator {
    MULTIPLY, DIVIDE, MODULO,      // Arithmetic
    PLUS, MINUS,                   // Arithmetic
    LESS, LESS_EQUALS, GREATER,    // Comparison
    GREATER_EQUALS, EQUALS, NOT_EQUALS,
    AND, OR,                       // Logical
    ASSIGN                         // Assignment
};

class BinaryExpression : public Expression {
    const Expression& getLeft() const;
    BinaryOperator getOperator() const;
    const Expression& getRight() const;
};
```

### ArrayLiteral

```cpp
class ArrayLiteral : public Expression {
    const ExpressionList* getElements() const;
};
```

### LambdaExpression

```cpp
class LambdaExpression : public Expression {
    const std::string& getParameter() const;
    const Expression& getBody() const;
};

class MultiParamLambda : public Expression {
    const std::vector<std::unique_ptr<Parameter>>& getParameters() const;
    const Expression& getBody() const;
};
```

## Statement Classes

### Block

```cpp
class Block : public Statement {
    const std::vector<std::unique_ptr<Statement>>& getStatements() const;
};
```

### IfStatement

```cpp
class IfStatement : public Statement {
    const Expression& getCondition() const;
    const Block& getThenBlock() const;
    const Block* getElseBlock() const;
    bool hasElse() const;
};
```

### WhileStatement

```cpp
class WhileStatement : public Statement {
    const Expression& getCondition() const;
    const Block& getBody() const;
};
```

### ForInStatement

For-each loop: `for item in collection`

```cpp
class ForInStatement : public Statement {
    const std::string& getVariable() const;
    const Expression& getIterable() const;
    const Block& getBody() const;
};
```

### ForRangeStatement

Numeric range loop: `for i in 0..10`

```cpp
class ForRangeStatement : public Statement {
    const std::string& getVariable() const;
    const Expression& getStart() const;
    const Expression& getEnd() const;
    const Block& getBody() const;
};
```

### ReturnStatement

```cpp
class ReturnStatement : public Statement {
    const Expression* getExpression() const;
    bool hasExpression() const;  // false for `return;`
};
```

## Class Declaration Classes

### ClassDeclaration

```cpp
enum class ClassModifier {
    SINGLETON, IMMUTABLE, FACTORY, OBSERVABLE,
    SERVICE, STRATEGY, ACTOR, FINAL
};

class ClassDeclaration : public Declaration {
    const std::vector<ClassModifier>& getModifiers() const;
    const std::string& getName() const;
    const std::string& getBaseClass() const;
    const ClassBody& getBody() const;
    bool hasModifier(ClassModifier modifier) const;
    bool hasBaseClass() const;
};
```

### ClassBody / ClassMember

```cpp
class ClassBody : public ASTNode {
    const std::vector<std::unique_ptr<ClassMember>>& getMembers() const;
};

class ClassMember : public ASTNode {
    const ConstructorDeclaration* getConstructor() const;
    const Declaration* getDeclaration() const;
    const EventDeclaration* getEvent() const;
    bool isConstructor() const;
    bool isEvent() const;
};
```

## Import Classes

### QualifiedIdentifier

Represents module-qualified names like `hoo.String`:

```cpp
class QualifiedIdentifier : public ASTNode {
    const std::vector<std::string>& getComponents() const;  // ["hoo", "String"]
    const std::string& getName() const;                     // "String"
    std::vector<std::string> getModulePath() const;         // ["hoo"]
    bool isQualified() const;                               // true
    std::string getFullName() const;                        // "hoo.String"
};
```

### Import Statement Types

```cpp
// import os.path
BasicImport

// from std import String
FromImport

// import { User, Role } from "module"
NamedImports

// import * as math from "module"
NamespaceImport

// import "module"  (side-effect only)
SideEffectImport
```

## Compilation Unit

```cpp
class CompilationUnit : public ASTNode {
    const std::vector<std::unique_ptr<ImportStatement>>& getImports() const;
    const std::vector<std::unique_ptr<Declaration>>& getDeclarations() const;
};
```

## Utility Functions

```cpp
// Defined in AST.h
std::string classModifierToString(ClassModifier modifier);
std::string primitiveTypeToString(PrimitiveTypeKind kind);
std::string binaryOperatorToString(BinaryOperator op);
```

## Usage Example

```cpp
#include "ast/AST.h"

using namespace hooc::ast;

// Create a simple program: func main() { print("Hello"); }
auto program = makeAST<CompilationUnit>(
    std::vector<std::unique_ptr<ImportStatement>>{},  // no imports
    std::vector<std::unique_ptr<Declaration>>{
        makeAST<FunctionDeclaration>(
            "main",
            std::vector<std::unique_ptr<Parameter>>{},  // no params
            makeAST<PrimitiveType>(PrimitiveTypeKind::VOID),
            makeAST<Block>(std::vector<std::unique_ptr<Statement>>{
                makeAST<ExpressionStatement>(
                    makeAST<FunctionCall>(
                        makeAST<Identifier>("print"),
                        makeAST<ArgumentList>(std::vector<std::unique_ptr<Expression>>{
                            makeAST<StringLiteral>("Hello")
                        })
                    )
                )
            })
        )
    }
);

// Walk the AST
for (const auto& decl : program->getDeclarations()) {
    std::cout << decl->toString() << std::endl;
}
```

## Adding New AST Nodes

1. **Add class declaration** to the appropriate header (e.g., `Expression.h`)
2. **Implement `toString()`** in `ASTImpl.cpp`
3. **Update `SimpleASTBuilder`** to construct the new node
4. **Update `LLVMCodeGenerator`** to handle code generation
5. **Add tests** to verify parsing and code generation

### Template for new node:

```cpp
// In Expression.h
class MyNewExpression : public Expression {
public:
    MyNewExpression(/* params */) : /* initialize */ {}

    std::string toString() const override;
    // getters...

private:
    // members
};

// In ASTImpl.cpp
std::string MyNewExpression::toString() const {
    return "MyNewExpression";
}
```

## Visitor Pattern (Future)

The AST currently uses the "tell, don't ask" pattern (getters return child nodes). A visitor pattern can be added for operations that need to traverse all nodes uniformly:

```cpp
// Future: ASTVisitor.h
class ASTVisitor {
public:
    virtual void visit(CompilationUnit*) = 0;
    virtual void visit(FunctionDeclaration*) = 0;
    virtual void visit(Identifier*) = 0;
    // ... all node types
};
```
