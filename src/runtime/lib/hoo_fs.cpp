#include "hoo_fs.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <random>
#include <chrono>

namespace fs = std::filesystem;

#ifdef _MSC_VER
#define hoo_strdup _strdup
#else
#define hoo_strdup strdup
#endif

// -------------------------------------------------------------------
// Internal helpers
// -------------------------------------------------------------------
namespace {

int random_int() noexcept
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 35);
    return dis(gen);
}

// Internal write helper used by both File::writeBytes and hoo_fs_write_bytes
// to avoid a vector copy in the C-ABI bridge path.
bool write_bytes_impl(const std::string& path, const uint8_t* data, std::streamsize len) noexcept
{
    try {
        std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        file.write(reinterpret_cast<const char*>(data), len);
        file.close();
        return file.good();
    } catch (...) {
        return false;
    }
}

} // anonymous namespace

// ============================================================================
// hoo::fs::Path
// ============================================================================

namespace hoo { namespace fs {

std::string Path::getTempDir()
{
    try {
        return ::fs::temp_directory_path().string();
    } catch (...) {
        return {};
    }
}

std::string Path::createTempFile(const std::string& prefix)
{
    try {
        ::fs::path dir = ::fs::temp_directory_path();
        const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
        for (int attempt = 0; attempt < 256; attempt++) {
            std::string name = prefix + "_XXXXXX";
            for (int i = 0; i < 6; i++) {
                name += chars[random_int()];
            }
            ::fs::path tmp = dir / name;
            std::ofstream file(tmp, std::ios::out | std::ios::binary);
            if (file.is_open()) {
                file.close();
                return tmp.string();
            }
        }
        return {};
    } catch (...) {
        return {};
    }
}

std::string Path::dirname(const std::string& path)
{
    try {
        ::fs::path p(path);
        ::fs::path parent = p.parent_path();
        if (parent.empty())
            return ".";
        return parent.string();
    } catch (...) {
        return {};
    }
}

std::string Path::basename(const std::string& path)
{
    try {
        std::string pstr = path;
        while (pstr.size() > 1 && (pstr.back() == '/' || pstr.back() == '\\'))
            pstr.pop_back();
        return ::fs::path(pstr).filename().string();
    } catch (...) {
        return {};
    }
}

std::string Path::extension(const std::string& path)
{
    try {
        return ::fs::path(path).extension().string();
    } catch (...) {
        return {};
    }
}

std::string Path::stem(const std::string& path)
{
    try {
        return ::fs::path(path).stem().string();
    } catch (...) {
        return {};
    }
}

std::string Path::root(const std::string& path)
{
    try {
        return ::fs::path(path).root_path().string();
    } catch (...) {
        return {};
    }
}

std::string Path::join(const std::string& a, const std::string& b)
{
    try {
        return (::fs::path(a) / b).string();
    } catch (...) {
        return {};
    }
}

std::string Path::joinMulti(const std::vector<std::string>& parts)
{
    try {
        ::fs::path result;
        for (const auto& part : parts) {
            result /= part;
        }
        return result.string();
    } catch (...) {
        return {};
    }
}

std::string Path::normalize(const std::string& path)
{
    try {
        return ::fs::path(path).lexically_normal().string();
    } catch (...) {
        return {};
    }
}

std::string Path::absolute(const std::string& path)
{
    try {
        return ::fs::absolute(path).string();
    } catch (...) {
        return {};
    }
}

std::string Path::relative(const std::string& path, const std::string& base)
{
    try {
        return ::fs::relative(path, base).string();
    } catch (...) {
        return {};
    }
}

bool Path::isAbsolute(const std::string& path)
{
    try {
        return ::fs::path(path).is_absolute();
    } catch (...) {
        return false;
    }
}

bool Path::isRelative(const std::string& path)
{
    try {
        return !::fs::path(path).is_absolute();
    } catch (...) {
        return false;
    }
}

bool Path::hasExtension(const std::string& path)
{
    try {
        return ::fs::path(path).has_extension();
    } catch (...) {
        return false;
    }
}

bool Path::hasRoot(const std::string& path)
{
    try {
        return ::fs::path(path).has_root_path();
    } catch (...) {
        return false;
    }
}

std::vector<std::string> Path::split(const std::string& path)
{
    try {
        std::vector<std::string> components;
        for (const auto& part : ::fs::path(path)) {
            components.push_back(part.string());
        }
        return components;
    } catch (...) {
        return {};
    }
}

char Path::separator()
{
    return ::fs::path::preferred_separator;
}

char Path::listSeparator()
{
#ifdef _WIN32
    return ';';
#else
    return ':';
#endif
}

// ============================================================================
// hoo::fs::File
// ============================================================================

bool File::exists(const std::string& path)
{
    try { return ::fs::exists(path); } catch (...) { return false; }
}

bool File::isFile(const std::string& path)
{
    try { return ::fs::is_regular_file(path); } catch (...) { return false; }
}

int64_t File::size(const std::string& path)
{
    try { return static_cast<int64_t>(::fs::file_size(path)); } catch (...) { return -1; }
}

int64_t File::lastModified(const std::string& path)
{
    try {
        auto ft = ::fs::last_write_time(path);
        auto s = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ft - ::fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        return static_cast<int64_t>(std::chrono::system_clock::to_time_t(s));
    } catch (...) {
        return -1;
    }
}

bool File::remove(const std::string& path)
{
    try { return ::fs::remove(path); } catch (...) { return false; }
}

bool File::rename(const std::string& oldPath, const std::string& newPath)
{
    try { ::fs::rename(oldPath, newPath); return true; } catch (...) { return false; }
}

bool File::copy(const std::string& src, const std::string& dst)
{
    try { ::fs::copy(src, dst, ::fs::copy_options::overwrite_existing); return true; } catch (...) { return false; }
}

std::string File::readText(const std::string& path)
{
    try {
        std::ifstream file(path, std::ios::in);
        if (!file.is_open()) return {};
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    } catch (...) {
        return {};
    }
}

bool File::writeText(const std::string& path, const std::string& content)
{
    try {
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file.is_open()) return false;
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        file.close();
        return file.good();
    } catch (...) {
        return false;
    }
}

bool File::appendText(const std::string& path, const std::string& content)
{
    try {
        std::ofstream file(path, std::ios::out | std::ios::app);
        if (!file.is_open()) return false;
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        file.close();
        return file.good();
    } catch (...) {
        return false;
    }
}

bool File::readBytes(const std::string& path, std::vector<uint8_t>& outData)
{
    try {
        std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        outData.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(outData.data()), size)) {
            outData.clear();
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool File::writeBytes(const std::string& path, const std::vector<uint8_t>& data)
{
    return write_bytes_impl(path, data.data(), static_cast<std::streamsize>(data.size()));
}

// ============================================================================
// hoo::fs::Directory
// ============================================================================

bool Directory::isDirectory(const std::string& path)
{
    try { return ::fs::is_directory(path); } catch (...) { return false; }
}

bool Directory::create(const std::string& path)
{
    try { return ::fs::create_directory(path); } catch (...) { return false; }
}

bool Directory::createTree(const std::string& path)
{
    try { return ::fs::create_directories(path); } catch (...) { return false; }
}

bool Directory::remove(const std::string& path)
{
    try { return ::fs::remove(path); } catch (...) { return false; }
}

std::vector<std::string> Directory::list(const std::string& path)
{
    try {
        std::vector<std::string> entries;
        for (const auto& entry : ::fs::directory_iterator(path)) {
            entries.push_back(entry.path().filename().string());
        }
        return entries;
    } catch (...) {
        return {};
    }
}

}} // namespace hoo::fs

