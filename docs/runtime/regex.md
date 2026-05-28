# Regular Expressions (`hoo.regex`)

The `hoo.regex` module provides compile, match, search, find-all, replace, split, and capture groups via C++ `<regex>` with opaque handles and reference counting.

## 1. Compilation

- `hoo_regex_compile(pattern)` — Compile ECMAScript regex pattern. Returns opaque `HooRegex` handle, or NULL on failure.
- `hoo_regex_compile_with_flags(pattern, flags)` — Compile with flags: `'i'` (case-insensitive), `'m'` (multiline).
- `hoo_regex_error()` — Get last error message string (thread-local), or NULL if no error.

## 2. Matching & Searching

- `hoo_regex_match(re, str)` — Full string match. Returns 1 if match, 0 if not, -1 on error.
- `hoo_regex_search(re, str)` — Partial string search. Returns 1 if found, 0 if not, -1 on error.
- `hoo_regex_find(re, str)` — Find first match, returns allocated string or NULL.
- `hoo_regex_find_all(re, str, &out_matches, &out_count)` — Find all matches. Returns 0 on success, -1 on error. Free with `hoo_regex_free_matches`.

## 3. Replace & Split

- `hoo_regex_replace(re, str, replacement)` — Replace all matches with replacement string.
- `hoo_regex_split(re, str, &out_count)` — Split string by regex matches. Returns array of strings (free with `hoo_regex_free_matches`).

## 4. Capture Groups

- `hoo_regex_group(re, str, group_index)` — Get capture group content from first match. Returns allocated string or NULL if group not found.

## 5. Reference Counting

The regex handle is an opaque pointer with manual reference counting:

- `hoo_regex_retain(re)` — Increment reference count.
- `hoo_regex_release(re)` — Decrement reference count; frees when reaching zero.

## Memory Management

- `hoo_regex_free_matches(matches, count)` — Free matches array.
- `hoo_regex_free_string(str)` — Free allocated string.
