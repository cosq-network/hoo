# File System (`hoo.fs`)

The `hoo.fs` module provides file I/O, directory traversal, temporary files, and file metadata queries, wrapping C++17 `<filesystem>`.

The module exposes two API layers:
- **C++ API** — object-oriented classes: `hoo::fs::File`, `hoo::fs::Directory`, `hoo::fs::Path`
- **C-ABI bridge** — flat `hoo_fs_*` functions used by the JIT / FFI layer

All Hoo language calls (`Fs.exists`, `Fs.delete`, etc.) resolve through the JIT bridge to the C-ABI layer, which delegates to the C++ classes.

---

## 1. `hoo::fs::File`

Static methods on the `File` class:

| Method | Signature | Description |
|---|---|---|
| `exists` | `(path) -> bool` | Check if a path exists. |
| `isFile` | `(path) -> bool` | Check if path is a regular file. |
| `size` | `(path) -> i64` | File size in bytes; `-1` on error. |
| `lastModified` | `(path) -> i64` | Unix timestamp (seconds since epoch); `-1` on error. |
| `remove` | `(path) -> bool` | Delete a file or empty directory. |
| `rename` | `(old, new) -> bool` | Rename / move a file or directory. |
| `copy` | `(src, dst) -> bool` | Copy a file (overwrites destination). |
| `readText` | `(path) -> string` | Read entire text file. Returns empty string on error. |
| `writeText` | `(path, content) -> bool` | Write string to file, overwriting. |
| `appendText` | `(path, content) -> bool` | Append string to file. Creates file if missing. |
| `readBytes` | `(path, out vector<u8>) -> bool` | Read entire binary file into vector. |
| `writeBytes` | `(path, vector<u8>) -> bool` | Write raw bytes to file, overwriting. |

## 2. `hoo::fs::Directory`

Static methods on the `Directory` class:

| Method | Signature | Description |
|---|---|---|
| `isDirectory` | `(path) -> bool` | Check if path is a directory. |
| `create` | `(path) -> bool` | Create a single directory (parent must exist). |
| `createTree` | `(path) -> bool` | Create directory tree (mkdir -p). |
| `remove` | `(path) -> bool` | Remove an empty directory. |
| `list` | `(path) -> vector<string>` | List directory contents (filenames only, no paths). |

## 3. `hoo::fs::Path`

The `Path` class has been merged from the standalone `hoo.path` module. It now
provides component extraction, construction, normalization, properties, and
platform queries alongside the existing temporary-file utilities.

### Temporary Files

| Method | Signature | Description |
|---|---|---|
| `getTempDir` | `() -> string` | System temporary directory path. Returns empty on error. |
| `createTempFile` | `(prefix) -> string` | Create a temporary file with the given prefix. Returns the full path. |

### Component Extraction

| Method | Signature | Description |
|---|---|---|
| `dirname` | `(path) -> string` | Parent directory. `"a/b/c.txt"` → `"a/b"`. Returns `"."` for bare filenames. |
| `basename` | `(path) -> string` | Last path component. `"a/b/c.txt"` → `"c.txt"`. |
| `extension` | `(path) -> string` | Extension including dot. `"archive.tar.gz"` → `".gz"`. |
| `stem` | `(path) -> string` | Filename without extension. `"resume.pdf"` → `"resume"`. |
| `root` | `(path) -> string` | Root component. `"/foo"` → `"/"`, `"C:\foo"` → `"C:\"`. |

### Construction

| Method | Signature | Description |
|---|---|---|
| `join` | `(a, b) -> string` | Join two components with platform separator. |
| `joinMulti` | `(parts) -> string` | Join multiple components. |

### Normalization & Resolution

| Method | Signature | Description |
|---|---|---|
| `normalize` | `(path) -> string` | Resolve `.`/`..`, collapse redundant separators. |
| `absolute` | `(path) -> string` | Convert to absolute path relative to CWD. |
| `relative` | `(path, base) -> string` | Compute relative path from `base` to `path`. |

### Properties

| Method | Signature | Description |
|---|---|---|
| `isAbsolute` | `(path) -> bool` | Returns true if path is absolute. |
| `isRelative` | `(path) -> bool` | Returns true if path is relative. |
| `hasExtension` | `(path) -> bool` | Returns true if path has an extension. |
| `hasRoot` | `(path) -> bool` | Returns true if path has a root component. |

### Split

| Method | Signature | Description |
|---|---|---|
| `split` | `(path) -> vector<string>` | Split path into individual components. |

### Platform-Specific

| Method | Signature | Description |
|---|---|---|
| `separator` | `() -> char` | `'/'` on Unix, `'\\'` on Windows. |
| `listSeparator` | `() -> char` | `':'` on Unix, `';'` on Windows. |

