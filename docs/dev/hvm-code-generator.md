# How HVMCodeGenerator Works

**File:** `src/codegen/HVMCodeGenerator.h` / `.cpp` (~4492 lines)

The code generator walks the `ASTNode` tree and emits bytecode into an `HOModule`. It is the heart of the compiler and spans over 4400 lines.

## Architecture

```cpp
class HVMCodeGenerator : public CodeGenerator {
    // Core state
    std::unique_ptr<hvm::HOModule> module_;
    std::vector<hvm::HVMInstruction> instructions_;
    uint32_t currentByteOffset_ = 0;
    std::vector<uint8_t> compressedInstructions_;
    std::vector<std::string> errors_;
    
    // Module context
    std::vector<std::string> modulePath_;
    std::unordered_set<std::string> importedModules_;
    std::unordered_map<std::string, std::string> importedSymbols_;
    
    // Register management (r9-r20 available for temps)
    bool usedRegs_[32];
    uint8_t allocateRegister();
    void freeRegister(uint8_t reg);
    
    // Local variable & stack management
    struct Local {
        int32_t offset;       // Offset relative to FP (r30)
        uint32_t typeId;
        std::string className;
        uint32_t elementTypeId;  // For Array types
        uint32_t keyTypeId;      // For Dict types
    };
    std::vector<std::unordered_map<std::string, Local>> scopeStack_;
    int32_t currentStackOffset_ = 0;
    
    // Class layout tracking
    struct ClassLayout {
        std::string name;
        std::string baseClass;
        std::unordered_map<std::string, int32_t> fieldOffsets;
        std::unordered_map<std::string, bool> privateMethods;
        std::unordered_map<std::string, FieldAccess> fieldAccess;
        std::unordered_map<std::string, uint32_t> methodReturnTypes;
        int32_t totalSize = 0;
        bool isSingleton = false, isFinal = false;
        bool isImmutable = false, isService = false, isSerializable = false;
        uint32_t singletonDataOffset = 0;
    };
    std::unordered_map<std::string, ClassLayout> classes_;
    
    // Label & control flow
    struct Label {
        int32_t targetByteOffset = -1;
        struct Fixup { size_t instructionIndex; uint32_t instructionByteOffset; };
        std::vector<Fixup> fixups;
    };
    struct SymbolFixup {
        std::string symbolName;
        size_t instructionIndex;
        uint32_t instructionByteOffset;
    };
    std::vector<SymbolFixup> symbolFixups_;
};
```

## Register allocation

Register r4 is reserved as the thread pointer. Arguments start at r0, skipping r4:

```cpp
static uint8_t argReg(uint8_t first, size_t i) {
    uint8_t reg = static_cast<uint8_t>(first + i);
    if (reg >= 4) ++reg;
    return reg;
}
```

Temporary registers r9-r20 are managed via `allocateRegister()` / `freeRegister()` with a bitmask (`usedRegs_[32]`). Loop and switch control-flow scopes also checkpoint this mask so `break`, `continue`, and join labels cannot carry stale temporary-register state into subsequent code generation. Managed-local cleanup remains separate from temporary-register state.

## Compilation phases

1. **Module-level declarations** — Top-level functions, classes, imports, globals.
2. **Forward declarations** — Allocates symbol table entries, collects class layouts.
3. **Function body compilation** — Emits bytecode for each function body.
4. **Class layout** — Generates field offsets, method tables, vtable entries.
5. **Serialization metadata** — Encodes class structure for `serialize()`/`deserialize()`.
6. **Module init** — `emitModuleInit()` generates runtime initialization code.

## Key responsibilities

### Function prologues/epilogues

`beginFunction()` generates the entry sequence (stack frame setup), and `endFunction()` emits the return sequence:

```cpp
FunctionPrologueInfo beginFunction(const ast::FunctionDeclaration* decl,
                                   const ast::ConstructorDeclaration* ctorDecl,
                                   bool isMethod, bool isConstructor);
void endFunction(const FunctionPrologueInfo& info);
```

