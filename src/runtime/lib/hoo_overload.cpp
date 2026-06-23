/**
 * @file hoo_overload.cpp
 * @brief Runtime function overload resolution implementation.
 *
 * Implements the overload registry, scorer, and resolver described in
 * hoo_overload.h.  The registry is a flat vector protected by a mutex;
 * registration happens once at startup, resolution is read-only and lock-free
 * after that.
 *
 * Built-in overload sets are registered by hoo_overload_init():
 *   - Math:     abs, min, max, sign
 *   - String:   String.from (factory)
 *   - Regex:    Regex.compile (arity overload)
 *   - DateTime: DateTime.parse (arity overload), DateTime constructors
 *   - Buffer:   new Buffer() / new Buffer(capacity)
 *   - Tensor:   Tensor.new (arity overload 1D/2D/3D)
 *   - Exception: Exception.create (arity overload)
 */

#include "hoo_overload.h"
#include "hoo_runtime.h"
#include "hoo_exception.h"

#include <vector>
#include <string>
#include <mutex>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <climits>
#include <cinttypes>

/* ═══════════════════════════════════════════════════════════════════════════
   Internal Registry
   ═══════════════════════════════════════════════════════════════════════════ */

namespace {

/* Sentinel pointer returned for ambiguous resolution */
static const char kAmbiguousSentinel[] = "__HOO_AMBIGUOUS_OVERLOAD__";

struct RegistryEntry {
    HooOverloadKind kind;
    std::string source_name;
    std::string receiver_type;       // empty for free functions
    std::vector<int64_t> param_type_ids;
    int64_t return_type_id;
    std::string runtime_symbol;
    int rank_bias;
    bool is_legacy_alias;
};

static std::vector<RegistryEntry> g_registry;
static std::recursive_mutex g_registry_mutex;
static bool g_initialized = false;

/* ─────────────────────── Scoring helpers ───────────────────────────────── */

/**
 * Score a single argument match.
 * Returns -1 (reject) or a non-negative score (lower = better).
 */
static int scoreArgument(int64_t actual, int64_t expected) {
    if (actual == expected) return 0;                    // exact match

    // Approved widening conversions
    if (expected == HOO_TYPE_INT64 &&
        (actual == HOO_TYPE_INT8 || actual == HOO_TYPE_BYTE))  return 1;
    if (expected == HOO_TYPE_FLOAT64 && actual == 7 /*f8*/)   return 1;

    // Object / any fallback
    if (expected == HOO_TYPE_OBJECT) return 3;

    // any type fallback (type id 0 represents any/unknown)
    if (expected == 0) return 20;

    return -1; // reject
}

/**
 * Score a candidate against the supplied argument types.
 * Returns INT_MAX if rejected, otherwise a non-negative sum.
 */
static int scoreCandidate(const RegistryEntry& e,
                          const int64_t* arg_type_ids, size_t argc) {
    if (e.param_type_ids.size() != argc) return INT_MAX;

    int total = e.rank_bias;
    for (size_t i = 0; i < argc; ++i) {
        int s = scoreArgument(arg_type_ids[i], e.param_type_ids[i]);
        if (s < 0) return INT_MAX;
        total += s;
    }
    return total;
}

} // anonymous namespace

/* ═══════════════════════════════════════════════════════════════════════════
   Public Sentinels
   ═══════════════════════════════════════════════════════════════════════════ */

const char* const HOO_OVERLOAD_AMBIGUOUS = kAmbiguousSentinel;

/* ═══════════════════════════════════════════════════════════════════════════
   Registration
   ═══════════════════════════════════════════════════════════════════════════ */

int hoo_register_overload(const HooOverloadCandidate* c) {
    if (!c || !c->source_name || !c->runtime_symbol) return -1;

    RegistryEntry e;
    e.kind           = c->kind;
    e.source_name    = c->source_name;
    e.receiver_type  = c->receiver_type ? c->receiver_type : "";
    e.return_type_id = c->return_type_id;
    e.runtime_symbol = c->runtime_symbol;
    e.rank_bias      = c->rank_bias;
    e.is_legacy_alias = (c->is_legacy_alias != 0);

    for (size_t i = 0; i < c->param_count; ++i) {
        e.param_type_ids.push_back(c->param_type_ids[i]);
    }

    std::lock_guard<std::recursive_mutex> lk(g_registry_mutex);
    g_registry.push_back(std::move(e));
    return 0;
}

