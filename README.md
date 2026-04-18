# Hooc - The Hoo Programming Language Compiler

Hooc is a modern, statically-typed programming language with automatic memory management, generics, and a clean syntax. It combines the best features of modern languages with a focus on safety, performance, and developer productivity.

## Features

- **Automatic Reference Counting (ARC)**: Built-in memory management with zero-cost abstractions
- **Generics**: Full generic support for functions and classes with type parameter inference
- **Static Type System**: Strong typing with nullable types, union types, and type safety
- **Modern Syntax**: Clean, expressive syntax inspired by modern languages
- **LLVM Backend**: Leverages LLVM for optimization and native code generation
- **Module System**: Python-style imports with hierarchical module organization
- **Rich Standard Library**: Built-in support for strings, arrays, and collections
- **JIT Compilation**: Fast execution with LLVM ORC JIT engine

## Quick Start

### Prerequisites

- CMake 3.16 or higher
- LLVM 14+ (with development headers)
- ANTLR4 C++ runtime
- GoogleTest (for running tests)
- C++17 compatible compiler

### Installation

#### macOS (via Homebrew)

```bash
brew install llvm antlr4-cpp-runtime googletest cmake
```

#### Build from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/hooc.git
cd hooc

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run tests
./hoo-tests

# Install (optional)
sudo make install
```

#### Windows

See [docs/building-on-windows.md](docs/building-on-windows.md) for detailed Windows build instructions.

## Hello World

```hoo
func main() {
    var message: string = "Hello, World!";
    hoo.println(message);
}
```

Or with direct print:

```hoo
func main() {
    hoo.println("Hello, World!");
}
```

## Language Features

### Variables and Types

```hoo
var x: int64 = 42;
var pi: double = 3.14159;
var flag: bool = true;
var name: string = "Hooc";

// Type inference
var count = 100;  // inferred as int64
```

### Functions

```hoo
func add(a: int64, b: int64) -> int64 {
    return a + b;
}

func greet(name: string) -> void {
    var message = "Hello, " + name;
    hoo.println(message);
}
```

### Classes and Objects

```hoo
class Point {
    var x: int64;
    var y: int64;

    constructor(x: int64, y: int64) {
        var self.x = x;
        var self.y = y;
    }

    func distance() -> double {
        return sqrt(x * x + y * y);
    }
}

func main() {
    var p = new Point(10, 20);
    var dist = p.distance();
}
```

### Generics

```hoo
func identity<T>(value: T) -> T {
    return value;
}

class Box<T> {
    var value: T;

    constructor(value: T) {
        var self.value = value;
    }

    func get() -> T {
        return value;
    }
}

func main() {
    var intBox = new Box<int64>(42);
    var strBox = new Box<string>("hello");
}
```

### Arrays

```hoo
var numbers: int64[] = [1, 2, 3, 4, 5];
var first = numbers[0];
var length = numbers.length();

// Multi-dimensional arrays
var matrix: int64[][] = [[1, 2], [3, 4]];
```

### Control Flow

```hoo
// If-else
if x > 0 {
    print("positive");
} else {
    print("non-positive");
}

// While loops
while count < 10 {
    count = count + 1;
}

// For loops
for i in 0..10 {
    print(i);
}
```

### Nullable Types

```hoo
var maybeValue: int64? = null;
var name: string? = "John";

if name != null {
    print(name);
}
```

### Module System

```hoo
// Import entire module
import std.collections;

// Import specific items
from std.io import File, Directory;

// Import with alias
import std.collections as coll;
```

## Project Structure

```
hooc/
├── src/              # Compiler source code
│   ├── Hooc.g4       # ANTLR4 grammar definition
│   ├── HooCompiler.h/cpp   # Main compiler interface
│   ├── LLVMCodeGenerator.h/cpp  # LLVM IR code generator
│   ├── SimpleASTBuilder.h/cpp   # AST construction
│   ├── ModuleSystem.h/cpp       # Module resolution
│   ├── core/           # Core CLI and I/O utilities
│   │   ├── HooCLI.h/cpp         # Command-line interface
│   │   ├── IOProvider.h         # I/O abstraction interface
│   │   └── DefaultIOProvider.h/cpp  # Default file/stdin/stdout/stderr
│   ├── ast/          # AST node definitions
│   └── rt/           # Runtime library (ARC, strings, arrays)
├── tests/            # Comprehensive test suite
├── docs/             # Documentation
├── antlr4/           # Generated parser code
└── CMakeLists.txt    # Build configuration
```

### Core CLI Components

- **HooCLI** (`src/core/HooCLI.h`): Command-line interface for the compiler
- **IOProvider** (`src/core/IOProvider.h`): Abstract interface for I/O operations
- **DefaultIOProvider** (`src/core/DefaultIOProvider.h`): Default implementation using file I/O and stdio

The CLI supports:
- Compiling and executing `.hoo` source files
- `--help` / `-h`: Display usage information
- `--version` / `-v`: Display version information
- `--verbose`: Enable verbose logging
- `--print-ir`: Print generated LLVM IR

## Architecture

Hooc uses a multi-stage compilation pipeline:

1. **Lexing & Parsing**: ANTLR4 generates a parse tree from source code
2. **AST Building**: Parse tree is converted to an Abstract Syntax Tree
3. **Semantic Analysis**: Type checking and name resolution
4. **Code Generation**: AST is compiled to LLVM IR
5. **Optimization**: LLVM optimizes the IR
6. **Execution**: Code is JIT-compiled and executed via LLVM OrcJIT

### Runtime System

The runtime provides:
- **Reference Counting**: `hoo_retain()`, `hoo_release()`, `hoo_alloc()`
- **String Library**: UTF-8 strings with comprehensive operations
- **Generic Arrays**: Type-safe dynamic arrays using `std::any`
- **Runtime Class Registry**: Dynamic type information for reflection

## Development

### Running Tests

```bash
cd build
./hoo-tests

# Or use CTest
ctest --verbose
```

### Adding New Features

1. Update the grammar in `src/Hooc.g4`
2. Add AST node types in `src/ast/`
3. Implement code generation in `LLVMCodeGenerator.cpp`
4. Add tests in `tests/`
5. Run the test suite to verify

## Documentation

- [Grammar Specification](docs/grammar.md) - Complete language grammar
- [Features Guide](docs/features.md) - Detailed feature documentation
- [Implementation Status](docs/implementation-status.md) - Current implementation status
- [Roadmap](docs/roadmap.md) - Future development plans

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Built with [LLVM](https://llvm.org/) for code generation and optimization
- Parsing powered by [ANTLR4](https://www.antlr.org/)
- Inspired by modern languages like Swift, Kotlin, and TypeScript

## Contact

For questions, bug reports, or feature requests, please open an issue on GitHub.
