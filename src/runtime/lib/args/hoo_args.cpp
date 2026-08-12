#include "runtime/lib/args/hoo_args.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cerrno>
#include <climits>
#include <mutex>

extern "C" {

// ── Low-level raw argv parsing (existing) ──────────────────────────────────

struct HooArg {
    const char* key;
    const char* value;
    int64_t index;
    int has_value;
};

struct HooArgsResult {
    HooArg* args;
    int64_t count;
    char* program_name;
    int64_t refs;
};

static HooArgsResult* g_result = NULL;
static std::mutex g_result_mutex;

// ── Argparse-style high-level API types ────────────────────────────────────

struct ArgDef {
    int type;
    char* name;
    char* short_opt;
    char* long_opt;
    char* help;
    char* default_str;
    int64_t default_int;
    double default_float;
    int is_positional;
    int is_required;
    char* parsed_str;
    int64_t parsed_int;
    double parsed_float;
    int parsed_flag;
    int parsed_ok;
};

struct HooArgsHandle {
    HooArgsResult* result;
    ArgDef* defs;
    int64_t def_count;
    int64_t def_capacity;
    int parsed;
    int positional_index;
};

// ── Internal: raw argv parsing ─────────────────────────────────────────────

static HooArgsResult args_parse(int64_t argc, const char* const* argv) {
    HooArgsResult result = {NULL, 0, NULL};

    if (argc > 0 && argv && argv[0]) {
        result.program_name = strdup(argv[0]);
    } else {
        result.program_name = strdup("");
    }

    int64_t capacity = 0;
    int64_t positional_index = 0;
    int positional_mode = 0;

    for (int64_t i = 1; i < argc && argv; i++) {
        const char* token = argv[i] ? argv[i] : "";

        if (!positional_mode && strcmp(token, "--") == 0) {
            positional_mode = 1;
            continue;
        }

        const char* key = NULL;
        const char* value = NULL;
        int64_t idx = -1;
        int has_value = 0;

        if (!positional_mode && token[0] == '-' && token[1] == '-') {
            const char* eq = strchr(token + 2, '=');
            if (eq) {
                size_t key_len = (size_t)(eq - (token + 2));
                char* k = (char*)malloc(key_len + 1);
                memcpy(k, token + 2, key_len);
                k[key_len] = '\0';
                key = k;
                value = strdup(eq + 1);
                has_value = 1;
            } else {
                key = strdup(token + 2);
                if (i + 1 < argc && argv[i + 1] &&
                    (argv[i + 1][0] != '-' ||
                     (argv[i + 1][1] >= '0' && argv[i + 1][1] <= '9') ||
                     argv[i + 1][1] == '.')) {
                    i++;
                    value = strdup(argv[i]);
                    has_value = 1;
                } else {
                    value = strdup("");
                }
            }
        } else if (!positional_mode && token[0] == '-' && token[1] != '\0') {
            char* k = (char*)malloc(2);
            k[0] = token[1];
            k[1] = '\0';
            key = k;
            if (i + 1 < argc && argv[i + 1] &&
                (argv[i + 1][0] != '-' ||
                 (argv[i + 1][1] >= '0' && argv[i + 1][1] <= '9') ||
                 argv[i + 1][1] == '.')) {
                i++;
                value = strdup(argv[i]);
                has_value = 1;
            } else {
                value = strdup("");
            }
        } else {
            key = strdup("");
            value = strdup(token);
            idx = positional_index++;
            has_value = 1;
        }

        if (result.count >= capacity) {
            capacity = capacity ? capacity * 2 : 16;
            result.args = (HooArg*)realloc(result.args, (size_t)capacity * sizeof(HooArg));
        }

        HooArg* arg = &result.args[result.count++];
        arg->key = key;
        arg->value = value;
        arg->index = idx;
        arg->has_value = has_value;
    }

    return result;
}

static void free_result(HooArgsResult* result) {
    if (!result) return;
    for (int64_t i = 0; i < result->count; i++) {
        free((void*)result->args[i].key);
        free((void*)result->args[i].value);
    }
    free(result->args);
    free(result->program_name);
    free(result);
}

static void release_result_locked(HooArgsResult* result) {
    if (!result) return;
    if (--result->refs == 0) free_result(result);
}

static void release_result(HooArgsResult* result) {
    std::lock_guard<std::mutex> lock(g_result_mutex);
    release_result_locked(result);
}

static HooArgsResult* result_for_handle(void* args) {
    if (args) return ((HooArgsHandle*)args)->result;
    return g_result;
}

// ── Existing public API ────────────────────────────────────────────────────

void hoo_args_init(int64_t argc, const char* const* argv) {
    HooArgsResult* result = (HooArgsResult*)malloc(sizeof(HooArgsResult));
    if (!result) return;
    *result = args_parse(argc, argv);
    result->refs = 1; // global owner

    std::lock_guard<std::mutex> lock(g_result_mutex);
    HooArgsResult* old_result = g_result;
    g_result = result;
    release_result_locked(old_result);
}

void hoo_args_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_result_mutex);
    HooArgsResult* result = g_result;
    g_result = NULL;
    release_result_locked(result);
}

