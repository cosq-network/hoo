# ISSUE-056: CSV Methods Referenced in Codegen But Missing From Runtime

## 1. Overview
The code generator's type inference system recognizes several CSV methods — `sort`, `filter`, `select`, `describe`, `avg`, `min`, `max`, `sum` — and assigns return types for them, but the runtime library does not export corresponding implementations. This will cause linker errors at runtime when these methods are called.

## 2. Technical Analysis
- **Codegen reference**: `src/codegen/HVMCodeGenerator.cpp:3994-4002`
  ```cpp
  if (member == "parse" || member == "readFile" || member == "parseAsMaps" || member == "readFileAsMaps" || member == "select" || member == "filter" || member == "sort")
  ```
  Further down, `avg`, `min`, `max`, `sum` return type inference is also present.
- **Runtime header**: `src/runtime/lib/hoo_csv.h` — no implementations for `sort`, `filter`, `select`, `describe`, `avg`, `min`, `max`, `sum`.

## 3. Impact
- Any Hoo code calling `csv.sort()`, `csv.filter()`, `csv.select()` etc. will compile but fail at link time or runtime with unresolved symbols.
- The type system claims these methods exist (return type inference), but the runtime cannot fulfill the contract.

## 4. Suggested Fix
1. Add runtime implementations for each recognized CSV method:
   - `hoo_csv_sort(csv)` — returns sorted copy
   - `hoo_csv_filter(csv, column, predicate)` — returns filtered copy
   - `hoo_csv_select(csv, columns)` — returns subset of columns
   - `hoo_csv_avg(csv, column)` / `hoo_csv_min` / `hoo_csv_max` / `hoo_csv_sum` — aggregation
2. Alternatively, remove the unreferenced return-type inference entries from codegen until the runtime catches up.

## 5. Status
- **Date**: 2026-06-23
- **Status**: **PROPOSED**
- **Priority**: **MEDIUM**
