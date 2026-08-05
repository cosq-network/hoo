# ISSUE-021: Statement Grammar, Lowering, and Execution Completeness

## Status

**Implemented for the supported statement model.** The issue was audited and
completed across grammar, AST construction, code generation, JIT bridges, and
runtime tests. C-style `for (;;)` and custom iterable protocols remain outside
the language design; `for-in` supports arrays, strings, and map keys.

## Implemented behavior

### Loops and branching

- `do { ... } while (condition);` is parsed, represented by
  `DoWhileStatement`, lowered to HVM control flow, and covered by JIT tests.
- `switch` supports integer-like discriminants, fall-through, `break`, and
  interaction with enclosing loops.
- `for-in` supports typed array access, strings as character sequences, and
  numeric, character, and string map keys.
- Ranges continue to use the dedicated `for-range` lowering rather than being
  treated as collection objects.

### Explicit scopes

`scope { ... }` is now a first-class grammar and AST statement. It creates a
lexical lifetime boundary and uses the existing ARC cleanup machinery when the
scope exits.

### Exceptions

The HVM/JIT exception path now has explicit handler-PC lowering, catch-type
compatibility support, unmatched-exception rethrow paths, and separate normal
and exceptional `finally` paths. The native exception API exposes type
compatibility through `hoo_exception_matches_type`.

## Verification

Coverage includes:

- parser and AST construction for statements and explicit scopes;
- code-generation checks for handler registration and cleanup;
- JIT execution for do-while, switch, typed array iteration, string/map
  iteration, explicit scopes, and normal `finally` execution;
- runtime tests for exception type compatibility and existing ARC behavior.

## Design boundaries

- Map iteration binds keys only: `for key in map { ... }`.
- Key/value pair iteration is not part of the current grammar.
- User-defined iterable classes are not implicitly iterable; a future protocol
  should be tracked as a separate language feature.
