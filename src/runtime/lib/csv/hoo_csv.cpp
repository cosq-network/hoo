#include "runtime/lib/csv/hoo_csv.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/generic_array/hoo_generic_array.h"
#include "runtime/lib/fs/hoo_fs.h"
#include "runtime/lib/map/hoo_map.h"
#include "runtime/lib/exception/hoo_exception.h"
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <limits>

#ifdef _MSC_VER
#define hoo_strdup _strdup
#else
#define hoo_strdup strdup
#endif

// ============================================================================
// Helpers
// ============================================================================

static std::string trim_ws(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t'))
        start++;
    if (start == s.size())
        return "";
    size_t end = s.size() - 1;
    while (end > start && (s[end] == ' ' || s[end] == '\t'))
        end--;
    return s.substr(start, end - start + 1);
}

static bool field_needs_quoting(const std::string& field, char delimiter, char quote_char)
{
    for (size_t i = 0; i < field.size(); i++) {
        char c = field[i];
        if (c == delimiter || c == quote_char || c == '\n' || c == '\r')
            return true;
    }
    return false;
}

static bool valid_csv_options(char delimiter, char quote_char)
{
    return delimiter != '\0' && quote_char != '\0' && delimiter != quote_char;
}

static void append_quoted(std::ostringstream& ss, const std::string& field, char quote_char)
{
    ss << quote_char;
    for (size_t i = 0; i < field.size(); i++) {
        char c = field[i];
        if (c == quote_char)
            ss << quote_char << quote_char;
        else
            ss << c;
    }
    ss << quote_char;
}

// ============================================================================
// CsvHandle
// ============================================================================

struct CsvHandle {
    char delimiter;
    char quote_char;
};

