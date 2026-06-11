# Adding a New Runtime Module — Developer Guide

This document describes how a runtime library function (e.g. `hoo_thread_spawn`) is
wired through the three layers so that Hoo source code can call it via class-based
method syntax (`Thread.spawn()`, `thread.join()`, etc.).

## Architecture Overview

```
Hoo source               Codegen                     JIT                     Runtime (hoort)
────────────              ──────                     ───                     ──────────────
Thread.spawn(a,b)  ───►   _F_M_hoo_E_      ───►   jit_thread_spawn  ───►   hoo_thread_spawn()
                           thread_spawn_v_p_p        (HVMState*)             (libc)
                                                     │
                                                     └── registered in
                                                         buildRuntimeSymbols()
                                                         & bootstrapRuntimeModules()
```

The Hoo-level syntax uses `ClassName.method(args)` or `object.method(args)`. The
codegen internally maps each class name to its module prefix via `classToPrefix()` 
(e.g. `Thread` → `thread_`, `Fs` → `fs_`, `Path` → `path_`) and generates the
mangled symbol accordingly.

Three touch points are required for every new runtime function:

| # | Layer | File | What to add |
|---|-------|------|-------------|
| 1 | **Runtime** | `src/runtime/lib/hoo_*.h/.cpp` | C function implementation |
| 2 | **JIT wrapper** | `src/hvm/HVMJIT.cpp` | `jit_*` wrapper + symbol table entry |
| 3 | **Codegen redirect** | `src/codegen/HVMCodeGenerator.cpp` | Prefix match in the redirect block |
| 4 | **Build** | `CMakeLists.txt` | Source file for `hoort` library |
| 5 | **Tests** | `tests/jit/*.cpp` | JIT integration tests |

---

## Layer 1: Runtime implementation

**Location:** `src/runtime/lib/hoo_<module>.h` and `src/runtime/lib/hoo_<module>.cpp`

### Header conventions
- Functions must be declared `extern "C"` for stable ABI
- All pointer types are `void*` (opaque handles)
- Return type `int64_t` for integers (maps to int64 in Hoo)
- Return type `void*` for objects (maps to opaque ptr in Hoo)

```c
// src/runtime/lib/hoo_thread.h
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t hoo_thread_spawn(int64_t (*func)(void*), void* arg);
int64_t hoo_thread_join(int64_t thread_id);
int64_t hoo_thread_self(void);

#ifdef __cplusplus
}
#endif
```

### CMake registration
Add the `.cpp` to the `hoort` library in `CMakeLists.txt` alongside the other modules:
```cmake
src/runtime/lib/hoo_thread.cpp
```

---

## Layer 2: JIT wrapper and symbol table

**Location:** `src/hvm/HVMJIT.cpp`

### 2a — Add the `#include`

Insert alongside the other runtime includes (alphabetically):

```cpp
#include "runtime/lib/hoo_thread.h"
```

### 2b — Write a JIT wrapper function

Wrappers follow a strict convention:

```cpp
extern "C" {
    uint64_t jit_thread_spawn(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        // Arguments arrive in state->regs[1..N]
        auto func = reinterpret_cast<int64_t (*)(void*)>(state->regs[1]);
        void* arg = reinterpret_cast<void*>(state->regs[2]);
        // Call the runtime function, cast result to uint64_t
        return static_cast<uint64_t>(hoo_thread_spawn(func, arg));
    }
}
```

**Rules:**
- Signature is always `uint64_t(void* state_ptr)`
- Read arguments from `state->regs[1]`, `state->regs[2]`, etc.
- Return value is `uint64_t` (even for void functions, return 0)
- Place wrappers in the `extern "C" { ... }` block under the appropriate section comment
- Section comments follow the pattern `// ── Thread module ──────────────────────────────────`

### 2c — Add symbol table entry

Find `buildRuntimeSymbols()` (around line 1130) and add entries in the `hoo` module section:

```cpp
{"_F_M_hoo_E_thread_spawn_v_p_p", reinterpret_cast<void*>(&jit_thread_spawn)},
{"_F_M_hoo_E_thread_join_v_p",   reinterpret_cast<void*>(&jit_thread_join)},
{"_F_M_hoo_E_thread_self_v",     reinterpret_cast<void*>(&jit_thread_self)},
```

**Mangling rule:** All runtime module calls use `returnType = "void"` → code `v`
and each parameter uses type `ptr` → code `p`. The format is:

```
_F_M_<module>_E_<name>_v[_p]*
```

| Parameter count | Mangled suffix | Example |
|-----------------|----------------|---------|
| 0 | `_v` | `_F_M_hoo_E_thread_self_v` |
| 1 | `_v_p` | `_F_M_hoo_E_thread_join_v_p` |
| 2 | `_v_p_p` | `_F_M_hoo_E_thread_spawn_v_p_p` |
| 3 | `_v_p_p_p` | `_F_M_hoo_E_encoding_base64_encode_v_p_p` → wait, that takes (data, len). |

(Note: `encoding_base64_encode` is `_v_p_p` with 2 params, not 3.)

This convention is hardcoded in `HVMCodeGenerator.cpp:1072-1077`:

```cpp
mp.returnType = "void";
if (funcCall->getArguments()) {
    for (const auto& arg : funcCall->getArguments()->getArguments()) {
         mp.parameterTypes.push_back("ptr");
    }
}
```

**Important:** The mangled name in the symbol table MUST match exactly what the
codegen produces. The codegen always uses `ptr` (→ `p`) for all runtime function
parameters, regardless of the actual C type.

---

## Layer 3: Codegen prefix redirect

**Location:** `src/codegen/HVMCodeGenerator.cpp` around line 1047-1058