void* hoo_args_new(void) {
    HooArgsHandle* handle = (HooArgsHandle*)calloc(1, sizeof(HooArgsHandle));
    if (!handle) return NULL;
    std::lock_guard<std::mutex> lock(g_result_mutex);
    if (g_result) {
        handle->result = g_result;
        handle->result->refs++;
    }
    return handle;
}

void hoo_args_release(void* args) {
    if (!args) return;
    HooArgsHandle* handle = (HooArgsHandle*)args;
    hoo_args_clear(args);
    release_result(handle->result);
    free(args);
}

int64_t hoo_args_count(void* args) {
    HooArgsResult* result = result_for_handle(args);
    if (!result) return 0;
    int64_t count = 0;
    for (int64_t i = 0; i < result->count; i++) {
        if (result->args[i].index >= 0) count++;
    }
    return count;
}

const char* hoo_args_get(void* args, int64_t index) {
    HooArgsResult* result = result_for_handle(args);
    if (!result) return NULL;
    for (int64_t i = 0; i < result->count; i++) {
        if (result->args[i].index == index)
            return result->args[i].value;
    }
    return NULL;
}

int64_t hoo_args_has(void* args, const char* key) {
    HooArgsResult* result = result_for_handle(args);
    if (!result || !key) return 0;
    for (int64_t i = 0; i < result->count; i++) {
        if (result->args[i].key && strcmp(result->args[i].key, key) == 0)
            return 1;
    }
    return 0;
}

const char* hoo_args_value(void* args, const char* key) {
    HooArgsResult* result = result_for_handle(args);
    if (!result || !key) return NULL;
    for (int64_t i = 0; i < result->count; i++) {
        if (result->args[i].key && strcmp(result->args[i].key, key) == 0)
            return result->args[i].value;
    }
    return NULL;
}

const char* hoo_args_program_name(void* args) {
    HooArgsResult* result = result_for_handle(args);
    if (!result) return "";
    return result->program_name ? result->program_name : "";
}

// ── Internal: argparse helpers ─────────────────────────────────────────────

static void ensure_capacity(HooArgsHandle* handle) {
    if (handle->def_count >= handle->def_capacity) {
        int64_t new_cap = handle->def_capacity ? handle->def_capacity * 2 : 8;
        handle->defs = (ArgDef*)realloc(handle->defs, (size_t)new_cap * sizeof(ArgDef));
        memset(handle->defs + handle->def_capacity, 0,
               (size_t)(new_cap - handle->def_capacity) * sizeof(ArgDef));
        handle->def_capacity = new_cap;
    }
}

static int parse_int64(const char* s, int64_t* out) {
    if (!s || !*s || !out) return 0;
    errno = 0;
    char* end = NULL;
    int64_t val = strtoll(s, &end, 10);
    if (errno == ERANGE || !end || *end != '\0') return 0;
    *out = val;
    return 1;
}

static int parse_double(const char* s, double* out) {
    if (!s || !*s || !out) return 0;
    errno = 0;
    char* end = NULL;
    double val = strtod(s, &end);
    if (errno == ERANGE || !end || *end != '\0' || !std::isfinite(val)) return 0;
    *out = val;
    return 1;
}

static const HooArg* find_named_arg(HooArgsResult* result, const char* key) {
    if (!result || !key) return NULL;
    for (int64_t i = 0; i < result->count; i++) {
        if (result->args[i].key && strcmp(result->args[i].key, key) == 0)
            return &result->args[i];
    }
    return NULL;
}

// ── New argparse-style API ─────────────────────────────────────────────────

