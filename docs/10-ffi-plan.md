# Foreign Function Interface (FFI) Implementation Plan
## Dynamic/Shared Library Import System for hooc

**Version:** 0.6 (Proposed)  
**Date:** December 27, 2024  
**Status:** Design Document  
**Author:** Architecture Team

---

## 🎯 Executive Summary

This document outlines a comprehensive plan to extend the **hooc** compiler with Foreign Function Interface (FFI) capabilities, enabling hoo code to import and use external shared/dynamic libraries (`.dll`, `.so`, `.dylib`) as if they were native hoo modules.

### Goals
1. **Import external libraries** using hoo's existing `import` syntax
2. **Access C functions** from shared libraries with automatic type marshaling
3. **Access C constants and variables** (exported symbols)
4. **Access C structs** mapped to hoo classes
5. **Seamless integration** with LLVM ORC JIT for dynamic loading
6. **Type safety** with explicit FFI declarations

---

## 📋 Table of Contents

1. [Overview](#overview)
2. [Grammar Modifications](#grammar-modifications)
3. [AST Extensions](#ast-extensions)
4. [SimpleASTBuilder Changes](#simpleastbuilder-changes)
5. [Runtime Library Extensions](#runtime-library-extensions)
6. [LLVMCodeGenerator Changes](#llvmcodegenerator-changes)
7. [HoocJIT Enhancements](#hoocjit-enhancements)
8. [Module Resolver System](#module-resolver-system)
9. [Type Marshaling](#type-marshaling)
10. [Example Usage](#example-usage)
11. [Implementation Phases](#implementation-phases)
12. [Testing Strategy](#testing-strategy)

---

## 1. Overview

### Current State
- ✅ Import statements are **parsed** (`BasicImport`, `FromImport`)
- ✅ AST nodes exist for imports (`ImportStatement.h`)
- ❌ **No resolution logic** - imports are not processed
- ❌ **No module system** - no search paths or loading

### Target Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    hoo Source Code                          │
│   import math from "libm.so"                                │
│   var x = math.sqrt(16.0)                                   │
└──────────────────────┬──────────────────────────────────────┘
                       ↓
┌──────────────────────────────────────────────────────────────┐
│              Parser (ANTLR4)                                 │
│  - Parses import statements                                  │
│  - Extracts module path and symbols                          │
└──────────────────────┬───────────────────────────────────────┘
                       ↓
┌──────────────────────────────────────────────────────────────┐
│         SimpleASTBuilder                                     │
│  - Builds ImportStatement AST nodes                          │
│  - Validates syntax                                          │
└──────────────────────┬───────────────────────────────────────┘
                       ↓
┌──────────────────────────────────────────────────────────────┐
│         ModuleResolver (NEW)                                 │
│  - Resolves module paths to library files                    │
│  - Searches standard paths + custom paths                    │
│  - Parses FFI declaration files (.ffi.hoo)                   │
│  - Builds symbol table (functions, constants, types)         │
└──────────────────────┬───────────────────────────────────────┘
                       ↓
┌──────────────────────────────────────────────────────────────┐
│         LLVMCodeGenerator                                    │
│  - Generates LLVM IR for external function declarations      │
│  - Inserts type marshaling code                              │
│  - Links symbol references                                   │
└──────────────────────┬───────────────────────────────────────┘
                       ↓
┌──────────────────────────────────────────────────────────────┐
│         HoocJIT                                              │
│  - Dynamically loads shared libraries (dlopen/LoadLibrary)   │
│  - Resolves external symbols via JITDylib                    │
│  - Registers symbols in LLVM ORC JIT                         │
│  - Executes code with external calls                         │
└──────────────────────────────────────────────────────────────┘
```

---

## 2. Grammar Modifications

### 2.1 Current Grammar (Already Sufficient)

The existing grammar in `Hooc.g4` already supports the syntax we need:

```antlr
// Lines 109-117 in Hooc.g4
importStatement
    : IMPORT modulePath (AS IDENTIFIER)? SEMICOLON              # basicImport
    | FROM modulePath IMPORT importItem (COMMA importItem)* SEMICOLON  # fromImport
    ;

modulePath: IDENTIFIER (DOT IDENTIFIER)*;
importItem: IDENTIFIER (AS IDENTIFIER)?;
```

### 2.2 Proposed Extensions

Add **FFI-specific syntax** for explicit external declarations:

```antlr
// Add to keyword list
EXTERN: 'extern';
CDECL: 'cdecl';
STDCALL: 'stdcall';

// Add new declaration type
declaration
    : functionDeclaration
    | classDeclaration
    | interfaceDeclaration
    | variableDeclaration
    | externDeclaration  // NEW
    ;

// External function/variable declarations
externDeclaration
    : EXTERN callingConvention? externItem SEMICOLON
    ;

callingConvention: CDECL | STDCALL;

externItem
    : externFunction
    | externVariable
    | externConstant
    ;

externFunction
    : FUNC IDENTIFIER LPAREN parameterList? RPAREN (ARROW type)? (FROM STRING_LITERAL)?
    ;

externVariable
    : VAR IDENTIFIER COLON type (FROM STRING_LITERAL)?
    ;

externConstant
    : FINAL VAR IDENTIFIER COLON type ASSIGN primary (FROM STRING_LITERAL)?
    ;
```

### 2.3 Example Syntax

```hoo
// Option 1: Import with FFI declaration file
import math from "libm.so"

// Option 2: Explicit extern declarations
extern cdecl func sqrt(x: double) -> double from "libm.so";
extern cdecl func printf(fmt: string, ...) -> int64 from "libc.so";

// Option 3: Namespace import with FFI
from "libm.so" import sqrt, pow, sin, cos;
```

---

## 3. AST Extensions

### 3.1 New AST Nodes

Create `src/ast/ExternDeclaration.h`:

```cpp
#pragma once

#include "Declaration.h"
#include "Type.h"
#include <string>
#include <memory>
#include <vector>

namespace hooc {
namespace ast {

enum class CallingConvention {
    CDecl,      // Default C calling convention
    StdCall,    // Windows stdcall
    FastCall,   // Compiler-specific fast call
    Default     // Platform default
};

// Base class for external declarations
class ExternDeclaration : public Declaration {
public:
    ExternDeclaration(const std::string& name, 
                      const std::string& libraryPath,
                      CallingConvention convention = CallingConvention::CDecl)
        : name_(name), libraryPath_(libraryPath), convention_(convention) {}
    
    virtual ~ExternDeclaration() = default;

    const std::string& getName() const { return name_; }
    const std::string& getLibraryPath() const { return libraryPath_; }
    CallingConvention getCallingConvention() const { return convention_; }

protected:
    std::string name_;
    std::string libraryPath_;
    CallingConvention convention_;
};

// External function declaration
class ExternFunctionDeclaration : public ExternDeclaration {
public:
    ExternFunctionDeclaration(
        const std::string& name,
        std::vector<std::unique_ptr<Parameter>> parameters,
        std::unique_ptr<Type> returnType,
        const std::string& libraryPath,
        CallingConvention convention = CallingConvention::CDecl,
        bool isVariadic = false)
        : ExternDeclaration(name, libraryPath, convention),
          parameters_(std::move(parameters)),
          returnType_(std::move(returnType)),
          isVariadic_(isVariadic) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<Parameter>>& getParameters() const { 
        return parameters_; 
    }
    const Type* getReturnType() const { return returnType_.get(); }
    bool isVariadic() const { return isVariadic_; }

private:
    std::vector<std::unique_ptr<Parameter>> parameters_;
    std::unique_ptr<Type> returnType_;
    bool isVariadic_;
};

// External variable declaration
class ExternVariableDeclaration : public ExternDeclaration {
public:
    ExternVariableDeclaration(
        const std::string& name,
        std::unique_ptr<Type> type,
        const std::string& libraryPath,
        bool isConst = false)
        : ExternDeclaration(name, libraryPath),
          type_(std::move(type)),
          isConst_(isConst) {}

    std::string toString() const override;

    const Type* getType() const { return type_.get(); }
    bool isConst() const { return isConst_; }

private:
    std::unique_ptr<Type> type_;
    bool isConst_;
};

// External constant declaration (compile-time known)
class ExternConstantDeclaration : public ExternDeclaration {
public:
    ExternConstantDeclaration(
        const std::string& name,
        std::unique_ptr<Type> type,
        std::unique_ptr<Primary> value,
        const std::string& libraryPath)
        : ExternDeclaration(name, libraryPath),
          type_(std::move(type)),
          value_(std::move(value)) {}

    std::string toString() const override;

    const Type* getType() const { return type_.get(); }
    const Primary* getValue() const { return value_.get(); }

private:
    std::unique_ptr<Type> type_;
    std::unique_ptr<Primary> value_;
};

} // namespace ast
} // namespace hooc
```

### 3.2 Extend ImportStatement

Modify `src/ast/ImportStatement.h` to track resolved symbols:

```cpp
// Add to BasicImport class
class BasicImport : public ImportStatement {
public:
    // ... existing code ...

    void setResolvedPath(const std::string& path) { resolvedPath_ = path; }
    const std::string& getResolvedPath() const { return resolvedPath_; }
    
    void addResolvedSymbol(const std::string& name, void* address) {
        resolvedSymbols_[name] = address;
    }
    const std::unordered_map<std::string, void*>& getResolvedSymbols() const {
        return resolvedSymbols_;
    }

private:
    std::string resolvedPath_;  // Full path to library file
    std::unordered_map<std::string, void*> resolvedSymbols_;  // Symbol name -> address
};
```

---

## 4. SimpleASTBuilder Changes

### 4.1 Add Extern Declaration Building

Extend `src/SimpleASTBuilder.h`:

```cpp
class SimpleASTBuilder {
public:
    // ... existing methods ...

    // New methods for extern declarations
    std::unique_ptr<ast::ExternDeclaration> buildExternDeclaration(
        HoocParser::ExternDeclarationContext* ctx);
    
    std::unique_ptr<ast::ExternFunctionDeclaration> buildExternFunction(
        HoocParser::ExternFunctionContext* ctx);
    
    std::unique_ptr<ast::ExternVariableDeclaration> buildExternVariable(
        HoocParser::ExternVariableContext* ctx);
    
    ast::CallingConvention getCallingConvention(
        HoocParser::CallingConventionContext* ctx);
};
```

### 4.2 Implementation in `src/SimpleASTBuilder.cpp`

```cpp
std::unique_ptr<ast::ExternDeclaration> 
SimpleASTBuilder::buildExternDeclaration(HoocParser::ExternDeclarationContext* ctx) {
    if (!ctx) return nullptr;

    // Extract calling convention (default to CDecl)
    ast::CallingConvention convention = ast::CallingConvention::CDecl;
    if (ctx->callingConvention()) {
        convention = getCallingConvention(ctx->callingConvention());
    }

    // Dispatch based on extern item type
    if (auto funcCtx = ctx->externItem()->externFunction()) {
        return buildExternFunction(funcCtx);
    } else if (auto varCtx = ctx->externItem()->externVariable()) {
        return buildExternVariable(varCtx);
    }
    // ... handle other types
    
    return nullptr;
}

std::unique_ptr<ast::ExternFunctionDeclaration> 
SimpleASTBuilder::buildExternFunction(HoocParser::ExternFunctionContext* ctx) {
    if (!ctx) return nullptr;

    std::string name = ctx->IDENTIFIER()->getText();
    
    // Build parameter list
    std::vector<std::unique_ptr<ast::Parameter>> params;
    if (ctx->parameterList()) {
        for (auto paramCtx : ctx->parameterList()->parameter()) {
            params.push_back(buildParameter(paramCtx));
        }
    }

    // Build return type
    std::unique_ptr<ast::Type> returnType;
    if (ctx->type()) {
        returnType = buildType(ctx->type());
    } else {
        returnType = std::make_unique<ast::PrimitiveType>(ast::PrimitiveTypeKind::Void);
    }

    // Extract library path from FROM clause
    std::string libraryPath;
    if (ctx->STRING_LITERAL()) {
        libraryPath = getStringValue(ctx->STRING_LITERAL());
    }

    return std::make_unique<ast::ExternFunctionDeclaration>(
        name, std::move(params), std::move(returnType), 
        libraryPath, ast::CallingConvention::CDecl);
}
```

---

## 5. Runtime Library Extensions

### 5.1 Dynamic Library Loading

Create `runtime/hoo_ffi.h`:

```c
#ifndef HOO_FFI_H
#define HOO_FFI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to loaded library
typedef void* HooLibHandle;

/**
 * Load a shared/dynamic library
 * 
 * @param path Path to library file (.so, .dll, .dylib)
 * @return Handle to loaded library, or NULL on failure
 */
HooLibHandle hoo_load_library(const char* path);

/**
 * Resolve a symbol from a loaded library
 * 
 * @param handle Library handle from hoo_load_library
 * @param symbol_name Name of the symbol to resolve
 * @return Address of symbol, or NULL if not found
 */
void* hoo_get_symbol(HooLibHandle handle, const char* symbol_name);

/**
 * Unload a library
 * 
 * @param handle Library handle to unload
 * @return true if successful, false otherwise
 */
bool hoo_unload_library(HooLibHandle handle);

/**
 * Get the last FFI error message
 * 
 * @return Error string, or NULL if no error
 */
const char* hoo_ffi_error(void);

/**
 * Search for library in standard paths
 * Returns full path if found, NULL otherwise
 */
const char* hoo_find_library(const char* lib_name);

#ifdef __cplusplus
}
#endif

#endif // HOO_FFI_H
```

### 5.2 Implementation in `runtime/hoo_ffi.c`

```c
#include "hoo_ffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define dlopen(path, flags) LoadLibraryA(path)
#define dlsym(handle, name) GetProcAddress((HMODULE)handle, name)
#define dlclose(handle) FreeLibrary((HMODULE)handle)
#define dlerror() "Windows LoadLibrary error"
#else
#include <dlfcn.h>
#endif

static __thread char last_error[512] = {0};

HooLibHandle hoo_load_library(const char* path) {
    if (!path) {
        snprintf(last_error, sizeof(last_error), "NULL library path");
        return NULL;
    }

#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path);
    if (!handle) {
        snprintf(last_error, sizeof(last_error), 
                 "Failed to load library '%s': error code %lu", 
                 path, GetLastError());
    }
    return (HooLibHandle)handle;
#else
    void* handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        const char* err = dlerror();
        snprintf(last_error, sizeof(last_error), 
                 "Failed to load library '%s': %s", 
                 path, err ? err : "unknown error");
    }
    return handle;
#endif
}

void* hoo_get_symbol(HooLibHandle handle, const char* symbol_name) {
    if (!handle || !symbol_name) {
        snprintf(last_error, sizeof(last_error), 
                 "Invalid handle or symbol name");
        return NULL;
    }

#ifdef _WIN32
    void* addr = (void*)GetProcAddress((HMODULE)handle, symbol_name);
    if (!addr) {
        snprintf(last_error, sizeof(last_error), 
                 "Symbol '%s' not found: error code %lu", 
                 symbol_name, GetLastError());
    }
#else
    dlerror(); // Clear previous errors
    void* addr = dlsym(handle, symbol_name);
    const char* err = dlerror();
    if (err) {
        snprintf(last_error, sizeof(last_error), 
                 "Symbol '%s' not found: %s", symbol_name, err);
        addr = NULL;
    }
#endif
    
    return addr;
}

bool hoo_unload_library(HooLibHandle handle) {
    if (!handle) return false;
    
#ifdef _WIN32
    return FreeLibrary((HMODULE)handle) != 0;
#else
    return dlclose(handle) == 0;
#endif
}

const char* hoo_ffi_error(void) {
    return last_error[0] ? last_error : NULL;
}

const char* hoo_find_library(const char* lib_name) {
    // Search paths (platform-dependent)
    static char full_path[1024];
    
    const char* search_paths[] = {
        "./",
        "/usr/lib/",
        "/usr/local/lib/",
#ifdef _WIN32
        "C:\\Windows\\System32\\",
#endif
        NULL
    };

    for (int i = 0; search_paths[i]; i++) {
        snprintf(full_path, sizeof(full_path), "%s%s", 
                 search_paths[i], lib_name);
        
        // Try to open the file
        FILE* f = fopen(full_path, "rb");
        if (f) {
            fclose(f);
            return full_path;
        }
    }

    return NULL;
}
```

---

## 6. LLVMCodeGenerator Changes

### 6.1 Symbol Table Extensions

Modify `src/LLVMCodeGenerator.h`:

```cpp
class LLVMCodeGenerator : public CodeGenerator {
public:
    // ... existing methods ...

    // New FFI-specific methods
    void processImportStatements(const std::vector<std::unique_ptr<ast::ImportStatement>>& imports);
    llvm::Function* generateExternFunction(const ast::ExternFunctionDeclaration& externFunc);
    llvm::GlobalVariable* generateExternVariable(const ast::ExternVariableDeclaration& externVar);

private:
    // ... existing members ...

    // FFI symbol tracking
    std::unordered_map<std::string, void*> externalSymbols_;  // symbol name -> runtime address
    std::unordered_map<std::string, std::string> symbolToLibrary_;  // symbol -> library path
    std::vector<std::string> loadedLibraries_;  // Tracks loaded library paths
};
```

### 6.2 Implementation

Add to `src/LLVMCodeGenerator.cpp`:

```cpp
void LLVMCodeGenerator::processImportStatements(
    const std::vector<std::unique_ptr<ast::ImportStatement>>& imports) {
    
    for (const auto& import : imports) {
        if (auto basicImport = dynamic_cast<const ast::BasicImport*>(import.get())) {
            // Resolve module path to library file
            std::string modulePath = basicImport->getModule()->toString();
            std::string libraryPath = resolveLibraryPath(modulePath);
            
            if (libraryPath.empty()) {
                std::cerr << "Warning: Could not resolve module '" 
                          << modulePath << "'\n";
                continue;
            }

            // Load FFI declaration file (module.ffi.hoo)
            std::string ffiDeclPath = modulePath + ".ffi.hoo";
            auto ffiDecls = loadFFIDeclarations(ffiDeclPath);
            
            // Generate LLVM declarations for imported symbols
            for (const auto& decl : ffiDecls) {
                if (auto funcDecl = dynamic_cast<ast::ExternFunctionDeclaration*>(decl.get())) {
                    generateExternFunction(*funcDecl);
                }
            }
            
            loadedLibraries_.push_back(libraryPath);
        }
    }
}

llvm::Function* LLVMCodeGenerator::generateExternFunction(
    const ast::ExternFunctionDeclaration& externFunc) {
    
    // Build parameter types
    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : externFunc.getParameters()) {
        llvm::Type* paramType = generateLLVMType(*param->getType());
        paramTypes.push_back(paramType);
    }

    // Build return type
    llvm::Type* returnType = generateLLVMType(*externFunc.getReturnType());

    // Create function type
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        returnType, paramTypes, externFunc.isVariadic());

    // Create external function declaration
    llvm::Function* func = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        externFunc.getName(),
        module_.get());

    // Set calling convention
    switch (externFunc.getCallingConvention()) {
        case ast::CallingConvention::CDecl:
            func->setCallingConv(llvm::CallingConv::C);
            break;
        case ast::CallingConvention::StdCall:
            func->setCallingConv(llvm::CallingConv::X86_StdCall);
            break;
        default:
            func->setCallingConv(llvm::CallingConv::C);
    }

    // Register in symbol table
    functions_[externFunc.getName()] = func;
    symbolToLibrary_[externFunc.getName()] = externFunc.getLibraryPath();

    return func;
}
```

---

## 7. HoocJIT Enhancements

### 7.1 Dynamic Library Loading

Modify `src/HoocJIT.h`:

```cpp
#include "runtime/hoo_ffi.h"
#include <unordered_map>
#include <string>

class HoocJIT {
public:
    // ... existing methods ...

    // New FFI methods
    bool loadExternalLibrary(const std::string& libraryPath);
    void* resolveExternalSymbol(const std::string& symbolName);
    void registerExternalSymbol(const std::string& name, void* address);

private:
    // ... existing members ...

    // FFI support
    std::unordered_map<std::string, HooLibHandle> loadedLibraries_;
    std::unordered_map<std::string, void*> externalSymbols_;
};
```

### 7.2 Implementation in `src/HoocJIT.cpp`

```cpp
#include "HoocJIT.h"
#include "runtime/hoo_ffi.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include <iostream>

bool HoocJIT::loadExternalLibrary(const std::string& libraryPath) {
    // Check if already loaded
    if (loadedLibraries_.find(libraryPath) != loadedLibraries_.end()) {
        return true;
    }

    // Load the library
    HooLibHandle handle = hoo_load_library(libraryPath.c_str());
    if (!handle) {
        std::cerr << "Failed to load library: " << hoo_ffi_error() << "\n";
        return false;
    }

    loadedLibraries_[libraryPath] = handle;
    std::cout << "Loaded external library: " << libraryPath << "\n";
    return true;
}

void* HoocJIT::resolveExternalSymbol(const std::string& symbolName) {
    // Check if already resolved
    auto it = externalSymbols_.find(symbolName);
    if (it != externalSymbols_.end()) {
        return it->second;
    }

    // Search in all loaded libraries
    for (const auto& [path, handle] : loadedLibraries_) {
        void* addr = hoo_get_symbol(handle, symbolName.c_str());
        if (addr) {
            externalSymbols_[symbolName] = addr;
            return addr;
        }
    }

    std::cerr << "Symbol '" << symbolName << "' not found in any loaded library\n";
    return nullptr;
}

void HoocJIT::registerExternalSymbol(const std::string& name, void* address) {
    externalSymbols_[name] = address;

    // Register with LLVM ORC JIT using absolute symbols
    auto& mainJD = JIT->getMainJITDylib();
    
    llvm::orc::SymbolMap symbols;
    symbols[JIT->mangleAndIntern(name)] = 
        llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(address),
                                  llvm::JITSymbolFlags::Exported);

    llvm::cantFail(mainJD.define(llvm::orc::absoluteSymbols(symbols)));
}
```

### 7.3 Integration in Compilation Pipeline

Update `HoocJIT::compileHoocCode`:

```cpp
bool HoocJIT::compileHoocCode(const std::string& code) {
    // Step 1: Parse
    parser_->parseForAST(code);
    auto parseTree = /* get parse tree */;

    // Step 2: Build AST
    auto ast = astBuilder_->buildAST(parseTree);
    if (!ast) return false;

    // Step 3: Process imports and load external libraries
    for (const auto& import : ast->getImports()) {
        if (auto basicImport = dynamic_cast<const ast::BasicImport*>(import.get())) {
            std::string libraryPath = resolveLibraryPath(basicImport->getModule());
            if (!loadExternalLibrary(libraryPath)) {
                std::cerr << "Failed to load library for import\n";
                return false;
            }
        }
    }

    // Step 4: Generate LLVM IR
    auto llvmModule = codeGenerator_->generateLLVMModule(*ast);
    if (!llvmModule) return false;

    // Step 5: Resolve external symbols and register with JIT
    for (auto& func : llvmModule->functions()) {
        if (func.isDeclaration() && func.hasExternalLinkage()) {
            void* addr = resolveExternalSymbol(func.getName().str());
            if (addr) {
                registerExternalSymbol(func.getName().str(), addr);
            }
        }
    }

    // Step 6: Add module to JIT
    auto err = JIT->addIRModule(llvm::orc::ThreadSafeModule(
        std::move(llvmModule), 
        std::make_unique<llvm::LLVMContext>()));
    
    if (err) {
        llvm::handleAllErrors(std::move(err), [](const llvm::ErrorInfoBase& e) {
            std::cerr << "JIT error: " << e.message() << "\n";
        });
        return false;
    }

    return true;
}
```

---

## 8. Module Resolver System

### 8.1 Module Resolver Interface

Create `src/ModuleResolver.h`:

```cpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ast/AST.h"

namespace hooc {

struct ModuleInfo {
    std::string name;
    std::string fullPath;
    std::string ffiDeclPath;
    bool isNativeHoo;
    std::vector<std::string> exportedSymbols;
};

class ModuleResolver {
public:
    ModuleResolver();

    // Configure search paths
    void addSearchPath(const std::string& path);
    void setSystemSearchPaths();

    // Resolve module name to file path
    ModuleInfo resolveModule(const std::string& moduleName);

    // Load FFI declarations from .ffi.hoo file
    std::vector<std::unique_ptr<ast::ExternDeclaration>> 
        loadFFIDeclarations(const std::string& ffiFilePath);

    // Check if module is a hoo module or external library
    bool isHooModule(const std::string& moduleName);

private:
    std::vector<std::string> searchPaths_;

    std::string findLibraryFile(const std::string& name);
    std::string getPlatformLibraryName(const std::string& name);
};

} // namespace hooc
```

### 8.2 FFI Declaration Files

Create `.ffi.hoo` files alongside external libraries:

**Example: `libm.ffi.hoo`**

```hoo
// FFI declarations for libm (math library)

extern cdecl func sqrt(x: double) -> double;
extern cdecl func pow(x: double, y: double) -> double;
extern cdecl func sin(x: double) -> double;
extern cdecl func cos(x: double) -> double;
extern cdecl func tan(x: double) -> double;
extern cdecl func log(x: double) -> double;
extern cdecl func exp(x: double) -> double;

// Constants
final var M_PI: double = 3.14159265358979323846;
final var M_E: double = 2.71828182845904523536;
```

**Example: `opengl.ffi.hoo`**

```hoo
// FFI declarations for OpenGL

// Types
class GLuint { var value: uint32; }
class GLfloat { var value: float; }

// Functions
extern cdecl func glClear(mask: uint32) -> void;
extern cdecl func glColor3f(r: float, g: float, b: float) -> void;
extern cdecl func glBegin(mode: uint32) -> void;
extern cdecl func glEnd() -> void;
extern cdecl func glVertex3f(x: float, y: float, z: float) -> void;

// Constants
final var GL_COLOR_BUFFER_BIT: uint32 = 0x00004000;
final var GL_TRIANGLES: uint32 = 0x0004;
```

---

## 9. Type Marshaling

### 9.1 Type Mapping Table

| hoo Type | C Type | LLVM Type | Notes |
|----------|--------|-----------|-------|
| `byte` | `uint8_t` | `i8` | Direct mapping |
| `uint8` | `uint8_t` | `i8` | Direct mapping |
| `int64` | `int64_t` | `i64` | Direct mapping |
| `float` | `float` | `f32` | Direct mapping |
| `double` | `double` | `f64` | Direct mapping |
| `bool` | `bool` (C99) | `i1` | Convert to i8 for C ABI |
| `char` | `char` | `i8` | Direct mapping |
| `string` | `const char*` | `i8*` | Null-terminated C string |
| `T[]` | `T*` | `ptr` | Pointer to first element |
| `T?` | `struct { bool, T }` | `{ i1, T }` | **Needs marshaling** |

### 9.2 Marshaling Functions

Add to `LLVMCodeGenerator`:

```cpp
// Convert hoo nullable to C pointer (NULL for null, ptr for value)
llvm::Value* LLVMCodeGenerator::marshalNullableToPtr(llvm::Value* nullable, llvm::Type* ptrType) {
    // Extract null flag
    llvm::Value* isNull = extractNullFlagFromNullable(nullable);
    
    // Extract value
    llvm::Value* value = extractValueFromNullable(nullable);
    
    // Allocate space for value
    llvm::AllocaInst* tempPtr = builder_->CreateAlloca(value->getType());
    builder_->CreateStore(value, tempPtr);
    
    // Return NULL if null flag is true, otherwise return pointer
    return builder_->CreateSelect(
        isNull,
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptrType)),
        tempPtr
    );
}

// Convert C pointer to hoo nullable
llvm::Value* LLVMCodeGenerator::marshalPtrToNullable(llvm::Value* ptr, llvm::Type* nullableType) {
    // Check if pointer is NULL
    llvm::Value* isNull = builder_->CreateIsNull(ptr);
    
    // Load value if not NULL
    llvm::Value* value = builder_->CreateLoad(ptr);
    
    // Create nullable struct
    return builder_->CreateSelect(
        isNull,
        createNullValue(value->getType()),
        wrapValueInNullable(value, nullableType)
    );
}
```

---

## 10. Example Usage

### 10.1 Using Standard C Library

```hoo
// example_libc.hoo

// Import math functions
from "libm.so" import sqrt, pow, sin;

func calculate_distance(x1: double, y1: double, 
                       x2: double, y2: double) -> double {
    var dx = x2 - x1;
    var dy = y2 - y1;
    return sqrt(pow(dx, 2.0) + pow(dy, 2.0));
}

func main() -> void {
    var dist = calculate_distance(0.0, 0.0, 3.0, 4.0);
    // dist = 5.0
    
    var angle = sin(3.14159 / 4.0);  // 45 degrees
    // angle ≈ 0.707
}
```

### 10.2 Using Custom C Library

**C Library (`mylib.c`):**

```c
// mylib.c
#include <stdio.h>

void greet(const char* name) {
    printf("Hello, %s!\n", name);
}

int add(int a, int b) {
    return a + b;
}
```

**Compile to shared library:**
```bash
gcc -shared -fPIC -o libmylib.so mylib.c
```

**FFI Declaration (`mylib.ffi.hoo`):**

```hoo
extern cdecl func greet(name: string) -> void;
extern cdecl func add(a: int64, b: int64) -> int64;
```

**hoo Program:**

```hoo
// example_custom.hoo

from "libmylib.so" import greet, add;

func main() -> void {
    greet("World");
    
    var sum = add(10, 32);
    // sum = 42
}
```

### 10.3 Using OpenGL

```hoo
// example_opengl.hoo

from "libGL.so" import glClear, glColor3f, glBegin, glEnd, glVertex3f;
from "libGL.so" import GL_COLOR_BUFFER_BIT, GL_TRIANGLES;

func render_triangle() -> void {
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBegin(GL_TRIANGLES);
    
    glColor3f(1.0, 0.0, 0.0);  // Red
    glVertex3f(0.0, 1.0, 0.0);
    
    glColor3f(0.0, 1.0, 0.0);  // Green
    glVertex3f(-1.0, -1.0, 0.0);
    
    glColor3f(0.0, 0.0, 1.0);  // Blue
    glVertex3f(1.0, -1.0, 0.0);
    
    glEnd();
}
```

---

## 11. Implementation Phases

### Phase 1: Foundation (2-3 weeks)
**Goal:** Basic FFI infrastructure

- [ ] Add `extern` keyword to grammar
- [ ] Create `ExternDeclaration` AST nodes
- [ ] Implement `SimpleASTBuilder` support for extern declarations
- [ ] Create `runtime/hoo_ffi.{h,c}` with library loading
- [ ] Add basic tests for parsing extern declarations

**Deliverable:** Can parse `extern func` declarations and load libraries

---

### Phase 2: Symbol Resolution (2 weeks)
**Goal:** Link external symbols

- [ ] Create `ModuleResolver` class
- [ ] Implement library search paths (platform-specific)
- [ ] Add FFI declaration file parsing (`.ffi.hoo`)
- [ ] Extend `LLVMCodeGenerator` to generate external function declarations
- [ ] Add symbol resolution in `HoocJIT`

**Deliverable:** Can resolve and call simple C functions (printf, sqrt)

---

### Phase 3: Type Marshaling (2-3 weeks)
**Goal:** Safe type conversions

- [ ] Implement type marshaling for primitive types
- [ ] Add string marshaling (hoo string ↔ C `char*`)
- [ ] Implement nullable type marshaling
- [ ] Add array/slice marshaling
- [ ] Create comprehensive marshaling tests

**Deliverable:** Can pass complex types to/from C functions

---

### Phase 4: Advanced Features (2-3 weeks)
**Goal:** Full FFI support

- [ ] Support extern variables and constants
- [ ] Implement calling convention support (cdecl, stdcall)
- [ ] Add variadic function support (`printf`, `scanf`)
- [ ] Implement struct marshaling (C structs ↔ hoo classes)
- [ ] Add callback support (C function pointers)

**Deliverable:** Full-featured FFI system

---

### Phase 5: Standard Library Bindings (2 weeks)
**Goal:** Useful standard bindings

- [ ] Create `libm.ffi.hoo` (math library)
- [ ] Create `libc.ffi.hoo` (stdio, stdlib, string)
- [ ] Create `pthread.ffi.hoo` (threading)
- [ ] Create platform-specific bindings (Windows API, POSIX)
- [ ] Documentation and examples

**Deliverable:** Ready-to-use standard library FFI bindings

---

### Phase 6: Testing & Documentation (1-2 weeks)
**Goal:** Production-ready

- [ ] Comprehensive test suite (50+ FFI tests)
- [ ] Example programs using FFI
- [ ] Performance benchmarks
- [ ] User documentation
- [ ] API reference for FFI system

**Deliverable:** Production-ready FFI implementation

---

## 12. Testing Strategy

### 12.1 Unit Tests

**Test file:** `tests/FFIParsingTest.cpp`

```cpp
TEST(FFIParsingTest, ParseExternFunction) {
    std::string code = R"(
        extern cdecl func sqrt(x: double) -> double;
    )";
    
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1);
    
    auto* externFunc = dynamic_cast<ast::ExternFunctionDeclaration*>(
        ast->getDeclarations()[0].get());
    ASSERT_NE(externFunc, nullptr);
    EXPECT_EQ(externFunc->getName(), "sqrt");
}
```

**Test file:** `tests/FFICodeGenTest.cpp`

```cpp
TEST_F(FFICodeGenTest, GenerateExternFunctionCall) {
    std::string code = R"(
        extern cdecl func sqrt(x: double) -> double;
        
        func test() -> double {
            return sqrt(16.0);
        }
    )";
    
    auto ast = parseAndBuildAST(code);
    auto module = codeGen->generateLLVMModule(*ast);
    
    auto* sqrtFunc = module->getFunction("sqrt");
    ASSERT_NE(sqrtFunc, nullptr);
    EXPECT_TRUE(sqrtFunc->isDeclaration());
    EXPECT_EQ(sqrtFunc->getCallingConv(), llvm::CallingConv::C);
}
```

### 12.2 Integration Tests

**Test file:** `tests/FFIIntegrationTest.cpp`

```cpp
TEST_F(FFIIntegrationTest, CallStandardCLibFunction) {
    HoocJIT jit;
    
    std::string code = R"(
        extern cdecl func sqrt(x: double) -> double;
        
        func compute() -> double {
            return sqrt(25.0);
        }
    )";
    
    ASSERT_TRUE(jit.compileHoocCode(code));
    
    auto result = jit.executeFunction<double>("compute");
    EXPECT_DOUBLE_EQ(result, 5.0);
}
```

### 12.3 Example Programs

Create `tests/examples/ffi/` directory:

- `ffi_math.hoo` - Math library usage
- `ffi_stdio.hoo` - Standard I/O usage
- `ffi_custom.hoo` - Custom library usage
- `ffi_opengl.hoo` - OpenGL example
- `ffi_callbacks.hoo` - Callback functions

---

## 13. Performance Considerations

### 13.1 Symbol Caching
- Cache resolved symbols to avoid repeated `dlsym()` calls
- Build symbol table during module loading

### 13.2 Lazy Loading
- Load libraries on-demand (first use)
- Unload unused libraries to free memory

### 13.3 Type Marshaling Overhead
- Minimize marshaling for primitive types (direct mapping)
- Use LLVM optimization passes to eliminate redundant marshaling

### 13.4 JIT Compilation
- Pre-compile frequently used FFI wrappers
- Inline small external functions when possible

---

## 14. Security Considerations

### 14.1 Library Path Validation
- Restrict search paths to prevent arbitrary library loading
- Use whitelist of allowed library paths

### 14.2 Symbol Validation
- Validate FFI declarations against actual library exports
- Detect ABI mismatches at compile time

### 14.3 Sandboxing
- Consider using seccomp/pledge to restrict library loading
- Implement permission system for external library access

---

## 15. Platform-Specific Notes

### 15.1 Windows
- Use `LoadLibrary()` instead of `dlopen()`
- Support both `.dll` and `.lib` files
- Handle stdcall vs cdecl calling conventions
- Search `System32`, `SysWOW64`, and executable directory

### 15.2 macOS
- Use `.dylib` extension
- Search `/usr/lib`, `/usr/local/lib`, and framework directories
- Support framework imports (`@rpath`, `@executable_path`)

### 15.3 Linux
- Use `.so` extension (and version suffixes like `.so.1`)
- Search `/lib`, `/usr/lib`, `/usr/local/lib`
- Respect `LD_LIBRARY_PATH` environment variable

---

## 16. Future Enhancements

### 16.1 Automatic FFI Generation
- Parse C header files (`.h`) to auto-generate `.ffi.hoo`
- Use libclang or CppParser

### 16.2 Hot Reloading
- Support reloading shared libraries without restarting JIT
- Useful for plugin systems

### 16.3 FFI Debugging
- Add FFI call tracing
- Log parameter marshaling for debugging

### 16.4 WebAssembly FFI
- Support WASM imports/exports
- Enable browser-based hoo execution

---

## 17. Documentation Requirements

### 17.1 User Guide
- **"Using External Libraries"** chapter
- FFI syntax reference
- Type marshaling guide
- Platform-specific notes

### 17.2 API Reference
- `hoo_ffi.h` API documentation
- `ModuleResolver` API
- `HoocJIT` FFI methods

### 17.3 Examples
- 10+ working examples
- Standard library usage patterns
- Custom library integration guide

---

## 18. Success Criteria

FFI implementation is complete when:

1. ✅ Can import and call C functions from shared libraries
2. ✅ Type marshaling works for all primitive types
3. ✅ String and array marshaling implemented
4. ✅ Nullable type marshaling works correctly
5. ✅ Cross-platform support (Windows, macOS, Linux)
6. ✅ 50+ passing FFI tests
7. ✅ Documentation complete with examples
8. ✅ Standard library bindings available (libc, libm)
9. ✅ Performance overhead < 10% vs direct C calls
10. ✅ Can run example OpenGL/SDL application

---

## 19. Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| ABI incompatibility | High | Validate calling conventions, test on all platforms |
| Memory safety issues | High | Strict type checking, marshaling validation |
| Symbol name conflicts | Medium | Use namespace prefixes, explicit aliasing |
| Dynamic loading failures | Medium | Fallback to static linking, error handling |
| Performance overhead | Low | Optimization passes, caching, benchmarking |

---

## 20. References

- **LLVM ORC JIT Documentation**: https://llvm.org/docs/ORCv2.html
- **dlopen/dlsym Manual**: `man 3 dlopen`
- **Windows LoadLibrary**: https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/
- **System V ABI**: https://wiki.osdev.org/System_V_ABI
- **C FFI Best Practices**: https://www.rust-lang.org/what/networking

---

## Appendix A: File Checklist

### New Files to Create
- [ ] `src/ast/ExternDeclaration.h` - AST nodes for extern declarations
- [ ] `src/ast/ExternDeclaration.cpp` - Implementation
- [ ] `src/ModuleResolver.h` - Module resolution interface
- [ ] `src/ModuleResolver.cpp` - Implementation
- [ ] `runtime/hoo_ffi.h` - FFI runtime interface
- [ ] `runtime/hoo_ffi.c` - FFI runtime implementation
- [ ] `tests/FFIParsingTest.cpp` - Parsing tests
- [ ] `tests/FFICodeGenTest.cpp` - Code generation tests
- [ ] `tests/FFIIntegrationTest.cpp` - Integration tests
- [ ] `tests/examples/ffi/*.hoo` - Example programs
- [ ] `docs/ffi-user-guide.md` - User documentation

### Files to Modify
- [ ] `src/Hooc.g4` - Add `extern` keyword and rules
- [ ] `src/SimpleASTBuilder.h` - Add extern building methods
- [ ] `src/SimpleASTBuilder.cpp` - Implement extern building
- [ ] `src/LLVMCodeGenerator.h` - Add FFI methods
- [ ] `src/LLVMCodeGenerator.cpp` - Implement FFI code generation
- [ ] `src/HoocJIT.h` - Add FFI methods
- [ ] `src/HoocJIT.cpp` - Implement library loading
- [ ] `CMakeLists.txt` - Link FFI runtime library
- [ ] `README.md` - Document FFI features
- [ ] `CLAUDE.md` - Update with FFI architecture

---

## Conclusion

This implementation plan provides a comprehensive roadmap for adding Foreign Function Interface (FFI) capabilities to the hooc compiler. The phased approach ensures incremental progress with testable milestones at each stage.

**Estimated Total Time:** 12-15 weeks (3-4 months)

**Key Success Metrics:**
- ✅ Can call C functions from hoo code
- ✅ Type-safe marshaling between hoo and C types
- ✅ Cross-platform support (Windows, macOS, Linux)
- ✅ Comprehensive test coverage
- ✅ Production-ready standard library bindings

**Next Steps:**
1. Review and approve this plan
2. Create feature branch: `feature/ffi-implementation`
3. Begin Phase 1: Foundation (grammar + AST)
4. Set up CI/CD for FFI tests
5. Document progress in weekly updates

---

**End of Document**
