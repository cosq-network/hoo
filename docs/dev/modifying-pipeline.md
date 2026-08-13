# How to Modify SimpleASTBuilder, HVMCodeGenerator, and HVMJIT

## Modifying SimpleASTBuilder

**Scenario:** Adding a new language construct.

1. **Add the grammar rule** in `src/parsing/Hooc.g4`.
2. **Regenerate** the ANTLR parser.
3. **Add `build*` method** in `SimpleASTBuilder`:
   - Dispatch from the parent context with `dynamic_cast`.
   - Construct the new AST node type (or reuse existing).
4. **Update codegen** in `HVMCodeGenerator` to handle the new node kind.

## Modifying HVMCodeGenerator

**Scenario:** Adding a new bytecode instruction.

1. **Define the opcode** in the instruction set (the `Opcode` enum in `src/hvm/HVMInstruction.h`).
2. **Add compilation logic** in `HVMCodeGenerator` — typically a new method or a branch in the main `compileNode` dispatcher.
3. **Add execution logic** in the VM interpreter if applicable.
4. **Update test expectations** — bytecode snapshots may need regeneration.

## Modifying HVMJIT

**Scenario:** Adding a new JIT runtime intrinsic.

1. **Declare the `jit_hoo_*` function** in `HVMJIT.cpp`.
2. **Implement the runtime helper** with the correct calling convention and linkage.
3. **Register the symbol** in the runtime JITDylib (`registerRuntimeSymbolsInJITDylib()`), then resolve it through `HVMJIT::getSymbolAddress()` when codegen needs the address.
4. **Test** — compile source that triggers the intrinsic and verify behavior.

## General workflow

```
Edit grammar (.g4)  →  Regenerate parser
       ↓
Edit SimpleASTBuilder  →  Add build* / new node type
       ↓
Edit HVMCodeGenerator  →  Add bytecode emission
       ↓
Edit VM / JIT runtime  →  Add execution support
       ↓
Edit HVMJIT  →  Register new runtime symbols
       ↓
Add tests  →  Verify end-to-end
```
