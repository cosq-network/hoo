#include "hoo_csv.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

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

extern "C" {

// ============================================================================
// Parsing
// ============================================================================

char*** hoo_csv_parse(const char* csv, int64_t* out_rows, int64_t* out_cols)
{
    return hoo_csv_parse_with_opts(csv, ',', '"', out_rows, out_cols);
}

char*** hoo_csv_parse_with_opts(const char* csv, char delimiter, char quote_char,
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
            // Skip leading whitespace
            while (pos < len && (csv[pos] == ' ' || csv[pos] == '\t'))
                pos++;

            if (pos >= len) {
                current_row.push_back("");
                rows.push_back(current_row);
                break;
            }

            // Empty line (consecutive newlines)
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
                // Quoted field
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

                // Skip whitespace after closing quote
                while (pos < len && (csv[pos] == ' ' || csv[pos] == '\t'))
                    pos++;
            } else {
                // Unquoted field
                size_t start = pos;
                while (pos < len && csv[pos] != delimiter
                       && csv[pos] != '\n' && csv[pos] != '\r')
                {
                    pos++;
                }
                field = trim_ws(std::string(csv + start, pos - start));
                current_row.push_back(field);
            }

            // Separator or line end
            if (pos >= len) {
                rows.push_back(current_row);
                break;
            }

            if (csv[pos] == delimiter) {
                pos++;
                continue;
            }

            // Newline
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

// ============================================================================
// Generation
// ============================================================================

char* hoo_csv_generate(const char** headers, const char*** data,
                       int64_t rows, int64_t cols)
{
    return hoo_csv_generate_with_opts(headers, data, rows, cols, ',', '"');
}

char* hoo_csv_generate_with_opts(const char** headers, const char*** data,
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

// ============================================================================
// File I/O
// ============================================================================

char*** hoo_csv_read_file(const char* path, int64_t* out_rows, int64_t* out_cols)
{
    *out_rows = 0;
    *out_cols = 0;

    if (!path) return NULL;

    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return NULL;

        std::ostringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string content = buffer.str();
        return hoo_csv_parse(content.c_str(), out_rows, out_cols);

    } catch (...) {
        *out_rows = 0;
        *out_cols = 0;
        return NULL;
    }
}

int64_t hoo_csv_write_file(const char* path, const char** headers,
                           const char*** data, int64_t rows, int64_t cols)
{
    if (!path) return 1;

    try {
        char* csv = hoo_csv_generate(headers, data, rows, cols);
        if (!csv) return 1;

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::free(csv);
            return 1;
        }

        file.write(csv, (std::streamsize)std::strlen(csv));
        file.close();
        std::free(csv);
        return 0;

    } catch (...) {
        return 1;
    }
}

// ============================================================================
// Utility
// ============================================================================

int64_t hoo_csv_escape(char c)
{
    return (c == ',' || c == '"' || c == '\n' || c == '\r') ? 1 : 0;
}

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
