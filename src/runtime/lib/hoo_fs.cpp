#include "hoo_fs.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>

namespace fs = std::filesystem;

extern "C" {

// ============================================================================
// File Operations
// ============================================================================

int64_t hoo_fs_exists(const char* path)
{
    try {
        return fs::exists(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_is_file(const char* path)
{
    try {
        return fs::is_regular_file(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_is_dir(const char* path)
{
    try {
        return fs::is_directory(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_size(const char* path)
{
    try {
        return (int64_t)fs::file_size(path);
    } catch (...) {
        return -1;
    }
}

int64_t hoo_fs_last_modified(const char* path)
{
    try {
        auto ft = fs::last_write_time(path);
        auto s = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ft - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        return (int64_t)std::chrono::system_clock::to_time_t(s);
    } catch (...) {
        return -1;
    }
}

int64_t hoo_fs_delete(const char* path)
{
    try {
        return fs::remove(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_rename(const char* old_path, const char* new_path)
{
    try {
        fs::rename(old_path, new_path);
        return 1;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_copy(const char* src, const char* dst)
{
    try {
        fs::copy(src, dst, fs::copy_options::overwrite_existing);
        return 1;
    } catch (...) {
        return 0;
    }
}

// ============================================================================
// Read/Write Text Files
// ============================================================================

char* hoo_fs_read_text(const char* path)
{
    try {
        std::ifstream file(path, std::ios::in);
        if (!file.is_open()) return NULL;
        std::ostringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();
        char* result = (char*)std::malloc(content.size() + 1);
        if (!result) return NULL;
        std::memcpy(result, content.data(), content.size() + 1);
        return result;
    } catch (...) {
        return NULL;
    }
}

int64_t hoo_fs_write_text(const char* path, const char* content)
{
    try {
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file.is_open()) return 0;
        file.write(content, std::strlen(content));
        file.close();
        return file.good() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_append_text(const char* path, const char* content)
{
    try {
        std::ofstream file(path, std::ios::out | std::ios::app);
        if (!file.is_open()) return 0;
        file.write(content, std::strlen(content));
        file.close();
        return file.good() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

// ============================================================================
// Read/Write Binary Files
// ============================================================================

int64_t hoo_fs_read_bytes(const char* path, uint8_t** out_data, int64_t* out_len)
{
    try {
        std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open()) return 0;
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        uint8_t* buf = (uint8_t*)std::malloc((size_t)size);
        if (!buf) return 0;
        if (!file.read((char*)buf, size)) {
            std::free(buf);
            return 0;
        }
        *out_data = buf;
        *out_len = (int64_t)size;
        return 1;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_write_bytes(const char* path, const uint8_t* data, int64_t len)
{
    try {
        std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return 0;
        file.write((const char*)data, (std::streamsize)len);
        file.close();
        return file.good() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

// ============================================================================
// Directory Operations
// ============================================================================

int64_t hoo_fs_mkdir(const char* path)
{
    try {
        return fs::create_directory(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_mkdirs(const char* path)
{
    try {
        return fs::create_directories(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_fs_rmdir(const char* path)
{
    try {
        return fs::remove(path) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

char** hoo_fs_list_dir(const char* path, int64_t* out_count)
{
    try {
        std::vector<std::string> entries;
        for (const auto& entry : fs::directory_iterator(path)) {
            entries.push_back(entry.path().filename().string());
        }
        char** list = (char**)std::malloc(entries.size() * sizeof(char*));
        if (!list) return NULL;
        for (size_t i = 0; i < entries.size(); i++) {
            list[i] = strdup(entries[i].c_str());
        }
        *out_count = (int64_t)entries.size();
        return list;
    } catch (...) {
        return NULL;
    }
}

void hoo_fs_free_list(char** list, int64_t count)
{
    if (!list) return;
    for (int64_t i = 0; i < count; i++) {
        if (list[i]) std::free(list[i]);
    }
    std::free(list);
}

// ============================================================================
// Temp Files
// ============================================================================

char* hoo_fs_temp_dir(void)
{
    try {
        return strdup(fs::temp_directory_path().string().c_str());
    } catch (...) {
        return NULL;
    }
}

char* hoo_fs_create_temp_file(const char* prefix)
{
    try {
        fs::path dir = fs::temp_directory_path();
        fs::path tmp_path;
        for (int attempt = 0; attempt < 256; attempt++) {
            std::string name = std::string(prefix) + "_XXXXXX";
            // Use mkstemps-like approach: generate random suffix
            std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
            for (int i = 0; i < 6; i++) {
                name += chars[rand() % chars.size()];
            }
            tmp_path = dir / name;
            // Try to create and open exclusively
            std::ofstream file(tmp_path, std::ios::out | std::ios::binary);
            if (file.is_open()) {
                file.close();
                return strdup(tmp_path.string().c_str());
            }
        }
        return NULL;
    } catch (...) {
        return NULL;
    }
}

// ============================================================================
// Memory Management
// ============================================================================

void hoo_fs_free_string(char* str)
{
    std::free(str);
}

} // extern "C"
