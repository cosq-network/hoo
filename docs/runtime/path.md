# Path Manipulation (`hoo.path`)

The `hoo.path` module provides dirname, basename, extension, join, normalize, absolute/relative resolution, and split operations via `std::filesystem`.

> **Implementation note**: The Path module has been merged into the File System module (`hoo.fs`). The C++ class `hoo::fs::Path` in `src/runtime/lib/hoo_fs.h` provides instance methods (construct with `Path(path)`), while free functions handle operations without a natural instance (`join`, `relative`, `separator`, etc.). The C-ABI bridge functions (`hoo_path_*`) are defined in `src/runtime/lib/hoo_fs.cpp` and create temporary `Path` objects. The `hoo_path.h` header is retained as a thin compatibility shim that includes `hoo_fs.h`. This change is transparent to Hoo source code — all `Path.*` calls resolve identically.

## 1. Component Extraction (C++ instance methods)

- `Path(path).str()` — Returns the original path string the instance was constructed with.
- `Path(path).dirname()` — Parent directory path. For `"a/b/c.txt"` returns `"a/b"`.
- `Path(path).basename()` — Last path component. For `"a/b/c.txt"` returns `"c.txt"`.
- `Path(path).extension()` — File extension including dot. For `"archive.tar.gz"` returns `".gz"`.
- `Path(path).stem()` — Filename without extension. For `"a/b/resume.pdf"` returns `"resume"`.
- `Path(path).root()` — Root component. On Unix, `"/"` for absolute paths, `""` for relative. On Windows, `"C:\"` or `"\\server\share\"`.

## 2. Free Functions (no instance)

- `hoo::fs::join(a, b)` — Join two components with platform separator.
- `hoo::fs::joinMulti(parts)` — Join multiple components.
- `hoo::fs::relative(path, base)` — Compute relative path from `base` to `path`.

## 3. Normalization (C++ instance methods)

- `Path(path).normalized()` — Resolve `.` and `..` components, collapse redundant separators. Returns a new `Path` (call `.str()` to get the result string).
- `Path(path).absolute()` — Convert to absolute path relative to current working directory. Returns a new `Path` (call `.str()` to get the result string).

## 4. Properties (C++ instance methods)

- `Path(path).isAbsolute()` — Returns `true` if absolute.
- `Path(path).isRelative()` — Returns `true` if relative.
- `Path(path).hasExtension()` — Returns `true` if path has an extension.
- `Path(path).hasRoot()` — Returns `true` if path has a root component.

## 5. Split (C++ instance method)

- `Path(path).split()` — Split path into individual components. Returns vector of strings.

## 6. Platform-Specific (free functions)

- `hoo::fs::separator()` — Returns `'/'` on Unix, `'\\'` on Windows.
- `hoo::fs::listSeparator()` — Returns `':'` on Unix, `';'` on Windows.

## Usage from Hoo Source

All `Path.*` functions are available on the `Path` class:

```hoo
func :int64 demo() {
    var dir = Path.dirname("a/b/c.txt");            // "a/b"
    var base = Path.basename("a/b/c.txt");          // "c.txt"
    var ext = Path.extension("file.txt");           // ".txt"
    var joined = Path.join("a", "b");               // "a/b"
    var norm = Path.normalize("a/b/../c");          // "a/c"
    var abs = Path.isAbsolute("/usr/bin");          // true
    var sep = Path.separator();                     // '/' on Unix
    return string_length(dir);
}
```
