#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooCsv - CSV Parsing and Generation
// ============================================================================

/**
 * Parse a CSV string into a 2D array of strings [row][col]
 * @param csv Input CSV string
 * @param out_rows Output row count
 * @param out_cols Output column count
 * @return 2D array of strings (caller must free with hoo_csv_free_table)
 */
char*** hoo_csv_parse(const char* csv, int64_t* out_rows, int64_t* out_cols);

/**
 * Free a table returned by hoo_csv_parse or hoo_csv_parse_with_opts
 * @param table Table to free
 * @param rows Number of rows
 * @param cols Number of columns
 */
void hoo_csv_free_table(char*** table, int64_t rows, int64_t cols);

/**
 * Parse a CSV string with custom delimiter and quote character
 * @param csv Input CSV string
 * @param delimiter Field delimiter character
 * @param quote_char Quote character
 * @param out_rows Output row count
 * @param out_cols Output column count
 * @return 2D array of strings (caller must free with hoo_csv_free_table)
 */
char*** hoo_csv_parse_with_opts(const char* csv, char delimiter, char quote_char,
                                int64_t* out_rows, int64_t* out_cols);

/**
 * Generate a CSV string from headers and data
 * @param headers Array of column header strings (may be NULL)
 * @param data 2D array of row/column data strings
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Allocated CSV string (caller must free with hoo_csv_free_string)
 */
char* hoo_csv_generate(const char** headers, const char*** data,
                       int64_t rows, int64_t cols);

/**
 * Generate a CSV string with custom delimiter and quote character
 * @param headers Array of column header strings (may be NULL)
 * @param data 2D array of row/column data strings
 * @param rows Number of rows
 * @param cols Number of columns
 * @param delimiter Field delimiter character
 * @param quote_char Quote character
 * @return Allocated CSV string (caller must free with hoo_csv_free_string)
 */
char* hoo_csv_generate_with_opts(const char** headers, const char*** data,
                                 int64_t rows, int64_t cols,
                                 char delimiter, char quote_char);

/**
 * Read a CSV file into a 2D array of strings
 * @param path File path
 * @param out_rows Output row count
 * @param out_cols Output column count
 * @return 2D array of strings (caller must free with hoo_csv_free_table)
 */
char*** hoo_csv_read_file(const char* path, int64_t* out_rows, int64_t* out_cols);

/**
 * Write headers and data to a CSV file
 * @param path File path
 * @param headers Array of column header strings (may be NULL)
 * @param data 2D array of row/column data strings
 * @param rows Number of rows
 * @param cols Number of columns
 * @return 0 on success, non-zero on failure
 */
int64_t hoo_csv_write_file(const char* path, const char** headers,
                           const char*** data, int64_t rows, int64_t cols);

/**
 * Check if a character needs escaping in CSV output
 * @param c Character to check
 * @return 1 if the character requires escaping, 0 otherwise
 */
int64_t hoo_csv_escape(char c);

/**
 * Free a string allocated by a hoo_csv_* function
 * @param str String to free
 */
void hoo_csv_free_string(char* str);

#ifdef __cplusplus
}
#endif
