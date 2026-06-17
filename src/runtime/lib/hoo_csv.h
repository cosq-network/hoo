#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooCsv - CSV Parsing and Generation
// ============================================================================

// ── Instance-based OOP API ────────────────────────────────────────────────

void*   hoo_csv_new(void);
void*   hoo_csv_new_with_opts(char delimiter, char quote_char);
void    hoo_csv_release(void* handle);

void*   hoo_csv_parse(void* handle, const char* csv);            // Returns HooArray<HooArray<HooString>>
char*   hoo_csv_generate(void* handle, void* data_arr);          // Returns CSV string (caller frees via hoo_csv_free_string)
void*   hoo_csv_read_file(void* handle, const char* path);       // Returns HooArray<HooArray<HooString>>
int64_t hoo_csv_write_file(void* handle, const char* path, void* data_arr);
int64_t hoo_csv_escape(void* handle, char c);

// ── Low-level C API (used internally, also available for direct use) ────

char*** hoo_csv_parse_raw(const char* csv, int64_t* out_rows, int64_t* out_cols);
char*** hoo_csv_parse_raw_with_opts(const char* csv, char delimiter, char quote_char,
                                    int64_t* out_rows, int64_t* out_cols);
char*   hoo_csv_generate_raw(const char** headers, const char*** data,
                             int64_t rows, int64_t cols);
char*   hoo_csv_generate_raw_with_opts(const char** headers, const char*** data,
                                       int64_t rows, int64_t cols,
                                       char delimiter, char quote_char);
char*** hoo_csv_read_file_raw(const char* path, int64_t* out_rows, int64_t* out_cols);
int64_t hoo_csv_write_file_raw(const char* path, const char** headers,
                               const char*** data, int64_t rows, int64_t cols);
int64_t hoo_csv_escape_raw(char c);

void hoo_csv_free_table(char*** table, int64_t rows, int64_t cols);
void hoo_csv_free_string(char* str);

#ifdef __cplusplus
}
#endif
