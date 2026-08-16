#include "runtime/lib/fs/hoo_fs.h"
#include "runtime/lib/buffer/hoo_buffer.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <random>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

#ifdef _MSC_VER
#define hoo_strdup _strdup
#else
#define hoo_strdup strdup
#endif

// ── Internal helpers ────────────────────────────────────────────────────────
namespace {

int random_int() noexcept
{
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    thread_local std::uniform_int_distribution<> dis(0, 35);
    return dis(gen);
}

bool write_bytes_impl(const std::string& path, const uint8_t* data, std::streamsize len) noexcept
{
    try {
        std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        if (len > 0 && data) {
            file.write(reinterpret_cast<const char*>(data), len);
        }
        file.close();
        return file.good();
    } catch (...) {
        return false;
    }
}

} // anonymous namespace

// ============================================================================
// Free functions
// ============================================================================

namespace hoo { namespace fs {

std::string join(const std::string& a, const std::string& b)
{
    try {
        return (::fs::path(a) / b).string();
    } catch (...) {
        return {};
    }
}

std::string joinMulti(const std::vector<std::string>& parts)
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

std::string relative(const std::string& path, const std::string& base)
{
    try {
        return ::fs::relative(path, base).string();
    } catch (...) {
        return {};
    }
}

char separator()
{
    return ::fs::path::preferred_separator;
}

char listSeparator()
{
#ifdef _WIN32
    return ';';
#else
    return ':';
#endif
}

std::string tempDir()
{
    try {
        return ::fs::temp_directory_path().string();
    } catch (...) {
        return {};
    }
}

std::string createTempFile(const std::string& prefix)
{
    try {
        ::fs::path dir = ::fs::temp_directory_path();
        const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
        for (int attempt = 0; attempt < 256; attempt++) {
            std::string name = prefix;
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

std::string createTempDir()
{
    try {
        ::fs::path dir = ::fs::temp_directory_path();
        const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
        for (int attempt = 0; attempt < 256; attempt++) {
            std::string name = "hoo";
            for (int i = 0; i < 6; i++) {
                name += chars[random_int()];
            }
            ::fs::path tmp = dir / name;
            if (::fs::create_directory(tmp)) {
                return tmp.string();
            }
        }
        return {};
    } catch (...) {
        return {};
    }
}

std::string currentDir()
{
    try {
        return ::fs::current_path().string();
    } catch (...) {
        return {};
    }
}

std::string currentExeDir()
{
    std::string exe;
#ifdef _WIN32
    char buf[MAX_PATH];
    const DWORD n = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return {};
    exe = buf;
#elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return {};
    exe = buf;
    char resolved[PATH_MAX];
    if (::realpath(buf, resolved) != nullptr) exe = resolved;
#else
    char buf[PATH_MAX];
    const ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return {};
    buf[n] = '\0';
    exe = buf;
#endif
    try {
        ::fs::path p(exe);
        ::fs::path parent = p.parent_path();
        return parent.empty() ? std::string() : parent.string();
    } catch (...) {
        return {};
    }
}

bool copyFile(const std::string& src, const std::string& dst)
{
    try {
        ::fs::copy(src, dst, ::fs::copy_options::overwrite_existing);
        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// Path
// ============================================================================

std::string Path::dirname() const
{
    try {
        ::fs::path p(path_);
        ::fs::path parent = p.parent_path();
        if (parent.empty())
            return ".";
        return parent.string();
    } catch (...) {
        return {};
    }
}

std::string Path::basename() const
{
    try {
        std::string pstr = path_;
        while (pstr.size() > 1) {
            char c = pstr.back();
#ifdef _WIN32
            if (c != '/' && c != '\\') break;
#else
            if (c != '/') break;
#endif
            pstr.pop_back();
        }
        return ::fs::path(pstr).filename().string();
    } catch (...) {
        return {};
    }
}

std::string Path::extension() const
{
    try {
        return ::fs::path(path_).extension().string();
    } catch (...) {
        return {};
    }
}

std::string Path::stem() const
{
    try {
        return ::fs::path(path_).stem().string();
    } catch (...) {
        return {};
    }
}

std::string Path::root() const
{
    try {
        return ::fs::path(path_).root_path().string();
    } catch (...) {
        return {};
    }
}

Path Path::normalized() const
{
    try {
        return Path(::fs::path(path_).lexically_normal().string());
    } catch (...) {
        return Path("");
    }
}

Path Path::absolute() const
{
    try {
        return Path(::fs::absolute(path_).string());
    } catch (...) {
        return Path("");
    }
}

bool Path::isAbsolute() const
{
    try {
        return ::fs::path(path_).is_absolute();
    } catch (...) {
        return false;
    }
}

bool Path::isRelative() const
{
    try {
        return !::fs::path(path_).is_absolute();
    } catch (...) {
        return false;
    }
}

bool Path::hasExtension() const
{
    try {
        return ::fs::path(path_).has_extension();
    } catch (...) {
        return false;
    }
}

bool Path::hasRoot() const
{
    try {
        return ::fs::path(path_).has_root_path();
    } catch (...) {
        return false;
    }
}

std::vector<std::string> Path::split() const
{
    try {
        std::vector<std::string> components;
        for (const auto& part : ::fs::path(path_)) {
            components.push_back(part.string());
        }
        return components;
    } catch (...) {
        return {};
    }
}

// ============================================================================
// File
// ============================================================================

bool File::exists() const
{
    try { return ::fs::exists(path_); } catch (...) { return false; }
}

bool File::isFile() const
{
    try { return ::fs::is_regular_file(path_); } catch (...) { return false; }
}

int64_t File::size() const
{
    try { return static_cast<int64_t>(::fs::file_size(path_)); } catch (...) { return -1; }
}

int64_t File::lastModified() const
{
    try {
        auto ft = ::fs::last_write_time(path_);
        auto s = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ft - ::fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        return static_cast<int64_t>(std::chrono::system_clock::to_time_t(s));
    } catch (...) {
        return -1;
    }
}

bool File::remove()
{
    try { return ::fs::remove(path_); } catch (...) { return false; }
}

bool File::rename(const std::string& newPath)
{
    try {
        ::fs::rename(path_, newPath);
        path_ = newPath;
        return true;
    } catch (...) {
        return false;
    }
}

std::string File::readText() const
{
    try {
        std::ifstream file(path_, std::ios::in);
        if (!file.is_open()) return {};
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    } catch (...) {
        return {};
    }
}

bool File::writeText(const std::string& content)
{
    try {
        std::ofstream file(path_, std::ios::out | std::ios::trunc);
        if (!file.is_open()) return false;
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        file.close();
        return file.good();
    } catch (...) {
        return false;
    }
}

bool File::appendText(const std::string& content)
{
    try {
        std::ofstream file(path_, std::ios::out | std::ios::app);
        if (!file.is_open()) return false;
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        file.close();
        return file.good();
    } catch (...) {
        return false;
    }
}

bool File::readBytes(std::vector<uint8_t>& outData) const
{
    try {
        std::ifstream file(path_, std::ios::in | std::ios::binary | std::ios::ate);
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

bool File::writeBytes(const std::vector<uint8_t>& data)
{
    return write_bytes_impl(path_, data.data(), static_cast<std::streamsize>(data.size()));
}

// ============================================================================
// Directory
// ============================================================================

bool Directory::exists() const
{
    try { return ::fs::exists(path_); } catch (...) { return false; }
}

bool Directory::isDirectory() const
{
    try { return ::fs::is_directory(path_); } catch (...) { return false; }
}

bool Directory::create()
{
    try { return ::fs::create_directory(path_); } catch (...) { return false; }
}

bool Directory::createTree()
{
    try { return ::fs::create_directories(path_); } catch (...) { return false; }
}

bool Directory::remove()
{
    // fs_rmdir must only remove directories, never regular files.
    try {
        if (!::fs::is_directory(path_)) return false;
        return ::fs::remove(path_);
    } catch (...) { return false; }
}

std::vector<std::string> Directory::list() const
{
    try {
        std::vector<std::string> entries;
        for (const auto& entry : ::fs::directory_iterator(path_)) {
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
    if (!path) return 0;
    return hoo::fs::File(path).exists() ? 1 : 0;
}

int64_t hoo_fs_is_file(const char* path)
{
    if (!path) return 0;
    return hoo::fs::File(path).isFile() ? 1 : 0;
}

int64_t hoo_fs_is_dir(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Directory(path).isDirectory() ? 1 : 0;
}

int64_t hoo_fs_size(const char* path)
{
    if (!path) return -1;
    return hoo::fs::File(path).size();
}

int64_t hoo_fs_last_modified(const char* path)
{
    if (!path) return -1;
    return hoo::fs::File(path).lastModified();
}

int64_t hoo_fs_delete(const char* path)
{
    if (!path) return 0;
    return hoo::fs::File(path).remove() ? 1 : 0;
}

int64_t hoo_fs_remove(const char* path)
{
    return hoo_fs_delete(path);
}

int64_t hoo_fs_rename(const char* old_path, const char* new_path)
{
    if (!old_path || !new_path) return 0;
    return hoo::fs::File(old_path).rename(new_path) ? 1 : 0;
}

int64_t hoo_fs_move(const char* old_path, const char* new_path)
{
    return hoo_fs_rename(old_path, new_path);
}

int64_t hoo_fs_copy(const char* src, const char* dst)
{
    if (!src || !dst) return 0;
    return hoo::fs::copyFile(src, dst) ? 1 : 0;
}

char* hoo_fs_read_text(const char* path)
{
    if (!path) return nullptr;
    std::string content = hoo::fs::File(path).readText();
    // readText() returns {} both when the file is missing/unreadable and when
    // it exists but is empty. Distinguish: a path that is not a regular file
    // (missing, a directory, unreadable) yields null; an existing empty
    // regular file yields an empty string ("").
    if (content.empty() && !hoo::fs::File(path).isFile()) return nullptr;
    char* result = static_cast<char*>(std::malloc(content.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, content.data(), content.size() + 1);
    return result;
}

int64_t hoo_fs_write_text(const char* path, const char* content)
{
    if (!path) return 0;
    return hoo::fs::File(path).writeText(content ? content : "") ? 1 : 0;
}

int64_t hoo_fs_append_text(const char* path, const char* content)
{
    if (!path) return 0;
    return hoo::fs::File(path).appendText(content ? content : "") ? 1 : 0;
}

int64_t hoo_fs_read_bytes(const char* path, uint8_t** out_data, int64_t* out_len)
{
    if (!path || !out_data || !out_len) return 0;
    std::vector<uint8_t> buf;
    if (!hoo::fs::File(path).readBytes(buf)) return 0;
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
    if (!path) return 0;
    return hoo::fs::Directory(path).create() ? 1 : 0;
}

int64_t hoo_fs_mkdirs(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Directory(path).createTree() ? 1 : 0;
}

int64_t hoo_fs_rmdir(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Directory(path).remove() ? 1 : 0;
}

char** hoo_fs_list_dir(const char* path, int64_t* out_count)
{
    if (!path || !out_count) return nullptr;
    // Missing, unreadable, or not-a-directory: signal null with count -1 so
    // callers can distinguish "empty directory" (count 0) from "cannot list".
    if (!hoo::fs::Directory(path).isDirectory()) {
        *out_count = -1;
        return nullptr;
    }
    std::vector<std::string> entries = hoo::fs::Directory(path).list();
    if (entries.empty()) {
        *out_count = 0;
        return nullptr;
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
    std::string tmp = hoo::fs::tempDir();
    if (tmp.empty()) return nullptr;
    return hoo_strdup(tmp.c_str());
}

char* hoo_fs_create_temp_dir(void)
{
    std::string tmp = hoo::fs::createTempDir();
    if (tmp.empty()) return nullptr;
    return hoo_strdup(tmp.c_str());
}

char* hoo_fs_create_temp_file(const char* prefix)
{
    std::string tmp = hoo::fs::createTempFile(prefix ? prefix : "hoo");
    if (tmp.empty()) return nullptr;
    return hoo_strdup(tmp.c_str());
}

char* hoo_fs_current_dir(void)
{
    std::string cwd = hoo::fs::currentDir();
    if (cwd.empty()) return nullptr;
    return hoo_strdup(cwd.c_str());
}

char* hoo_fs_current_exe_dir(void)
{
    std::string dir = hoo::fs::currentExeDir();
    if (dir.empty()) return nullptr;
    return hoo_strdup(dir.c_str());
}

void hoo_fs_free_string(char* str)
{
    std::free(str);
}

// ── hoo_path_* C-ABI bridges (delegate to hoo::fs API) ─────────────────────

char* hoo_path_dirname(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path(path).dirname();
    return hoo_strdup(result.c_str());
}

char* hoo_path_basename(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path(path).basename();
    return hoo_strdup(result.c_str());
}

char* hoo_path_filename(const char* path)
{
    return hoo_path_basename(path);
}

char* hoo_path_parent(const char* path)
{
    return hoo_path_dirname(path);
}

char* hoo_path_extension(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path(path).extension();
    return hoo_strdup(result.c_str());
}

char* hoo_path_stem(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path(path).stem();
    return hoo_strdup(result.c_str());
}

char* hoo_path_root(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path(path).root();
    return hoo_strdup(result.c_str());
}

char* hoo_path_join(const char* a, const char* b)
{
    if (!a || !b) return nullptr;
    std::string result = hoo::fs::join(a, b);
    return !result.empty() ? hoo_strdup(result.c_str()) : nullptr;
}

char* hoo_path_join_multi(const char** parts, int64_t count)
{
    if (!parts || count <= 0) return nullptr;
    std::vector<std::string> vec;
    vec.reserve(static_cast<size_t>(count));
    for (int64_t i = 0; i < count; i++) {
        vec.push_back(parts[i] ? parts[i] : "");
    }
    std::string result = hoo::fs::joinMulti(vec);
    return !result.empty() ? hoo_strdup(result.c_str()) : nullptr;
}

char* hoo_path_normalize(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path(path).normalized().str();
    return !result.empty() ? hoo_strdup(result.c_str()) : nullptr;
}

char* hoo_path_absolute(const char* path)
{
    if (!path) return nullptr;
    std::string result = hoo::fs::Path(path).absolute().str();
    return !result.empty() ? hoo_strdup(result.c_str()) : nullptr;
}

char* hoo_path_relative(const char* path, const char* base)
{
    if (!path || !base) return nullptr;
    std::string result = hoo::fs::relative(path, base);
    return !result.empty() ? hoo_strdup(result.c_str()) : nullptr;
}

int64_t hoo_path_is_absolute(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Path(path).isAbsolute() ? 1 : 0;
}

int64_t hoo_path_is_relative(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Path(path).isRelative() ? 1 : 0;
}

int64_t hoo_path_has_extension(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Path(path).hasExtension() ? 1 : 0;
}

int64_t hoo_path_has_root(const char* path)
{
    if (!path) return 0;
    return hoo::fs::Path(path).hasRoot() ? 1 : 0;
}

char** hoo_path_split(const char* path, int64_t* out_count)
{
    if (!path || !out_count) return nullptr;
    std::vector<std::string> components = hoo::fs::Path(path).split();
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
    return hoo::fs::separator();
}

char hoo_path_list_separator(void)
{
    return hoo::fs::listSeparator();
}

void hoo_path_free_string(char* str)
{
    std::free(str);
}

// ── Buffer overloads ─────────────────────────────────────────────────────────

int64_t hoo_fs_write_bytes_buffer(const char* path, HooBuffer buf) {
    return hoo_fs_write_bytes(path, hoo_buffer_data(buf), hoo_buffer_length(buf));
}

HooBuffer hoo_fs_read_bytes_buffer(const char* path) {
    uint8_t* data = nullptr;
    int64_t len = 0;
    int64_t result = hoo_fs_read_bytes(path, &data, &len);
    if (result == 0) return nullptr;
    if (!data) return hoo_buffer_new(0); // empty file -> empty buffer
    HooBuffer buf = hoo_buffer_from_bytes(data, len);
    free(data);
    return buf;
}

} // extern "C"
