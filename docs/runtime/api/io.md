# I/O — Console Input and Output

These global functions provide basic console I/O and are available without a class prefix.

## Functions

`print(str: string)`
Prints the string to stdout.

`println(str: string)`
Prints the string followed by a newline to stdout.

`readline() :string`
Reads a line of text from stdin. Returns an empty string if EOF is reached immediately.

`readchar() :int64`
Reads a single character from stdin. Returns the character value (0-255), or -1 on EOF.

## Example

```hoo
print("Enter your name: ")
let name = readline()
println("Hello, " + name)

print("Press any key...")
let ch = readchar()
if ch != -1 {
    println("You pressed: " + ch)
}
```
