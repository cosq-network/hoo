# HVM vs WebAssembly: Architectural Comparison

This document compares the current HVM architecture in `docs/hvm` with WebAssembly (Wasm) at the architectural level. It is written from the perspective of the current HVM v1.4 "hardware-ready" profile, where HVM is treated as a pure RISC ISA and the compiler/runtime are responsible for lowering high-level language features.

## 1. Executive Summary

HVM and WebAssembly solve related but different problems:

- HVM is designed as a **hardware-aligned register ISA** with a flat physical memory model, explicit calling convention, and direct control-flow instructions.
- WebAssembly is designed as a **portable validated execution format** with a stack machine, structured control flow, linear memory, and embedding-defined host interfaces.

The practical difference is this:

- HVM is optimized for implementation on physical hardware, soft cores, or a JIT that wants to preserve hardware semantics as closely as possible.
- WebAssembly is optimized for safe, portable distribution and execution across hosts, especially within browsers and other sandboxed runtimes.

## 2. Core Design Goals

### 2.1 HVM

HVM, as documented in `docs/hvm/hvm-spec.md`, aims to be a pure RISC architecture:

- 64-bit register machine
- byte-addressable little-endian memory
- downward-growing stack
- explicit register-based calling convention
- direct load/store and branch instructions
- high-level features lowered to runtime calls or compiler-emitted sequences

In this repo, the architectural priority is hardware compatibility and implementation clarity. HVM is not trying to hide machine structure from the compiler.

### 2.2 WebAssembly

WebAssembly is a portable binary instruction format and execution model. The official spec defines Wasm modules independently of any concrete embedding, and execution is specified using an abstract machine with:

- an operand stack
- control constructs
- a store of global runtime state
- validation rules that must succeed before instantiation

Wasm is intentionally not a hardware ISA. It is a compilation target and deployment format that can run inside browsers, server-side runtimes, and other embeddings.

## 3. Execution Model

### 3.1 HVM: Register Machine

HVM executes instructions against named registers `r0..r31`.

Key properties:

- Operands are held in registers, not on an implicit operand stack.
- Control flow uses explicit branch and jump instructions.
- Function calls follow an ABI with argument registers and a dedicated return register.
- The architecture exposes frame, link, and stack registers directly.

This makes instruction behavior close to conventional RISC hardware and easier to map onto LLVM IR or native code generation.

### 3.2 WebAssembly: Stack Machine

WebAssembly execution is stack-based:

- instructions consume operands from the implicit operand stack
- instructions push results back to the stack
- structured blocks govern control flow and validation

That model is compact and easy to validate statically. It also makes Wasm a good portable target for many source languages because compilers can map SSA-like or expression-oriented intermediate forms into a standardized stack discipline.

### 3.3 Consequence

The execution model difference affects almost everything else:

- HVM tends to make data flow explicit and low-level.
- Wasm tends to make data flow implicit and structured.

In practice, HVM is better suited to implementations that want predictable register allocation and direct hardware mapping. Wasm is better suited to implementations that want portability and a strong validation boundary.

## 4. Memory Model

### 4.1 HVM Memory

HVM uses a flat, byte-addressable physical memory model. In this repo, high-level constructs such as objects, arrays, strings, and exceptions are lowered into:

- raw memory layouts
- runtime-managed handles
- explicit pointer arithmetic
- load/store instructions

This is a classic systems-style model. The compiler and runtime are responsible for the semantic burden that a higher-level VM would otherwise carry.

### 4.2 WebAssembly Memory

WebAssembly uses **linear memory**:

- a contiguous range of untyped bytes
- accessed by load/store instructions
- sandboxed within an instance unless explicitly shared by the embedding
- typically grown in 64 KiB pages

The important distinction is that linear memory is an abstraction with explicit host boundaries. It is not the process address space, and it is not exposed as an unconstrained flat machine memory in the same way HVM is described.

### 4.3 Consequence

