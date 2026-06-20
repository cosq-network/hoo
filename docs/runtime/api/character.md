# Character API Reference (`Character`)

**Import Requirement:**
```hoo
import hoo.character;
```

The `Character` class provides instance methods for Unicode character operations. Create a character with `new Character(codepoint)`, then call methods on the instance.

## Constructor

### `new Character(codepoint: int64) :Character`
Creates a character from its Unicode code point value. Returns a character handle.

## Free Functions

### `character_from_utf8(string: string) :Character`
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
var ch = new Character(65)
var cp = ch.codepoint()  // 65
var len = ch.length()    // 1
ch.release()

ch = new Character(0x1F600)
len = ch.length()  // 4

var ch2 = new Character(0x1F431)
ch2.print()  // 🐱
ch2.release()
```
