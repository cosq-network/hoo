# String Library Integration Guide

## Overview

The hoort String library (`runtime/hoo_string.h` and `runtime/hoo_string.cpp`) provides a complete UTF-8 string implementation with automatic reference counting (ARC). This guide explains how to integrate it with HoocJIT and the code generator.

## Files Created

### 1. **runtime/hoo_string.h**
- Complete C API for string operations
- 30+ functions covering:
  - Creation/destruction (from_cstr, new, from_bytes, repeat)
  - Manipulation (concat, substring, to_upper, to_lower, trim, replace)
  - Query (length, data, byte_at, is_empty, index_of, contains, starts_with, ends_with)
  - Comparison (compare, equals, equals_ignore_case)
  - Reference counting (retain, release, refcount)
  - Conversion (to/from int64, double, bool, format)
  - Debugging (print, println, debug)

### 2. **runtime/hoo_string.cpp**
- Complete implementation with UTF-8 support
- Reference counting for ARC-based memory management
- Efficient string operations using standard C library
- Null-terminated strings for C compatibility

### 3. **CMakeLists.txt** (Updated)
- Added `runtime/hoo_string.cpp` to the `hoort` library compilation

## Integration Steps

### Step 1: HoocJIT Symbol Registration

In `src/HoocJIT.h`, add:

```cpp
class HoocJIT {
public:
    // ... existing methods ...

    /**
     * Register string runtime functions with the JIT engine
     */
    void registerStringFunctions();
};
```

### Step 2: HoocJIT Implementation

In `src/HoocJIT.cpp`, add the implementation:

```cpp
#include "runtime/hoo_string.h"

void HoocJIT::registerStringFunctions() {
    auto& mainJD = JIT->getMainJITDylib();

    llvm::orc::SymbolMap symbols;

    // String creation functions
    symbols[JIT->mangleAndIntern("hoo_string_from_cstr")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_from_cstr),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_new")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_new),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_from_bytes")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_from_bytes),
            llvm::JITSymbolFlags::Exported
        );

    // String manipulation functions
    symbols[JIT->mangleAndIntern("hoo_string_concat")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_concat),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_substring")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_substring),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_to_upper")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_to_upper),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_to_lower")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_to_lower),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_trim")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_trim),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_replace")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_replace),
            llvm::JITSymbolFlags::Exported
        );

    // String query functions
    symbols[JIT->mangleAndIntern("hoo_string_length")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_length),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_data")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_data),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_byte_at")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_byte_at),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_is_empty")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_is_empty),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_index_of")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_index_of),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_contains")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_contains),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_starts_with")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_starts_with),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_ends_with")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_ends_with),
            llvm::JITSymbolFlags::Exported
        );

    // String comparison functions
    symbols[JIT->mangleAndIntern("hoo_string_compare")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_compare),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_equals")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_equals),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_equals_ignore_case")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_equals_ignore_case),
            llvm::JITSymbolFlags::Exported
        );

    // Reference counting functions
    symbols[JIT->mangleAndIntern("hoo_string_retain")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_retain),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_release")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_release),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_refcount")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_refcount),
            llvm::JITSymbolFlags::Exported
        );

    // Conversion functions
    symbols[JIT->mangleAndIntern("hoo_string_from_int64")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_from_int64),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_from_double")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_from_double),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_from_bool")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_from_bool),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_to_int64")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_to_int64),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_to_double")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_to_double),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_format")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_format),
            llvm::JITSymbolFlags::Exported
        );

    // Debugging functions
    symbols[JIT->mangleAndIntern("hoo_string_print")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_print),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_println")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_println),
            llvm::JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_debug")] =
        llvm::JITEvaluatedSymbol(
            llvm::pointerToJITTargetAddress((void*)&hoo_string_debug),
            llvm::JITSymbolFlags::Exported
        );

    // Register all symbols with JIT
    llvm::cantFail(mainJD.define(llvm::orc::absoluteSymbols(symbols)));
}

// In HoocJIT constructor, call:
HoocJIT::HoocJIT() {
    // ... existing initialization ...
    registerStringFunctions();
}
```

### Step 3: Code Generator Integration

In `src/LLVMCodeGenerator.h`, add string support:

```cpp
class LLVMCodeGenerator : public CodeGenerator {
private:
    // String type and function declarations
    llvm::PointerType* stringType_;  // Represents HooString (opaque i8*)

    // String function pointers
    llvm::Function* hoo_string_from_cstr_func_;
    llvm::Function* hoo_string_new_func_;
    llvm::Function* hoo_string_concat_func_;
    llvm::Function* hoo_string_length_func_;
    // ... other functions

    // Helper methods
    void declareStringFunctions();
    llvm::Value* generateStringLiteral(const std::string& value);
};
```

