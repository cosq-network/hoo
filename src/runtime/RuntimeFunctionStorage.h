#pragma once

#include "llvm/IR/Function.h"

namespace hooc {
namespace runtime {

// ============================================================================
// String Function Storage
// ============================================================================

/**
 * Storage for String runtime function pointers.
 *
 * Each LLVM function pointer in this structure corresponds to a runtime
 * String function. The String registration callback populates these during
 * module creation, and the code generator uses them when generating calls
 * to string operations.
 */
struct StringFunctionStorage {
    // Creation functions
    llvm::Function* hoo_string_from_cstr_func = nullptr;
    llvm::Function* hoo_string_new_func = nullptr;
    llvm::Function* hoo_string_from_bytes_func = nullptr;
    llvm::Function* hoo_string_repeat_func = nullptr;

    // Manipulation functions
    llvm::Function* hoo_string_concat_func = nullptr;
    llvm::Function* hoo_string_substring_func = nullptr;
    llvm::Function* hoo_string_to_upper_func = nullptr;
    llvm::Function* hoo_string_to_lower_func = nullptr;
    llvm::Function* hoo_string_trim_func = nullptr;
    llvm::Function* hoo_string_replace_func = nullptr;

    // Query functions
    llvm::Function* hoo_string_length_func = nullptr;
    llvm::Function* hoo_string_data_func = nullptr;
    llvm::Function* hoo_string_byte_at_func = nullptr;
    llvm::Function* hoo_string_is_empty_func = nullptr;
    llvm::Function* hoo_string_index_of_func = nullptr;
    llvm::Function* hoo_string_last_index_of_func = nullptr;
    llvm::Function* hoo_string_contains_func = nullptr;
    llvm::Function* hoo_string_starts_with_func = nullptr;
    llvm::Function* hoo_string_ends_with_func = nullptr;

    // Comparison functions
    llvm::Function* hoo_string_compare_func = nullptr;
    llvm::Function* hoo_string_equals_func = nullptr;
    llvm::Function* hoo_string_equals_ignore_case_func = nullptr;

    // Reference counting
    llvm::Function* hoo_string_retain_func = nullptr;
    llvm::Function* hoo_string_release_func = nullptr;
    llvm::Function* hoo_string_refcount_func = nullptr;

    // Conversion functions
    llvm::Function* hoo_string_from_int64_func = nullptr;
    llvm::Function* hoo_string_from_double_func = nullptr;
    llvm::Function* hoo_string_from_bool_func = nullptr;
    llvm::Function* hoo_string_to_int64_func = nullptr;
    llvm::Function* hoo_string_to_double_func = nullptr;
    llvm::Function* hoo_string_format_func = nullptr;

    // Debugging
    llvm::Function* hoo_string_print_func = nullptr;
    llvm::Function* hoo_string_println_func = nullptr;
    llvm::Function* hoo_string_debug_func = nullptr;
};

// ============================================================================
// Central Runtime Function Storage
// ============================================================================

/**
 * Central storage for all runtime function pointers.
 *
 * This structure is passed as userData to all runtime registration callbacks,
 * allowing each runtime to populate its specific function storage section.
 *
 * Usage in code generator:
 *   RuntimeFunctionStorage storage;
 *   registry.declareAllFunctions(module, context, &storage);
 *   // Now storage.strings contains all String function pointers
 *   // Callbacks can access them via:
 *   // auto* strings = &static_cast<RuntimeFunctionStorage*>(userData)->strings;
 */
struct RuntimeFunctionStorage {
    StringFunctionStorage strings;
    // Future runtimes (Array, Dict, etc.) would add their storage here
};

} // namespace runtime
} // namespace hooc