extern "C" {

HooCsv hoo_csv_new(void) {
    CsvHandle* h = (CsvHandle*)hoo_alloc(sizeof(CsvHandle), HOO_TYPE_CSV);
    if (h) {
        h->delimiter = ',';
        h->quote_char = '"';
    }
    return (HooCsv)h;
}

HooCsv hoo_csv_new_with_opts(int32_t delimiter, int32_t quote_char) {
    if (delimiter <= 0 || delimiter > 255 || quote_char <= 0 || quote_char > 255 ||
        delimiter == quote_char)
        return NULL;
    CsvHandle* h = (CsvHandle*)hoo_alloc(sizeof(CsvHandle), HOO_TYPE_CSV);
    if (h) {
        h->delimiter = (char)delimiter;
        h->quote_char = (char)quote_char;
    }
    return (HooCsv)h;
}

HooCsv hoo_csv_from_opts(int32_t delimiter, int32_t quote_char) {
    return hoo_csv_new_with_opts(delimiter, quote_char);
}

HooCsv hoo_csv_retain(HooCsv csv) {
    return (HooCsv)hoo_retain(csv);
}

void hoo_csv_release(HooCsv csv) {
    if (!csv) return;
    hoo_release(csv);
}

int64_t hoo_csv_refcount(HooCsv csv) {
    return hoo_get_refcount(csv);
}

// ============================================================================
// Low-level parsing and generation (raw C API)
// ============================================================================

static CsvHandle* get_handle(HooCsv csv) {
    return csv ? (CsvHandle*)csv : NULL;
}

char*** hoo_csv_parse_raw(const char* csv, int64_t* out_rows, int64_t* out_cols)
{
    return hoo_csv_parse_raw_with_opts(csv, ',', '"', out_rows, out_cols);
}

char*** hoo_csv_parse_raw_with_opts(const char* csv, char delimiter, char quote_char,
                                    int64_t* out_rows, int64_t* out_cols)
{
    if (!out_rows || !out_cols)
        return NULL;
    *out_rows = 0;
    *out_cols = 0;

    if (!valid_csv_options(delimiter, quote_char) || !csv || !*csv)
        return NULL;

    try {
        std::vector<std::vector<std::string>> rows;
        std::vector<std::string> current_row;
        size_t pos = 0;
        size_t len = std::strlen(csv);

        while (pos < len) {
            while (pos < len && (csv[pos] == ' ' || csv[pos] == '\t'))
                pos++;

            if (pos >= len) {
                current_row.push_back("");
                rows.push_back(current_row);
                break;
            }

            if (csv[pos] == '\r' || csv[pos] == '\n') {
                current_row.push_back("");
                if (csv[pos] == '\r') {
                    pos++;
                    if (pos < len && csv[pos] == '\n') pos++;
                } else {
                    pos++;
                    if (pos < len && csv[pos] == '\r') pos++;
                }
                rows.push_back(current_row);
                current_row.clear();
                continue;
            }

            std::string field;

            if (csv[pos] == quote_char) {
                pos++;
                bool closed = false;
                while (pos < len) {
                    if (csv[pos] == quote_char) {
                        if (pos + 1 < len && csv[pos + 1] == quote_char) {
                            field += quote_char;
                            pos += 2;
                        } else {
                            pos++;
                            closed = true;
                            break;
                        }
                    } else {
                        field += csv[pos];
                        pos++;
                    }
                }
                if (!closed) {
                    *out_rows = 0;
                    *out_cols = 0;
                    return NULL;
                }
                current_row.push_back(field);

                while (pos < len && (csv[pos] == ' ' || csv[pos] == '\t'))
                    pos++;

                // A quoted field may only be followed by a delimiter or a
                // record separator. Reject trailing non-whitespace content
                // instead of silently treating it as a new row.
                if (pos < len && csv[pos] != delimiter &&
                    csv[pos] != '\n' && csv[pos] != '\r') {
                    *out_rows = 0;
                    *out_cols = 0;
                    return NULL;
                }
            } else {
                size_t start = pos;
                while (pos < len && csv[pos] != delimiter
                       && csv[pos] != '\n' && csv[pos] != '\r')
                {
                    pos++;
                }
                field = trim_ws(std::string(csv + start, pos - start));
                current_row.push_back(field);
            }

            if (pos >= len) {
                rows.push_back(current_row);
                break;
            }

            if (csv[pos] == delimiter) {
                pos++;
                if (pos >= len) {
                    current_row.push_back("");
                    rows.push_back(current_row);
                    current_row.clear();
                }
                continue;
            }

            if (csv[pos] == '\r') {
                pos++;
                if (pos < len && csv[pos] == '\n') pos++;
            } else {
                pos++;
                if (pos < len && csv[pos] == '\r') pos++;
            }
            rows.push_back(current_row);
            current_row.clear();
        }

        int64_t nrows = (int64_t)rows.size();
        int64_t ncols = 0;
        for (int64_t i = 0; i < nrows; i++) {
            int64_t rc = (int64_t)rows[i].size();
            if (rc > ncols) ncols = rc;
        }

        if (nrows == 0) {
            *out_rows = 0;
            *out_cols = 0;
            return NULL;
        }

        char*** table = (char***)std::malloc(sizeof(char**) * (size_t)nrows);
        if (!table) return NULL;

        for (int64_t i = 0; i < nrows; i++) {
            table[i] = (char**)std::malloc(sizeof(char*) * (size_t)ncols);
            if (!table[i]) {
                for (int64_t k = 0; k < i; k++) {
                    for (int64_t j = 0; j < ncols; j++)
                        std::free(table[k][j]);
                    std::free(table[k]);
                }
                std::free(table);
                return NULL;
            }
            for (int64_t j = 0; j < ncols; j++) {
                if (j < (int64_t)rows[i].size())
                    table[i][j] = hoo_strdup(rows[i][j].c_str());
                else
                    table[i][j] = hoo_strdup("");
            }
        }

        *out_rows = nrows;
        *out_cols = ncols;
        return table;

    } catch (...) {
        *out_rows = 0;
        *out_cols = 0;
        return NULL;
    }
}

char* hoo_csv_generate_raw(const char** headers, const char*** data,
                           int64_t rows, int64_t cols)
{
    return hoo_csv_generate_raw_with_opts(headers, data, rows, cols, ',', '"');
}

char* hoo_csv_generate_raw_with_opts(const char** headers, const char*** data,
                                     int64_t rows, int64_t cols,
                                     char delimiter, char quote_char)
{
    if (!valid_csv_options(delimiter, quote_char) || rows < 0 || cols < 0)
        return NULL;
    try {
        std::ostringstream ss;

        if (headers) {
            for (int64_t j = 0; j < cols; j++) {
                if (j > 0) ss << delimiter;
                std::string field = headers[j] ? headers[j] : "";
                if (field_needs_quoting(field, delimiter, quote_char))
                    append_quoted(ss, field, quote_char);
                else
                    ss << field;
            }
            ss << '\n';
        }

        for (int64_t i = 0; i < rows; i++) {
            for (int64_t j = 0; j < cols; j++) {
                if (j > 0) ss << delimiter;
                std::string field = (data && data[i] && data[i][j]) ? data[i][j] : "";
                if (field_needs_quoting(field, delimiter, quote_char))
                    append_quoted(ss, field, quote_char);
                else
                    ss << field;
            }
            if (i < rows - 1 || headers)
                ss << '\n';
        }

        std::string result = ss.str();
        return hoo_strdup(result.c_str());

    } catch (...) {
        return NULL;
    }
}

char*** hoo_csv_read_file_raw(const char* path, int64_t* out_rows, int64_t* out_cols)
{
    if (!out_rows || !out_cols)
        return NULL;
    *out_rows = 0;
    *out_cols = 0;

    if (!path) return NULL;

    try {
        char* content = hoo_fs_read_text(path);
        if (!content) return NULL;

        char*** table = hoo_csv_parse_raw_with_opts(content, ',', '"', out_rows, out_cols);
        hoo_fs_free_string(content);
        return table;

    } catch (...) {
        *out_rows = 0;
        *out_cols = 0;
        return NULL;
    }
}

int64_t hoo_csv_write_file_raw(const char* path, const char** headers,
                               const char*** data, int64_t rows, int64_t cols)
{
    if (!path) return 1;

    try {
        char* csv = hoo_csv_generate_raw(headers, data, rows, cols);
        if (!csv) return 1;

        int64_t result = hoo_fs_write_text(path, csv);
        std::free(csv);
        return result ? 0 : 1;

    } catch (...) {
        return 1;
    }
}

int64_t hoo_csv_escape_raw(char c)
{
    return (c == ',' || c == '"' || c == '\n' || c == '\r') ? 1 : 0;
}

// ============================================================================
// Instance-based OOP API
// ============================================================================

HooArray hoo_csv_parse(HooCsv csv, const char* csv_str)
{
    if (!csv || !csv_str) return NULL;
    CsvHandle* h = get_handle(csv);

    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw_with_opts(csv_str, h->delimiter, h->quote_char, &rows, &cols);
    if (!table) return NULL;

    HooArray result = hoo_array_new();
    if (!result) {
        hoo_csv_free_table(table, rows, cols);
        return NULL;
    }

    for (int64_t i = 0; i < rows; i++) {
        HooArray row = hoo_array_new();
        if (!row) {
            hoo_array_release(result);
            hoo_csv_free_table(table, rows, cols);
            return NULL;
        }
        for (int64_t j = 0; j < cols; j++) {
            HooString s = hoo_string_from_cstr(table[i][j] ? table[i][j] : "");
            if (s) {
                hoo_string_retain(s);
                row = (HooArray)hoo_array_push_object(row, s);
                hoo_string_release(s);
            }
        }
        // Mark row as holding strings so hoo_array_release frees them
        if (row) ((int64_t*)row)[2] = HOO_TYPE_STRING;
        result = hoo_array_push_array(result, row);
        hoo_array_release(row);
    }

    // Mark result as holding arrays so hoo_array_release frees rows
    if (result) ((int64_t*)result)[2] = HOO_TYPE_ARRAY;

    hoo_csv_free_table(table, rows, cols);
    return result;
}

HooString hoo_csv_generate(HooCsv csv, void* data_arr)
{
    if (!csv || !data_arr) return NULL;
    CsvHandle* h = get_handle(csv);

    try {
        int64_t rows = hoo_array_length(data_arr);

        // Determine max columns
        int64_t cols = 0;
        for (int64_t i = 0; i < rows; i++) {
            HooArray row = NULL;
            if (hoo_array_get_array(data_arr, i, &row) && row) {
                int64_t rc = hoo_array_length(row);
                if (rc > cols) cols = rc;
            }
        }

        if (rows == 0 || cols == 0) return hoo_string_from_cstr("");
        if ((size_t)rows > std::numeric_limits<size_t>::max() / sizeof(const char**) ||
            (size_t)cols > std::numeric_limits<size_t>::max() / sizeof(const char*))
            return NULL;

        // Convert HooArray to char***
        const char*** cdata = (const char***)std::malloc(sizeof(const char**) * (size_t)rows);
        if (!cdata) return NULL;

        for (int64_t i = 0; i < rows; i++) {
            cdata[i] = (const char**)std::malloc(sizeof(const char*) * (size_t)cols);
            if (!cdata[i]) {
                for (int64_t k = 0; k < i; k++) std::free((void*)cdata[k]);
                std::free((void*)cdata);
                return NULL;
            }
            HooArray row = NULL;
            if (hoo_array_get_array(data_arr, i, &row) && row) {
                for (int64_t j = 0; j < cols; j++) {
                    const char* s = NULL;
                    if (j < hoo_array_length(row)) {
                        HooString hoos = NULL;
                        if (hoo_array_get_object(row, j, (void**)&hoos) && hoos)
                            s = hoo_string_data(hoos);
                    }
                    cdata[i][j] = s ? s : "";
                }
            } else {
                for (int64_t j = 0; j < cols; j++)
                    cdata[i][j] = "";
            }
        }

        char* raw = hoo_csv_generate_raw_with_opts(NULL, cdata, rows, cols,
                                                   h->delimiter, h->quote_char);

        for (int64_t i = 0; i < rows; i++)
            std::free((void*)cdata[i]);
        std::free((void*)cdata);

        if (!raw) return NULL;
        HooString result = hoo_string_from_cstr(raw);
        std::free(raw);
        return result;

    } catch (...) {
        return NULL;
    }
}

HooArray hoo_csv_read_file(HooCsv csv, const char* path)
{
    if (!csv || !path) return NULL;
    CsvHandle* h = get_handle(csv);

    char* content = hoo_fs_read_text(path);
    if (!content) return NULL;

    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw_with_opts(content, h->delimiter, h->quote_char,
                                                  &rows, &cols);
    hoo_fs_free_string(content);

    if (!table) return NULL;

    HooArray result = hoo_array_new();
    if (!result) {
        hoo_csv_free_table(table, rows, cols);
        return NULL;
    }

    for (int64_t i = 0; i < rows; i++) {
        HooArray row = hoo_array_new();
        if (!row) {
            hoo_array_release(result);
            hoo_csv_free_table(table, rows, cols);
            return NULL;
        }
        for (int64_t j = 0; j < cols; j++) {
            HooString s = hoo_string_from_cstr(table[i][j] ? table[i][j] : "");
            if (s) {
                hoo_string_retain(s);
                row = (HooArray)hoo_array_push_object(row, s);
                hoo_string_release(s);
            }
        }
        if (row) ((int64_t*)row)[2] = HOO_TYPE_STRING;
        result = hoo_array_push_array(result, row);
        hoo_array_release(row);
    }

    if (result) ((int64_t*)result)[2] = HOO_TYPE_ARRAY;

    hoo_csv_free_table(table, rows, cols);
    return result;
}

int64_t hoo_csv_write_file(HooCsv csv, const char* path, void* data_arr)
{
    if (!csv || !path || !data_arr) return 1;

    try {
        HooString csv_str = hoo_csv_generate(csv, data_arr);
        if (!csv_str) return 1;

        const char* data = hoo_string_data(csv_str);
        if (!data) { hoo_string_release(csv_str); return 1; }

        int64_t result = hoo_fs_write_text(path, data);
        hoo_string_release(csv_str);
        return result ? 0 : 1;

    } catch (...) {
        return 1;
    }
}

int64_t hoo_csv_escape(HooCsv csv, int32_t c)
{
    if (!csv) return 0;
    CsvHandle* h = get_handle(csv);
    char value = (char)c;
    return (value == h->delimiter || value == h->quote_char ||
            value == '\n' || value == '\r') ? 1 : 0;
}

// ============================================================================
// Map-based API (first row used as headers, returns HooArray<HooMap>)
// ============================================================================

HooArray hoo_csv_parse_as_maps(HooCsv csv, const char* csv_str)
{
    if (!csv || !csv_str) return NULL;
    CsvHandle* h = get_handle(csv);

    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw_with_opts(csv_str, h->delimiter, h->quote_char,
                                                 &rows, &cols);
    if (!table || rows < 1) return NULL;

    HooArray result = hoo_array_new();
    if (!result) { hoo_csv_free_table(table, rows, cols); return NULL; }

    for (int64_t i = 1; i < rows; i++) {
        HooMap map = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);
        if (!map) {
            hoo_array_release(result);
            hoo_csv_free_table(table, rows, cols);
            return NULL;
        }

        for (int64_t j = 0; j < cols; j++) {
            const char* key = table[0][j] ? table[0][j] : "";
            const char* val = table[i][j] ? table[i][j] : "";
            hoo_map_set(map, key, val);
        }

        // Retain so map survives in the array
        hoo_map_retain(map);
        hoo_array_push_object(result, map);
        hoo_map_release(map);
    }

    // Mark as holding maps so hoo_array_release frees them
    if (result) ((int64_t*)result)[2] = HOO_TYPE_MAP;

    hoo_csv_free_table(table, rows, cols);
    return result;
}

