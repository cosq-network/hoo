#include "hoo_generic_array.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

// ============================================================================
// Internal Structures (Hidden from hoo Code)
// ============================================================================

/**
 * Internal generic array header containing metadata.
 * The actual array data follows immediately after in memory.
 */
struct HooArrayImpl {
    int64_t refcount;      // Reference count for ARC
    int64_t length;        // Number of elements currently in array
    int64_t capacity;      // Allocated capacity (number of elements)
    size_t element_size;   // Size of each element in bytes
    uint8_t data[1];       // Flexible array member - actual array data
};

// ============================================================================
// Internal Utilities
// ============================================================================

static HooArrayImpl* get_impl(HooArray arr) {
    return (HooArrayImpl*)arr;
}

static HooArray from_impl(HooArrayImpl* impl) {
    return (HooArray)impl;
}

/**
 * Get pointer to element at given index in array data
 */
static uint8_t* get_element_ptr(HooArrayImpl* impl, int64_t index) {
    if (index < 0 || index >= impl->length) {
        return nullptr;
    }
    return impl->data + (index * impl->element_size);
}

/**
 * Internal allocation - creates a new generic array with given capacity
 */
static HooArray allocate_array(size_t element_size, int64_t capacity) {
    if (element_size == 0) {
        std::fprintf(stderr, "ERROR: Array element size must be > 0\n");
        std::exit(1);
    }

    if (capacity < 0) capacity = 0;
    if (capacity == 0) capacity = 10;  // Default minimum capacity

    // Calculate total size: header + array data
    size_t header_size = offsetof(HooArrayImpl, data);
    size_t total_size = header_size + (capacity * element_size);

    HooArrayImpl* impl = (HooArrayImpl*)std::malloc(total_size);
    if (!impl) {
        std::fprintf(stderr, "ERROR: Out of memory allocating HooArray (size: %zu bytes)\n", total_size);
        std::exit(1);
    }

    impl->refcount = 1;
    impl->length = 0;
    impl->capacity = capacity;
    impl->element_size = element_size;

    return from_impl(impl);
}

/**
 * Grow the array capacity if needed
 */
static void ensure_capacity(HooArrayImpl* impl, int64_t needed_capacity) {
    if (needed_capacity <= impl->capacity) {
        return;
    }

    // Double the capacity until it fits
    int64_t new_capacity = impl->capacity;
    while (new_capacity < needed_capacity) {
        new_capacity *= 2;
    }

    size_t header_size = offsetof(HooArrayImpl, data);
    size_t total_size = header_size + (new_capacity * impl->element_size);

    HooArrayImpl* new_impl = (HooArrayImpl*)std::realloc(impl, total_size);
    if (!new_impl) {
        std::fprintf(stderr, "ERROR: Out of memory reallocating HooArray\n");
        std::exit(1);
    }

    new_impl->capacity = new_capacity;
}

// ============================================================================
// Creation and Destruction
// ============================================================================

HooArray hoo_array_new(size_t element_size, int64_t capacity) {
    return allocate_array(element_size, capacity);
}

HooArray hoo_array_from_buffer(size_t element_size, const void* data, int64_t length) {
    if (element_size == 0) {
        std::fprintf(stderr, "ERROR: Array element size must be > 0\n");
        std::exit(1);
    }

    if (length < 0) length = 0;

    HooArray arr = allocate_array(element_size, length);
    if (!arr) {
        return nullptr;
    }

    HooArrayImpl* impl = get_impl(arr);

    if (data && length > 0) {
        std::memcpy(impl->data, data, length * element_size);
        impl->length = length;
    }

    return arr;
}

HooArray hoo_array_repeat(size_t element_size, const void* value, int64_t count) {
    if (element_size == 0) {
        std::fprintf(stderr, "ERROR: Array element size must be > 0\n");
        std::exit(1);
    }

    if (count < 0) count = 0;

    HooArray arr = allocate_array(element_size, count);
    if (!arr) {
        return nullptr;
    }

    HooArrayImpl* impl = get_impl(arr);

    if (value && count > 0) {
        for (int64_t i = 0; i < count; i++) {
            std::memcpy(impl->data + (i * element_size), value, element_size);
        }
        impl->length = count;
    }

    return arr;
}

// ============================================================================
// Basic Operations
// ============================================================================

int64_t hoo_array_length(HooArray arr) {
    if (!arr) return 0;
    HooArrayImpl* impl = get_impl(arr);
    return impl->length;
}

int64_t hoo_array_get(HooArray arr, int64_t index, void* dest) {
    if (!arr || !dest) return 0;

    HooArrayImpl* impl = get_impl(arr);
    if (index < 0 || index >= impl->length) {
        return 0;
    }

    uint8_t* src = impl->data + (index * impl->element_size);
    std::memcpy(dest, src, impl->element_size);
    return 1;
}

int64_t hoo_array_set(HooArray arr, int64_t index, const void* src) {
    if (!arr || !src) return 0;

    HooArrayImpl* impl = get_impl(arr);
    if (index < 0 || index >= impl->length) {
        return 0;
    }

    uint8_t* dest = impl->data + (index * impl->element_size);
    std::memcpy(dest, src, impl->element_size);
    return 1;
}