The codegen detects runtime module calls by function name prefix. Add your
module's prefix to the chain:

```cpp
// Redirect built-ins and standard library to hoo namespace
if (functionName == "print" || functionName == "println" ||
    functionName == "readline" || functionName == "readchar" ||
    functionName.rfind("fs_", 0) == 0 ||
    functionName.rfind("system_", 0) == 0 ||
    functionName.rfind("regex_", 0) == 0 ||
    functionName.rfind("uuid_", 0) == 0 ||
    functionName.rfind("encoding_", 0) == 0 ||
    functionName.rfind("math_", 0) == 0 ||
    functionName.rfind("thread_", 0) == 0 ||
    functionName.rfind("csv_", 0) == 0 ||
    functionName.rfind("datetime_", 0) == 0 ||
    functionName.rfind("path_", 0) == 0 ||
    functionName.rfind("hashing_", 0) == 0 ||
    functionName.rfind("process_", 0) == 0 ||
    functionName.rfind("compression_", 0) == 0 ||
    functionName.rfind("args_", 0) == 0 ||
    functionName.rfind("net_", 0) == 0 ||
    functionName.rfind("json_", 0) == 0) {
    mp.modulePath = {"hoo"};
}
```

When this block fires, `mp.modulePath = {"hoo"}` causes the mangler to produce
`_F_M_hoo_E_<name>_v[_p]*` instead of the default module-less form.

---

## Layer 4: JIT integration tests

**Location:** `tests/jit/Hoo<Module>JitTest.cpp`

### Test pattern

```cpp
#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hoo;

class HooThreadJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooThreadJitTest, SelfId) {
    const std::string source = R"(
        func:int64 test() {
            return Thread.self();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}
```

### Key observations
- The Hoo test function is always named `test` with return type `int64`
- The mangled entry point is always `_F_M_test_E_test_i8`
- Use `R"(...)"` raw string literals for Hoo source
- Always `ASSERT_TRUE(jit.loadSourceCode(...))` with the error message, so test
  failures show the compiler/JIT error

### CMake registration
Add the test `.cpp` to `CMakeLists.txt` in the `hoo-tests` sources section:
```cmake
tests/jit/HooThreadJitTest.cpp
```

---

## Complete checklist for adding a new module

Example: adding a new module (e.g. `hoo.xml`).

- [ ] **Runtime header** — `src/runtime/lib/hoo_<module>.h`
  - `extern "C"` functions with `int64_t`/`void*` types
- [ ] **Runtime impl** — `src/runtime/lib/hoo_<module>.cpp`
  - Implementation using platform APIs
- [ ] **CMake (runtime)** — source file added to `hoort` in `CMakeLists.txt`
- [ ] **CMake (tests)** — JIT test file added to `hoo-tests` in `CMakeLists.txt`
- [ ] **JIT include** — `#include "runtime/lib/hoo_<module>.h"` in `HVMJIT.cpp`
- [ ] **JIT wrappers** — `jit_<module>_*()` functions in the `extern "C"` block
- [ ] **Symbol table** — entries in `buildRuntimeSymbols()` with mangled names
- [ ] **Codegen prefix** — `functionName.rfind("<module>_", 0) == 0` in the
      redirect block
- [ ] **JIT tests** — `tests/jit/Hoo<Module>JitTest.cpp` exercising the full
      compile → JIT → run pipeline
- [ ] **Runtime tests** — `tests/runtime/Hoo<Module>Test.cpp` for C-level
      complex scenarios
- [ ] **Documentation** — module reference doc in `docs/runtime/<module>.md`
- [ ] **README** — add module entry to `docs/runtime/README.md`
- [ ] **Build & verify** — `cmake --build build && ./build/hoo-tests`

**Currently integrated modules** (class name → prefix):
`Fs` → `fs_`, `System` → `system_`, `Regex` → `regex_`, `Uuid` → `uuid_`,
`Encoding` → `encoding_`, `Math` → `math_`, `Thread` → `thread_`,
`Csv` → `csv_`, `DateTime` → `datetime_`, `Path` → `path_`,
`Hash` → `hashing_`, `Process` → `process_`, `Compression` → `compression_`,
`Args` → `args_`, `Net` → `net_`, `Json` → `json_`, `Character` → `character_`

---

## Common pitfalls

1. **Mangling mismatch.** The codegen always uses `ptr` for all runtime params.
   If you register a symbol as `_F_M_hoo_E_foo_v_p` but the codegen produces
   `_F_M_hoo_E_foo_v_i8` (e.g. because you changed the codegen), the JIT won't
   resolve it. Keep the codegen's hardcoded `ptr` → `p` convention.

2. **Wrapper signature.** Every JIT wrapper must be `uint64_t(void*)`. The
   argument is an opaque `HVMState*` pointer. Do NOT change this signature.

3. **Module not found.** If `run()` falls back to the interpreter and the
   function returns `-1`, check `getLastError()`. The most common cause is a
   forgotten prefix match in the codegen redirect block.

4. **Segfault in thread tests.** If using `pthread_create`, the callback
   function pointer must have C calling convention. Captureless lambdas with the
   right signature can work, but a plain static function is safest.

5. **String returns.** If your runtime function returns a string (e.g.
   `system_hostname()`), the JIT wrapper must:
   ```cpp
   char* cstr = hoo_system_hostname();
   void* str = hoo_string_from_cstr(cstr);
   hoo_system_free_string(cstr);  // if applicable
   return reinterpret_cast<uint64_t>(str);
   ```
   This converts the C string to a managed Hoo string object.

6. **Null handling.** If a runtime function can return null/error, test it in
   both C-level and JIT tests. The JIT wrapper should handle null gracefully
   (return 0, don't crash).
