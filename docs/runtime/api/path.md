# Path — Path Manipulation

The `Path` class provides static methods for manipulating filesystem paths.

## Methods

`Path.basename(p: string) :string`
Returns the last component of the path. For `"a/b/c.txt"` returns `"c.txt"`.

`Path.dirname(p: string) :string`
Returns the parent directory portion of the path. For `"a/b/c.txt"` returns `"a/b"`.

`Path.extension(p: string) :string`
Returns the file extension including the leading dot. For `"archive.tar.gz"` returns `".gz"`. Returns an empty string if there is no extension.

`Path.filename(p: string) :string`
Returns the filename without its extension. For `"a/b/resume.pdf"` returns `"resume"`.

`Path.join(parts: array) :string`
Joins path components using the platform path separator.

`Path.absolute(p: string) :string`
Resolves a path to an absolute path by expanding relative paths against the current working directory.

`Path.separator() :string`
Returns the platform path separator: `"/"` on Unix, `"\"` on Windows.

`Path.is_absolute(p: string) :int64`
Returns 1 if the path is absolute, 0 otherwise.

`Path.normalize(p: string) :string`
Normalizes a path by collapsing `".."` and `"."` components and redundant separators.

## Example

```hoo
let p = "/home/user/docs/../file.txt"

println(Path.basename(p))       // file.txt
println(Path.dirname(p))        // /home/user/docs/..
println(Path.extension(p))      // .txt
println(Path.filename(p))       // file
println(Path.normalize(p))      // /home/user/file.txt
println(Path.is_absolute(p))     // 1
println(Path.separator())       // /

let parts = ["usr", "local", "bin"]
println(Path.join(parts))       // usr/local/bin

let rel = "doc/readme.md"
println(Path.absolute(rel))     // /current/working/dir/doc/readme.md
```