int64_t hoo_array_push(HooArray arr, const void* value) {
    if (!arr || !value) return 0;

    HooArrayImpl* impl = get_impl(arr);

    // Ensure we have space for one more element
    ensure_capacity(impl, impl->length + 1);

    // Copy the new element
    uint8_t* dest = impl->data + (impl->length * impl->element_size);
    std::memcpy(dest, value, impl->element_size);
    impl->length++;

    return 1;
}

int64_t hoo_array_pop(HooArray arr, void* dest) {
    if (!arr || !dest) return 0;

    HooArrayImpl* impl = get_impl(arr);
    if (impl->length == 0) {
        return 0;
    }

    // Copy the last element to destination
    impl->length--;
    uint8_t* src = impl->data + (impl->length * impl->element_size);
    std::memcpy(dest, src, impl->element_size);

    return 1;
}

void hoo_array_clear(HooArray arr) {
    if (!arr) return;
    HooArrayImpl* impl = get_impl(arr);
    impl->length = 0;
}

HooArray hoo_array_concat(HooArray arr1, HooArray arr2) {
    int64_t len1 = hoo_array_length(arr1);
    int64_t len2 = hoo_array_length(arr2);
    int64_t total_length = len1 + len2;

    if (total_length == 0) {
        // Return empty array - infer element size from first non-null array
        size_t element_size = 8;  // Default to 8 bytes if both null
        if (arr1) {
            element_size = get_impl(arr1)->element_size;
        } else if (arr2) {
            element_size = get_impl(arr2)->element_size;
        }
        return allocate_array(element_size, 0);
    }

    // Get element size from first non-null array
    size_t element_size = 8;
    if (arr1) {
        element_size = get_impl(arr1)->element_size;
    } else if (arr2) {
        element_size = get_impl(arr2)->element_size;
    }

    HooArray result = allocate_array(element_size, total_length);
    if (!result) {
        return nullptr;
    }

    HooArrayImpl* result_impl = get_impl(result);

    // Copy elements from arr1
    if (len1 > 0) {
        HooArrayImpl* arr1_impl = get_impl(arr1);
        std::memcpy(result_impl->data, arr1_impl->data, len1 * element_size);
        result_impl->length = len1;
    }

    // Copy elements from arr2
    if (len2 > 0) {
        HooArrayImpl* arr2_impl = get_impl(arr2);
        uint8_t* dest = result_impl->data + (len1 * element_size);
        std::memcpy(dest, arr2_impl->data, len2 * element_size);
        result_impl->length += len2;
    }

    return result;
}

HooArray hoo_array_slice(HooArray arr, int64_t start, int64_t length) {
    if (!arr || length <= 0) {
        if (arr) {
            HooArrayImpl* impl = get_impl(arr);
            return allocate_array(impl->element_size, 0);
        }
        return allocate_array(8, 0);
    }

    HooArrayImpl* impl = get_impl(arr);

    // Clamp start and length to valid range
    if (start < 0) start = 0;
    if (start >= impl->length) {
        return allocate_array(impl->element_size, 0);
    }

    if (start + length > impl->length) {
        length = impl->length - start;
    }

    HooArray result = allocate_array(impl->element_size, length);
    if (!result) {
        return nullptr;
    }

    HooArrayImpl* result_impl = get_impl(result);

    // Copy the slice
    uint8_t* src = impl->data + (start * impl->element_size);
    std::memcpy(result_impl->data, src, length * impl->element_size);
    result_impl->length = length;

    return result;
}

HooArray hoo_array_clone(HooArray arr) {
    if (!arr) return nullptr;

    HooArrayImpl* impl = get_impl(arr);
    HooArray result = allocate_array(impl->element_size, impl->length);
    if (!result) {
        return nullptr;
    }

    HooArrayImpl* result_impl = get_impl(result);

    // Copy all elements
    std::memcpy(result_impl->data, impl->data, impl->length * impl->element_size);
    result_impl->length = impl->length;

    return result;
}

// ============================================================================
// Element Size Query
// ============================================================================

size_t hoo_array_element_size(HooArray arr) {
    if (!arr) return 0;
    HooArrayImpl* impl = get_impl(arr);
    return impl->element_size;
}

// ============================================================================
// Reference Counting
// ============================================================================

HooArray hoo_array_retain(HooArray arr) {
    if (!arr) return nullptr;
    HooArrayImpl* impl = get_impl(arr);
    impl->refcount++;
    return arr;
}

void hoo_array_release(HooArray arr) {
    if (!arr) return;
    HooArrayImpl* impl = get_impl(arr);
    impl->refcount--;
    if (impl->refcount <= 0) {
        std::free(impl);
    }
}

int64_t hoo_array_refcount(HooArray arr) {
    if (!arr) return 0;
    HooArrayImpl* impl = get_impl(arr);
    return impl->refcount;
}

