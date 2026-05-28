#include "hoo_path.h"
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

namespace fs = std::filesystem;

extern "C" {

// ============================================================================
// Component Extraction
// ============================================================================

char* hoo_path_dirname(const char* path)
{
    try {
        fs::path p(path);
        fs::path parent = p.parent_path();
        if (parent.empty())
            return strdup(".");
        return strdup(parent.string().c_str());
    } catch (...) {
        return NULL;
    }
}

char* hoo_path_basename(const char* path)
{
    try {
        std::string pstr(path);
        while (pstr.size() > 1 && pstr.back() == '/')
            pstr.pop_back();
        return strdup(fs::path(pstr).filename().string().c_str());
    } catch (...) {
        return NULL;
    }
}

char* hoo_path_extension(const char* path)
{
    try {
        return strdup(fs::path(path).extension().string().c_str());
    } catch (...) {
        return NULL;
    }
}

char* hoo_path_stem(const char* path)
{
    try {
        return strdup(fs::path(path).stem().string().c_str());
    } catch (...) {
        return NULL;
    }
}

char* hoo_path_root(const char* path)
{
    try {
        return strdup(fs::path(path).root_path().string().c_str());
    } catch (...) {
        return NULL;
    }
}

// ============================================================================
// Construction
// ============================================================================

char* hoo_path_join(const char* a, const char* b)
{
    try {
        return strdup((fs::path(a) / b).string().c_str());
    } catch (...) {
        return NULL;
    }
}

char* hoo_path_join_multi(const char** parts, int64_t count)
{
    try {
        fs::path result;
        for (int64_t i = 0; i < count; i++) {
            result /= parts[i];
        }
        return strdup(result.string().c_str());
    } catch (...) {
        return NULL;
    }
}

// ============================================================================
// Normalization
// ============================================================================

char* hoo_path_normalize(const char* path)
{
    try {
        return strdup(fs::path(path).lexically_normal().string().c_str());
    } catch (...) {
        return NULL;
    }
}

char* hoo_path_absolute(const char* path)
{
    try {
        return strdup(fs::absolute(path).string().c_str());
    } catch (...) {
        return NULL;
    }
}

char* hoo_path_relative(const char* path, const char* base)
{
    try {
        return strdup(fs::relative(path, base).string().c_str());
    } catch (...) {
        return NULL;
    }
}

// ============================================================================
// Properties
// ============================================================================

int64_t hoo_path_is_absolute(const char* path)
{
    try {
        return fs::path(path).is_absolute() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_path_is_relative(const char* path)
{
    try {
        return !fs::path(path).is_absolute() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_path_has_extension(const char* path)
{
    try {
        return fs::path(path).has_extension() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_path_has_root(const char* path)
{
    try {
        return fs::path(path).has_root_path() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

// ============================================================================
// Split
// ============================================================================

char** hoo_path_split(const char* path, int64_t* out_count)
{
    try {
        fs::path p(path);
        std::vector<std::string> components;
        for (const auto& part : p) {
            components.push_back(part.string());
        }
        char** result = (char**)std::malloc(components.size() * sizeof(char*));
        if (!result) return NULL;
        for (size_t i = 0; i < components.size(); i++) {
            result[i] = strdup(components[i].c_str());
        }
        *out_count = (int64_t)components.size();
        return result;
    } catch (...) {
        return NULL;
    }
}

void hoo_path_free_parts(char** parts, int64_t count)
{
    if (!parts) return;
    for (int64_t i = 0; i < count; i++) {
        if (parts[i]) std::free(parts[i]);
    }
    std::free(parts);
}

// ============================================================================
// Platform-Specific
// ============================================================================

char hoo_path_separator(void)
{
    return fs::path::preferred_separator;
}

char hoo_path_list_separator(void)
{
#ifdef _WIN32
    return ';';
#else
    return ':';
#endif
}

// ============================================================================
// Memory Management
// ============================================================================

void hoo_path_free_string(char* str)
{
    std::free(str);
}

} // extern "C"