### Class layouts

The `classes_` map tracks each class's fields, methods, base class, and modifier flags. Field offsets are computed sequentially during class body processing.

### Modifier handling

Each modifier has distinct codegen behavior:
- **Singleton** — A single instance pointer stored in `.data` section; `new` returns the singleton.
- **Immutable** — Fields cannot be reassigned after construction; `canWriteField()` checks this.
- **Final** — No subclassing allowed (enforced during class declaration).
- **Service** — Special lifecycle management codegen.
- **Serializable** — Auto-generates `serialize()` and `deserialize()` methods (`emitSerializeMethod()`, `emitDeserializeMethod()`).

### ARC (Automatic Reference Counting)

The codegen inserts `retain` calls after object allocations and `release` calls at end of scope for reference-typed locals. The JIT provides `jit_hoo_retain` / `jit_hoo_release` runtime helpers.

### Type inference

`inferExpressionTypeId()` determines the runtime type of an expression dynamically, while `typeIdFromDeclaredType()` resolves static type declarations. The type ID system is used for:
- Register allocation
- Mangling (`typeIdToMangleType()`)
- Method dispatch
- Serialization

`inferExpressionTypeInfo()` is the authoritative recursive path. It preserves
type ID, class name, element type, key type, and nullability through method and
function chains, fields, array access, await, collection expressions, and
scalar/logical expressions. Method dispatch keeps a multi-class candidate index
(`methodNameToClasses_`) and only uses a single class after receiver inference;
ambiguous unknown receivers are rejected during code generation.

For low-precision comparisons, byte-to-byte relational expressions use the
native HVM 1.5 `CMP_B` family. Mixed byte/integer expressions continue to use
the wide comparison family so their existing 64-bit ABI behavior is preserved.

Imported archive functions may provide `ExternalFunctionInfo` metadata with
module path, return type, and parameter types. The code generator retains this
metadata across module generation and uses it for direct chained inference and
external symbol mangling. Overload ranking prefers exact matches, then safe
widening conversions (`int8`/`byte` to `int64`, numeric values to `double`,
`f8` to `double`, and `bit` to `bool`), followed by generic/object fallbacks.

### Module imports

Three tracking structures manage imports:
- `importedModules_` — Set of imported module names (deduplication)
- `importedSymbols_` — Map of symbol name → required module
- `isModuleImported()`, `isSymbolImported()`, `getRequiredModule()` — Lookup helpers

### Built-in class dispatch

The codegen has special handling for ~20+ built-in classes via `classToPrefix()`:

```cpp
static std::string classToPrefix(const std::string& className) {
    static const std::unordered_map<std::string, std::string> map = {
        {"String", "string"}, {"DateTime", "datetime"}, {"Math", "math"},
        {"Fs", "fs"}, {"System", "system"}, {"Regex", "regex"},
        {"Net", "net"}, {"Path", "path"}, {"Hashing", "hashing"},
        {"Uuid", "uuid"}, {"Compression", "compression"},
        {"Character", "character"}, {"Args", "args"}, {"Csv", "csv"},
        {"Console", "console"}, {"URL", "net_url"},
        {"HttpClient", "net_http_client"}, {"HttpResponse", "net_http_response"},
        {"Thread", "thread"}, {"Mutex", "thread_mutex"},
        {"Array", "array"}, {"Map", "map"}, {"Buffer", "buffer"},
        {"Random", "random"}, {"List", "anyarray"}, {"Dict", "hashmap"},
    };
    ...
}
```

These map to JIT symbol prefixes for the `prefix_methodname` calling convention.

### Free function dispatch

Separate static helper functions identify and route external library calls:

