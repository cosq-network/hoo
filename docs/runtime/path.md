# Path Manipulation (`hoo.path`)

The `hoo.path` module provides dirname, basename, extension, join, normalize, absolute/relative resolution, and split operations via `std::filesystem`.

## 1. Component Extraction

- `p.dirname()` — Parent directory path. For `"a/b/c.txt"` returns `"a/b"`.
- `p.basename()` — Last path component. For `"a/b/c.txt"` returns `"c.txt"`.
- `p.extension()` — File extension including dot. For `"archive.tar.gz"` returns `".gz"`.
- `p.stem()` — Filename without extension. For `"a/b/resume.pdf"` returns `"resume"`.
- `p.root()` — Root component. On Unix, `"/"` for absolute paths, `""` for relative. On Windows, `"C:\"` or `"\\server\share\"`.

## 2. Construction

- `Path.join(a, b)` — Join two components with platform separator. Handles edge cases.
- `Path.join_multi(parts, count)` — Join multiple components.

## 3. Normalization

- `p.normalize()` — Resolve `.` and `..` components, collapse redundant separators. Does not resolve symlinks.
- `p.absolute()` — Convert to absolute path relative to current working directory.
- `Path.relative(path, base)` — Compute relative path from `base` to `path`.

## 4. Properties

- `p.is_absolute()` — Returns 1 if absolute.
- `p.is_relative()` — Returns 1 if relative.
- `p.has_extension()` — Returns 1 if path has an extension.
- `p.has_root()` — Returns 1 if path has a root component.

## 5. Split

- `p.split()` — Split path into individual components. Returns array of strings (free with `Path.free_parts`).
- `Path.free_parts(parts, count)` — Free parts array.

## 6. Platform-Specific

- `Path.separator()` — Returns `'/'` on Unix, `'\\'` on Windows.
- `Path.list_separator()` — Returns `':'` on Unix, `';'` on Windows.

## Usage from Hoo Source

All `Path.*` functions are available on the `Path` class:

```hoo
func :int64 demo() {
    var p = Path.new("a/b/c.txt");
    var dir = p.dirname();                          // "a/b"
    var base = p.basename();                        // "c.txt"
    var ext = p.extension();                        // ".txt"
    var stem = p.stem();                            // "resume"
    var joined = Path.join("a", "b");               // "a/b"
    var norm = Path.new("a/b/../c").normalize();    // "a/c"
    var abs = Path.new("/usr/bin").is_absolute();   // 1
    var sep = Path.separator();                     // '/' on Unix
    return string_length(dir);
}
```

## Memory Management

Strings allocated by `Path` functions must be freed with `Path.free_string(str)`. Parts arrays must be freed with `Path.free_parts(parts, count)`.
