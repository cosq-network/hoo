# Foreign Function Interface (FFI)

The Hooc FFI allows seamless interaction with native C/C++ libraries and system-level symbols.

## 1. Native Library Imports
Native libraries can be linked using the `library` keyword.
- `library "libc.so" as libc;`

## 2. Dynamic Linkage
The `link dynamic` statement provides more control over the linkage process.
- `link dynamic math.utils at [1.0..2.0] ["/usr/lib", "/usr/local/lib"];`

## 3. External Functions
Native functions are declared using the `extern native` keywords.
- `extern native int64 printf(string fmt, pointer[void] val);`
- `native func: void myNativeFunc();` (Short-hand for internal native symbols).

## 4. FFI Specific Types

| Type | Description |
| :--- | :--- |
| `pointer[T]` | A raw memory pointer to type `T`. |
| `array[N] T` | A fixed-size array of `N` elements of type `T`. |
| `function(args) -> ret` | A native function pointer (callback). |

## 5. Usage in Backends
The `HVMCodeGenerator` lowers these calls to standard RISC `CALL` instructions targeting external symbols. The `HVMJIT` resolves these symbols at load-time using `llvm::sys::DynamicLibrary`. 

**Implementation Note:** Native C/C++ callbacks to Hooc functions are supported via the JIT's Trampoline system. For details on how the JIT bridges native execution contexts with the `HVMState`, see [Runtime JIT Integration](../runtime/jit-integration.md).
