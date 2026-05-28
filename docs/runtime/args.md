# Args (`hoo.args`)

The `hoo.args` module provides CLI argument parsing for `--key=value`, `--flag`, `-k`, and positional args. Returns a struct result.

## 1. Data Structures

```c
typedef struct {
    const char* key;
    const char* value;
    int64_t index;
} HooArg;

typedef struct {
    HooArg* args;
    int64_t count;
} HooArgsResult;
```

## 2. Parsing

- `hoo_args_parse(argc, argv)` — Parse command-line arguments. Returns a `HooArgsResult` struct. Free with `hoo_args_free`.

## 3. Querying

- `hoo_args_get(result, key)` — Get value for named argument. Returns NULL if not found.
- `hoo_args_has(result, key)` — Returns 1 if flag/option is present.
- `hoo_args_count(result)` — Total parsed argument count.
- `hoo_args_positional(result, index)` — Get positional argument by index. Returns NULL if out of range.

## Usage from Hoo Source

All `args_` functions are available with the `args_` prefix. Note that `args_parse` requires `(argc, argv)` from the host, so args parsing cannot be driven entirely from within HVM bytecode — the `HooArgsResult` handle must be created on the C/C++ side first.

```hoo
func :int64 demo() {
    return 42;
}
```

## Memory Management

The result struct and its strings must be freed with `hoo_args_free(result)`.