HVM memory is closer to the machine itself. Wasm memory is closer to a sandboxed heap container.

That difference matters for:

- safety guarantees
- pointer identity
- host integration
- escape analysis and aliasing assumptions
- how much the compiler must know about the runtime

## 5. Control Flow

### 5.1 HVM

HVM uses direct control-transfer instructions such as:

- conditional branches
- unconditional jumps
- call and return instructions
- tail calls where supported

This is natural for hardware and makes control-flow lowering straightforward. The compiler can emit standard branch sequences for `if`, `while`, `for`, `try/catch`, and similar constructs.

### 5.2 WebAssembly

Wasm uses structured control instructions:

- `block`
- `loop`
- `if`
- `br`
- `br_if`
- `br_table`

Branches are relative to the nesting structure, and validation ensures control flow is well formed. This is one of Wasm's defining properties: branch targets are constrained by the structure of the program.

### 5.3 Consequence

HVM is more directly expressive for compiler backends that already think in terms of labels, jumps, and machine frames.

Wasm is more constrained, but that constraint enables:

- strong static validation
- safer embedding
- easier reasoning about well-formedness
- compact binary encodings for nested control

## 6. Calling Convention and ABI

### 6.1 HVM ABI

HVM defines a fixed register convention:

- `r1..r8` carry arguments
- `r1` is also the return-value register
- `r29` is the link register
- `r30` is the frame pointer
- `r31` is the stack pointer

This means function boundaries are explicit and architecture-like. The compiler is expected to honor the ABI directly.

### 6.2 WebAssembly ABI

Wasm does not impose a universal machine-level ABI in the same way. At the module level:

- functions have typed parameters and results
- values are passed through the Wasm operand stack
- the embedding defines how host functions, imports, and exports are called

The effective ABI is therefore split across two layers:

- Wasm-internal calling semantics
- host embedding ABI

### 6.3 Consequence

HVM exposes a concrete register ABI that is useful for low-level code generation and hardware targets.

Wasm exposes a language-neutral function type system and leaves the host ABI to the embedding. That makes it more portable, but less hardware-specific.

## 7. Validation and Safety

### 7.1 HVM

HVM, as documented here, is not primarily a validation-centric format. Its correctness model is based on:

- a well-defined instruction set
- fixed register semantics
- loader and linker correctness
- runtime discipline

Safety depends on the implementation, the compiler, and the runtime model. If HVM is used as a physical ISA or soft-core target, it behaves like other low-level machine architectures: correctness is enforced by hardware or by the runtime stack around it.

### 7.2 WebAssembly

WebAssembly is fundamentally validation-driven:

- a module must be valid before instantiation
- validation proves structural and type correctness
- execution rules assume validation has already succeeded

This is one of the main reasons Wasm is attractive as a distribution format. The runtime can reject malformed code before execution rather than relying only on dynamic checks.

### 7.3 Consequence

HVM trades validation-centric safety for hardware realism.

Wasm trades hardware realism for strong static well-formedness guarantees.

## 8. Module and Object Format

### 8.1 HVM Modules

HVM uses `.ho` object/module files, with a format described in `docs/hvm/ho-file-format.md`. The module carries sections such as:

- `.text`
- `.symtab`
- `.strtab`
- `.funcmeta`
- `.rodata`

The module format is deliberately close to object-file thinking: code, metadata, relocation-style information, and symbol management are all first-class concerns.

### 8.2 WebAssembly Modules

Wasm modules are compact binary modules with a standardized binary format and a corresponding text format. A module can contain:

- functions
- memories
- tables
- globals
- imports and exports
- element and data segments

The module structure is designed for portability and embedding, not for native object-file parity.

### 8.3 Consequence

HVM `.ho` files feel like a low-level executable/object pipeline.

Wasm modules feel like a self-contained portable artifact that can be validated and instantiated by many different hosts.

## 9. Toolchain Strategy

### 9.1 HVM Toolchain

