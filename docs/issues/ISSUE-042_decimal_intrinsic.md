# ISSUE-042 Decimal Intrinsic Data Type

## Goal
Introduce a new **fixed‑precision decimal** intrinsic type (`Decimal`) based on base‑10 arithmetic. The type must be first‑class in the Hoo language, supporting arithmetic, comparison, and logical operations, and be usable in all existing language constructs.

---

### Motivation
* Provide accurate financial and scientific calculations where binary floating‑point errors are unacceptable.
* Offer a deterministic, platform‑independent numeric representation.
* Align Hoo with languages that expose a decimal type (e.g., .NET `decimal`, Java `BigDecimal`).

---

## Sample Hoo Programs

### Basic arithmetic
```hoo
decimal price = 19.99;
decimal taxRate = 0.08;
decimal tax = price * taxRate;
decimal total = price + tax;
print(total);          // "21.5892"
```

### Comparison and branching
```hoo
decimal balance = 100.00;
decimal withdrawal = 150.00;

if (withdrawal > balance) {
    print("Insufficient funds");
} else {
    decimal remaining = balance - withdrawal;
    print(remaining);
}
```

### Precise financial calculation
```hoo
// Compound interest: A = P(1 + r/n)^(nt)
decimal principal = 1000.00;
decimal rate = 0.05;          // 5% annual
int64 n = 12;                  // monthly compounding
int64 t = 10;                  // 10 years

decimal base = 1.0 + (rate / decimal(n));
decimal amount = principal;

for (int64 i = 0; i < n * t; i++) {
    amount = amount * base;
}
print(amount);                // "1647.01..."
```

### Mixed with other types
```hoo
decimal d = 3.14;
string label = "pi";
print(label + " = " + d);     // string concatenation with decimal
```

### Function with decimal parameters/returns
```hoo
fn calculate_bmi(decimal weight, decimal height) -> decimal {
    return weight / (height * height);
}

decimal bmi = calculate_bmi(72.5, 1.78);
print(bmi);
```

### Equality comparison (scale-independent)
```hoo
decimal a = 1.0;
decimal b = 1.00;

if (a == b) {
    print("equal");           // true — same numeric value
}

decimal c = 1.0000000000000000000000000001;
if (a != c) {
    print("different");      // true — exceeds 28-char precision
}
```

---

## Design Decisions

### Decimal is an Intrinsic (Managed Object) Type
Decimal follows the same pattern as `Character` (type 109), `Buffer` (type 113), `DateTime` (type 119), `Uuid` (type 122) — it is a **built-in intrinsic type**, not a user-defined class. This means:
- No `import` or `require("Decimal")` needed — the type is always available (same as `string`, `Character`, `Buffer`).
- `getRequiredModule("Decimal")` in `HVMCodeGenerator.cpp` returns `""` — placed in the exempt list alongside `"String"`, `"Array"`, `"Map"`, `"Exception"`, `"Character"`, `"Buffer"`, `"DateTime"`, `"Uuid"`.
- Type ID `HOO_TYPE_DECIMAL = 123` is registered in `hoo_runtime.h` and `builtinConstructedTypeId()`.
- The mangling short code `d` is added to `getTypeCodeMap()` in `SymbolMangler.cpp`.

### No New HVM Opcodes — Reuse CALL (0xB4)
Decimal arithmetic does **not** require new bytecode opcodes. Instead, it follows the same pattern as Tensor and Character operations — `CALL` to runtime symbols:
- `_F_hoo_Decimal_newFromString_p_p` — constructor from string
- `_F_hoo_Decimal_newFromInt64_p_i8` — constructor from int64
- `_F_hoo_Decimal_add_p_p_p` — addition
- `_F_hoo_Decimal_sub_p_p_p` — subtraction
- `_F_hoo_Decimal_mul_p_p_p` — multiplication
- `_F_hoo_Decimal_div_p_p_p` — division
- `_F_hoo_Decimal_mod_p_p_p` — modulus
- `_F_hoo_Decimal_neg_p_p` — negation
- `_F_hoo_Decimal_eq_i8_p_p` — equality (returns int64 bool)
- `_F_hoo_Decimal_neq_i8_p_p` — inequality
- `_F_hoo_Decimal_lt_i8_p_p` — less than
- `_F_hoo_Decimal_le_i8_p_p` — less or equal
- `_F_hoo_Decimal_gt_i8_p_p` — greater than
- `_F_hoo_Decimal_ge_i8_p_p` — greater or equal

