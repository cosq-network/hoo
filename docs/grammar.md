# Hooc Language Grammar Specification

This document provides a comprehensive reference for the Hooc programming language grammar. The grammar is defined using ANTLR4 notation and is located in `src/Hooc.g4`.

## Table of Contents

1. [Lexical Structure](#lexical-structure)
2. [Keywords](#keywords)
3. [Primitive Types](#primitive-types)
4. [Operators](#operators)
5. [Literals](#literals)
6. [Syntax Rules](#syntax-rules)
7. [Type System](#type-system)
8. [Examples](#examples)

## Lexical Structure

### Keywords

Hooc reserves the following keywords:

**Control Flow:**
- `if`, `else` - Conditional statements
- `for`, `while` - Loop constructs
- `in` - For loop iteration
- `return` - Function returns

**Declarations:**
- `func` - Function declaration
- `class` - Class declaration
- `constructor` - Class constructor
- `var` - Variable declaration

**Object-Oriented:**
- `new` - Object instantiation
- `extends` - Class inheritance

**Modifiers:**
- `final` - Prevent inheritance/reassignment
- `singleton` - Singleton pattern
- `immutable` - Immutable class
- `factory` - Factory pattern
- `observable` - Observable pattern
- `service` - Service pattern
- `strategy` - Strategy pattern
- `actor` - Actor model pattern

**Module System:**
- `import` - Import modules
- `from` - Import specific items
- `as` - Alias for imports

**Special:**
- `scope` - Scope management
- `event` - Event declaration
- `this` - Current object instance

**Literals:**
- `true`, `false` - Boolean literals
- `null` - Null literal

### Primitive Types

```
byte      - 8-bit signed integer
uint8     - 8-bit unsigned integer
int64     - 64-bit signed integer
float     - 32-bit floating point
double    - 64-bit floating point
f64       - Alias for double
bool      - Boolean type
char      - Single character
string    - UTF-8 string
void      - No return value
```

### Operators

**Arithmetic:**
```
+    Addition
-    Subtraction (or unary negation)
*    Multiplication
/    Division
%    Modulo
```

**Comparison:**
```
==   Equality
!=   Inequality
<    Less than
<=   Less than or equal
>    Greater than
>=   Greater than or equal
```

**Logical:**
```
&&   Logical AND
||   Logical OR
!    Logical NOT
```

**Assignment:**
```
=    Assignment
```

**Special:**
```
?    Nullable type marker
->   Function return type
..   Range operator
.    Member access
```

### Delimiters

```
;    Semicolon (statement terminator)
,    Comma (separator)
:    Colon (type annotation)
( )  Parentheses
{ }  Braces
[ ]  Brackets (arrays, indexing)
```

### Literals

**Integer Literals:**
```antlr
INTEGER_LITERAL: [0-9]+
```
Examples: `0`, `42`, `1000`

**Floating Point Literals:**
```antlr
FLOATING_LITERAL: [0-9]+ '.' [0-9]+
```
Examples: `3.14`, `0.5`, `100.0`

**String Literals:**
```antlr
STRING_LITERAL: '"' (~["\\\r\n] | '\\' .)* '"'
```
Examples: `"hello"`, `"line 1\nline 2"`

**Character Literals:**
```antlr
CHAR_LITERAL: '\'' (~['\\\r\n] | '\\' .) '\''
```
Examples: `'a'`, `'\n'`, `'\t'`

**Boolean Literals:**
```
true
false
```

**Null Literal:**
```
null
```

### Identifiers

```antlr
IDENTIFIER: [a-zA-Z_][a-zA-Z0-9_]*
```

Rules:
- Must start with letter or underscore
- Can contain letters, digits, and underscores
- Case-sensitive

Examples: `myVar`, `_private`, `User`, `value123`

### Comments

**Single-line comments:**
```antlr
SINGLE_LINE_COMMENT: '//' ~[\r\n]* -> skip
```
Example: `// This is a comment`

**Multi-line comments:**
```antlr
MULTI_LINE_COMMENT: '/*' .*? '*/' -> skip
```
Example:
```
/*
  Multi-line
  comment
*/
```

## Syntax Rules

### Compilation Unit

The top-level structure of a Hooc file:

```antlr
compilationUnit: importStatement* declaration* EOF;
```

A file consists of:
1. Zero or more import statements
2. Zero or more declarations (functions, classes, variables)

### Import Statements

**Basic import:**
```antlr
importStatement: IMPORT modulePath (AS IDENTIFIER)? SEMICOLON
```

Examples:
```hoo
import std.io;
import std.collections as coll;
```

**From import:**
```antlr
importStatement: FROM modulePath IMPORT importItem (COMMA importItem)* SEMICOLON
```

Examples:
```hoo
from std.io import File, Directory;
from std.collections import List as ArrayList;
```

**Module path:**
```antlr
modulePath: IDENTIFIER (DOT IDENTIFIER)*
```

Examples: `std`, `std.io`, `std.collections`

### Declarations

#### Function Declaration

```antlr
functionDeclaration:
    FUNC (COLON type)? IDENTIFIER LPAREN parameterList? RPAREN block
```

**Components:**
- Function name (identifier)
- Parameter list: `(param1: type1, param2: type2)`
- Optional return type: `: type` (preceding name)
- Function body (block)

**Examples:**

```hoo
// Simple function (returns void)
func greet() {
    print("Hello");
}

// With parameters and return type
func:int64 add(a: int64, b: int64) {
    return a + b;
}
```

**Parameter list:**
```antlr
parameterList: parameter (COMMA parameter)*
parameter: IDENTIFIER COLON type
```

#### Class Declaration

```antlr
classDeclaration:
    classModifier* CLASS IDENTIFIER (EXTENDS IDENTIFIER)? classBody
```

**Components:**
- Optional modifiers: `singleton`, `immutable`, `final`, etc.
- Class name
- Optional base class: `extends BaseClass`
- Class body

**Class modifiers:**
```antlr
classModifier:
    SINGLETON | IMMUTABLE | FACTORY | OBSERVABLE |
    SERVICE | STRATEGY | ACTOR | FINAL
```

**Examples:**

```hoo
// Simple class
class Point {
    var x: int64;
    var y: int64;
}

// With constructor
class Rectangle {
    var width: int64;
    var height: int64;

    constructor(w: int64, h: int64) {
        this.width = w;
        this.height = h;
    }
}

// With inheritance
class ColoredPoint extends Point {
    var color: string;
}

// Singleton pattern
singleton class Database {
    // ...
}

// Immutable class
immutable class ImmutablePoint {
    var x: int64;
    var y: int64;
}
```

**Class body:**
```antlr
classBody: LBRACE classMember* RBRACE

classMember:
    | variableDeclaration SEMICOLON
    | constructorDeclaration
    | functionDeclaration
    | eventDeclaration SEMICOLON
```

**Constructor:**
```antlr
constructorDeclaration: CONSTRUCTOR LPAREN parameterList? RPAREN block
```

#### Variable Declaration

```antlr
variableDeclaration:
    | VAR IDENTIFIER ASSIGN expression
    | VAR IDENTIFIER COLON type (ASSIGN expression)?
```

**Examples:**

```hoo
// Type inference
var x = 42;
var name = "Alice";

// Explicit type
var count: int64 = 0;
var flag: bool;

// With nullable type
var maybeValue: int64? = null;
```

### Type System

```antlr
type: optionalType

optionalType: arrayType QUESTION?

arrayType: baseType (LBRACKET RBRACKET)*

baseType:
    | primitiveType
    | qualifiedIdentifier

primitiveType:
    BYTE | UINT8 | INT64 | FLOAT | DOUBLE | F64 |
    BOOL | CHAR | STRING | VOID
```

**Type Examples:**

```hoo
// Primitive types
var a: int64;
var b: double;
var c: bool;
var s: string;

// Array types
var arr: int64[];
var matrix: double[][];

// Nullable types
var maybe: int64?;
var optionalStr: string?;

// Complex types using arrays
var arrays: int64[][]?;
```

**Qualified identifiers (for modules):**
```antlr
qualifiedIdentifier: IDENTIFIER (DOT IDENTIFIER)*
```

Examples: `String`, `std.String`, `std.collections.List`

### Statements

```antlr
statement:
    | block
    | variableDeclaration SEMICOLON
    | expressionStatement SEMICOLON
    | returnStatement SEMICOLON
    | ifStatement
    | whileStatement
    | forStatement
    | scopeStatement
```

#### Block Statement

```antlr
block: LBRACE statement* RBRACE
```

Example:
```hoo
{
    var x = 10;
    var y = 20;
    print(x + y);
}
```

#### If Statement

```antlr
ifStatement: IF expression block (ELSE block)?
```

**Examples:**

```hoo
// Simple if
if x > 0 {
    print("positive");
}

// If-else
if x > 0 {
    print("positive");
} else {
    print("non-positive");
}

// Nested if-else
if x > 0 {
    print("positive");
} else {
    if x < 0 {
        print("negative");
    } else {
        print("zero");
    }
}
```

#### While Statement

```antlr
whileStatement: WHILE expression block
```

**Example:**

```hoo
var count = 0;
while count < 10 {
    print(count);
    count = count + 1;
}
```

#### For Statement

```antlr
forStatement:
    | FOR IDENTIFIER IN expression block              // For-in
    | FOR IDENTIFIER IN expression RANGE expression (BY expression)? block  // For-range
```

**Examples:**

```hoo
// For-in (iterate over collection)
for item in collection {
    print(item);
}

// For-range loop
for i in 0 .. 10 {
    print(i);
}

// For-range with step
for i in 0 .. 10 by 2 {
    print(i);
}

// Reverse range
for i in 10 .. 0 by -1 {
    print(i);
}

```

#### Return Statement

```antlr
returnStatement: RETURN expression?
```

**Examples:**

```hoo
return;           // Return void
return 42;        // Return value
return x + y;     // Return expression
```

#### Scope Statement

```antlr
scopeStatement: SCOPE block
```

**Example:**

```hoo
scope {
    var temp = getValue();
    // temp is automatically released at end of scope
}
```

### Expressions

Expressions follow standard operator precedence (from lowest to highest):

1. Assignment (`=`)
2. Logical OR (`||`)
3. Logical AND (`&&`)
4. Relational (`==`, `!=`, `<`, `<=`, `>`, `>=`)
5. Additive (`+`, `-`)
6. Multiplicative (`*`, `/`, `%`)
7. Unary (`-`, `!`)
8. Postfix (member access, calls, indexing)
9. Primary (literals, identifiers, parentheses)

#### Expression Grammar

```antlr
expression: assignmentExpression

assignmentExpression:
    logicalOrExpression (ASSIGN assignmentExpression)?

logicalOrExpression:
    logicalAndExpression (OR logicalAndExpression)*

logicalAndExpression:
    relationalExpression (AND relationalExpression)*

relationalExpression:
    additiveExpression ((EQUALS | NOT_EQUALS | LESS | LESS_EQUALS |
                        GREATER | GREATER_EQUALS) additiveExpression)*

additiveExpression:
    multiplicativeExpression ((PLUS | MINUS) multiplicativeExpression)*

multiplicativeExpression:
    unaryExpression ((MULTIPLY | DIVIDE | MODULO) unaryExpression)*

unaryExpression:
    (MINUS | NOT)? postfixExpression

postfixExpression:
    primary (
        DOT IDENTIFIER                             // Member access
      | LBRACKET expression RBRACKET              // Array indexing
      | LPAREN argumentList? RPAREN               // Function call
    )*

primary:
    | IDENTIFIER
    | INTEGER_LITERAL
    | FLOATING_LITERAL
    | STRING_LITERAL
    | CHAR_LITERAL
    | TRUE
    | FALSE
    | NULL
    | LBRACKET expressionList? RBRACKET          // Array literal
    | LPAREN expression RPAREN                    // Grouped expression
    | newExpression                               // Object creation
```

#### Object Creation (New Expression)

```antlr
newExpression:
    NEW qualifiedIdentifier LPAREN argumentList? RPAREN
```

**Examples:**

```hoo
var obj = new Object();
var point = new Point(10, 20);
var rect = new geometry.Rectangle(5, 10);
```

#### Array Literals

```antlr
LBRACKET expressionList? RBRACKET

expressionList: expression (COMMA expression)*
```

**Examples:**

```hoo
var empty: int64[] = [];
var numbers = [1, 2, 3, 4, 5];
var mixed = [1, 2 + 3, getValue()];
var matrix = [[1, 2], [3, 4]];
```

#### Function Calls

```antlr
LPAREN argumentList? RPAREN

argumentList: expression (COMMA expression)*
```

**Examples:**

```hoo
// Simple call
print("hello");

// With multiple arguments
add(10, 20);

// Method call
obj.method(arg1, arg2);

// Chained calls
obj.getChild().getName();
```

## Operator Precedence Table

| Precedence | Operator | Description | Associativity |
|------------|----------|-------------|---------------|
| 1 (lowest) | `=` | Assignment | Right |
| 2 | `\|\|` | Logical OR | Left |
| 3 | `&&` | Logical AND | Left |
| 4 | `==`, `!=` | Equality | Left |
| 5 | `<`, `<=`, `>`, `>=` | Relational | Left |
| 6 | `+`, `-` | Addition, Subtraction | Left |
| 7 | `*`, `/`, `%` | Multiplication, Division, Modulo | Left |
| 8 | `-`, `!` | Unary minus, Logical NOT | Right |
| 9 | `.`, `[]`, `()` | Member access, Index, Call | Left |
| 10 (highest) | literals, `()` | Literals, Grouping | N/A |

## Complete Grammar Examples

### Example 1: Simple Program

```hoo
func main() {
    var message: string = "Hello, World!";
    print(message);
}
```

### Example 2: Classes and Objects

```hoo
class Person {
    var name: string;
    var age: int64;

    constructor(name: string, age: int64) {
        this.name = name;
        this.age = age;
    }

    func greet() {
        print("Hello, my name is " + name);
    }
}

func main() {
    var person = new Person("Alice", 30);
    person.greet();
}
```

### Example 4: Control Flow

```hoo
func:int64 factorial(n: int64) {
    if n <= 1 {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

func printNumbers(max: int64) {
    var i = 0;
    while i < max {
        print(i);
        i = i + 1;
    }
}

func main() {
    var result = factorial(5);
    print(result);

    printNumbers(10);
}
```

### Example 5: Module System

```hoo
import std.io as io;

func main() {
    var arr: int64[] = [1, 2, 3, 4, 5];
    var total = 0;
    for item in arr {
        total = total + item;
    }
    io.println("Sum: " + total);
}

```

## Grammar File Location

The complete, authoritative grammar is maintained in:
```
src/Hooc.g4
```

To regenerate the parser after modifying the grammar:
```bash
cd build
make clean_generated
make generate_parser
make
```

## See Also

- [Features Guide](features.md) - Detailed feature documentation
- [Implementation Status](implementation-status.md) - Current implementation status
- [ANTLR4 Documentation](https://github.com/antlr/antlr4/blob/master/doc/index.md)
