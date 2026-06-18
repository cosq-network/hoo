# Path Manipulation (`hoo.path`)

The `hoo.path` module provides dirname, basename, extension, join, normalize, absolute/relative resolution, and split operations via `std::filesystem`.

> **Implementation note**: The Path module has been merged into the File System module (`hoo.fs`). The C++ class `hoo::fs::Path` in `src/runtime/lib/hoo_fs.h` provides all methods, while the C-ABI bridge functions (`hoo_path_*`) are defined in `src/runtime/lib/hoo_fs.cpp`. The `hoo_path.h` header is retained as a thin compatibility shim that includes `hoo_fs.h`. This change is transparent to Hoo source code — all `Path.*` calls resolve identically.

## 1. Component Extraction

- `Path.dirname(path)` — Parent directory path. For `"a/b/c.txt"` returns `"a/b"`.
- `Path.basename(path)` — Last path component. For `"a/b/c.txt"` returns `"c.txt"`.
- `Path.extension(path)` — File extension including dot. For `"archive.tar.gz"` returns `".gz"`.
- `Path.stem(path)` — Filename without extension. For `"a/b/resume.pdf"` returns `"resume"`.
- `Path.root(path)` — Root component. On Unix, `"/"` for absolute paths, `""` for relative. On Windows, `"C:\"` or `"\\server\share\"`.

## 2. Construction

- `Path.join(a, b)` — Join two components with platform separator. Handles edge cases.
- `Path.joinMulti(parts, count)` — Join multiple components.

## 3. Normalization

- `Path.normalize(path)` — Resolve `.` and `..` components, collapse redundant separators. Does not resolve symlinks.
- `Path.absolute(path)` — Convert to absolute path relative to current working directory.
- `Path.relative(path, base)` — Compute relative path from `base` to `path`.

## 4. Properties

- `Path.isAbsolute(path)` — Returns `true` if absolute.
- `Path.isRelative(path)` — Returns `true` if relative.
- `Path.hasExtension(path)` — Returns `true` if path has an extension.
- `Path.hasRoot(path)` — Returns `true` if path has a root component.

## 5. Split

- `Path.split(path)` — Split path into individual components. Returns array of strings.

## 6. Platform-Specific

- `Path.separator()` — Returns `'/'` on Unix, `'\\'` on Windows.
- `Path.listSeparator()` — Returns `':'` on Unix, `';'` on Windows.

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
