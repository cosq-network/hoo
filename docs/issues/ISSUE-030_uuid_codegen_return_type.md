# ISSUE-030: UUID Free Functions Return Wrong Type in Codegen

**Status**: OPEN

## 1. Overview

`HVMCodeGenerator::uuidFreeFunctionReturnTypeId` maps `uuid_v4()` and
`uuid_nil()` to `HOO_TYPE_STRING` (string) instead of `HOO_TYPE_UUID`
(Uuid). The documented API declares these functions as returning `:Uuid`
(docs/runtime/api/uuid.md:349), so the codegen return-type mapping is
incorrect.

This means any source that calls `uuid_v4()` and then attempts a Uuid
method (e.g. `.is_nil()`) or assigns the result to a `Uuid` variable will
fail at codegen or JIT load time, because the type system sees a `string`.

## 2. Root Cause

In `src/codegen/HVMCodeGenerator.cpp:373`:

```cpp
if (functionName == "uuid_v4" || functionName == "uuid_nil" || functionName == "uuid_to_string")
    return HOO_TYPE_STRING; // string
```

This should be:

```cpp
if (functionName == "uuid_v4" || functionName == "uuid_nil")
    return HOO_TYPE_UUID; // Uuid
if (functionName == "uuid_to_string")
    return HOO_TYPE_STRING; // string
```

`uuid_to_string` returning `HOO_TYPE_STRING` is correct and should be
kept as-is.

## 3. Impact

- Any Hoo source calling `uuid_v4()` and using the result as a `Uuid`
  will fail at codegen type-checking or JIT load.
- This is a preexisting bug unrelated to the ARC conversion landed in
  ISSUE-030's scope (runtime/lib/data layer).
- No existing JIT tests exercise this path, so the bug has gone undetected.

## 4. Fix

One-line change in `HVMCodeGenerator.cpp` (splitting the condition).
Add a JIT test (`tests/integration/jit/HooUuidJitTest.cpp`) that:

```hoo
import hoo.uuid;
func :bool test() {
    var id = uuid_v4();
    return id.is_nil() == 0;
}
```

## 5. Verification

After the fix and the new JIT test:

```bash
cmake --build --preset macos-homebrew-ninja --target hoo-tests
./build/macos-homebrew-ninja/hoo-tests --gtest_filter='HooUuidJitTest.*'
# Expect PASS
```

Run the full suite to confirm no regressions.
