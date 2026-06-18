#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>
#include "hoo_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// C-ABI Bridge Functions (for JIT / FFI linkage)
// ============================================================================
// These remain stable so the JIT bridge in HVMJIT.cpp continues to link.
// Each delegates to the corresponding hoo::fs functions / methods.

int64_t hoo_fs_exists(const char* path);
int64_t hoo_fs_is_file(const char* path);
int64_t hoo_fs_is_dir(const char* path);
int64_t hoo_fs_size(const char* path);
int64_t hoo_fs_last_modified(const char* path);
int64_t hoo_fs_delete(const char* path);
int64_t hoo_fs_rename(const char* old_path, const char* new_path);
int64_t hoo_fs_copy(const char* src, const char* dst);
char*   hoo_fs_read_text(const char* path);
int64_t hoo_fs_write_text(const char* path, const char* content);
int64_t hoo_fs_append_text(const char* path, const char* content);
int64_t hoo_fs_read_bytes(const char* path, uint8_t** out_data, int64_t* out_len);
int64_t hoo_fs_write_bytes(const char* path, const uint8_t* data, int64_t len);
int64_t hoo_fs_write_bytes_buffer(const char* path, HooBuffer buf);
HooBuffer hoo_fs_read_bytes_buffer(const char* path);
int64_t hoo_fs_mkdir(const char* path);
int64_t hoo_fs_mkdirs(const char* path);
int64_t hoo_fs_rmdir(const char* path);
char**  hoo_fs_list_dir(const char* path, int64_t* out_count);
void    hoo_fs_free_list(char** list, int64_t count);
char*   hoo_fs_temp_dir(void);
char*   hoo_fs_create_temp_file(const char* prefix);
void    hoo_fs_free_string(char* str);

// Path module C-ABI bridges (merged from hoo_path.h)
char*   hoo_path_dirname(const char* path);
char*   hoo_path_basename(const char* path);
char*   hoo_path_extension(const char* path);
char*   hoo_path_stem(const char* path);
char*   hoo_path_root(const char* path);
char*   hoo_path_join(const char* a, const char* b);
char*   hoo_path_join_multi(const char** parts, int64_t count);
char*   hoo_path_normalize(const char* path);
char*   hoo_path_absolute(const char* path);
char*   hoo_path_relative(const char* path, const char* base);
int64_t hoo_path_is_absolute(const char* path);
int64_t hoo_path_is_relative(const char* path);
int64_t hoo_path_has_extension(const char* path);
int64_t hoo_path_has_root(const char* path);
char**  hoo_path_split(const char* path, int64_t* out_count);
void    hoo_path_free_parts(char** parts, int64_t count);
char    hoo_path_separator(void);
char    hoo_path_list_separator(void);
void    hoo_path_free_string(char* str);

#ifdef __cplusplus
}
#endif

// ============================================================================
// C++ OOP API
// ============================================================================
// Classes have constructors and instance methods. Free functions are used for
// operations that don't have a natural instance (join, separator, etc.).

namespace hoo {
namespace fs {

// ── Free functions ──────────────────────────────────────────────────────────

std::string join(const std::string& a, const std::string& b);
std::string joinMulti(const std::vector<std::string>& parts);
std::string relative(const std::string& path, const std::string& base);
char separator();
char listSeparator();
std::string tempDir();
std::string createTempFile(const std::string& prefix);
bool copyFile(const std::string& src, const std::string& dst);

// ── Path ────────────────────────────────────────────────────────────────────

class Path {
    std::string path_;
public:
    explicit Path(const std::string& path) : path_(path) {}

    const std::string& str() const { return path_; }

    std::string dirname() const;
    std::string basename() const;
    std::string extension() const;
    std::string stem() const;
    std::string root() const;
    Path normalized() const;
    Path absolute() const;
    bool isAbsolute() const;
    bool isRelative() const;
    bool hasExtension() const;
    bool hasRoot() const;
    std::vector<std::string> split() const;
};

// ── File ────────────────────────────────────────────────────────────────────

class File {
    std::string path_;
public:
    explicit File(const std::string& path) : path_(path) {}

    const std::string& path() const { return path_; }

    bool exists() const;
    bool isFile() const;
    int64_t size() const;
    int64_t lastModified() const;
    bool remove();
    bool rename(const std::string& newPath);
    std::string readText() const;
    bool writeText(const std::string& content);
    bool appendText(const std::string& content);
    bool readBytes(std::vector<uint8_t>& outData) const;
    bool writeBytes(const std::vector<uint8_t>& data);
};

// ── Directory ───────────────────────────────────────────────────────────────

class Directory {
    std::string path_;
public:
    explicit Directory(const std::string& path) : path_(path) {}

    const std::string& path() const { return path_; }

    bool exists() const;
    bool isDirectory() const;
    bool create();
    bool createTree();
    bool remove();
    std::vector<std::string> list() const;
};

} // namespace fs
} // namespace hoo