// ============================================================================
// Type-Specific Wrappers for int64 Arrays
// ============================================================================

typedef HooArray HooInt64Array;

HooInt64Array hoo_int64_array_new(int64_t capacity) {
    return hoo_array_new(sizeof(int64_t), capacity);
}

HooInt64Array hoo_int64_array_from_buffer(const int64_t* data, int64_t length) {
    return hoo_array_from_buffer(sizeof(int64_t), (const void*)data, length);
}

HooInt64Array hoo_int64_array_repeat(int64_t value, int64_t count) {
    return hoo_array_repeat(sizeof(int64_t), &value, count);
}

int64_t hoo_int64_array_length(HooInt64Array arr) {
    return hoo_array_length(arr);
}

int64_t hoo_int64_array_get(HooInt64Array arr, int64_t index) {
    int64_t result = 0;
    hoo_array_get(arr, index, &result);
    return result;
}

int64_t hoo_int64_array_set(HooInt64Array arr, int64_t index, int64_t value) {
    return hoo_array_set(arr, index, &value);
}

int64_t hoo_int64_array_push(HooInt64Array arr, int64_t value) {
    return hoo_array_push(arr, &value);
}

int64_t hoo_int64_array_pop(HooInt64Array arr) {
    int64_t result = 0;
    hoo_array_pop(arr, &result);
    return result;
}

void hoo_int64_array_clear(HooInt64Array arr) {
    hoo_array_clear(arr);
}

int64_t hoo_int64_array_contains(HooInt64Array arr, int64_t value) {
    int64_t len = hoo_array_length(arr);
    for (int64_t i = 0; i < len; i++) {
        int64_t elem = 0;
        hoo_array_get(arr, i, &elem);
        if (elem == value) {
            return 1;
        }
    }
    return 0;
}

int64_t hoo_int64_array_index_of(HooInt64Array arr, int64_t value) {
    int64_t len = hoo_array_length(arr);
    for (int64_t i = 0; i < len; i++) {
        int64_t elem = 0;
        hoo_array_get(arr, i, &elem);
        if (elem == value) {
            return i;
        }
    }
    return -1;
}

HooInt64Array hoo_int64_array_concat(HooInt64Array arr1, HooInt64Array arr2) {
    return hoo_array_concat(arr1, arr2);
}

HooInt64Array hoo_int64_array_slice(HooInt64Array arr, int64_t start, int64_t length) {
    return hoo_array_slice(arr, start, length);
}

HooInt64Array hoo_int64_array_clone(HooInt64Array arr) {
    return hoo_array_clone(arr);
}

HooInt64Array hoo_int64_array_retain(HooInt64Array arr) {
    return hoo_array_retain(arr);
}

void hoo_int64_array_release(HooInt64Array arr) {
    hoo_array_release(arr);
}

int64_t hoo_int64_array_refcount(HooInt64Array arr) {
    return hoo_array_refcount(arr);
}

// ============================================================================
// Type-Specific Wrappers for double Arrays
// ============================================================================

typedef HooArray HooDoubleArray;

HooDoubleArray hoo_double_array_new(int64_t capacity) {
    return hoo_array_new(sizeof(double), capacity);
}

HooDoubleArray hoo_double_array_from_buffer(const double* data, int64_t length) {
    return hoo_array_from_buffer(sizeof(double), (const void*)data, length);
}

HooDoubleArray hoo_double_array_repeat(double value, int64_t count) {
    return hoo_array_repeat(sizeof(double), &value, count);
}

int64_t hoo_double_array_length(HooDoubleArray arr) {
    return hoo_array_length(arr);
}

double hoo_double_array_get(HooDoubleArray arr, int64_t index) {
    double result = 0.0;
    hoo_array_get(arr, index, &result);
    return result;
}

int64_t hoo_double_array_set(HooDoubleArray arr, int64_t index, double value) {
    return hoo_array_set(arr, index, &value);
}

int64_t hoo_double_array_push(HooDoubleArray arr, double value) {
    return hoo_array_push(arr, &value);
}

double hoo_double_array_pop(HooDoubleArray arr) {
    double result = 0.0;
    hoo_array_pop(arr, &result);
    return result;
}

void hoo_double_array_clear(HooDoubleArray arr) {
    hoo_array_clear(arr);
}

HooDoubleArray hoo_double_array_concat(HooDoubleArray arr1, HooDoubleArray arr2) {
    return hoo_array_concat(arr1, arr2);
}

HooDoubleArray hoo_double_array_slice(HooDoubleArray arr, int64_t start, int64_t length) {
    return hoo_array_slice(arr, start, length);
}

HooDoubleArray hoo_double_array_clone(HooDoubleArray arr) {
    return hoo_array_clone(arr);
}

HooDoubleArray hoo_double_array_retain(HooDoubleArray arr) {
    return hoo_array_retain(arr);
}

void hoo_double_array_release(HooDoubleArray arr) {
    hoo_array_release(arr);
}

int64_t hoo_double_array_refcount(HooDoubleArray arr) {
    return hoo_array_refcount(arr);
}