void hoo_args_add_arg(void* args, int type,
                      const char* name,
                      const char* short_opt,
                      const char* long_opt,
                      const char* help,
                      const char* default_str,
                      int64_t default_int,
                      double default_float,
                      int is_positional) {
    if (!args || !name) return;
    HooArgsHandle* handle = (HooArgsHandle*)args;
    ensure_capacity(handle);
    ArgDef* def = &handle->defs[handle->def_count++];
    def->type = type;
    def->name = strdup(name);
    def->short_opt = short_opt ? strdup(short_opt) : strdup("");
    def->long_opt = long_opt ? strdup(long_opt) : strdup("");
    def->help = help ? strdup(help) : strdup("");
    def->default_str = default_str ? strdup(default_str) : strdup("");
    def->default_int = default_int;
    def->default_float = default_float;
    def->is_positional = is_positional;
    def->is_required = 0;
    def->parsed_str = NULL;
    def->parsed_int = 0;
    def->parsed_float = 0.0;
    def->parsed_flag = 0;
    def->parsed_ok = 0;
}

void hoo_args_add_string(void* args, const char* name,
                         const char* short_opt, const char* long_opt,
                         const char* help, const char* default_val) {
    hoo_args_add_arg(args, HOO_ARG_STRING, name, short_opt, long_opt, help,
                     default_val, 0, 0.0, 0);
}

void hoo_args_add_int(void* args, const char* name,
                      const char* short_opt, const char* long_opt,
                      const char* help, int64_t default_val) {
    hoo_args_add_arg(args, HOO_ARG_INT, name, short_opt, long_opt, help,
                     NULL, default_val, 0.0, 0);
}

void hoo_args_add_flag(void* args, const char* name,
                       const char* short_opt, const char* long_opt,
                       const char* help) {
    hoo_args_add_arg(args, HOO_ARG_FLAG, name, short_opt, long_opt, help,
                     NULL, 0, 0.0, 0);
}

void hoo_args_add_float(void* args, const char* name,
                        const char* short_opt, const char* long_opt,
                        const char* help, double default_val) {
    hoo_args_add_arg(args, HOO_ARG_FLOAT, name, short_opt, long_opt, help,
                     NULL, 0, default_val, 0);
}

void hoo_args_add_positional(void* args, const char* name, const char* help) {
    hoo_args_add_arg(args, HOO_ARG_STRING, name, "", "", help,
                     NULL, 0, 0.0, 1);
}

int64_t hoo_args_set_required(void* args, const char* name, int64_t required) {
    if (!args || !name) return 0;
    HooArgsHandle* handle = (HooArgsHandle*)args;
    for (int64_t i = 0; i < handle->def_count; i++) {
        if (handle->defs[i].name && strcmp(handle->defs[i].name, name) == 0) {
            handle->defs[i].is_required = required != 0;
            return 1;
        }
    }
    return 0;
}

int64_t hoo_args_parse(void* args) {
    HooArgsHandle* handle = (HooArgsHandle*)args;
    HooArgsResult* result = result_for_handle(args);
    if (!handle || !result) return 0;

    handle->parsed = 0;
    handle->positional_index = 0;
    for (int64_t i = 0; i < handle->def_count; i++) {
        free(handle->defs[i].parsed_str);
        handle->defs[i].parsed_str = NULL;
        handle->defs[i].parsed_int = 0;
        handle->defs[i].parsed_float = 0.0;
        handle->defs[i].parsed_flag = 0;
        handle->defs[i].parsed_ok = 0;
    }

    for (int64_t i = 0; i < result->count; i++) {
        if (result->args[i].key &&
            (strcmp(result->args[i].key, "help") == 0)) {
            handle->parsed = 1;
            return 0;
        }
    }

    int pos_idx = 0;
    for (int64_t i = 0; i < handle->def_count; i++) {
        ArgDef* def = &handle->defs[i];
        const char* raw_value = NULL;
        int found = 0;
        int has_value = 0;

        if (def->is_positional) {
            raw_value = hoo_args_get(args, pos_idx);
            if (raw_value) {
                found = 1;
                has_value = 1;
                pos_idx++;
            }
        } else {
            if (def->long_opt && def->long_opt[0] != '\0') {
                const char* lookup = def->long_opt;
                if (lookup[0] == '-' && lookup[1] == '-') lookup += 2;
                const HooArg* raw = find_named_arg(result, lookup);
                if (raw) {
                    raw_value = raw->value;
                    found = 1;
                    has_value = raw->has_value;
                }
            }
            if (!found && def->short_opt && def->short_opt[0] != '\0') {
                const char* lookup = def->short_opt;
                if (lookup[0] == '-') lookup += 1;
                const HooArg* raw = find_named_arg(result, lookup);
                if (raw) {
                    raw_value = raw->value;
                    found = 1;
                    has_value = raw->has_value;
                }
            }
        }

        if (def->is_required && !found) return 0;

        if (def->type == HOO_ARG_FLAG) {
            def->parsed_flag = found ? 1 : 0;
            def->parsed_ok = 1;
        } else if (found) {
            if (!has_value) return 0;
            def->parsed_str = strdup(raw_value);
            if (def->type == HOO_ARG_INT && !parse_int64(raw_value, &def->parsed_int)) return 0;
            if (def->type == HOO_ARG_FLOAT && !parse_double(raw_value, &def->parsed_float)) return 0;
            def->parsed_ok = 1;
        } else {
            def->parsed_str = strdup(def->default_str ? def->default_str : "");
            def->parsed_int = def->default_int;
            def->parsed_float = def->default_float;
            def->parsed_ok = 1;
        }
    }

    handle->parsed = 1;
    handle->positional_index = pos_idx;
    return 1;
}

