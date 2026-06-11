# Expressions & Operators

Expressions are combinations of literals, identifiers, and operators that evaluate to a single value.

## 1. Literals
Hoo supports several types of literals:
- **Numeric**: `42`, `3.14`.
- **String**: `"Hello"`, `"""Multiline"""`.
- **Interpolated String**: `"Value: ${x}"` (automatically converts primitives and joins parts).
- **Char**: `'A'`, `'€'`, `'😀'` (supports multi-byte UTF-8 Unicode codepoints).
- **Boolean**: `true`, `false`.
- **Null**: `null`.
- **Array**: `[1, 2, 3]`.

## 2. Operator Precedence

| Level | Operators | Description |
| :--- | :--- | :--- |
| 1 | `(expr)`, `new`, `this` | Primary / Grouping |
| 2 | `.`, `[idx]`, `(args)`, `++`, `--` | Postfix |
| 3 | `-`, `!` | Unary (Prefix) |
| 4 | `*`, `/`, `%` | Multiplicative |
| 5 | `+`, `-` | Additive |
| 6 | `==`, `!=`, `<`, `<=`, `>`, `>=` | Relational / Equality |
| 7 | `&&` | Logical AND |
| 8 | `||` | Logical OR |
| 9 | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `<<=`, `>>=` | Assignment |

## 3. Special Expressions

### New Expression
Creates an instance of a class.
- `new User("John")`
- `new math.Matrix(3, 3)`

### Member Access
- `obj.field`
- `obj.method(10)`

### Index Access
- `arr[0]`
- `map["key"]`

### This Keyword
Refers to the current object instance within a class method or constructor.
