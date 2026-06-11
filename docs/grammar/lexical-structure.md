# Lexical Structure

This document describes the low-level lexical components of the Hoo language, including keywords, identifiers, literals, and delimiters.

## 1. Keywords

Hoo reserves the following keywords for language constructs:

### General Keywords
| Keyword | Description |
| :--- | :--- |
| `func` | Declares a function. |
| `public`, `private` | Access modifiers. |
| `async` | Declares an asynchronous function. |
| `return` | Returns from a function. |
| `if`, `else` | Conditional statements. |
| `for`, `while`, `in` | Loop constructs. |
| `break`, `continue` | Loop control. |
| `class`, `extends` | Object-oriented declarations. |
| `import`, `from`, `as` | Module system keywords. |
| `new` | Creates a new object instance. |
| `var`, `const` | Variable and constant declarations. |
| `scope` | Isolated execution block. |
| `constructor` | Explicit instance initializer. |
| `this` | Reference to the current instance. |
| `true`, `false`, `null` | Literals. |
| `try`, `catch`, `finally` | Exception handling. |
| `throw`, `rethrow` | Raising exceptions. |
| `map` | Built-in dictionary type. |
| `function` | Function pointer type. |
| `__hoo_init` | Reserved module initialization marker. |

### Class Modifiers
Hoo supports advanced class-level modifiers:
`singleton`, `immutable`, `factory`, `observable`, `service`, `strategy`, `actor`, `final`.

## 2. Literals

### Numeric Literals
- **Integer**: Sequences of digits `[0-9]+`.
- **Floating Point**: Decimal numbers `[0-9]+ '.' [0-9]+`.

### String Literals
- **Standard**: Double-quoted strings with escape support: `"Hello\nWorld"`.
- **Interpolated**: Strings containing `${expression}` blocks.
- **Multiline**: Triple-quoted strings: `"""Line 1\nLine 2"""`.

### Character Literals
- Single-quoted Unicode codepoints: `'A'`, `'€'`, `'😀'`, `'\n'`. Supports multi-byte UTF-8.

### Boolean & Null
- `true`, `false`, `null`.

## 3. Identifiers

Identifiers must start with a letter or underscore, followed by any number of letters, digits, or underscores:
`[a-zA-Z_][a-zA-Z0-9_]*`

## 4. Delimiters & Operators

- **Delimiters**: `;`, `,`, `.`, `:`, `(`, `)`, `{`, `}`, `[`, `]`.
- **Operators**: `+`, `-`, `*`, `/`, `%`, `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `<<=`, `>>=`, `++`, `--`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`, `?`, `..`.

## 5. Comments & Whitespace

- **Single Line**: `// comment`
- **Multi-line**: `/* comment */`
- **Whitespace**: Spaces, tabs, and newlines are ignored (`-> skip`).
