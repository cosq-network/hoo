# ISSUE-013: Missing Unicode and Hex Escape Sequences in String Literals

## 1. Overview
The string literal escape processor in `SimpleASTBuilder` handles `\"`, `\\`, `\/`, `\n`, `\r`, `\t`, `\b`, `\f` but does **not** handle `\uXXXX` (Unicode), `\xXX` (hex byte), `\0` (null), or `\v` (vertical tab). These unknown escapes silently produce corrupted output.

## 2. Technical Analysis
- **Location**: `src/ast/SimpleASTBuilder.cpp` lines 1005-1015
- **Issue**: The escape switch has a `default:` branch that appends the character after the backslash (`inner[i]`) without consuming the escape prefix, producing literal `u` characters from `\u` sequences.

```cpp
// Line ~1005: escape switch
case 'n': result += '\n'; break;
case 't': result += '\t'; break;
// ... other known escapes ...
default:
    result += inner[i]; // Appends 'u' for "\u0041", producing "u0041"
    break;
```

### Affected sequences:
| Input | Expected | Actual |
|-------|----------|--------|
| `"\u0041"` | `"A"` | `"u0041"` |
| `"\x41"` | `"A"` | `"x41"` |
| `"\0"` | null byte | `"0"` |
| `"\v"` | vertical tab | `"v"` |

## 3. Impact
- Data corruption in strings containing Unicode escapes.
- Inability to represent characters outside ASCII in string literals.
- Broken interoperability with JSON, HTML, and other formats requiring `\u` or `\x` escapes.

## 4. Suggested Fix
Add handlers for the missing escape sequences in the `getStringValue()` switch:

```cpp
case 'u': {
    // Parse 4 hex digits after \u
    std::string hex = inner.substr(i + 1, 4);
    char32_t cp = std::stoul(hex, nullptr, 16);
    result += encodeUtf8(cp); // Append UTF-8 encoding
    i += 4;
    break;
}
case 'x': {
    // Parse 2 hex digits after \x
    std::string hex = inner.substr(i + 1, 2);
    char byte = static_cast<char>(std::stoul(hex, nullptr, 16));
    result += byte;
    i += 2;
    break;
}
case '0': result += '\0'; break;
case 'v': result += '\v'; break;
```

## 5. Status
- **Date**: 2026-06-08
- **Status**: **PROPOSED**
- **Priority**: **HIGH**
- **Audit 2026-06-21**: Verified string escape handling still supports only the basic escape set and falls through on `\u`, `\x`, `\0`, and `\v`; char literals handle `\0` separately.