---

## C-ABI Bridge Functions (JIT / FFI)

These functions are declared `extern "C"` and remain stable for JIT linkage.

| Function | Description |
|---|---|
| `hoo_fs_exists(path)` | Returns 1 if exists, 0 otherwise. |
| `hoo_fs_is_file(path)` | Returns 1 if regular file. |
| `hoo_fs_is_dir(path)` | Returns 1 if directory. |
| `hoo_fs_size(path)` | Returns size in bytes, or -1. |
| `hoo_fs_last_modified(path)` | Returns Unix timestamp, or -1. |
| `hoo_fs_delete(path)` | Deletes file, returns 1 on success. |
| `hoo_fs_rename(old, new)` | Renames file/directory, returns 1 on success. |
| `hoo_fs_copy(src, dst)` | Copies file, returns 1 on success. |
| `hoo_fs_read_text(path)` | Returns malloc'd string (free with `hoo_fs_free_string`), or NULL. |
| `hoo_fs_write_text(path, content)` | Writes string, returns 1 on success. |
| `hoo_fs_append_text(path, content)` | Appends string, returns 1 on success. |
| `hoo_fs_read_bytes(path, &out_data, &out_len)` | Reads binary into malloc'd buffer, returns 1 on success. |
| `hoo_fs_write_bytes(path, data, len)` | Writes binary, returns 1 on success. |
| `hoo_fs_mkdir(path)` | Creates single directory, returns 1 on success. |
| `hoo_fs_mkdirs(path)` | Creates directory tree, returns 1 on success. |
| `hoo_fs_rmdir(path)` | Removes empty directory, returns 1 on success. |
| `hoo_fs_list_dir(path, &count)` | Lists directory into malloc'd array (free with `hoo_fs_free_list`). |
| `hoo_fs_free_list(list, count)` | Frees array returned by `hoo_fs_list_dir`. |
| `hoo_fs_temp_dir()` | Returns malloc'd temp directory path string. |
| `hoo_fs_create_temp_file(prefix)` | Returns malloc'd temp file path string. |
| `hoo_fs_free_string(str)` | Frees a string returned by any `hoo_fs_*` function. |

### Path C-ABI Bridges

These functions (originally from `hoo_path.h`) are now defined in `hoo_fs.cpp`
and delegate to `hoo::fs::Path`. They remain stable for JIT/FFI linkage.

| Function | Description |
|---|---|
| `hoo_path_dirname(path)` | Parent directory (malloc'd, free with `hoo_path_free_string`). |
| `hoo_path_basename(path)` | Last component (malloc'd). |
| `hoo_path_extension(path)` | Extension with dot (malloc'd). |
| `hoo_path_stem(path)` | Filename without extension (malloc'd). |
| `hoo_path_root(path)` | Root component (malloc'd). |
| `hoo_path_join(a, b)` | Join two components (malloc'd). |
| `hoo_path_join_multi(parts, count)` | Join multiple components (malloc'd). |
| `hoo_path_normalize(path)` | Normalize path (malloc'd). |
| `hoo_path_absolute(path)` | Absolute path (malloc'd). |
| `hoo_path_relative(path, base)` | Relative path (malloc'd). |
| `hoo_path_is_absolute(path)` | Returns 1 if absolute. |
| `hoo_path_is_relative(path)` | Returns 1 if relative. |
| `hoo_path_has_extension(path)` | Returns 1 if has extension. |
| `hoo_path_has_root(path)` | Returns 1 if has root component. |
| `hoo_path_split(path, &count)` | Split into malloc'd array (free with `hoo_path_free_parts`). |
| `hoo_path_free_parts(parts, count)` | Free parts array. |
| `hoo_path_separator()` | Returns platform path separator character. |
| `hoo_path_list_separator()` | Returns platform list separator character. |
| `hoo_path_free_string(str)` | Frees a string returned by any `hoo_path_*` function. |

## Hoo Language API

Hoo code accesses FS operations through the `Fs` module class:

```
import hoo.fs

val ok = Fs.exists("myfile.txt")
val text = Fs.readText("myfile.txt")
val size = Fs.size("myfile.txt")
```

These calls resolve via the JIT bridge to the C-ABI functions above.

## Memory Management

- Strings returned from `hoo_fs_read_text`, `hoo_fs_temp_dir`, `hoo_fs_create_temp_file` must be freed with `hoo_fs_free_string`.
- Directory listings from `hoo_fs_list_dir` must be freed with `hoo_fs_free_list(list, count)`.
- The C++ API (`File::readText`, `Path::getTempDir`, etc.) returns `std::string` / `std::vector`, which manages its own memory.
