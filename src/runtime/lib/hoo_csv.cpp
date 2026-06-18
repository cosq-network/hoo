#include "hoo_csv.h"
#include "hoo_runtime.h"
#include "hoo_string.h"
#include "hoo_generic_array.h"
#include "hoo_fs.h"
#include "hoo_map.h"
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>

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
    CsvHandle* h = (CsvHandle*)hoo_alloc(sizeof(CsvHandle), HOO_TYPE_CSV);
    if (h) {
        h->delimiter = (char)delimiter;
        h->quote_char = (char)quote_char;
    }
    return (HooCsv)h;
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
    *out_rows = 0;
    *out_cols = 0;

    if (!csv || !*csv)
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
                while (pos < len) {
                    if (csv[pos] == quote_char) {
                        if (pos + 1 < len && csv[pos + 1] == quote_char) {
                            field += quote_char;
                            pos += 2;
                        } else {
                            pos++;
                            break;
                        }
                    } else {
                        field += csv[pos];
                        pos++;
                    }
                }
                current_row.push_back(field);

                while (pos < len && (csv[pos] == ' ' || csv[pos] == '\t'))
                    pos++;
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
                    table[i][j] = strdup(rows[i][j].c_str());
                else
                    table[i][j] = strdup("");
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
        return strdup(result.c_str());

    } catch (...) {
        return NULL;
    }
}

char*** hoo_csv_read_file_raw(const char* path, int64_t* out_rows, int64_t* out_cols)
{
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

    // Convert char*** to HooArray<HooArray<HooString>>
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
                hoo_array_push_object(row, s);
                hoo_string_release(s);
            }
        }
        hoo_array_push_array(result, row);
        hoo_array_release(row);
    }

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

        // Convert HooArray to char***
        const char*** cdata = (const char***)malloc(sizeof(const char**) * (size_t)rows);
        if (!cdata) return NULL;

        for (int64_t i = 0; i < rows; i++) {
            cdata[i] = (const char**)malloc(sizeof(const char*) * (size_t)cols);
            if (!cdata[i]) {
                for (int64_t k = 0; k < i; k++) free((void*)cdata[k]);
                free((void*)cdata);
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
            free((void*)cdata[i]);
        free((void*)cdata);

        if (!raw) return NULL;
        HooString result = hoo_string_from_cstr(raw);
        free(raw);
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

    // Convert char*** to HooArray
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
                hoo_array_push_object(row, s);
                hoo_string_release(s);
            }
        }
        hoo_array_push_array(result, row);
        hoo_array_release(row);
    }

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
    (void)csv;
    return hoo_csv_escape_raw((char)c);
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

        hoo_array_push_object(result, map);
        hoo_map_release(map);
    }

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
