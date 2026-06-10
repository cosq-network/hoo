# Character API Reference (`Character`)

The `Character` class provides static methods for Unicode character operations.

## Methods

`Character.fromCodepoint(codepoint: int64) :ptr`
Creates a character from its Unicode code point value. Returns a character handle.

`Character.fromUtf8(bytes: string) :ptr`
Creates a character from a UTF-8 encoded byte sequence. Returns a character handle.

`Character.codepoint(ch: ptr) :int64`
Returns the Unicode code point of the character.

`Character.length(ch: ptr) :int64`
Returns the number of bytes in the character's UTF-8 encoding (1-4).

`Character.data(ch: ptr) :string`
Returns the character's raw UTF-8 byte sequence as a string.

`Character.print(ch: ptr)`
Writes the character to standard output.

`Character.release(ch: ptr)`
Releases the character handle.

## Example

```hoo
var ch = Character.fromCodepoint(65)
var cp = Character.codepoint(ch)  // 65
var len = Character.length(ch)     // 1
Character.release(ch)

ch = Character.fromCodepoint(0x1F600)
len = Character.length(ch)  // 4

var fromCp = Character.fromCodepoint(0x1F431)
Character.print(fromCp)  // 🐱
Character.release(fromCp)
```