// ============================================================================
// C-ABI Bridge Functions (JIT / FFI linkage)
// ============================================================================

extern "C" {

int64_t hoo_fs_exists(const char* path)
{
    return hoo::fs::File::exists(path ? path : "") ? 1 : 0;
}

int64_t hoo_fs_is_file(const char* path)
{
    return hoo::fs::File::isFile(path ? path : "") ? 1 : 0;
}

int64_t hoo_fs_is_dir(const char* path)
{
    return hoo::fs::Directory::isDirectory(path ? path : "") ? 1 : 0;
}

int64_t hoo_fs_size(const char* path)
{
    return hoo::fs::File::size(path ? path : "");
}

int64_t hoo_fs_last_modified(const char* path)
{
    return hoo::fs::File::lastModified(path ? path : "");
}

int64_t hoo_fs_delete(const char* path)
{
    return hoo::fs::File::remove(path ? path : "") ? 1 : 0;
}

int64_t hoo_fs_rename(const char* old_path, const char* new_path)
{
    return hoo::fs::File::rename(old_path ? old_path : "", new_path ? new_path : "") ? 1 : 0;
}

int64_t hoo_fs_copy(const char* src, const char* dst)
{
    return hoo::fs::File::copy(src ? src : "", dst ? dst : "") ? 1 : 0;
}

char* hoo_fs_read_text(const char* path)
{
    std::string content = hoo::fs::File::readText(path ? path : "");
    if (content.empty()) return nullptr;
    char* result = static_cast<char*>(std::malloc(content.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, content.data(), content.size() + 1);
    return result;
}

int64_t hoo_fs_write_text(const char* path, const char* content)
{
    return hoo::fs::File::writeText(path ? path : "", content ? content : "") ? 1 : 0;
}

int64_t hoo_fs_append_text(const char* path, const char* content)
{
    return hoo::fs::File::appendText(path ? path : "", content ? content : "") ? 1 : 0;
}

int64_t hoo_fs_read_bytes(const char* path, uint8_t** out_data, int64_t* out_len)
{
    if (!path || !out_data || !out_len) return 0;
    std::vector<uint8_t> buf;
    if (!hoo::fs::File::readBytes(path, buf)) return 0;
    if (buf.empty()) {
        *out_data = nullptr;
        *out_len = 0;
        return 1;
    }
    *out_data = static_cast<uint8_t*>(std::malloc(buf.size()));
    if (!*out_data) return 0;
    std::memcpy(*out_data, buf.data(), buf.size());
    *out_len = static_cast<int64_t>(buf.size());
    return 1;
}

int64_t hoo_fs_write_bytes(const char* path, const uint8_t* data, int64_t len)
{
    if (!path) return 0;
    return write_bytes_impl(path, data, static_cast<std::streamsize>(len)) ? 1 : 0;
}

int64_t hoo_fs_mkdir(const char* path)
{
    return hoo::fs::Directory::create(path ? path : "") ? 1 : 0;
}

int64_t hoo_fs_mkdirs(const char* path)
{
    return hoo::fs::Directory::createTree(path ? path : "") ? 1 : 0;
}

int64_t hoo_fs_rmdir(const char* path)
{
    return hoo::fs::Directory::remove(path ? path : "") ? 1 : 0;
}

char** hoo_fs_list_dir(const char* path, int64_t* out_count)
{
    if (!path || !out_count) return nullptr;
    std::vector<std::string> entries = hoo::fs::Directory::list(path);
    if (entries.empty()) {
        // Return an allocated sentinel so callers can distinguish
        // "empty directory" (*out_count == 0, non-null) from
        // "error / not found" (nullptr).
        *out_count = 0;
        return static_cast<char**>(std::malloc(1));
    }
    char** list = static_cast<char**>(std::malloc(entries.size() * sizeof(char*)));
    if (!list) return nullptr;
    for (size_t i = 0; i < entries.size(); i++) {
        list[i] = hoo_strdup(entries[i].c_str());
    }
    *out_count = static_cast<int64_t>(entries.size());
    return list;
}

void hoo_fs_free_list(char** list, int64_t count)
{
    if (!list) return;
    for (int64_t i = 0; i < count; i++) {
        if (list[i]) std::free(list[i]);
    }
    std::free(list);
}

char* hoo_fs_temp_dir(void)
{
    std::string tmp = hoo::fs::Path::getTempDir();
    if (tmp.empty()) return nullptr;
    return hoo_strdup(tmp.c_str());
}

char* hoo_fs_create_temp_file(const char* prefix)
{
    std::string tmp = hoo::fs::Path::createTempFile(prefix ? prefix : "hoo");
    if (tmp.empty()) return nullptr;
    return hoo_strdup(tmp.c_str());
}

void hoo_fs_free_string(char* str)
{
    std::free(str);
}

// ── hoo_path_* C-ABI bridges (delegate to hoo::fs::Path) ────────────────────

char* hoo_path_dirname(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path::dirname(path);
    return hoo_strdup(result.c_str());
}

char* hoo_path_basename(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path::basename(path);
    return hoo_strdup(result.c_str());
}

char* hoo_path_extension(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path::extension(path);
    return hoo_strdup(result.c_str());
}

char* hoo_path_stem(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path::stem(path);
    return hoo_strdup(result.c_str());
}

char* hoo_path_root(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path::root(path);
    return hoo_strdup(result.c_str());
}

char* hoo_path_join(const char* a, const char* b)
{
    if (!a || !b) return nullptr;
    std::string result = hoo::fs::Path::join(a, b);
    return hoo_strdup(result.c_str());
}

char* hoo_path_join_multi(const char** parts, int64_t count)
{
    if (!parts || count <= 0) return nullptr;
    std::vector<std::string> vec;
    vec.reserve(static_cast<size_t>(count));
    for (int64_t i = 0; i < count; i++) {
        vec.push_back(parts[i] ? parts[i] : "");
    }
    std::string result = hoo::fs::Path::joinMulti(vec);
    return hoo_strdup(result.c_str());
}

char* hoo_path_normalize(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path::normalize(path);
    return hoo_strdup(result.c_str());
}

char* hoo_path_absolute(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path::absolute(path);
    return hoo_strdup(result.c_str());
}

char* hoo_path_relative(const char* path, const char* base)
{
    if (!path || !base) return nullptr;
    std::string result = hoo::fs::Path::relative(path, base);
    return hoo_strdup(result.c_str());
}

int64_t hoo_path_is_absolute(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Path::isAbsolute(path) ? 1 : 0;
}

int64_t hoo_path_is_relative(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Path::isRelative(path) ? 1 : 0;
}

int64_t hoo_path_has_extension(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Path::hasExtension(path) ? 1 : 0;
}

int64_t hoo_path_has_root(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Path::hasRoot(path) ? 1 : 0;
}

char** hoo_path_split(const char* path, int64_t* out_count)
{
    if (!path || !out_count) return nullptr;
    std::vector<std::string> components = hoo::fs::Path::split(path);
    if (components.empty()) {
        *out_count = 0;
        return nullptr;
    }
    char** result = static_cast<char**>(std::malloc(components.size() * sizeof(char*)));
    if (!result) return nullptr;
    for (size_t i = 0; i < components.size(); i++) {
        result[i] = hoo_strdup(components[i].c_str());
    }
    *out_count = static_cast<int64_t>(components.size());
    return result;
}

void hoo_path_free_parts(char** parts, int64_t count)
{
    if (!parts) return;
    for (int64_t i = 0; i < count; i++) {
        if (parts[i]) std::free(parts[i]);
    }
    std::free(parts);
}

char hoo_path_separator(void)
{
    return hoo::fs::Path::separator();
}

char hoo_path_list_separator(void)
{
    return hoo::fs::Path::listSeparator();
}

void hoo_path_free_string(char* str)
{
    std::free(str);
}

} // extern "C"
