/**
 * @file hoo_overload.h
 * @brief Runtime function overload resolution API for the Hoo language.
 *
 * This header provides the infrastructure for resolving overloaded function calls
 * at runtime. Overloaded functions share the same source name but differ in their
 * parameter type signatures. The runtime maintains an overload registry that maps
 * (function_name, arg_type_ids[]) → concrete function pointer.
 *
 * Phase 0: Metadata structures and registration API.
 * Phase 1: Static resolution used by the code generator for built-in overloads.
 * Phase 3: Dynamic resolution for call sites that cannot be resolved at compile time.
 */

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
extern "C" {
#else
#include <stdint.h>
#include <stddef.h>
#endif

/* ─────────────────────────── Type IDs ──────────────────────────────────── */

/**
 * Exception type IDs for overload resolution errors.
 * These extend the core type IDs defined in hoo_runtime.h.
 */
#define HOO_TYPE_AMBIGUOUS_OVERLOAD    130   /**< AmbiguousOverloadException */
#define HOO_TYPE_NO_MATCHING_OVERLOAD  131   /**< NoMatchingOverloadException */

/* ─────────────────────────── Overload Kind ─────────────────────────────── */

typedef enum {
    HOO_OVERLOAD_FREE_FUNCTION  = 0, /**< Top-level / free function             */
    HOO_OVERLOAD_STATIC_METHOD  = 1, /**< Static class method (e.g. String.from)*/
    HOO_OVERLOAD_INSTANCE_METHOD = 2,/**< Instance method (e.g. array.push)     */
    HOO_OVERLOAD_CONSTRUCTOR    = 3  /**< Constructor (new ClassName(...))       */
} HooOverloadKind;

/* ─────────────────────────── Overload Candidate ────────────────────────── */

/**
 * Describes a single overload candidate.
 *
 * Each candidate is a tuple of:
 *   (kind, source_name, receiver_type, param_type_ids[], return_type_id,
 *    runtime_symbol, rank_bias, is_legacy_alias)
 *
 * The runtime_symbol is the name of the concrete C function / JIT symbol to
 * call once this candidate wins overload resolution.
 */
typedef struct HooOverloadCandidate {
    HooOverloadKind kind;
    const char*     source_name;     /**< Unmangled source name, e.g. "abs"      */
    const char*     receiver_type;   /**< Class name for methods, NULL for free  */
    const int64_t*  param_type_ids;  /**< Array of HOO_TYPE_* parameter type IDs */
    size_t          param_count;     /**< Length of param_type_ids               */
    int64_t         return_type_id;  /**< HOO_TYPE_* for the return value        */
    const char*     runtime_symbol;  /**< Concrete runtime / JIT symbol name     */
    int             rank_bias;       /**< Added to match score; 0 = exact         */
    int             is_legacy_alias; /**< 1 = keep for compat, prefer newer      */
} HooOverloadCandidate;

/* ─────────────────────────── Registry API ──────────────────────────────── */

/**
 * Register a single overload candidate in the global registry.
 * Thread-safe after the first call to hoo_overload_init().
 *
 * @param candidate  Pointer to a statically-allocated candidate descriptor.
 *                   The pointed-to memory must remain valid for program lifetime.
 * @return 0 on success, -1 on error (e.g. registry full).
 */
int hoo_register_overload(const HooOverloadCandidate* candidate);

/**
 * Register a NULL-terminated array of candidates in one call.
 * Convenience wrapper around hoo_register_overload().
 *
 * @param candidates Array terminated by a sentinel with source_name == NULL.
 * @return Number of candidates registered, or -1 on error.
 */
int hoo_register_overload_set(const HooOverloadCandidate* candidates);

/**
 * Initialise the overload registry and register all built-in overload sets.
 * Must be called once before any overload resolution.
 * Subsequent calls are no-ops.
 */
void hoo_overload_init(void);

/**
 * Free all overload registry resources (called at program exit / test teardown).
 */
void hoo_overload_shutdown(void);

/* ─────────────────────────── Resolution API ────────────────────────────── */

/**
 * Resolve an overloaded function at runtime.
 *
 * Searches the registry for candidates whose (kind, source_name, receiver_type,
 * param_count) match, scores each surviving candidate against the supplied
 * argument type IDs, and returns the runtime symbol of the winning candidate.
 *
 * Scoring (per argument):
 *   0  – exact type match
 *   1  – approved numeric widening  (int8/byte → int64, f8 → double)
 *   2  – literal-compatible conversion
 *   3  – nullable/object-compatible
 *   20 – any / object fallback
 *   -1 – reject (unsafe narrowing, incompatible container, or missing conversion)
 *
 * @param source_name     Unmangled function name, e.g. "abs".
 * @param receiver_type   Class name for methods; NULL for free functions.
 * @param kind            Overload kind (free / static / instance / constructor).
 * @param arg_type_ids    Array of argument type IDs (HOO_TYPE_*).
 * @param argc            Number of arguments.
 * @return  Pointer to the runtime_symbol string on success.
 *          NULL if no match is found (caller should raise NoMatchingOverloadException).
 *          HOO_OVERLOAD_AMBIGUOUS sentinel if multiple candidates tie (caller raises
 *          AmbiguousOverloadException).
 */
const char* hoo_resolve_overload(
    const char*     source_name,
    const char*     receiver_type,
    HooOverloadKind kind,
    const int64_t*  arg_type_ids,
    size_t          argc
);

/** Sentinel return value indicating an ambiguous overload. */
extern const char* const HOO_OVERLOAD_AMBIGUOUS;

/* ─────────────────────────── Exception API ─────────────────────────────── */

/**
 * Create an AmbiguousOverloadException managed object.
 * @param message  Human-readable description of the ambiguity.
 * @return ARC-managed exception object (HooException*).
 */
void* hoo_ambiguous_overload_new(const char* message);

/**
 * Create a NoMatchingOverloadException managed object.
 * @param message  Human-readable description of why no overload matched.
 * @return ARC-managed exception object (HooException*).
 */
void* hoo_no_matching_overload_new(const char* message);

/* ─────────────────────────── Diagnostic API ────────────────────────────── */

/**
 * Dump all registered overload candidates for a given source name to stdout.
 * Intended for debugging and test verification only.
 *
 * @param source_name  Function name to dump, or NULL to dump everything.
 */
void hoo_overload_dump(const char* source_name);

#ifdef __cplusplus
} // extern "C"
#endif