const char* hoo_args_get_string(void* args, const char* name) {
    if (!args || !name) return "";
    HooArgsHandle* handle = (HooArgsHandle*)args;
    for (int64_t i = 0; i < handle->def_count; i++) {
        if (handle->defs[i].name && strcmp(handle->defs[i].name, name) == 0) {
            if (handle->defs[i].parsed_str)
                return handle->defs[i].parsed_str;
            if (handle->defs[i].default_str)
                return handle->defs[i].default_str;
            return "";
        }
    }
    return "";
}

int64_t hoo_args_get_int(void* args, const char* name) {
    if (!args || !name) return 0;
    HooArgsHandle* handle = (HooArgsHandle*)args;
    for (int64_t i = 0; i < handle->def_count; i++) {
        if (handle->defs[i].name && strcmp(handle->defs[i].name, name) == 0) {
            return handle->defs[i].parsed_int;
        }
    }
    return 0;
}

int64_t hoo_args_get_bool(void* args, const char* name) {
    if (!args || !name) return 0;
    HooArgsHandle* handle = (HooArgsHandle*)args;
    for (int64_t i = 0; i < handle->def_count; i++) {
        if (handle->defs[i].name && strcmp(handle->defs[i].name, name) == 0) {
            return handle->defs[i].parsed_flag;
        }
    }
    return 0;
}

double hoo_args_get_float(void* args, const char* name) {
    if (!args || !name) return 0.0;
    HooArgsHandle* handle = (HooArgsHandle*)args;
    for (int64_t i = 0; i < handle->def_count; i++) {
        if (handle->defs[i].name && strcmp(handle->defs[i].name, name) == 0) {
            return handle->defs[i].parsed_float;
        }
    }
    return 0.0;
}