int hoo_register_overload_set(const HooOverloadCandidate* candidates) {
    if (!candidates) return -1;
    int count = 0;
    for (const HooOverloadCandidate* c = candidates; c->source_name != nullptr; ++c) {
        if (hoo_register_overload(c) == 0) ++count;
    }
    return count;
}

/* ═══════════════════════════════════════════════════════════════════════════
   Built-in Overload Sets
   ═══════════════════════════════════════════════════════════════════════════ */

static void registerMathOverloads() {
    // abs overloads
    static const int64_t abs_i64_params[]    = {HOO_TYPE_INT64};
    static const int64_t abs_i8_params[]     = {HOO_TYPE_INT8};
    static const int64_t abs_byte_params[]   = {HOO_TYPE_BYTE};
    static const int64_t abs_dbl_params[]    = {HOO_TYPE_FLOAT64};

    static const HooOverloadCandidate math_abs[] = {
        {HOO_OVERLOAD_STATIC_METHOD, "abs", "Math", abs_i64_params,  1, HOO_TYPE_INT64,   "hoo_math_abs_int64",  0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "abs", "Math", abs_i8_params,   1, HOO_TYPE_INT8,    "hoo_math_abs_int8",   0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "abs", "Math", abs_byte_params, 1, HOO_TYPE_BYTE,    "hoo_math_abs_byte",   0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "abs", "Math", abs_dbl_params,  1, HOO_TYPE_FLOAT64, "hoo_math_abs_double", 0, 0},
        {HOO_OVERLOAD_FREE_FUNCTION, "math_abs", nullptr, abs_i64_params,  1, HOO_TYPE_INT64,   "hoo_math_abs_int64",  0, 0},
        {HOO_OVERLOAD_FREE_FUNCTION, "math_abs", nullptr, abs_dbl_params,  1, HOO_TYPE_FLOAT64, "hoo_math_abs_double", 0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0} // sentinel
    };
    hoo_register_overload_set(math_abs);

    // min overloads
    static const int64_t min_i64_params[] = {HOO_TYPE_INT64, HOO_TYPE_INT64};
    static const int64_t min_dbl_params[] = {HOO_TYPE_FLOAT64, HOO_TYPE_FLOAT64};
    static const HooOverloadCandidate math_min[] = {
        {HOO_OVERLOAD_STATIC_METHOD, "min", "Math", min_i64_params, 2, HOO_TYPE_INT64,   "hoo_math_min_int64",  0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "min", "Math", min_dbl_params, 2, HOO_TYPE_FLOAT64, "hoo_math_min_double", 0, 0},
        {HOO_OVERLOAD_FREE_FUNCTION, "math_min", nullptr, min_i64_params, 2, HOO_TYPE_INT64,   "hoo_math_min_int64",  0, 0},
        {HOO_OVERLOAD_FREE_FUNCTION, "math_min", nullptr, min_dbl_params, 2, HOO_TYPE_FLOAT64, "hoo_math_min_double", 0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(math_min);

    // max overloads
    static const HooOverloadCandidate math_max[] = {
        {HOO_OVERLOAD_STATIC_METHOD, "max", "Math", min_i64_params, 2, HOO_TYPE_INT64,   "hoo_math_max_int64",  0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "max", "Math", min_dbl_params, 2, HOO_TYPE_FLOAT64, "hoo_math_max_double", 0, 0},
        {HOO_OVERLOAD_FREE_FUNCTION, "math_max", nullptr, min_i64_params, 2, HOO_TYPE_INT64,   "hoo_math_max_int64",  0, 0},
        {HOO_OVERLOAD_FREE_FUNCTION, "math_max", nullptr, min_dbl_params, 2, HOO_TYPE_FLOAT64, "hoo_math_max_double", 0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(math_max);

    // sign overloads
    static const HooOverloadCandidate math_sign[] = {
        {HOO_OVERLOAD_STATIC_METHOD, "sign", "Math", abs_i64_params,  1, HOO_TYPE_INT64,   "hoo_math_sign_int64",  0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "sign", "Math", abs_dbl_params,  1, HOO_TYPE_FLOAT64, "hoo_math_sign_double", 0, 0},
        {HOO_OVERLOAD_FREE_FUNCTION, "math_sign", nullptr, abs_i64_params,  1, HOO_TYPE_INT64,   "hoo_math_sign_int64",  0, 0},
        {HOO_OVERLOAD_FREE_FUNCTION, "math_sign", nullptr, abs_dbl_params,  1, HOO_TYPE_FLOAT64, "hoo_math_sign_double", 0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(math_sign);
}

static void registerStringOverloads() {
    static const int64_t from_i64_params[]  = {HOO_TYPE_INT64};
    static const int64_t from_dbl_params[]  = {HOO_TYPE_FLOAT64};
    static const int64_t from_bool_params[] = {HOO_TYPE_BOOL};
    static const int64_t from_any_params[]  = {HOO_TYPE_OBJECT};

    static const HooOverloadCandidate string_from[] = {
        {HOO_OVERLOAD_STATIC_METHOD, "from", "String", from_i64_params,  1, HOO_TYPE_STRING, "hoo_string_from_int64",  0,  0},
        {HOO_OVERLOAD_STATIC_METHOD, "from", "String", from_dbl_params,  1, HOO_TYPE_STRING, "hoo_string_from_double", 0,  0},
        {HOO_OVERLOAD_STATIC_METHOD, "from", "String", from_bool_params, 1, HOO_TYPE_STRING, "hoo_string_from_bool",   0,  0},
        {HOO_OVERLOAD_STATIC_METHOD, "from", "String", from_any_params,  1, HOO_TYPE_STRING, "hoo_string_from_any",    20, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(string_from);
}

static void registerRegexOverloads() {
    static const int64_t compile1_params[] = {HOO_TYPE_STRING};
    static const int64_t compile2_params[] = {HOO_TYPE_STRING, HOO_TYPE_STRING};

    static const HooOverloadCandidate regex_compile[] = {
        {HOO_OVERLOAD_STATIC_METHOD, "compile", "Regex", compile1_params, 1, HOO_TYPE_REGEX, "hoo_regex_compile",            0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "compile", "Regex", compile2_params, 2, HOO_TYPE_REGEX, "hoo_regex_compile_with_flags", 0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(regex_compile);
}

static void registerDateTimeOverloads() {
    // Constructors
    static const int64_t ctor1_params[] = {HOO_TYPE_INT64};  // epochMs
    // DateTime.parse
    static const int64_t parse1_params[] = {HOO_TYPE_STRING};
    static const int64_t parse2_params[] = {HOO_TYPE_STRING, HOO_TYPE_STRING}; // text, format

    static const HooOverloadCandidate datetime[] = {
        {HOO_OVERLOAD_CONSTRUCTOR, "new", "DateTime", nullptr,  0, HOO_TYPE_DATETIME, "hoo_datetime_new_now",      0, 0},
        {HOO_OVERLOAD_CONSTRUCTOR, "new", "DateTime", ctor1_params,  1, HOO_TYPE_DATETIME, "hoo_datetime_new",          0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "parse", "DateTime", parse1_params, 1, HOO_TYPE_DATETIME, "hoo_datetime_from_iso8601", 0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "parse", "DateTime", parse2_params, 2, HOO_TYPE_DATETIME, "hoo_datetime_parse",        0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(datetime);
}

static void registerBufferOverloads() {
    static const int64_t ctor1_params[] = {HOO_TYPE_INT64};  // capacity

    static const HooOverloadCandidate buffer[] = {
        {HOO_OVERLOAD_CONSTRUCTOR, "new", "Buffer", nullptr, 0, HOO_TYPE_BUFFER, "hoo_buffer_new_empty", 0, 0},
        {HOO_OVERLOAD_CONSTRUCTOR, "new", "Buffer", ctor1_params, 1, HOO_TYPE_BUFFER, "hoo_buffer_new",       0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(buffer);
}

static void registerTensorOverloads() {
    // Tensor.new(type, d0), Tensor.new(type, d0, d1), Tensor.new(type, d0, d1, d2)
    static const int64_t ctor1d_params[] = {HOO_TYPE_INT64, HOO_TYPE_INT64};
    static const int64_t ctor2d_params[] = {HOO_TYPE_INT64, HOO_TYPE_INT64, HOO_TYPE_INT64};
    static const int64_t ctor3d_params[] = {HOO_TYPE_INT64, HOO_TYPE_INT64, HOO_TYPE_INT64, HOO_TYPE_INT64};

    static const HooOverloadCandidate tensor[] = {
        {HOO_OVERLOAD_STATIC_METHOD, "new", "Tensor", ctor1d_params, 2, HOO_TYPE_OBJECT, "hoo_tensor_new1", 0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "new", "Tensor", ctor2d_params, 3, HOO_TYPE_OBJECT, "hoo_tensor_new2", 0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "new", "Tensor", ctor3d_params, 4, HOO_TYPE_OBJECT, "hoo_tensor_new3", 0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(tensor);
}

static void registerExceptionOverloads() {
    static const int64_t create2_params[] = {HOO_TYPE_INT64, HOO_TYPE_STRING};
    static const int64_t create3_params[] = {HOO_TYPE_INT64, HOO_TYPE_STRING, HOO_TYPE_OBJECT};

    static const HooOverloadCandidate exc[] = {
        {HOO_OVERLOAD_STATIC_METHOD, "create", "Exception", create2_params, 2, HOO_TYPE_EXCEPTION, "hoo_exception_create",            0, 0},
        {HOO_OVERLOAD_STATIC_METHOD, "create", "Exception", create3_params, 3, HOO_TYPE_EXCEPTION, "hoo_exception_create_with_cause", 0, 0},
        {(HooOverloadKind)0, nullptr, nullptr, nullptr, 0, 0, nullptr, 0, 0}
    };
    hoo_register_overload_set(exc);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Public Init / Shutdown
   ═══════════════════════════════════════════════════════════════════════════ */

void hoo_overload_init(void) {
    std::lock_guard<std::recursive_mutex> lk(g_registry_mutex);
    if (g_initialized) return;
    g_initialized = true;

    // Register all built-in overload sets
    registerMathOverloads();
    registerStringOverloads();
    registerRegexOverloads();
    registerDateTimeOverloads();
    registerBufferOverloads();
    registerTensorOverloads();
    registerExceptionOverloads();
}

void hoo_overload_shutdown(void) {
    std::lock_guard<std::recursive_mutex> lk(g_registry_mutex);
    g_registry.clear();
    g_initialized = false;
}

/* ═══════════════════════════════════════════════════════════════════════════
   Resolution
   ═══════════════════════════════════════════════════════════════════════════ */

const char* hoo_resolve_overload(
    const char*     source_name,
    const char*     receiver_type,
    HooOverloadKind kind,
    const int64_t*  arg_type_ids,
    size_t          argc)
{
    if (!source_name) return nullptr;
    const std::string sname(source_name);
    const std::string rtype(receiver_type ? receiver_type : "");

    // Collect candidates that match kind, name, receiver_type, and arity
    int best_score = INT_MAX;
    const RegistryEntry* best = nullptr;
    bool ambiguous = false;

    // Read-only scan — no lock needed after init
    for (const auto& e : g_registry) {
        if (e.kind != kind) continue;
        if (e.source_name != sname) continue;
        if (e.receiver_type != rtype) continue;

        int score = scoreCandidate(e, arg_type_ids, argc);
        if (score == INT_MAX) continue;

        if (score < best_score) {
            best_score = score;
            best = &e;
            ambiguous = false;
        } else if (score == best_score) {
            ambiguous = true;
        }
    }

    if (!best) return nullptr;
    if (ambiguous) return HOO_OVERLOAD_AMBIGUOUS;
    return best->runtime_symbol.c_str();
}

/* ═══════════════════════════════════════════════════════════════════════════
   Exception constructors
   ═══════════════════════════════════════════════════════════════════════════ */

void* hoo_ambiguous_overload_new(const char* message) {
    return hoo_exception_create(HOO_TYPE_AMBIGUOUS_OVERLOAD, message);
}

void* hoo_no_matching_overload_new(const char* message) {
    return hoo_exception_create(HOO_TYPE_NO_MATCHING_OVERLOAD, message);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Diagnostics
   ═══════════════════════════════════════════════════════════════════════════ */

void hoo_overload_dump(const char* source_name) {
    static const char* kind_names[] = {
        "FreeFunction", "StaticMethod", "InstanceMethod", "Constructor"
    };

    std::lock_guard<std::recursive_mutex> lk(g_registry_mutex);
    printf("=== Overload Registry Dump");
    if (source_name) printf(" [%s]", source_name);
    printf(" ===\n");

    for (const auto& e : g_registry) {
        if (source_name && e.source_name != source_name) continue;

        printf("  [%s] %s%s%s(",
               kind_names[static_cast<int>(e.kind)],
               e.receiver_type.empty() ? "" : (e.receiver_type + ".").c_str(),
               e.source_name.c_str(),
               e.is_legacy_alias ? "*" : "");

        for (size_t i = 0; i < e.param_type_ids.size(); ++i) {
            if (i) printf(", ");
            printf("%" PRId64, e.param_type_ids[i]);
        }
        printf(") → %s  [bias=%d]\n", e.runtime_symbol.c_str(), e.rank_bias);
    }
    printf("=== end ===\n");
}
