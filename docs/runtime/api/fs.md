# Fs — Filesystem Operations

The `Fs` class provides static methods for filesystem operations.

## Methods

`Fs.exists(path: string) :int64`
Returns 1 if the path exists, 0 otherwise.

`Fs.isFile(path: string) :int64`
Returns 1 if the path points to a regular file, 0 otherwise.

`Fs.isDir(path: string) :int64`
Returns 1 if the path points to a directory, 0 otherwise.

`Fs.size(path: string) :int64`
Returns the file size in bytes, or -1 on error.

`Fs.lastModified(path: string) :int64`
Returns the last modified time as a Unix timestamp (seconds since epoch), or -1 on error.

`Fs.delete(path: string) :int64`
Deletes a file. Returns 1 on success, 0 on failure.

`Fs.rename(oldPath: string, newPath: string) :int64`
Renames or moves a file or directory. Returns 1 on success, 0 on failure.

`Fs.copy(src: string, dst: string) :int64`
Copies a file from `src` to `dst`. Returns 1 on success, 0 on failure.

`Fs.readText(path: string) :string`
Reads the entire contents of a text file.

`Fs.writeText(path: string, content: string) :int64`
Writes `content` to a text file, overwriting any existing content. Creates the file if it does not exist. Returns 1 on success, 0 on failure.

`Fs.appendText(path: string, content: string) :int64`
Appends `content` to the end of a text file. Creates the file if it does not exist. Returns 1 on success, 0 on failure.

`Fs.mkdir(path: string) :int64`
Creates a single directory. The parent directory must already exist. Returns 1 on success, 0 on failure.

`Fs.mkdirs(path: string) :int64`
Creates a directory and all missing parent directories (mkdir -p). Returns 1 on success, 0 on failure.

`Fs.rmdir(path: string) :int64`
Removes an empty directory. Returns 1 on success, 0 on failure.

`Fs.writeBytes(path: string, buf: buffer) :int64`
Writes the entire contents of a Buffer to a file, overwriting any existing content. Returns 1 on success, 0 on failure.

`Fs.readBytes(path: string) :buffer`
Reads the entire contents of a file into a new Buffer. Returns null on error.

`Fs.listDir(path: string) :array`
Returns an array of filenames in the given directory.

`Fs.tempDir() :string`
Returns the system's temporary directory path.

`Fs.createTempFile(prefix: string) :string`
Creates a temporary file with the given prefix in the system's temporary directory. Returns the path to the new file.

## Example

```hoo
if Fs.exists("/tmp/data.txt") == 1 {
    let content = Fs.readText("/tmp/data.txt")
    println(content)
}

if Fs.mkdirs("/tmp/a/b/c") == 1 {
    let files = Fs.listDir("/tmp/a/b/c")
    println("Created directory with " + files.len() + " entries")
}

let tmp = Fs.createTempFile("hoo_")
Fs.writeText(tmp, "hello world")
println(Fs.size(tmp))
Fs.delete(tmp)

for file in Fs.listDir(".") {
    if Fs.isFile(file) == 1 {
        println(file + ": " + Fs.size(file))
    }
}
```