HooArray hoo_csv_read_file_as_maps(HooCsv csv, const char* path)
{
    if (!csv || !path) return NULL;

    char* content = hoo_fs_read_text(path);
    if (!content) return NULL;

    HooArray result = hoo_csv_parse_as_maps(csv, content);
    hoo_fs_free_string(content);
    return result;
}

// ============================================================================
// Aggregation — operate on HooArray<HooMap> data
// ============================================================================

static const char* get_column_value(void* data, int64_t index, const char* column)
{
    HooMap map = NULL;
    if (!hoo_array_get_object(data, index, (void**)&map) || !map)
        return NULL;
    const char* val = NULL;
    if (!hoo_map_try_get(map, column, &val))
        return NULL;
    return val;
}

static void require_numeric(const char* val, const char* column)
{
    if (!val || val[0] == '\0') return;
    char* end = NULL;
    double number = std::strtod(val, &end);
    if (!end || *end != '\0' || !std::isfinite(number)) {
        char msg[256];
        if (column) {
            snprintf(msg, sizeof(msg),
                "Non-numeric value '%s' in column '%s'", val, column);
        } else {
            snprintf(msg, sizeof(msg),
                "Non-numeric comparison value '%s'", val);
        }
        HooException exc = hoo_exception_invalid_cast(msg);
        hoo_exception_throw(exc);
    }
}