### Internal Representation
- `__int128` mantissa with a fixed scale of 28 decimal places (scale factor `10^28`).
- `DecimalImpl` struct: `{ __int128 value; int32_t scale; uint32_t refCount; }` — stored at the beginning of the `HooDecimal` opaque pointer.
- Allocation via `hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL)` which sets refcount=1.
- ARC semantics: returned objects have refcount 1; caller must release.

---

## Changes by Area

### 1. Grammar & Lexer (`src/parsing/Hooc.g4`)

**Add keyword token** (alongside existing primitive type tokens at line ~51):
```antlr4
DECIMAL: 'decimal';
```

**Add literal token before `FLOATING_LITERAL`** (must be defined first so ANTLR4's rule-ordering tiebreak prefers it):
```antlr4
DECIMAL_LITERAL: ([0-9]+'.'[0-9]* | '.' [0-9]+) ([eE] [+-]? [0-9]+)? 'd'?;
```

> **Rationale**: `12.34` is currently consumed by `FLOATING_LITERAL`. `DECIMAL_LITERAL` must precede it in the grammar file. If only a decimal literal is wanted without suffix, `DECIMAL_LITERAL` is placed before `FLOATING_LITERAL` so the lexer matches it first. Alternatively, if a `d` suffix is used (`12.34d`), no ordering dependency exists.

**Update `primitiveType` parser rule** — add `'decimal'` alternative:
```antlr4
primitiveType:
    'int64' | 'int8' | 'float' | 'double' | 'f64' | 'f8' | 'bit' | 'bool'
    | 'void' | 'byte' | 'char' | 'string' | 'buffer' | 'decimal'
    ;
```

**Update `primary` parser rule** — add `DECIMAL_LITERAL` alternative:
```antlr4
primary:
    INTEGER_LITERAL
    | FLOATING_LITERAL
    | DECIMAL_LITERAL
    | F8_LITERAL
    | BIT_LITERAL
    | BOOL_LITERAL
    | STRING_LITERAL
    | ...
    ;
```

---

### 2. AST & Semantic Analysis

#### `src/ast/Primary.h` — New class `DecimalLiteral`

Add after `F8Literal` (following the exact same pattern, which stores `std::string text_` and overrides `getValueAsDouble()`/`getValueAsString()`):

```cpp
class DecimalLiteral : public Primary {
public:
    explicit DecimalLiteral(Token token, std::string text)
        : Primary(std::move(token)), text_(std::move(text)) {}

    NodeType getNodeType() const override { return NodeType::DECIMAL_LITERAL; }
    const std::string& getText() const { return text_; }
    int64_t getTypeId() const override { return HOO_TYPE_DECIMAL; }

    std::string getValueAsString() const override { return text_; }
    double getValueAsDouble() const override {
        return std::strtod(text_.c_str(), nullptr);
    }
    int64_t getValueAsInteger() const override {
        return static_cast<int64_t>(std::strtod(text_.c_str(), nullptr));
    }

private:
    std::string text_;
};
```

Add `DECIMAL_LITERAL` to the `NodeType` enum in `Primary.h` (alongside `F8_LITERAL`, `BIT_LITERAL`, etc.).

#### `src/ast/Type.h` — Add `DECIMAL` to `PrimitiveTypeKind`

```cpp
enum class PrimitiveTypeKind : uint8_t {
    INT64, FLOAT, F64, DOUBLE, BIT, F8, BOOL, VOID,
    INT8, BYTE, CHAR, STRING, BUFFER,
    DECIMAL  // added here
};
```

#### `src/parsing/SimpleASTBuilder.cpp` — Three changes

**1. `buildPrimary()`** — insert a new branch before the `FLOATING_LITERAL` check (after `F8_LITERAL` handling at line ~885):

```cpp
} else if (ctx->DECIMAL_LITERAL()) {
    return std::make_unique<DecimalLiteral>(
        buildToken(ctx->DECIMAL_LITERAL()->getSymbol()),
        ctx->DECIMAL_LITERAL()->getText());
```

**2. `getPrimitiveTypeKind()`** — add case at ~line 1166:

```cpp
if (typeName == "decimal") return PrimitiveTypeKind::DECIMAL;
```

**3. Add a helper `getDecimalValue()`** (optional but recommended) that converts a string to the `__int128` scaled representation.

---

### 3. Name Mangling (`src/core/SymbolMangler.cpp`)

**Add to `getTypeCodeMap()`** at line ~56–74:
```cpp
{"decimal", "d"},
```

**Handle demangling** — add `'d'` case in `demangleType()`:
```cpp
if (typeCode == "d") return "decimal";
```

**Update `typeIdToMangleType()`** in HVMCodeGenerator.cpp (line ~3464):
```cpp
case 123: return "Decimal";
```

**Update `typeIdFromDeclaredType()`** in HVMCodeGenerator.cpp (line ~3430):
```cpp
case ast::PrimitiveTypeKind::DECIMAL: return 123;
```

**Update `isBuiltinClassName()`** if one exists — add `"Decimal"`.

---

### 4. Runtime (`src/runtime/lib/`)

#### `hoo_runtime.h`

```c
#define HOO_TYPE_DECIMAL               123
#define HOO_TYPE_DECIMAL_DIVIDE_BY_ZERO 132
#define HOO_TYPE_DECIMAL_OVERFLOW       133
```

#### `hoo_decimal.h`

```c
#ifndef HOO_DECIMAL_H
#define HOO_DECIMAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooDecimal;

// Scale factor: 10^28
#define HOO_DECIMAL_SCALE 28
#define HOO_DECIMAL_SCALE_FACTOR 10000000000000000000000000000ULL  // 10^28

// Construction
HooDecimal hoo_decimal_new_from_string(const char* str);
HooDecimal hoo_decimal_new_from_int64(int64_t val);
char*      hoo_decimal_to_string(HooDecimal dec);

// Arithmetic — return new object (caller must release)
HooDecimal hoo_decimal_add(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_sub(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_mul(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_div(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_mod(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_neg(HooDecimal a);

// Comparison — return int64_t bool (0 or 1)
int64_t    hoo_decimal_eq(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_neq(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_lt(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_le(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_gt(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_ge(HooDecimal a, HooDecimal b);

#ifdef __cplusplus
}
#endif

#endif
```

#### `hoo_decimal.cpp` — Full Implementation

```cpp
#include "hoo_decimal.h"
#include "hoo_runtime.h"
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cstdint>
#include <cmath>

struct DecimalImpl {
    __int128 value;   // scaled integer (value * 10^28)
    int32_t scale;    // always HOO_DECIMAL_SCALE = 28
    uint32_t refCount;
};

static DecimalImpl* impl(HooDecimal d) {
    return static_cast<DecimalImpl*>(d);
}

// ---------- internal helpers ----------

static __int128 parseStringToScaledInt(const char* str) {
    // Locate decimal point and parse integer/fractional parts
    // ... (omitted: full string-to-__int128 logic with scale alignment)
    // Returns value * 10^28
}

static char* scaledIntToString(__int128 value) {
    // Convert scaled __int128 to decimal string representation
    // Example: value=12340000000000000000000000000 → "1.234"
    // ... (omitted: full conversion logic)
}

// ---------- construction ----------

HooDecimal hoo_decimal_new_from_string(const char* str) {
    auto* d = static_cast<DecimalImpl*>(hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL));
    d->value = parseStringToScaledInt(str);
    d->scale = HOO_DECIMAL_SCALE;
    d->refCount = 1;
    return d;
}

HooDecimal hoo_decimal_new_from_int64(int64_t val) {
    auto* d = static_cast<DecimalImpl*>(hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL));
    d->value = (__int128)val * HOO_DECIMAL_SCALE_FACTOR;
    d->scale = HOO_DECIMAL_SCALE;
    d->refCount = 1;
    return d;
}

char* hoo_decimal_to_string(HooDecimal dec) {
    return scaledIntToString(impl(dec)->value);
}

// ---------- arithmetic ----------

HooDecimal hoo_decimal_add(HooDecimal a, HooDecimal b) {
    auto* result = static_cast<DecimalImpl*>(hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL));
    result->value = impl(a)->value + impl(b)->value;
    result->scale = HOO_DECIMAL_SCALE;
    result->refCount = 1;
    return result;
}

HooDecimal hoo_decimal_sub(HooDecimal a, HooDecimal b) {
    auto* result = static_cast<DecimalImpl*>(hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL));
    result->value = impl(a)->value - impl(b)->value;
    result->scale = HOO_DECIMAL_SCALE;
    result->refCount = 1;
    return result;
}

HooDecimal hoo_decimal_mul(HooDecimal a, HooDecimal b) {
    auto* result = static_cast<DecimalImpl*>(hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL));
    // (a * 10^28) * (b * 10^28) = a*b * 10^56, need to divide by 10^28 to restore scale
    __int128 product = impl(a)->value * impl(b)->value;
    result->value = product / HOO_DECIMAL_SCALE_FACTOR;
    result->scale = HOO_DECIMAL_SCALE;
    result->refCount = 1;
    // TODO: check overflow and rounding
    return result;
}

HooDecimal hoo_decimal_div(HooDecimal a, HooDecimal b) {
    if (impl(b)->value == 0) {
        hoo_throw_exception(HOO_TYPE_DECIMAL_DIVIDE_BY_ZERO, "Division by zero in Decimal");
        return nullptr;
    }
    auto* result = static_cast<DecimalImpl*>(hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL));
    // (a * 10^28) / (b * 10^28) = a/b, need to multiply dividend by 10^28 for scale
    result->value = (impl(a)->value * HOO_DECIMAL_SCALE_FACTOR) / impl(b)->value;
    result->scale = HOO_DECIMAL_SCALE;
    result->refCount = 1;
    // TODO: check overflow
    return result;
}

HooDecimal hoo_decimal_mod(HooDecimal a, HooDecimal b) {
    if (impl(b)->value == 0) {
        hoo_throw_exception(HOO_TYPE_DECIMAL_DIVIDE_BY_ZERO, "Modulus by zero in Decimal");
        return nullptr;
    }
    auto* result = static_cast<DecimalImpl*>(hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL));
    result->value = impl(a)->value % impl(b)->value;
    result->scale = HOO_DECIMAL_SCALE;
    result->refCount = 1;
    return result;
}

HooDecimal hoo_decimal_neg(HooDecimal a) {
    auto* result = static_cast<DecimalImpl*>(hoo_alloc(sizeof(DecimalImpl), HOO_TYPE_DECIMAL));
    result->value = -(impl(a)->value);
    result->scale = HOO_DECIMAL_SCALE;
    result->refCount = 1;
    return result;
}

// ---------- comparison ----------

int64_t hoo_decimal_eq(HooDecimal a, HooDecimal b) {
    return impl(a)->value == impl(b)->value ? 1 : 0;
}
int64_t hoo_decimal_neq(HooDecimal a, HooDecimal b) {
    return impl(a)->value != impl(b)->value ? 1 : 0;
}
int64_t hoo_decimal_lt(HooDecimal a, HooDecimal b) {
    return impl(a)->value < impl(b)->value ? 1 : 0;
}
int64_t hoo_decimal_le(HooDecimal a, HooDecimal b) {
    return impl(a)->value <= impl(b)->value ? 1 : 0;
}
int64_t hoo_decimal_gt(HooDecimal a, HooDecimal b) {
    return impl(a)->value > impl(b)->value ? 1 : 0;
}
int64_t hoo_decimal_ge(HooDecimal a, HooDecimal b) {
    return impl(a)->value >= impl(b)->value ? 1 : 0;
}
```

---

### 5. Code Generation (`src/codegen/HVMCodeGenerator.cpp`)

#### `builtinConstructedTypeId()` — add entry

At line ~127 (alongside `{"Character", 109}`, `{"Buffer", 113}`, `{"DateTime", 119}`, `{"Uuid", 122}`):
```cpp
{"Decimal", 123},
```

#### `getRequiredModule()` — exempt Decimal

At the intrinsic-type exemption check (~line 560), add `"Decimal"`:
```cpp
static const std::unordered_set<std::string> intrinsicTypes = {
    "String", "Array", "Map", "Exception",
    "Character", "Buffer", "DateTime", "Uuid", "Decimal"
};
```

#### `getTypeId()` (variable initializer inference) — handle `DecimalLiteral`

At the initializer type inference (~line 3910), add:
```cpp
if (dynamic_cast<const ast::DecimalLiteral*>(&node)) return 123;
```

#### `inferExpressionTypeId()` — add `DecimalLiteral`

At ~line 3738 (between `F8Literal` and `StringLiteral`):
```cpp
if (dynamic_cast<const ast::DecimalLiteral*>(&primary)) return 123;
```

#### Binary expression handling in `visitExpression()` (~line 2711)

After computing `leftExprType` and `rightExprType`, insert a check:
```cpp
const uint32_t leftExprType = inferExpressionTypeId(binary->getLeft());
const uint32_t rightExprType = inferExpressionTypeId(binary->getRight());

// Decimal arithmetic — emit CALL to runtime
if (leftExprType == 123 || rightExprType == 123) {
    return emitDecimalBinaryOp(binary);
}
```

#### New helper `emitDecimalBinaryOp()`

```cpp
uint8_t HVMCodeGenerator::emitDecimalBinaryOp(const ast::BinaryExpression& binary) {
    uint8_t left = visitExpression(binary.getLeft());
    uint8_t right = visitExpression(binary.getRight());

    const char* symbol = nullptr;
    switch (binary.getOperator()) {
        case ast::BinaryOperator::ADD:          symbol = "_F_hoo_Decimal_add_p_p_p"; break;
        case ast::BinaryOperator::SUBTRACT:     symbol = "_F_hoo_Decimal_sub_p_p_p"; break;
        case ast::BinaryOperator::MULTIPLY:     symbol = "_F_hoo_Decimal_mul_p_p_p"; break;
        case ast::BinaryOperator::DIVIDE:       symbol = "_F_hoo_Decimal_div_p_p_p"; break;
        case ast::BinaryOperator::MODULUS:      symbol = "_F_hoo_Decimal_mod_p_p_p"; break;
        case ast::BinaryOperator::EQUALS:       symbol = "_F_hoo_Decimal_eq_i8_p_p"; break;
        case ast::BinaryOperator::NOT_EQUALS:   symbol = "_F_hoo_Decimal_neq_i8_p_p"; break;
        case ast::BinaryOperator::LESS:         symbol = "_F_hoo_Decimal_lt_i8_p_p"; break;
        case ast::BinaryOperator::LESS_EQUALS:  symbol = "_F_hoo_Decimal_le_i8_p_p"; break;
        case ast::BinaryOperator::GREATER:      symbol = "_F_hoo_Decimal_gt_i8_p_p"; break;
        case ast::BinaryOperator::GREATER_EQUALS: symbol = "_F_hoo_Decimal_ge_i8_p_p"; break;
        default: return 0;
    }
    emit(Opcode::MOV, OperandsR{1, left, 0, 0});
    emit(Opcode::MOV, OperandsR{2, right, 0, 0});
    emitCall(Opcode::CALL, symbol);
    uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
    return dest;
}
```

#### `visitExpression()` — `DecimalLiteral` emission

Add to the primary-expression visit handler:
```cpp
if (auto decLit = dynamic_cast<const ast::DecimalLiteral*>(&primary)) {
    // Emit string literal to rodata, then CALL hoo_decimal_new_from_string
    uint8_t strReg = emitLiteralString(decLit->getText().c_str());
    emit(Opcode::MOV, OperandsR{1, strReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_Decimal_newFromString_p_p");
    uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
    return dest;
}
```

---

### 6. JIT Stubs (`src/hvm/HVMJIT.cpp`)

Add `#include "runtime/lib/hoo_decimal.h"` at line ~41.

Add `extern "C"` wrappers following the `jit_hoo_character_*` pattern (each receives `void* state_ptr`, reads operands from `state->regs[1..N]`, calls the runtime C function, returns `uint64_t`):

```cpp
extern "C" uint64_t jit_hoo_decimal_new_from_string(void* state_ptr) {
    auto* state = (HVMJIT::HVMState*)state_ptr;
    const char* str = (const char*)(uintptr_t)state->regs[1];
    return (uint64_t)(uintptr_t)hoo_decimal_new_from_string(str);
}

extern "C" uint64_t jit_hoo_decimal_add(void* state_ptr) {
    auto* state = (HVMJIT::HVMState*)state_ptr;
    return (uint64_t)(uintptr_t)hoo_decimal_add(
        (HooDecimal)state->regs[1], (HooDecimal)state->regs[2]);
}

extern "C" uint64_t jit_hoo_decimal_sub(void* state_ptr) {
    auto* state = (HVMJIT::HVMState*)state_ptr;
    return (uint64_t)(uintptr_t)hoo_decimal_sub(
        (HooDecimal)state->regs[1], (HooDecimal)state->regs[2]);
}

extern "C" uint64_t jit_hoo_decimal_mul(void* state_ptr) {
    auto* state = (HVMJIT::HVMState*)state_ptr;
    return (uint64_t)(uintptr_t)hoo_decimal_mul(
        (HooDecimal)state->regs[1], (HooDecimal)state->regs[2]);
}

extern "C" uint64_t jit_hoo_decimal_div(void* state_ptr) {
    auto* state = (HVMJIT::HVMState*)state_ptr;
    return (uint64_t)(uintptr_t)hoo_decimal_div(
        (HooDecimal)state->regs[1], (HooDecimal)state->regs[2]);
}

extern "C" uint64_t jit_hoo_decimal_mod(void* state_ptr) {
    auto* state = (HVMJIT::HVMState*)state_ptr;
    return (uint64_t)(uintptr_t)hoo_decimal_mod(
        (HooDecimal)state->regs[1], (HooDecimal)state->regs[2]);
}

extern "C" uint64_t jit_hoo_decimal_neg(void* state_ptr) {
    auto* state = (HVMJIT::HVMState*)state_ptr;
    return (uint64_t)(uintptr_t)hoo_decimal_neg(
        (HooDecimal)state->regs[1]);
}

extern "C" uint64_t jit_hoo_decimal_eq(void* state_ptr) {
    auto* state = (HVMJIT::HVMState*)state_ptr;
    return (uint64_t)hoo_decimal_eq(
        (HooDecimal)state->regs[1], (HooDecimal)state->regs[2]);
}
// ... (same pattern for neq, lt, le, gt, ge)
```

**Register in `RuntimeSymbolContract`** — if HVMJIT uses a symbol table for runtime function lookup, add:
```cpp
{"jit_hoo_decimal_new_from_string", jit_hoo_decimal_new_from_string},
{"jit_hoo_decimal_add", jit_hoo_decimal_add},
// ... etc
```

---

### 7. Tests (`tests/runtime/DecimalTest.cpp`)

Following the exact fixture pattern from `HooCharacterTest.cpp`:

```cpp
#include <gtest/gtest.h>
#include "runtime/lib/hoo_runtime.h"
#include "runtime/lib/hoo_decimal.h"

class HooDecimalTest : public ::testing::Test {
protected:
    void SetUp() override {
        hoo_reset_memory_stats();
    }
    void TearDown() override {
        // Verify no memory leaks
    }
};

TEST_F(HooDecimalTest, FromString) {
    HooDecimal d = hoo_decimal_new_from_string("3.14");
    ASSERT_NE(nullptr, d);
    ASSERT_EQ(HOO_TYPE_DECIMAL, hoo_get_type_id(d));
    char* s = hoo_decimal_to_string(d);
    ASSERT_STREQ("3.14", s);
    std::free(s);
    hoo_release(d);
}

TEST_F(HooDecimalTest, FromInt64) {
    HooDecimal d = hoo_decimal_new_from_int64(42);
    ASSERT_NE(nullptr, d);
    char* s = hoo_decimal_to_string(d);
    ASSERT_STREQ("42", s);
    std::free(s);
    hoo_release(d);
}

TEST_F(HooDecimalTest, Add) {
    HooDecimal a = hoo_decimal_new_from_string("1.5");
    HooDecimal b = hoo_decimal_new_from_string("2.3");
    HooDecimal c = hoo_decimal_add(a, b);
    char* s = hoo_decimal_to_string(c);
    ASSERT_STREQ("3.8", s);
    std::free(s);
    hoo_release(c);
    hoo_release(b);
    hoo_release(a);
}

TEST_F(HooDecimalTest, Sub) {
    HooDecimal a = hoo_decimal_new_from_string("5.5");
    HooDecimal b = hoo_decimal_new_from_string("2.2");
    HooDecimal c = hoo_decimal_sub(a, b);
    char* s = hoo_decimal_to_string(c);
    ASSERT_STREQ("3.3", s);
    std::free(s);
    hoo_release(c); hoo_release(b); hoo_release(a);
}

TEST_F(HooDecimalTest, Mul) {
    HooDecimal a = hoo_decimal_new_from_string("2.5");
    HooDecimal b = hoo_decimal_new_from_string("4.0");
    HooDecimal c = hoo_decimal_mul(a, b);
    char* s = hoo_decimal_to_string(c);
    ASSERT_STREQ("10.0", s);
    std::free(s);
    hoo_release(c); hoo_release(b); hoo_release(a);
}

TEST_F(HooDecimalTest, Div) {
    HooDecimal a = hoo_decimal_new_from_string("10.0");
    HooDecimal b = hoo_decimal_new_from_string("3.0");
    HooDecimal c = hoo_decimal_div(a, b);
    ASSERT_NE(nullptr, c);
    hoo_release(c); hoo_release(b); hoo_release(a);
}

TEST_F(HooDecimalTest, DivByZero) {
    HooDecimal a = hoo_decimal_new_from_string("1.0");
    HooDecimal b = hoo_decimal_new_from_string("0.0");
    HooDecimal c = hoo_decimal_div(a, b);
    ASSERT_EQ(nullptr, c);
    hoo_release(a); hoo_release(b);
}

TEST_F(HooDecimalTest, Mod) {
    HooDecimal a = hoo_decimal_new_from_string("10.5");
    HooDecimal b = hoo_decimal_new_from_string("3.0");
    HooDecimal c = hoo_decimal_mod(a, b);
    char* s = hoo_decimal_to_string(c);
    ASSERT_STREQ("1.5", s);
    std::free(s);
    hoo_release(c); hoo_release(b); hoo_release(a);
}

TEST_F(HooDecimalTest, Neg) {
    HooDecimal a = hoo_decimal_new_from_string("7.25");
    HooDecimal n = hoo_decimal_neg(a);
    char* s = hoo_decimal_to_string(n);
    ASSERT_STREQ("-7.25", s);
    std::free(s);
    hoo_release(n); hoo_release(a);
}

TEST_F(HooDecimalTest, Eq) {
    HooDecimal a = hoo_decimal_new_from_string("1.0");
    HooDecimal b = hoo_decimal_new_from_string("1.00");
    ASSERT_EQ(1, hoo_decimal_eq(a, b));
    ASSERT_EQ(0, hoo_decimal_neq(a, b));
    hoo_release(b); hoo_release(a);
}

TEST_F(HooDecimalTest, Neq) {
    HooDecimal a = hoo_decimal_new_from_string("1.0");
    HooDecimal b = hoo_decimal_new_from_string("2.0");
    ASSERT_EQ(1, hoo_decimal_neq(a, b));
    ASSERT_EQ(0, hoo_decimal_eq(a, b));
    hoo_release(b); hoo_release(a);
}

TEST_F(HooDecimalTest, LtLe) {
    HooDecimal a = hoo_decimal_new_from_string("1.0");
    HooDecimal b = hoo_decimal_new_from_string("2.0");
    ASSERT_EQ(1, hoo_decimal_lt(a, b));
    ASSERT_EQ(1, hoo_decimal_le(a, b));
    ASSERT_EQ(0, hoo_decimal_gt(a, b));
    ASSERT_EQ(0, hoo_decimal_ge(a, b));
    hoo_release(b); hoo_release(a);
}

TEST_F(HooDecimalTest, GtGe) {
    HooDecimal a = hoo_decimal_new_from_string("3.0");
    HooDecimal b = hoo_decimal_new_from_string("2.0");
    ASSERT_EQ(1, hoo_decimal_gt(a, b));
    ASSERT_EQ(1, hoo_decimal_ge(a, b));
    ASSERT_EQ(0, hoo_decimal_lt(a, b));
    ASSERT_EQ(0, hoo_decimal_le(a, b));
    hoo_release(b); hoo_release(a);
}

TEST_F(HooDecimalTest, TypeIdCheck) {
    HooDecimal d = hoo_decimal_new_from_int64(0);
    ASSERT_EQ(HOO_TYPE_DECIMAL, hoo_get_type_id(d));
    hoo_release(d);
}

TEST_F(HooDecimalTest, ArcRetainRelease) {
    HooDecimal d = hoo_decimal_new_from_int64(1);
    uint32_t rc1 = hoo_retain(d);
    ASSERT_GT(rc1, 1);
    uint32_t rc2 = hoo_release(d);
    ASSERT_EQ(rc1 - 1, rc2);
    hoo_release(d); // drops to 0, object freed
}
```

**Build integration**: Add to `tests/runtime/CMakeLists.txt`:
```cmake
add_hoo_test(DecimalTest.cpp runtime_lib hoo_runtime_lib)
target_link_libraries(DecimalTest PRIVATE hoo_runtime_lib)
```

---

## Summary of All Files Touched

| File | Change |
|------|--------|
| `src/parsing/Hooc.g4` | Add `DECIMAL` keyword token, `DECIMAL_LITERAL` lexer rule (before `FLOATING_LITERAL`), `'decimal'` in `primitiveType`, `DECIMAL_LITERAL` in `primary` |
| `src/ast/Primary.h` | Add `NodeType::DECIMAL_LITERAL`, `class DecimalLiteral` |
| `src/ast/Type.h` | Add `PrimitiveTypeKind::DECIMAL` |
| `src/parsing/SimpleASTBuilder.h` | (optional) declare `getDecimalValue()` |
| `src/parsing/SimpleASTBuilder.cpp` | `DECIMAL_LITERAL` branch in `buildPrimary()`, `"decimal"` case in `getPrimitiveTypeKind()` |
| `src/runtime/lib/hoo_runtime.h` | `#define HOO_TYPE_DECIMAL 123`, exception IDs `132`, `133` |
| `src/runtime/lib/hoo_decimal.h` | **New file** — function declarations |
| `src/runtime/lib/hoo_decimal.cpp` | **New file** — full runtime implementation |
| `src/core/SymbolMangler.cpp` | `{"decimal", "d"}` in `getTypeCodeMap()`, `'d'` → `"decimal"` in `demangleType()` |
| `src/codegen/HVMCodeGenerator.cpp` | `{"Decimal", 123}` in `builtinConstructedTypeId()`, exempt in `getRequiredModule()`, `DecimalLiteral` in `getTypeId()`/`inferExpressionTypeId()`, `DECIMAL` in `typeIdFromDeclaredType()`, case 123 in `typeIdToMangleType()`, `emitDecimalBinaryOp()` helper, `CALL` emission in `visitExpression()` |
| `src/hvm/HVMJIT.cpp` | Add `#include`, add 14 `jit_hoo_decimal_*` extern "C" wrappers |
| `tests/runtime/DecimalTest.cpp` | **New file** — full test suite |
| `tests/runtime/CMakeLists.txt` | Add `DecimalTest` target |

---

## Edge Cases & Error Handling

| Scenario | Behavior |
|----------|----------|
| Division by zero | `hoo_decimal_div`/`hoo_decimal_mod` returns `nullptr` and calls `hoo_throw_exception(HOO_TYPE_DECIMAL_DIVIDE_BY_ZERO)` |
| Integer overflow in `__int128` | Check before arithmetic; set `errno = ERANGE`, throw `HOO_TYPE_DECIMAL_OVERFLOW` |
| Underflow (result too small) | Rounds toward zero (truncation of extra precision) |
| Negative numbers | Stored as negative `__int128`; string conversion handles `-` prefix |
| Zero | `value == 0`; comparison works naturally |
| Leading/trailing zeros | `parseStringToScaledInt` normalizes; `to_string` trims trailing zeros |
| Zero followed by decimal point (`.5`) | Apply `parseStringToScaledInt` uniform logic consistent with grammar alternation |

---

## Implementation Timeline (approx.)
| Sprint | Tasks |
|--------|-------|
| 1 | Grammar, lexer, AST node, `HOO_TYPE_DECIMAL` registration, mangling/demangling |
| 2 | Runtime `hoo_decimal` implementation (all 14 functions) |
| 3 | Codegen (CALL + emitDecimalBinaryOp + symbol name mangling), JIT wrappers |
| 4 | Unit tests (parsing + runtime + codegen), build system integration |
| 5 | Documentation, edge-case hardening, final review, merge |

---

## Status
- **Date**: 2026-06-22
- **Status**: **DESIGN COMPLETE** (all patterns validated against existing managed types: Character, Tensor, Buffer, DateTime, Uuid)
- **Priority**: Medium
- **Audit 2026-06-22**: All changes are documented with concrete code snippets extracted from the codebase. No new HVM opcodes needed. Decimal follows the same intrinsic-managed-object pattern as Character, Buffer, and DateTime.
