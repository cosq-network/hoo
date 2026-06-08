# Regular Expressions (`hoo.regex`)

The `hoo.regex` module provides compile, match, search, find-all, replace, split, and capture groups via C++ `<regex>` with opaque handles and reference counting.

## 1. Compilation

- `Regex.compile(pattern)` — Compile ECMAScript regex pattern. Returns opaque `HooRegex` handle, or NULL on failure.
- `Regex.compile(pattern, flags)` — Compile with flags: `'i'` (case-insensitive), `'m'` (multiline).
- `Regex.error()` — Get last error message string (thread-local), or NULL if no error.

## 2. Matching & Searching

- `re.match(str)` — Full string match. Returns 1 if match, 0 if not, -1 on error.
- `re.search(str)` — Partial string search. Returns 1 if found, 0 if not, -1 on error.
- `re.find(str)` — Find first match, returns allocated string or NULL.
- `re.find_all(str)` — Find all matches. Returns array of strings (free with `Regex.free_matches`).

## 3. Replace & Split

- `re.replace(str, replacement)` — Replace all matches with replacement string.
- `re.split(str)` — Split string by regex matches. Returns array of strings (free with `Regex.free_matches`).

## 4. Capture Groups

- `re.group(str, group_index)` — Get capture group content from first match. Returns allocated string or NULL if group not found.

## 5. Reference Counting

The regex handle is an opaque pointer with manual reference counting:

- `re.retain()` — Increment reference count.
- `re.release()` — Decrement reference count; frees when reaching zero.

## Memory Management

- `Regex.free_matches(matches, count)` — Free matches array.
- `Regex.free_string(str)` — Free allocated string.