static int64_t parse_integer_value(const char* val, const char* column)
{
    if (!val || val[0] == '\0') return 0;
    char* end = NULL;
    errno = 0;
    long long number = std::strtoll(val, &end, 10);
    if (end == val || !end || *end != '\0' || errno == ERANGE) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Expected integer value '%s' in column '%s'",
                 val, column ? column : "");
        HooException exc = hoo_exception_invalid_cast(msg);
        hoo_exception_throw(exc);
    }
    return static_cast<int64_t>(number);
}

int64_t hoo_csv_count(HooCsv csv, void* data, const char* column)
{
    (void)csv;
    if (!data || !column) return 0;
    int64_t len = hoo_array_length(data);
    int64_t count = 0;
    for (int64_t i = 0; i < len; i++) {
        const char* val = get_column_value(data, i, column);
        if (val && val[0] != '\0') count++;
    }
    return count;
}

int64_t hoo_csv_sum(HooCsv csv, void* data, const char* column)
{
    (void)csv;
    if (!data || !column) return 0;
    int64_t len = hoo_array_length(data);
    int64_t sum = 0;
    for (int64_t i = 0; i < len; i++) {
        const char* val = get_column_value(data, i, column);
        if (val && val[0] != '\0') {
            sum += parse_integer_value(val, column);
        }
    }
    return sum;
}