char* hoo_args_help_text(void* args) {
    if (!args) return strdup("");
    HooArgsHandle* handle = (HooArgsHandle*)args;

    HooArgsResult* result = result_for_handle(args);
    const char* prog = result ? result->program_name : "program";
    if (!prog || !*prog) prog = "program";

    size_t buf_size = 4096;
    char* buf = (char*)calloc(buf_size, 1);
    size_t pos = 0;

    int r = snprintf(buf + pos, buf_size - pos, "usage: %s", prog);
    if (r > 0) pos = strlen(buf);

    int has_positional = 0;
    for (int64_t i = 0; i < handle->def_count; i++) {
        if (handle->defs[i].is_positional) {
            r = snprintf(buf + pos, buf_size - pos, " %s",
                         handle->defs[i].name);
            if (r > 0) pos += (size_t)((size_t)r > buf_size - pos ? buf_size - pos : (size_t)r);
            has_positional = 1;
        }
    }
    pos = strlen(buf);
    r = snprintf(buf + pos, buf_size - pos, "\n\n");
    if (r > 0) pos += (size_t)((size_t)r > buf_size - pos ? buf_size - pos : (size_t)r);

    if (has_positional) {
        pos = strlen(buf);
        r = snprintf(buf + pos, buf_size - pos, "positional arguments:\n");
        if (r > 0) pos += (size_t)((size_t)r > buf_size - pos ? buf_size - pos : (size_t)r);
        for (int64_t i = 0; i < handle->def_count; i++) {
            if (handle->defs[i].is_positional) {
                pos = strlen(buf);
                r = snprintf(buf + pos, buf_size - pos, "  %-20s %s\n",
                             handle->defs[i].name,
                             handle->defs[i].help && handle->defs[i].help[0]
                                 ? handle->defs[i].help : "");
                if (r > 0) pos += (size_t)((size_t)r > buf_size - pos ? buf_size - pos : (size_t)r);
            }
        }
        pos = strlen(buf);
        r = snprintf(buf + pos, buf_size - pos, "\n");
        if (r > 0) pos += (size_t)((size_t)r > buf_size - pos ? buf_size - pos : (size_t)r);
    }

    pos = strlen(buf);
    r = snprintf(buf + pos, buf_size - pos, "optional arguments:\n");
    if (r > 0) pos += (size_t)((size_t)r > buf_size - pos ? buf_size - pos : (size_t)r);

    pos = strlen(buf);
    r = snprintf(buf + pos, buf_size - pos, "  %-20s %s\n",
                 "-h, --help", "Show this help message and exit");
    if (r > 0) pos += (size_t)((size_t)r > buf_size - pos ? buf_size - pos : (size_t)r);

    for (int64_t i = 0; i < handle->def_count; i++) {
        if (!handle->defs[i].is_positional) {
            char opt_buf[64];
            if (handle->defs[i].short_opt && handle->defs[i].short_opt[0] &&
                handle->defs[i].long_opt && handle->defs[i].long_opt[0]) {
                snprintf(opt_buf, sizeof(opt_buf), "%s, %s",
                         handle->defs[i].short_opt, handle->defs[i].long_opt);
            } else if (handle->defs[i].long_opt && handle->defs[i].long_opt[0]) {
                snprintf(opt_buf, sizeof(opt_buf), "%s",
                         handle->defs[i].long_opt);
            } else if (handle->defs[i].short_opt && handle->defs[i].short_opt[0]) {
                snprintf(opt_buf, sizeof(opt_buf), "%s",
                         handle->defs[i].short_opt);
            } else {
                snprintf(opt_buf, sizeof(opt_buf), "%s",
                         handle->defs[i].name);
            }

            const char* default_desc = "";
            char def_buf[128];
            if (handle->defs[i].type != HOO_ARG_FLAG) {
                if (handle->defs[i].type == HOO_ARG_STRING &&
                    handle->defs[i].default_str && handle->defs[i].default_str[0]) {
                    snprintf(def_buf, sizeof(def_buf), " (default: %s)",
                             handle->defs[i].default_str);
                    default_desc = def_buf;
                } else if (handle->defs[i].type == HOO_ARG_INT) {
                    snprintf(def_buf, sizeof(def_buf), " (default: %lld)",
                             (long long)handle->defs[i].default_int);
                    default_desc = def_buf;
                } else if (handle->defs[i].type == HOO_ARG_FLOAT) {
                    snprintf(def_buf, sizeof(def_buf), " (default: %g)",
                             handle->defs[i].default_float);
                    default_desc = def_buf;
                }
            }

            pos = strlen(buf);
            r = snprintf(buf + pos, buf_size - pos, "  %-20s %s%s\n",
                         opt_buf,
                         handle->defs[i].help && handle->defs[i].help[0]
                             ? handle->defs[i].help : "",
                         default_desc);
            if (r > 0) pos += (size_t)((size_t)r > buf_size - pos ? buf_size - pos : (size_t)r);
        }
    }

    return buf;
}

void hoo_args_clear(void* args) {
    if (!args) return;
    HooArgsHandle* handle = (HooArgsHandle*)args;
    for (int64_t i = 0; i < handle->def_count; i++) {
        free(handle->defs[i].name);
        free(handle->defs[i].short_opt);
        free(handle->defs[i].long_opt);
        free(handle->defs[i].help);
        free(handle->defs[i].default_str);
        free(handle->defs[i].parsed_str);
    }
    free(handle->defs);
    handle->defs = NULL;
    handle->def_count = 0;
    handle->def_capacity = 0;
    handle->parsed = 0;
    handle->positional_index = 0;
}

} // extern "C"
