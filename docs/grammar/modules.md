# Module System

Hoo uses a Python-style module system for code organization and namespace management.

## 1. Compilation Units
A compilation unit is a single source file (`.hoo`). It consists of optional import statements followed by declarations (functions, classes, variables, constants, or FFI).

## 2. Imports

Hoo supports two main types of imports:

### Basic Import
Imports an entire module.
- `import math.utils;`
- `import hoo.io as io;` (with alias)

### From Import
Imports specific items from a module.
- `from math.utils import add;`
- `from hoo.io import println, readchar;`
- `from math.utils import add as plus;` (with alias)

## 3. Module Paths
Module paths are dot-separated identifiers representing the directory structure.
- `app.core.auth` maps to `app/core/auth.hoo` (or `.ho` in the JIT).

## 4. Qualified Identifiers
To access symbols from imported modules, use the dot notation or the assigned alias.
- `math.utils.add(1, 2)`
- `io.println("Hello")`

## 5. Reserved Module Init
The `__hoo_init` keyword is reserved for module-level initialization logic. This function is automatically called by the JIT/Runtime when the module is loaded.