HooString hoo_csv_avg(HooCsv csv, void* data, const char* column)
{
    (void)csv;
    if (!data || !column) return NULL;
    int64_t len = hoo_array_length(data);
    int64_t count = 0;
    double sum = 0.0;
    for (int64_t i = 0; i < len; i++) {
        const char* val = get_column_value(data, i, column);
        if (val && val[0] != '\0') {
            require_numeric(val, column);
            sum += std::strtod(val, NULL);
            count++;
        }
    }
    if (count == 0) return NULL;
    return hoo_string_from_double(sum / (double)count);
}

HooString hoo_csv_min(HooCsv csv, void* data, const char* column)
{
    (void)csv;
    if (!data || !column) return NULL;
    int64_t len = hoo_array_length(data);
    const char* best = NULL;
    for (int64_t i = 0; i < len; i++) {
        const char* val = get_column_value(data, i, column);
        if (!val || val[0] == '\0') continue;
        if (!best || std::strcmp(val, best) < 0)
            best = val;
    }
    return best ? hoo_string_from_cstr(best) : NULL;
}

HooString hoo_csv_max(HooCsv csv, void* data, const char* column)
{
    (void)csv;
    if (!data || !column) return NULL;
    int64_t len = hoo_array_length(data);
    const char* best = NULL;
    for (int64_t i = 0; i < len; i++) {
        const char* val = get_column_value(data, i, column);
        if (!val || val[0] == '\0') continue;
        if (!best || std::strcmp(val, best) > 0)
            best = val;
    }
    return best ? hoo_string_from_cstr(best) : NULL;
}

