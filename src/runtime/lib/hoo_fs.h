#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// C-ABI Bridge Functions (for JIT / FFI linkage)
// ============================================================================
// These remain stable so the JIT bridge in HVMJIT.cpp continues to link.
// Each delegates to the corresponding hoo::fs class method.

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
int64_t hoo_fs_mkdir(const char* path);
int64_t hoo_fs_mkdirs(const char* path);
int64_t hoo_fs_rmdir(const char* path);
char**  hoo_fs_list_dir(const char* path, int64_t* out_count);
void    hoo_fs_free_list(char** list, int64_t count);
char*   hoo_fs_temp_dir(void);
char*   hoo_fs_create_temp_file(const char* prefix);
void    hoo_fs_free_string(char* str);

#ifdef __cplusplus
}
#endif

// ============================================================================
// C++ Object-Oriented API
// ============================================================================

namespace hoo {
namespace fs {

class Path {
public:
    static std::string getTempDir();
    static std::string createTempFile(const std::string& prefix);
};

class File {
public:
    static bool exists(const std::string& path);
    static bool isFile(const std::string& path);
    static int64_t size(const std::string& path);
    static int64_t lastModified(const std::string& path);
    static bool remove(const std::string& path);
    static bool rename(const std::string& oldPath, const std::string& newPath);
    static bool copy(const std::string& src, const std::string& dst);
    static std::string readText(const std::string& path);
    static bool writeText(const std::string& path, const std::string& content);
    static bool appendText(const std::string& path, const std::string& content);
    static bool readBytes(const std::string& path, std::vector<uint8_t>& outData);
    static bool writeBytes(const std::string& path, const std::vector<uint8_t>& data);
};

class Directory {
public:
    static bool isDirectory(const std::string& path);
    static bool create(const std::string& path);
    static bool createTree(const std::string& path);
    static bool remove(const std::string& path);
    static std::vector<std::string> list(const std::string& path);
};

} // namespace fs
} // namespace hoo
