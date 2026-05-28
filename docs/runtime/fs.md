# File System (`hoo.fs`)

The `hoo.fs` module provides file I/O, directory traversal, temporary files, and file metadata queries, wrapping C++17 `<filesystem>`.

## 1. File Operations

- `hoo_fs_exists(path)` — Check if a path exists. Returns 1 if exists, 0 otherwise.
- `hoo_fs_is_file(path)` — Check if path is a regular file.
- `hoo_fs_is_dir(path)` — Check if path is a directory.
- `hoo_fs_size(path)` — Get file size in bytes, or -1 on error.
- `hoo_fs_last_modified(path)` — Get last modification time as Unix timestamp (seconds since epoch), or -1 on error.
- `hoo_fs_delete(path)` — Delete a file. Returns 1 on success.
- `hoo_fs_rename(old_path, new_path)` — Rename/move a file or directory.
- `hoo_fs_copy(src, dst)` — Copy a file.

## 2. Read/Write Text Files

- `hoo_fs_read_text(path)` — Read entire text file into an allocated string (free with `hoo_fs_free_string`).
- `hoo_fs_write_text(path, content)` — Write string to file, overwriting. Returns 1 on success.
- `hoo_fs_append_text(path, content)` — Append string to file. Creates file if missing.

## 3. Read/Write Binary Files

- `hoo_fs_read_bytes(path, &out_data, &out_len)` — Read entire binary file into allocated buffer.
- `hoo_fs_write_bytes(path, data, len)` — Write raw bytes to file, overwriting.

## 4. Directory Operations

- `hoo_fs_mkdir(path)` — Create a single directory (parent must exist).
- `hoo_fs_mkdirs(path)` — Create directory tree (mkdir -p).
- `hoo_fs_rmdir(path)` — Remove an empty directory.
- `hoo_fs_list_dir(path, &out_count)` — List directory contents into array of strings (free with `hoo_fs_free_list`).

## 5. Temp Files

- `hoo_fs_temp_dir()` — Get system temporary directory path.
- `hoo_fs_create_temp_file(prefix)` — Create a temporary file with given prefix.

## Memory Management

Allocated strings from `hoo.fs` functions must be freed with `hoo_fs_free_string(str)`. Directory listings must be freed with `hoo_fs_free_list(list, count)`. Binary buffers are freed via the generic `hoo_free_string`.