// ============================================================================
// Transformations — operate on HooArray<HooMap> data
// ============================================================================

HooArray hoo_csv_select(HooCsv csv, void* data, void* columns)
{
    (void)csv;
    if (!data || !columns) return NULL;
    int64_t num_cols = hoo_array_length(columns);
    if (num_cols < 1) return NULL;
    int64_t num_rows = hoo_array_length(data);
    if (num_rows < 1) return NULL;

    // Extract column names from HooArray<HooString> into temp array
    std::vector<const char*> col_names((size_t)num_cols);
    for (int64_t j = 0; j < num_cols; j++) {
        HooString hs = NULL;
        if (hoo_array_get_object(columns, j, (void**)&hs) && hs)
            col_names[(size_t)j] = hoo_string_data(hs);
        else
            col_names[(size_t)j] = "";
    }

    HooArray result = hoo_array_new();
    if (!result) return NULL;

    for (int64_t i = 0; i < num_rows; i++) {
        HooMap src = NULL;
        if (!hoo_array_get_object(data, i, (void**)&src) || !src) continue;

        HooMap dst = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);
        if (!dst) { hoo_array_release(result); return NULL; }

        for (int64_t j = 0; j < num_cols; j++) {
            const char* val = NULL;
            if (!hoo_map_try_get(src, col_names[(size_t)j], &val))
                val = "";
            hoo_map_set(dst, col_names[(size_t)j], val ? val : "");
        }

        // Retain so map survives in the array
        hoo_map_retain(dst);
        hoo_array_push_object(result, dst);
        hoo_map_release(dst);
    }

    // Mark as holding maps so hoo_array_release frees them
    if (result) ((int64_t*)result)[2] = HOO_TYPE_MAP;

    return result;
}

static bool compare_values(const std::string& lhs, const std::string& rhs, const char* op)
{
    if (strcmp(op, "==") == 0) return lhs == rhs;
    if (strcmp(op, "!=") == 0) return lhs != rhs;

    // Numeric comparison for ordering operators
    char* lhs_end = NULL;
    char* rhs_end = NULL;
    double dlhs = std::strtod(lhs.c_str(), &lhs_end);
    double drhs = std::strtod(rhs.c_str(), &rhs_end);
    bool lhs_num = (lhs_end && *lhs_end == '\0' && !lhs.empty());
    bool rhs_num = (rhs_end && *rhs_end == '\0' && !rhs.empty());

    if (lhs_num && rhs_num) {
        if (strcmp(op, ">") == 0) return dlhs > drhs;
        if (strcmp(op, ">=") == 0) return dlhs >= drhs;
        if (strcmp(op, "<") == 0) return dlhs < drhs;
        if (strcmp(op, "<=") == 0) return dlhs <= drhs;
    } else {
        int cmp = lhs.compare(rhs);
        if (strcmp(op, ">") == 0) return cmp > 0;
        if (strcmp(op, ">=") == 0) return cmp >= 0;
        if (strcmp(op, "<") == 0) return cmp < 0;
        if (strcmp(op, "<=") == 0) return cmp <= 0;
    }
    return false;
}

HooArray hoo_csv_filter(HooCsv csv, void* data, const char* column,
                          const char* op, const char* value)
{
    (void)csv;
    if (!data || !column || !op || !value) return NULL;
    int64_t len = hoo_array_length(data);
    if (len < 1) return NULL;

    bool is_ordering = (strcmp(op, ">") == 0 || strcmp(op, ">=") == 0 ||
                        strcmp(op, "<") == 0 || strcmp(op, "<=") == 0);
    if (is_ordering) {
        require_numeric(value, NULL);
    }

    HooArray result = hoo_array_new();
    if (!result) return NULL;

    for (int64_t i = 0; i < len; i++) {
        HooMap map = NULL;
        if (!hoo_array_get_object(data, i, (void**)&map) || !map) continue;

        const char* raw_val = NULL;
        if (!hoo_map_try_get(map, column, &raw_val))
            raw_val = "";

        if (is_ordering && raw_val && raw_val[0] != '\0') {
            require_numeric(raw_val, column);
        }

        if (compare_values(raw_val ? raw_val : "", value ? value : "", op)) {
            hoo_map_retain(map);
            hoo_array_push_object(result, map);
        }
    }

    // Mark as holding maps so hoo_array_release frees them
    if (result) ((int64_t*)result)[2] = HOO_TYPE_MAP;

    return result;
}