### Step 4: String Literal Handling

In `src/LLVMCodeGenerator.cpp`:

```cpp
void LLVMCodeGenerator::declareStringFunctions() {
    // Define string type as opaque pointer (i8*)
    stringType_ = llvm::PointerType::getInt8PtrTy(*context_);

    // Declare hoo_string_from_cstr(const char*)
    {
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            stringType_,
            {llvm::PointerType::getInt8PtrTy(*context_)},
            false
        );
        hoo_string_from_cstr_func_ = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            "hoo_string_from_cstr",
            module_.get()
        );
    }

    // Declare hoo_string_concat(HooString, HooString)
    {
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            stringType_,
            {stringType_, stringType_},
            false
        );
        hoo_string_concat_func_ = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            "hoo_string_concat",
            module_.get()
        );
    }

    // ... declare other string functions similarly
}

llvm::Value* LLVMCodeGenerator::generateStringLiteral(const std::string& value) {
    // Create a global constant for the string data
    llvm::Constant* str_const = llvm::ConstantDataArray::getString(*context_, value, true);

    llvm::GlobalVariable* global_str = new llvm::GlobalVariable(
        *module_,
        str_const->getType(),
        true,  // isConstant
        llvm::GlobalValue::PrivateLinkage,
        str_const
    );
    global_str->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

    // Convert to i8*
    llvm::Value* str_ptr = builder_->CreateBitCast(
        global_str,
        llvm::PointerType::getInt8PtrTy(*context_),
        "str_ptr"
    );

    // Call hoo_string_from_cstr(global_str)
    llvm::Value* hoo_str = builder_->CreateCall(
        hoo_string_from_cstr_func_,
        {str_ptr},
        "hoo_str"
    );

    return hoo_str;
}
```

In the `generateLLVMExpression` method, handle string literals:

```cpp
if (auto* strLit = dynamic_cast<const ast::StringLiteral*>(&expr)) {
    return generateStringLiteral(strLit->getValue());
}
```

## Usage Example

Once integrated, hoo code can use strings naturally:

```hoo
func main() -> void {
    // String literals
    var greeting = "Hello, World!";
    var name = "Alice";

    // String concatenation
    var message = greeting + " My name is " + name;

    // String length
    var len = message.length();

    // String operations
    var upper = message.to_upper();
    var lower = message.to_lower();

    // String comparison
    if message == "Hello, World! My name is Alice" {
        print("Perfect match!");
    }

    // String search
    if message.contains("Alice") {
        print("Found Alice!");
    }
}
```

## Key Design Decisions

| Aspect | Implementation |
|--------|----------------|
| **Memory Model** | ARC with refcount header |
| **Encoding** | UTF-8 (null-terminated for C compatibility) |
| **LLVM Type** | Opaque `i8*` pointer |
| **Thread Safety** | Single-threaded (can add atomic ops later) |
| **Performance** | Global string constants for literals |

## Building

The string library is automatically compiled as part of the `hoort` static library:

```bash
# macOS/Linux
cmake -B build && cmake --build build

# Windows
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

## Testing

To test string functionality:

```bash
./build/hoo-tests --gtest_filter="StringTest.*"
```

## Memory Management

All strings are reference counted. The compiler should:

1. **Retain on assignment**: `var y = x` → increment refcount of x
2. **Release on scope exit**: When variable goes out of scope → decrement refcount
3. **Auto-manage concatenation**: `a + b` → new string, old strings released when unused

Example generated LLVM IR pattern:

```llvm
; var greeting = "Hello";
%greeting_ptr = call i8* @hoo_string_from_cstr(i8* getelementptr...)
store i8* %greeting_ptr, i8** %greeting

; var message = greeting + "World"
%greeting_val = load i8*, i8** %greeting
%world_ptr = call i8* @hoo_string_from_cstr(i8* getelementptr...)
%message_ptr = call i8* @hoo_string_concat(i8* %greeting_val, i8* %world_ptr)
store i8* %message_ptr, i8** %message

; scope exit - release
%greeting_val2 = load i8*, i8** %greeting
call void @hoo_string_release(i8* %greeting_val2)
```

## Next Steps

1. Update code generator to emit string function declarations
2. Implement string literal code generation
3. Add string concatenation operator support (`+`)
4. Add string method syntax support (`.length()`, `.contains()`, etc.)
5. Write comprehensive string tests
6. Implement automatic ARC insertion for strings

---

See also: `runtime/hoo_string.h` for complete API documentation.
