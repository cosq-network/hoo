#include "runtime/lib/text/hoo_regex.h"
#include <regex>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>

static thread_local std::string g_regex_error;

// Opaque handle type: embeds the std::regex and its reference count so
// retain/release need no global bookkeeping and are safe across threads.
struct HooRegexImpl {
    std::regex re;
    int refcount;
    HooRegexImpl(const std::string& pattern, std::regex::flag_type flags)
        : re(pattern, flags), refcount(1) {}
};

static inline HooRegexImpl* impl_of(HooRegex re) {
    return reinterpret_cast<HooRegexImpl*>(re);
}

static inline std::regex& re_of(HooRegex re) {
    return impl_of(re)->re;
}

// ECMAScript has no direct dotall flag in std::regex; emulate 's' by
// rewriting each unescaped '.' outside a character class to [\s\S].
static std::string apply_dotall(const std::string& pattern) {
    std::string out;
    out.reserve(pattern.size() * 2);
    bool inClass = false;
    for (size_t i = 0; i < pattern.size(); ++i) {
        char c = pattern[i];
        char p = i ? pattern[i - 1] : 0;
        if (c == '.' && !inClass && p != '\\') {
            out += "[\\s\\S]";
            continue;
        }
        out += c;
        if (c == '[' && p != '\\') inClass = true;
        else if (c == ']' && p != '\\') inClass = false;
    }
    return out;
}

const char* hoo_regex_error(void) {
    return g_regex_error.empty() ? nullptr : g_regex_error.c_str();
}

HooRegex hoo_regex_compile(const char* pattern) {
    if (!pattern) {
        g_regex_error = "null pattern";
        return nullptr;
    }
    try {
        return reinterpret_cast<HooRegex>(new HooRegexImpl(pattern, std::regex::ECMAScript));
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    } catch (...) {
        g_regex_error = "regex compilation failed";
        return nullptr;
    }
}

HooRegex hoo_regex_compile_with_flags(const char* pattern, const char* flags) {
    if (!pattern) {
        g_regex_error = "null pattern";
        return nullptr;
    }
    std::regex::flag_type flag = std::regex::ECMAScript;
    bool dotall = false;
    if (flags) {
        for (const char* f = flags; *f; ++f) {
            switch (*f) {
                case 'i': flag |= std::regex::icase; break;
                case 'm': flag |= std::regex::multiline; break;
                // 's': dotall, emulated below by rewriting '.'
                case 's': dotall = true; break;
                // 'g': global; std::regex operations are already
                // global (replace/find_all cover every match).
                case 'g': break;
                default:
                    g_regex_error = "unsupported regex flag";
                    return nullptr;
            }
        }
    }
    std::string pat = pattern;
    if (dotall) {
        pat = apply_dotall(pat);
    }
    try {
        return reinterpret_cast<HooRegex>(new HooRegexImpl(pat, flag));
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    } catch (...) {
        g_regex_error = "regex compilation failed";
        return nullptr;
    }
}

int64_t hoo_regex_match(HooRegex re, const char* str) {
    // On error return 0 (not -1): a negative value would be truthy when
    // interpreted as a boolean by the JIT, masking failures as matches.
    if (!re || !str) {
        g_regex_error = "null regex or string";
        return 0;
    }
    try {
        return std::regex_match(str, re_of(re)) ? 1 : 0;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return 0;
    }
}

int64_t hoo_regex_search(HooRegex re, const char* str) {
    if (!re || !str) {
        g_regex_error = "null regex or string";
        return 0;
    }
    try {
        return std::regex_search(str, re_of(re)) ? 1 : 0;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return 0;
    }
}

char* hoo_regex_find(HooRegex re, const char* str) {
    if (!re || !str) {
        g_regex_error = "null regex or string";
        return nullptr;
    }
    try {
        std::string s(str);
        std::smatch m;
        if (std::regex_search(s, m, re_of(re))) {
            return strdup(m.str().c_str());
        }
        return nullptr;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    }
}

