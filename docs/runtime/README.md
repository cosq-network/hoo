# hoo Runtime Library (`hoort`) Reference

This directory contains the normative documentation for the hoo Runtime Library (`hoort`). The runtime provides the high-level services and intrinsic functions necessary to execute hoo applications, acting as a bridge between the physical HVM RISC core and the host system.

## Core Philosophy: The Opaque Handle Model

Because the HVM v1.4 specification describes a pure physical hardware architecture, it lacks native concepts of "managed objects" or "garbage collection". To bridge this gap, the runtime library implements all complex data structures (Strings, Arrays, Maps, Objects) as **Opaque Handles**. 

*   **JIT / HVM View**: An opaque 64-bit integer (`int64_t`) representing an absolute host memory pointer.
*   **Runtime View**: A fully managed C++ instance preceded by a normative 16-byte Automatic Reference Counting (ARC) header.

## Documentation Index

1. **[Memory Model & ARC](memory-model.md)**
   * Details the 16-byte object header, Reference Counting (ARC), and the Thread-Local Allocation Buffer (TLAB) system.
2. **[Strings & Unicode](strings.md)**
   * `HooString` and `HooCharacter` implementations, immutable UTF-8 buffers, and Unicode scalar support.
3. **[Collections](collections.md)**
   * Hardware-ready low-level arrays (`HooArray`) and type-safe dictionaries (`HooMap`).
4. **[Exceptions](exceptions.md)**
   * `HooException` type IDs, stack unwinding, and shadow stack management.
5. **[Math](math.md)**
   * Mathematical constants, functions, and the random number generator state.
6. **[I/O & Networking](io-net.md)**
   * Console input/output (`print`, `readline`) and the HTTP/URL client implementation.
7. **[JIT Integration](jit-integration.md)**
   * System call mapping (`SYSCALL` 1-11) with platform-specific behavior, ARC optimization passes, host symbol bridging, and flexible symbol resolution (`buildLookupCandidates`).

## Integration & C-ABI
The library exposes its API strictly via `extern "C"` to guarantee ABI stability with the JIT's LLVM `ExecutionEngine`. The `HVMJIT` maps absolute host function pointers into the isolated `hoo` JITDylib so HVM code can resolve `CALL` targets natively.
