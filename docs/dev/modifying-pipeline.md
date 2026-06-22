# How to Modify SimpleASTBuilder, HVMCodeGenerator, and HVMJIT

## Modifying SimpleASTBuilder

**Scenario:** Adding a new language construct.

1. **Add the grammar rule** in `hoo_parser.g4`.
2. **Regenerate** the ANTLR parser.
3. **Add `visit*` method** in `SimpleASTBuilder`:
   - Visit children.
   - Pop operands from stack.
   - Construct new AST node type (or reuse existing).
   - Push result back.
4. **Update codegen** in `HVMCodeGenerator` to handle the new node kind.

## Modifying HVMCodeGenerator

**Scenario:** Adding a new bytecode instruction.

1. **Define the opcode** in the instruction set (find the enum in `src/codegen/`).
2. **Add compilation logic** in `HVMCodeGenerator` — typically a new method or a branch in the main `compileNode` dispatcher.
3. **Add execution logic** in the VM interpreter if applicable.
4. **Update test expectations** — bytecode snapshots may need regeneration.

## Modifying HVMJIT

**Scenario:** Adding a new JIT runtime intrinsic.

1. **Declare the `jit_hoo_*` function** in `HVMJIT.cpp`.
2. **Implement the runtime helper** with the correct calling convention and linkage.
3. **Look up the symbol** using `LLVMOrcLookupSymbol` or equivalent.
4. **Register the intrinsic** in the JIT session so codegen can reference it.
5. **Test** — compile source that triggers the intrinsic and verify behavior.

## General workflow

```
Edit grammar (.g4)  →  Regenerate parser
       ↓
Edit SimpleASTBuilder  →  Add visit* / new node type
       ↓
Edit HVMCodeGenerator  →  Add bytecode emission
       ↓
Edit VM / JIT runtime  →  Add execution support
       ↓
Edit HVMJIT  →  Register new runtime symbols
       ↓
Add tests  →  Verify end-to-end
```
