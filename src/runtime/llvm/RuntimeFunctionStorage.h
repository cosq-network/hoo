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
    llvm::Function* hoo_string_split_func = nullptr;

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
// Array Function Storage
// ============================================================================

/**
 * Storage for Array runtime function pointers.
 *
 * Each LLVM function pointer in this structure corresponds to a runtime
 * Array function. The Array registration callback populates these during
 * module creation, and the code generator uses them when generating calls
 * to array operations.
 */
struct ArrayFunctionStorage {
    // Creation functions
    llvm::Function* hoo_array_new_func = nullptr;
    llvm::Function* hoo_array_from_buffer_func = nullptr;
    llvm::Function* hoo_array_repeat_func = nullptr;

    // Basic operations
    llvm::Function* hoo_array_length_func = nullptr;
    llvm::Function* hoo_array_get_func = nullptr;
    llvm::Function* hoo_array_set_func = nullptr;
    llvm::Function* hoo_array_push_func = nullptr;
    llvm::Function* hoo_array_pop_func = nullptr;

    // Type-specialized push functions (for common types)
    llvm::Function* hoo_array_push_int64_func = nullptr;
    llvm::Function* hoo_array_push_double_func = nullptr;
    llvm::Function* hoo_array_push_float_func = nullptr;
    llvm::Function* hoo_array_push_bool_func = nullptr;
    llvm::Function* hoo_array_push_char_func = nullptr;
    llvm::Function* hoo_array_push_byte_func = nullptr;

    // Type-specialized get functions (for common types)
    llvm::Function* hoo_array_get_int64_func = nullptr;
    llvm::Function* hoo_array_get_double_func = nullptr;
    llvm::Function* hoo_array_get_float_func = nullptr;
    llvm::Function* hoo_array_get_bool_func = nullptr;
    llvm::Function* hoo_array_get_char_func = nullptr;
    llvm::Function* hoo_array_get_string_func = nullptr;
    llvm::Function* hoo_array_get_object_func = nullptr;
    llvm::Function* hoo_array_get_array_func = nullptr;

    // Reference counting
    llvm::Function* hoo_array_retain_func = nullptr;
    llvm::Function* hoo_array_release_func = nullptr;
    llvm::Function* hoo_array_refcount_func = nullptr;

    // Utility
    llvm::Function* hoo_array_empty_func = nullptr;
    llvm::Function* hoo_array_clear_func = nullptr;
};

// ============================================================================
// Map Function Storage
// ============================================================================

struct MapFunctionStorage {
    // Creation functions
    llvm::Function* hoo_map_new_func = nullptr;
    llvm::Function* hoo_map_from_pairs_func = nullptr;

    // Basic operations
    llvm::Function* hoo_map_length_func = nullptr;
    llvm::Function* hoo_map_clear_func = nullptr;
    llvm::Function* hoo_map_empty_func = nullptr;

    // int8 key operations
    llvm::Function* hoo_map_contains_int8_func = nullptr;
    llvm::Function* hoo_map_remove_int8_func = nullptr;
    llvm::Function* hoo_map_set_int8_int64_func = nullptr;
    llvm::Function* hoo_map_get_int8_int64_func = nullptr;
    llvm::Function* hoo_map_set_int8_value_func = nullptr;
    llvm::Function* hoo_map_get_int8_value_func = nullptr;

    // int64 key operations
    llvm::Function* hoo_map_contains_int64_func = nullptr;
    llvm::Function* hoo_map_remove_int64_func = nullptr;
    llvm::Function* hoo_map_set_int64_int64_func = nullptr;
    llvm::Function* hoo_map_get_int64_int64_func = nullptr;
    llvm::Function* hoo_map_set_int64_value_func = nullptr;
    llvm::Function* hoo_map_get_int64_value_func = nullptr;

    // char key operations
    llvm::Function* hoo_map_contains_char_func = nullptr;
    llvm::Function* hoo_map_remove_char_func = nullptr;
    llvm::Function* hoo_map_set_char_double_func = nullptr;
    llvm::Function* hoo_map_get_char_double_func = nullptr;
    llvm::Function* hoo_map_set_char_value_func = nullptr;
    llvm::Function* hoo_map_get_char_value_func = nullptr;

    // string key operations
    llvm::Function* hoo_map_contains_string_func = nullptr;
    llvm::Function* hoo_map_remove_string_func = nullptr;
    llvm::Function* hoo_map_set_string_int64_func = nullptr;
    llvm::Function* hoo_map_get_string_int64_func = nullptr;
    llvm::Function* hoo_map_set_string_double_func = nullptr;
    llvm::Function* hoo_map_get_string_double_func = nullptr;
    llvm::Function* hoo_map_set_string_string_func = nullptr;
    llvm::Function* hoo_map_get_string_string_func = nullptr;
    llvm::Function* hoo_map_set_string_object_func = nullptr;
    llvm::Function* hoo_map_get_string_object_func = nullptr;
    llvm::Function* hoo_map_set_string_value_func = nullptr;
    llvm::Function* hoo_map_get_string_value_func = nullptr;

    // Reference counting
    llvm::Function* hoo_map_retain_func = nullptr;
    llvm::Function* hoo_map_release_func = nullptr;
    llvm::Function* hoo_map_refcount_func = nullptr;

    // Utility
    llvm::Function* hoo_map_key_type_func = nullptr;
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
 *   // Now storage.arrays contains all Array function pointers
 *   // Callbacks can access them via:
 *   // auto* strings = &static_cast<RuntimeFunctionStorage*>(userData)->strings;
 *   // auto* arrays = &static_cast<RuntimeFunctionStorage*>(userData)->arrays;
 */
struct RuntimeFunctionStorage {
    StringFunctionStorage strings;
    ArrayFunctionStorage arrays;
    MapFunctionStorage maps;
};

} // namespace runtime
} // namespace hooc
