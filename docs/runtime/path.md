# Path Manipulation (`hoo.path`)

The `hoo.path` module provides dirname, basename, extension, join, normalize, absolute/relative resolution, and split operations via `std::filesystem`.

## 1. Component Extraction

- `hoo_path_dirname(path)` — Parent directory path. For `"a/b/c.txt"` returns `"a/b"`.
- `hoo_path_basename(path)` — Last path component. For `"a/b/c.txt"` returns `"c.txt"`.
- `hoo_path_extension(path)` — File extension including dot. For `"archive.tar.gz"` returns `".gz"`.
- `hoo_path_stem(path)` — Filename without extension. For `"a/b/resume.pdf"` returns `"resume"`.
- `hoo_path_root(path)` — Root component. On Unix, `"/"` for absolute paths, `""` for relative. On Windows, `"C:\"` or `"\\server\share\"`.

## 2. Construction

- `hoo_path_join(a, b)` — Join two components with platform separator. Handles edge cases.
- `hoo_path_join_multi(parts, count)` — Join multiple components.

## 3. Normalization

- `hoo_path_normalize(path)` — Resolve `.` and `..` components, collapse redundant separators. Does not resolve symlinks.
- `hoo_path_absolute(path)` — Convert to absolute path relative to current working directory.
- `hoo_path_relative(path, base)` — Compute relative path from `base` to `path`.

## 4. Properties

- `hoo_path_is_absolute(path)` — Returns 1 if absolute.
- `hoo_path_is_relative(path)` — Returns 1 if relative.
- `hoo_path_has_extension(path)` — Returns 1 if path has an extension.
- `hoo_path_has_root(path)` — Returns 1 if path has a root component.

## 5. Split

- `hoo_path_split(path, &out_count)` — Split path into individual components. Returns array of strings (free with `hoo_path_free_parts`).
- `hoo_path_free_parts(parts, count)` — Free parts array.

## 6. Platform-Specific

- `hoo_path_separator()` — Returns `'/'` on Unix, `'\\'` on Windows.
- `hoo_path_list_separator()` — Returns `':'` on Unix, `';'` on Windows.

## Memory Management

Strings allocated by `hoo.path` functions must be freed with `hoo_path_free_string(str)`. Parts arrays must be freed with `hoo_path_free_parts(parts, count)`.
