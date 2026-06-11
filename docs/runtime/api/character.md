# Character API Reference (`Character`)

The `Character` class provides instance methods for Unicode character operations. Create a character via the `Character.new()` factory, then call methods on the instance.

## Factory Methods

`Character.new(codepoint: int64) :ptr`
Creates a character from its Unicode code point value. Returns a character handle.

`Character.fromUtf8(string: string) :ptr`
Creates a character from a UTF-8 encoded string. The string must contain exactly one Unicode scalar value.

## Instance Methods

`codepoint() :int64`
Returns the Unicode code point of the character.

`length() :int64`
Returns the number of bytes in the character's UTF-8 encoding (1-4).

`data() :string`
Returns the character's raw UTF-8 byte sequence as a string.

`print()`
Writes the character to standard output.

`release()`
Releases the character handle.

## Example

```hoo
var ch = Character.new(65)
var cp = ch.codepoint()  // 65
var len = ch.length()    // 1
ch.release()

ch = Character.new(0x1F600)
len = ch.length()  // 4

var ch2 = Character.new(0x1F431)
ch2.print()  // 🐱
ch2.release()
```