| Helper | Purpose |
|---|---|
| `isJsonFreeFunction()` | JSON serialization/deserialization functions |
| `isBufferFreeFunction()` | `byte_slice_from_buffer`, `byte_slice_release` |
| `isCsvFreeFunction()` | CSV parsing |
| `isFsFreeFunction()` | Filesystem operations (23 functions) |
| `isDatetimeFreeFunction()` | Date/time operations (16 functions) |
| `isEncodingFreeFunction()` | Base64/hex/URL encoding |
| `isMathFreeFunction()` | Math functions (37 functions) |
| `isHashingFreeFunction()` | SHA256/SHA1/MD5/CRC32/HMAC |
| `isSystemFreeFunction()` | Environment, process, system info |
| `isProcessFreeFunction()` | Process spawn/capture/kill |
| `isRegexFreeFunction()` | Regex match/search/replace/split |
| `isThreadFreeFunction()` | Thread spawn/join |
| `isUuidFreeFunction()` | UUID generation/manipulation |
| `isCharacterFreeFunction()` | UTF-8 character conversion |
| `isPathFreeFunction()` | Path manipulation (17 functions) |

Each has a corresponding `*ReturnTypeId()` helper that maps the function name to its runtime type ID.

### Serialization validation

`validateSerializableClass()` enforces:
- All fields must be valid serializable types (supported primitives, buffers,
  tensors, List, numeric-keyed Dict values, or serializable classes)
- No `any` type fields
- Cycle detection via `serializableAdjacency_` and `detectSerializableCycles()`
- `isValidSerializableType()` recursively validates field types

Generated serializable methods use a base-first positional schema backed by
`Dict<int64, any>`. Nested classes are lowered by calling their generated
methods; buffers use tagged base64 objects and tensors use tagged element-type,
dimensions, and raw-bit data objects. The source-level `Class.deserialize(json)`
form is recognized as a generated static call, while `instance.serialize()`
uses the modifier-aware instance symbol.

### Bytecode emission pattern

```cpp
void compileFunction(ASTNode *func) {
    enterFunction(func);
    compileSignature(func);
    compileBody(func);
    exitFunction();
}

void compileBody(ASTNode *body) {
    for (auto *stmt : body->children) {
        compileStmt(stmt);
    }
}
```

### Instruction emission

The codegen supports both standard and compressed (16-bit) instructions:
```cpp
void emit(hvm::Opcode op, const hvm::Operands& operands);  // standard
void emitCompressed(uint8_t opcode4, uint8_t rd, uint8_t rs1, uint8_t imm4);  // 16-bit
```

Forward references are resolved via:
- **Labels** — Control flow targets with fixup lists (`Label::fixups`)
- **Symbol fixups** — External call targets resolved at module finalization (`SymbolFixup`)

## Important globals

The code generator references ~100+ runtime functions via `extern` declarations (e.g., `jit_hoo_*` wrappers). These are linked at JIT time through `HVMJIT`. Each built-in class and free function set has corresponding JIT symbols registered in the ORC runtime.

## Test patterns

Tests in `tests/codegen/` use `HooCompiler` to compile source code and inspect the resulting `HOModule`:

```cpp
class HVMCodeGeneratorTest : public ::testing::Test {
protected:
    std::unique_ptr<HooCompiler> compiler_;
    
    const Symbol* findSymbol(const HOModule& mod, const std::string& baseName) {
        for (const auto& sym : mod.getSymbols()) {
            if (sym.name.find(baseName) != std::string::npos) return &sym;
        }
        return nullptr;
    }
};

TEST_F(HVMCodeGeneratorTest, IfElse) {
    std::string code = R"(
        func:int64 max(a: int64, b: int64) {
            if (a > b) { return a; } else { return b; }
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    auto insts = module->decodeInstructions(
        module->getSection(".text")->data);
    // Verify control flow opcodes exist
    bool foundCmp = false, foundBne = false;
    for (const auto& inst : insts) {
        if (inst.getOpcode() == Opcode::CMP) foundCmp = true;
        if (inst.getOpcode() == Opcode::BNE) foundBne = true;
    }
    EXPECT_TRUE(foundCmp);
    EXPECT_TRUE(foundBne);
}
```
