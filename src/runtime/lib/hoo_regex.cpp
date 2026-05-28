#include "hoo_regex.h"
#include <regex>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <unordered_map>

static thread_local std::string g_regex_error;

static std::mutex g_ref_mutex;
static std::unordered_map<std::regex*, int> g_ref_counts;

static void track_ref(std::regex* re) {
    std::lock_guard<std::mutex> lock(g_ref_mutex);
    g_ref_counts[re] = 1;
}

const char* hoo_regex_error(void) {
    return g_regex_error.empty() ? nullptr : g_regex_error.c_str();
}

HooRegex hoo_regex_compile(const char* pattern) {
    try {
        std::regex* re = new std::regex(pattern);
        track_ref(re);
        return reinterpret_cast<HooRegex>(re);
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    }
}

HooRegex hoo_regex_compile_with_flags(const char* pattern, const char* flags) {
    try {
        std::regex::flag_type flag = std::regex::ECMAScript;
        if (flags) {
            for (const char* f = flags; *f; ++f) {
                switch (*f) {
                    case 'i': flag |= std::regex::icase; break;
                    case 'm': flag |= std::regex::multiline; break;
                    case 's': /* dotall not available in C++17 */ break;
                    default: break;
                }
            }
        }
        std::regex* re = new std::regex(pattern, flag);
        track_ref(re);
        return reinterpret_cast<HooRegex>(re);
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    }
}

int64_t hoo_regex_match(HooRegex re, const char* str) {
    try {
        return std::regex_match(str, *reinterpret_cast<std::regex*>(re)) ? 1 : 0;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return -1;
    }
}

int64_t hoo_regex_search(HooRegex re, const char* str) {
    try {
        return std::regex_search(str, *reinterpret_cast<std::regex*>(re)) ? 1 : 0;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return -1;
    }
}

char* hoo_regex_find(HooRegex re, const char* str) {
    try {
        std::string s(str);
        std::smatch m;
        if (std::regex_search(s, m, *reinterpret_cast<std::regex*>(re))) {
            return strdup(m.str().c_str());
        }
        return nullptr;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    }
}

int64_t hoo_regex_find_all(HooRegex re, const char* str, char*** out_matches, int64_t* out_count) {
    try {
        std::string s(str);
        std::sregex_iterator it(s.begin(), s.end(), *reinterpret_cast<std::regex*>(re));
        std::sregex_iterator end;

        std::vector<char*> matches;
        for (; it != end; ++it) {
            matches.push_back(strdup(it->str().c_str()));
        }

        *out_count = static_cast<int64_t>(matches.size());
        *out_matches = static_cast<char**>(std::malloc(matches.size() * sizeof(char*)));
        for (size_t i = 0; i < matches.size(); ++i) {
            (*out_matches)[i] = matches[i];
        }
        return 0;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        *out_count = 0;
        *out_matches = nullptr;
        return -1;
    }
}

char* hoo_regex_replace(HooRegex re, const char* str, const char* replacement) {
    try {
        std::string result = std::regex_replace(str, *reinterpret_cast<std::regex*>(re), replacement);
        return strdup(result.c_str());
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        return nullptr;
    }
}

char** hoo_regex_split(HooRegex re, const char* str, int64_t* out_count) {
    try {
        std::string s(str);
        std::sregex_token_iterator it(s.begin(), s.end(), *reinterpret_cast<std::regex*>(re), -1);
        std::sregex_token_iterator end;

        std::vector<char*> parts;
        for (; it != end; ++it) {
            parts.push_back(strdup(it->str().c_str()));
        }

        *out_count = static_cast<int64_t>(parts.size());
        char** result = static_cast<char**>(std::malloc(parts.size() * sizeof(char*)));
        for (size_t i = 0; i < parts.size(); ++i) {
            result[i] = parts[i];
        }
        return result;
    } catch (const std::regex_error& e) {
        g_regex_error = e.what();
        *out_count = 0;
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
    try {
        std::string s(str);
        std::smatch m;
        if (std::regex_search(s, m, *reinterpret_cast<std::regex*>(re))) {
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
    std::regex* regex = reinterpret_cast<std::regex*>(re);
    std::lock_guard<std::mutex> lock(g_ref_mutex);
    g_ref_counts[regex]++;
    return re;
}

void hoo_regex_release(HooRegex re) {
    if (!re) return;
    std::regex* regex = reinterpret_cast<std::regex*>(re);
    bool delete_it = false;
    {
        std::lock_guard<std::mutex> lock(g_ref_mutex);
        auto it = g_ref_counts.find(regex);
        if (it != g_ref_counts.end()) {
            if (--it->second <= 0) {
                g_ref_counts.erase(it);
                delete_it = true;
            }
        } else {
            delete_it = true;
        }
    }
    if (delete_it) {
        delete regex;
    }
}

void hoo_regex_free_string(char* str) {
    std::free(str);
}