In this repository, HVM is used as the target of compiler lowering and JIT translation:

- Hooc language features are lowered into HVM instructions
- high-level runtime behavior is implemented by `hoort`
- the JIT translates HVM to LLVM IR or native code

This is a classic compiler-to-machine pipeline.

### 9.2 WebAssembly Toolchain

Wasm is commonly used as:

- a compiler target from languages such as C, C++, Rust, and others
- a portable binary for browsers and server-side runtimes
- an embedding boundary between host code and sandboxed code

The Wasm toolchain often emphasizes:

- validation
- portability
- host imports/exports
- predictable execution in an embedded runtime

### 9.3 Consequence

HVM is better aligned with a machine-code-centric compiler pipeline.

Wasm is better aligned with a portable deployment pipeline.

## 10. Runtime Responsibilities

### 10.1 HVM Runtime Responsibilities

The HVM docs in this repository place significant responsibility on the runtime and compiler:

- object allocation
- string management
- exception handling
- ARC or other ownership discipline
- FFI marshalling
- debugger integration

These are not treated as properties of the core ISA. They are layered on top.

### 10.2 WebAssembly Runtime Responsibilities

Wasm runtimes are responsible for:

- validation
- instantiation
- import resolution
- memory/table management
- host API integration

Depending on the embedding and the extension set used, some runtimes also provide additional capabilities such as exceptions, reference types, or other proposals. The key point is that the runtime boundary is central to Wasm's design.

### 10.3 Consequence

HVM runtime support is closer to a system runtime built around a real ISA.

Wasm runtime support is closer to an execution container that hosts validated portable code.

## 11. Performance When HVM Runs Through `HVMJIT`

This section is specific to the execution path in `src/hvm/HVMJIT.cpp`, where HVM bytecode is parsed, validated, translated to LLVM IR, linked against runtime symbols, and executed through LLVM ORC.

### 11.1 What HVM JIT Optimizes Well

HVM starts from a register machine, so the JIT does not need to invent a register discipline from a stack-only source format. That gives it a few practical advantages:

- fewer operand-stack shuffles than a stack-based IR would require
- direct mapping of HVM arguments and temporaries to native registers
- straightforward lowering for arithmetic, branches, loads, stores, and calls
- explicit calling convention that matches native backend expectations
- good conditions for devirtualization and bounds-check elimination when module metadata is available

This is especially relevant for compute-heavy code and code with many direct calls, because the input representation is already close to the machine model the LLVM backend wants.

### 11.2 Where HVM JIT Still Pays Overhead

HVM JIT is not zero-cost. The current execution path still includes:

- module loading and path resolution
- bytecode validation
- dependency graph resolution
- runtime symbol registration
- LLVM IR construction
- native code generation and linking
- optional debug-info emission and debugger registration
- runtime bridge calls for objects, strings, exceptions, and FFI

So the performance picture is split:

- startup cost is higher than an interpreter that simply dispatches instructions
- steady-state execution can be strong once hot code is translated and linked
- the quality of generated native code depends on how much lowering and analysis the compiler already performed before the JIT sees the module

### 11.3 HVM JIT vs Wasm JIT

Compared with a WebAssembly engine, HVM JIT has a different performance profile:

- HVM can avoid some stack-to-register reconstruction because the source ISA is already register-based.
- Wasm engines must lower a stack machine, but mature engines are heavily optimized for that job and have very advanced tiering strategies.
- HVM's explicit ABI can make FFI and runtime bridges cheaper when the host and JIT agree on register/state layout.
- Wasm's biggest performance strength is not raw ISA similarity to hardware, but the quality of its optimizing engines and the compactness of its validation model.

In practice, HVM JIT is likely to do best when:

- the workload is arithmetic- and branch-heavy
- there are many direct calls with stable targets
- runtime interactions are limited or highly specialized
- the compiler has already lowered most high-level constructs into efficient HVM sequences

