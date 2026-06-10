# File System (`hoo.fs`)

The `hoo.fs` module provides file I/O, directory traversal, temporary files, and file metadata queries, wrapping C++17 `<filesystem>`.

## 1. File Operations

- `Fs.exists(path)` — Check if a path exists. Returns 1 if exists, 0 otherwise.
- `Fs.isFile(path)` — Check if path is a regular file.
- `Fs.isDir(path)` — Check if path is a directory.
- `Fs.size(path)` — Get file size in bytes, or -1 on error.
- `Fs.lastModified(path)` — Get last modification time as Unix timestamp (seconds since epoch), or -1 on error.
- `Fs.delete(path)` — Delete a file. Returns 1 on success.
- `Fs.rename(old_path, new_path)` — Rename/move a file or directory.
- `Fs.copy(src, dst)` — Copy a file.

## 2. Read/Write Text Files

- `Fs.readText(path)` — Read entire text file into an allocated string (free with `Fs.freeString`).
- `Fs.writeText(path, content)` — Write string to file, overwriting. Returns 1 on success.
- `Fs.appendText(path, content)` — Append string to file. Creates file if missing.

## 3. Read/Write Binary Files

- `Fs.readBytes(path)` — Read entire binary file into allocated buffer.
- `Fs.writeBytes(path, data, len)` — Write raw bytes to file, overwriting.

## 4. Directory Operations

- `Fs.mkdir(path)` — Create a single directory (parent must exist).
- `Fs.mkdirs(path)` — Create directory tree (mkdir -p).
- `Fs.rmdir(path)` — Remove an empty directory.
- `Fs.listDir(path)` — List directory contents into array of strings (free with `Fs.freeList`).

## 5. Temp Files

- `Fs.tempDir()` — Get system temporary directory path.
- `Fs.createTempFile(prefix)` — Create a temporary file with given prefix.

## Memory Management

Allocated strings from `Fs` functions must be freed with `Fs.freeString(str)`. Directory listings must be freed with `Fs.freeList(list, count)`. Binary buffers are freed via the generic `free_string`.
