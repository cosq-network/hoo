#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooCsv - CSV Parsing and Generation
// ============================================================================

typedef void* HooCsv;
typedef void* HooArray;
typedef void* HooString;
typedef void* HooMap;

// ── Instance-based OOP API ────────────────────────────────────────────────

HooCsv    hoo_csv_new(void);
HooCsv    hoo_csv_new_with_opts(int32_t delimiter, int32_t quote_char);
HooCsv    hoo_csv_retain(HooCsv csv);
void      hoo_csv_release(HooCsv csv);
int64_t   hoo_csv_refcount(HooCsv csv);

HooArray  hoo_csv_parse(HooCsv csv, const char* csv_str);        // Returns HooArray<HooArray<HooString>>
HooString hoo_csv_generate(HooCsv csv, void* data_arr);          // Returns HooString
HooArray  hoo_csv_read_file(HooCsv csv, const char* path);       // Returns HooArray<HooArray<HooString>>
int64_t   hoo_csv_write_file(HooCsv csv, const char* path, void* data_arr);
int64_t   hoo_csv_escape(HooCsv csv, int32_t c);

// ── Map-based API (first row used as headers, returns HooArray<HooMap>) ─

HooArray  hoo_csv_parse_as_maps(HooCsv csv, const char* csv_str);       // Returns HooArray<HooMap>
HooArray  hoo_csv_read_file_as_maps(HooCsv csv, const char* path);      // Returns HooArray<HooMap>

// ── Aggregation (all operate on HooArray<HooMap> data) ──────────────────

int64_t   hoo_csv_count(HooCsv csv, void* data, const char* column);    // Non-null value count
int64_t   hoo_csv_sum(HooCsv csv, void* data, const char* column);      // Numeric sum (int64)
HooString hoo_csv_avg(HooCsv csv, void* data, const char* column);      // Numeric average (HooString)
HooString hoo_csv_min(HooCsv csv, void* data, const char* column);      // Lexicographic minimum
HooString hoo_csv_max(HooCsv csv, void* data, const char* column);      // Lexicographic maximum

// ── Transformations (all operate on HooArray<HooMap> data) ─────────────

HooArray  hoo_csv_select(HooCsv csv, void* data, void* columns);        // Select columns, returns HooArray<HooMap>
HooArray  hoo_csv_filter(HooCsv csv, void* data, const char* column,    // Filter rows by condition
                          const char* op, const char* value);
HooArray  hoo_csv_sort(HooCsv csv, void* data, const char* column,      // Sort rows by column
                        int64_t ascending);

// ── Statistics ──────────────────────────────────────────────────────────

HooMap    hoo_csv_describe(HooCsv csv, void* data, const char* column); // Returns HooMap with count,sum,avg,min,max

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
