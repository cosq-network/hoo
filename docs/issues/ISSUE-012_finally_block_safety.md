# ISSUE-012: Finally Block Not Guaranteed to Execute

## 1. Overview
The try-catch-finally lowering does not ensure the finally block executes when exceptions propagate out of the try block or a catch clause. Shadow-stack handlers are also leaked on exceptional paths.

## 2. Technical Analysis

### 2.1 Handler leak on try-block exception
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 623-663
- **Issue**: If an exception occurs in the try block, control jumps to `catchStartLabel`, bypassing the `hoo_pop_handler` call. The exception handler remains on the shadow stack indefinitely.

### 2.2 Finally skipped on catch clause throw
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 651-662
- **Issue**: After visiting each catch clause, the code jumps to `endLabel`. The finally block is visited after all catch clauses. If a catch clause throws (or re-throws), the `JMP endLabel` is never reached, so the finally block is never visited.

```
try_start:
  push_handler catch_start
  try_body
  pop_handler       ← not reached if exception in try_body
  jmp finally_start
catch_start:
  catch_body
  jmp finally_start  ← not reached if catch_body throws
finally_start:
  finally_body
```

## 3. Impact
- Resources expected to be released in `finally` (file handles, mutexes) are leaked when exceptions occur.
- Shadow-stack corruption from unpopped handlers.
- Non-compliance with the Hoo language specification.

## 4. Suggested Fix
Use a nested try-catch approach:
- The finally block should be executed in both the normal and exceptional paths.
- On exception in try body: pop the handler, execute finally, re-throw.
- On exception in catch clause: execute finally, re-throw.
- Use a flag register to track whether the finally path was entered normally or exceptionally.

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **HIGH**