HooArray hoo_csv_sort(HooCsv csv, void* data, const char* column, int64_t ascending)
{
    (void)csv;
    if (!data || !column) return NULL;
    int64_t len = hoo_array_length(data);
    if (len < 1) return NULL;

    // Collect indices and values for sorting
    std::vector<int64_t> indices((size_t)len);
    std::vector<std::string> values((size_t)len);
    for (int64_t i = 0; i < len; i++) {
        indices[(size_t)i] = i;
        const char* val = get_column_value(data, i, column);
        values[(size_t)i] = val ? val : "";
    }

    std::sort(indices.begin(), indices.end(),
        [&](int64_t a, int64_t b) {
            if (ascending)
                return values[(size_t)a] < values[(size_t)b];
            else
                return values[(size_t)a] > values[(size_t)b];
        });

    HooArray result = hoo_array_new();
    if (!result) return NULL;

    for (int64_t i = 0; i < len; i++) {
        HooMap src = NULL;
        if (hoo_array_get_object(data, indices[(size_t)i], (void**)&src) && src) {
            hoo_map_retain(src);
            hoo_array_push_object(result, src);
        }
    }

    // Mark as holding maps so hoo_array_release frees them
    if (result) ((int64_t*)result)[2] = HOO_TYPE_MAP;

    return result;
}

// ============================================================================
// Statistics
// ============================================================================

HooMap hoo_csv_describe(HooCsv csv, void* data, const char* column)
{
    (void)csv;
    if (!data || !column) return NULL;

    HooMap result = hoo_map_new(HOO_MAP_KEY_STRING, HOO_MAP_VAL_STRING);
    if (!result) return NULL;

    int64_t len = hoo_array_length(data);
    if (len < 1) {
        hoo_map_set(result, "count", "0");
        return result;
    }

    int64_t count = 0;
    double sum = 0.0;
    const char* min_val = NULL;
    const char* max_val = NULL;

    for (int64_t i = 0; i < len; i++) {
        const char* val = get_column_value(data, i, column);
        if (!val || val[0] == '\0') continue;
        count++;
        require_numeric(val, column);
        sum += std::strtod(val, NULL);
        if (!min_val || std::strcmp(val, min_val) < 0) min_val = val;
        if (!max_val || std::strcmp(val, max_val) > 0) max_val = val;
    }

    // Format values as C strings and store in map
    char count_buf[32];
    snprintf(count_buf, sizeof(count_buf), "%lld", (long long)count);
    hoo_map_set(result, "count", count_buf);

    if (count > 0) {
        char sum_buf[64];
        snprintf(sum_buf, sizeof(sum_buf), "%.6f", sum);
        char* sum_end = sum_buf + strlen(sum_buf) - 1;
        while (sum_end > sum_buf && *sum_end == '0') sum_end--;
        if (*sum_end == '.') sum_end--;
        *(sum_end + 1) = '\0';
        hoo_map_set(result, "sum", sum_buf);

        double avg = sum / (double)count;
        char avg_buf[64];
        snprintf(avg_buf, sizeof(avg_buf), "%.6f", avg);
        // Remove trailing zeros
        char* p = avg_buf + strlen(avg_buf) - 1;
        while (p > avg_buf && *p == '0') p--;
        if (*p == '.') p--;
        *(p + 1) = '\0';
        hoo_map_set(result, "avg", avg_buf);

        hoo_map_set(result, "min", min_val ? min_val : "");
        hoo_map_set(result, "max", max_val ? max_val : "");
    }

    return result;
}

// ============================================================================
// Memory management
// ============================================================================

void hoo_csv_free_table(char*** table, int64_t rows, int64_t cols)
{
    if (!table) return;
    for (int64_t i = 0; i < rows; i++) {
        if (table[i]) {
            for (int64_t j = 0; j < cols; j++)
                std::free(table[i][j]);
            std::free(table[i]);
        }
    }
    std::free(table);
}

void hoo_csv_free_string(char* str)
{
    std::free(str);
}

} // extern "C"