int64_t hoo_regex_find_all(HooRegex re, const char* str, char*** out_matches, int64_t* out_count) {
    if (!out_matches || !out_count) return -1;
    *out_count = 0;
    *out_matches = nullptr;
    if (!re || !str) {
        g_regex_error = "null regex or string";
        return -1;
    }
    try {
        std::string s(str);
        std::sregex_iterator it(s.begin(), s.end(), re_of(re));
        std::sregex_iterator end;

        std::vector<char*> matches;
        for (; it != end; ++it) {
            matches.push_back(strdup(it->str().c_str()));
        }

        if (matches.empty()) return 0;
        *out_count = static_cast<int64_t>(matches.size());
        char** result = static_cast<char**>(std::malloc(matches.size() * sizeof(char*)));
        if (!result) {
            for (char* m : matches) std::free(m);
            g_regex_error = "out of memory allocating match results";
            return -1;
        }
        for (size_t i = 0; i < matches.size(); ++i) {
            result[i] = matches[i];
        }
        *out_matches = result;
        return 0;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return -1;
    }
}

// Returns all capture groups (index 0 is the full match) of the first match.
int64_t hoo_regex_capture(HooRegex re, const char* str, char*** out_groups, int64_t* out_count) {
    if (!out_groups || !out_count) return -1;
    *out_count = 0;
    *out_groups = nullptr;
    if (!re || !str) {
        g_regex_error = "null regex or string";
        return -1;
    }
    try {
        std::string s(str);
        std::smatch m;
        if (std::regex_search(s, m, re_of(re))) {
            std::vector<char*> groups;
            for (size_t i = 0; i < m.size(); ++i) {
                groups.push_back(strdup(m[i].str().c_str()));
            }
            *out_count = static_cast<int64_t>(groups.size());
            char** result = static_cast<char**>(std::malloc(groups.size() * sizeof(char*)));
            if (!result) {
                for (char* g : groups) std::free(g);
                g_regex_error = "out of memory allocating capture groups";
                return -1;
            }
            for (size_t i = 0; i < groups.size(); ++i) {
                result[i] = groups[i];
            }
            *out_groups = result;
        }
        return 0;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return -1;
    }
}

char* hoo_regex_replace(HooRegex re, const char* str, const char* replacement) {
    if (!re || !str || !replacement) {
        g_regex_error = "null regex or string";
        return nullptr;
    }
    try {
        std::string result = std::regex_replace(str, re_of(re), replacement);
        return strdup(result.c_str());
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    }
}

char** hoo_regex_split(HooRegex re, const char* str, int64_t* out_count) {
    if (!out_count) return nullptr;
    *out_count = 0;
    if (!re || !str) {
        g_regex_error = "null regex or string";
        return nullptr;
    }
    try {
        std::string s(str);
        std::sregex_token_iterator it(s.begin(), s.end(), re_of(re), -1);
        std::sregex_token_iterator end;

        std::vector<char*> parts;
        for (; it != end; ++it) {
            parts.push_back(strdup(it->str().c_str()));
        }

        if (parts.empty()) return nullptr;
        *out_count = static_cast<int64_t>(parts.size());
        char** result = static_cast<char**>(std::malloc(parts.size() * sizeof(char*)));
        if (!result) {
            for (char* p : parts) std::free(p);
            g_regex_error = "out of memory allocating split parts";
            return nullptr;
        }
        for (size_t i = 0; i < parts.size(); ++i) {
            result[i] = parts[i];
        }
        return result;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    }
}

void hoo_regex_free_matches(char** matches, int64_t count) {
    if (matches) {
        for (int64_t i = 0; i < count; ++i) {
            std::free(matches[i]);
        }
        std::free(matches);
    }
}

char* hoo_regex_group(HooRegex re, const char* str, int64_t group_index) {
    if (!re || !str) {
        g_regex_error = "null regex or string";
        return nullptr;
    }
    try {
        std::string s(str);
        std::smatch m;
        if (std::regex_search(s, m, re_of(re))) {
            if (group_index >= 0 && group_index < static_cast<int64_t>(m.size()) && m[group_index].matched) {
                return strdup(m[group_index].str().c_str());
            }
        }
        return nullptr;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    }
}

HooRegex hoo_regex_retain(HooRegex re) {
    if (!re) return nullptr;
    ++impl_of(re)->refcount;
    return re;
}

void hoo_regex_release(HooRegex re) {
    if (!re) return;
    HooRegexImpl* impl = impl_of(re);
    if (--impl->refcount <= 0) {
        delete impl;
    }
}

void hoo_regex_free_string(char* str) {
    std::free(str);
}