Wasm is likely to do best when:

- portability and sandboxing matter more than machine-model fidelity
- the host engine has a very mature optimizing pipeline
- the application benefits from existing browser or server-side Wasm infrastructure

### 11.4 Cold Start vs Steady State

For HVM JIT, startup cost is dominated by translation, symbol resolution, and runtime setup. The upside is that the source format is already close to the machine model, so once translation is complete, execution can be very direct.

For Wasm, startup often benefits from compact binaries and a very standardized instantiation model, but the engine still has to validate and compile the module before execution. The exact balance depends heavily on the embedding and on whether the engine is using baseline, optimizing, or tiered compilation.

## 12. Side-By-Side Comparison

| Dimension | HVM | WebAssembly |
| :--- | :--- | :--- |
| Primary goal | Hardware-aligned execution | Portable validated execution |
| Execution style | Register machine | Stack machine |
| Memory model | Flat physical memory | Linear memory |
| Control flow | Direct branches and jumps | Structured blocks and branches |
| Function calls | Fixed register ABI | Typed stack-based function semantics |
| Validation | Implementation/runtime discipline | Core part of the format |
| Safety boundary | Hardware/runtime dependent | Built into module validation and embedding |
| Module format | `.ho` object/module pipeline | Portable binary module |
| Best fit | RISC cores, JIT-to-native, low-level systems | Browsers, portable plugins, sandboxed runtimes |

## 12. Practical Implications For Hooc

### 12.1 Why HVM Fits the Current Hooc Docs

The current Hooc documentation is consistent with a compiler that wants:

- direct control over layout and calling convention
- deterministic lowering of language features
- explicit handling of runtime services
- a close correspondence between source constructs and machine operations

That makes HVM a good match for the current architecture because the compiler can lower high-level semantics into a predictable low-level substrate.

### 12.2 What Would Change If Hooc Targeted WebAssembly

If Hooc targeted Wasm as a primary backend, the compiler and runtime design would shift:

- control flow would need to fit Wasm's structured blocks
- register allocation would be replaced or hidden by stack-based lowering
- object and runtime semantics would need to fit Wasm's module/memory/host model
- the embedding boundary would become central to FFI and system integration

That is feasible, but it is a different architecture. It would not preserve HVM's hardware-first shape.

## 13. Strengths and Tradeoffs

### 13.1 HVM Strengths

- Direct hardware mapping
- Simple instruction-level semantics
- Predictable ABI and register usage
- Good fit for JITs and native code generation
- Easier to model as a physical machine

### 13.2 HVM Tradeoffs

- Less inherent sandboxing
- More implementation burden on compiler/runtime
- More responsibility for memory safety and object semantics
- Less portable as a distribution format without an embedding layer

### 13.3 WebAssembly Strengths

- Strong portability
- Validation before instantiation
- Good sandboxing story
- Standardized module format
- Strong ecosystem around browser and server embeddings

### 13.4 WebAssembly Tradeoffs

- Stack-based execution is less hardware-like
- Control flow is more constrained
- Low-level system integration depends heavily on the embedding
- Some host capabilities are outside the core spec and depend on proposals or embeddings

## 14. Bottom Line

HVM and WebAssembly are both low-level execution targets, but they occupy different points in the design space:

- HVM is closer to a real machine architecture.
- WebAssembly is closer to a validated portable bytecode format.

If the goal is to model hardware, expose a stable RISC-style ABI, and keep control over code generation at the machine level, HVM is the more natural fit.

If the goal is to distribute code safely across many hosts with a strong validation boundary and a standardized embedding model, WebAssembly is the more natural fit.

## 15. References

- `docs/hvm/hvm-spec.md`
- `docs/hvm/ho-file-format.md`
- `docs/hvm/hvm-implementation-analysis.md`
- WebAssembly Specification: https://webassembly.github.io/spec/core/
- WebAssembly specifications overview: https://webassembly.org/specs/
