#include "hoo_array.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

// ============================================================================
// Internal Structures (Hidden from hoo Code)
// ============================================================================

/**
 * Internal int64 array header containing metadata.
 * The actual array data follows immediately after in memory.
 */
struct HooInt64ArrayImpl {
    int64_t refcount;    // Reference count for ARC
    int64_t length;      // Number of elements currently in array
    int64_t capacity;    // Allocated capacity (number of elements)
    int64_t data[1];     // Flexible array member - actual array data
};

/**
 * Internal double array header containing metadata.
 * The actual array data follows immediately after in memory.
 */
struct HooDoubleArrayImpl {
    int64_t refcount;    // Reference count for ARC
    int64_t length;      // Number of elements currently in array
    int64_t capacity;    // Allocated capacity (number of elements)
    double data[1];      // Flexible array member - actual array data
};

// ============================================================================
// Int64 Array - Internal Utilities
// ============================================================================

static HooInt64ArrayImpl* get_int64_impl(HooInt64Array arr) {
    return (HooInt64ArrayImpl*)arr;
}

static HooInt64Array from_int64_impl(HooInt64ArrayImpl* impl) {
    return (HooInt64Array)impl;
}

/**
 * Internal allocation - creates a new int64 array with given capacity
 */
static HooInt64Array allocate_int64_array(int64_t capacity) {
    if (capacity < 0) capacity = 0;
    if (capacity == 0) capacity = 10;  // Default minimum capacity

    // Calculate total size: header + array data
    size_t header_size = offsetof(HooInt64ArrayImpl, data);
    size_t total_size = header_size + (capacity * sizeof(int64_t));

    HooInt64ArrayImpl* impl = (HooInt64ArrayImpl*)std::malloc(total_size);
    if (!impl) {
        std::fprintf(stderr, "ERROR: Out of memory allocating HooInt64Array (size: %zu bytes)\n", total_size);
        std::exit(1);
    }

    impl->refcount = 1;
    impl->length = 0;
    impl->capacity = capacity;

    return from_int64_impl(impl);
}

/**
 * Grow the int64 array capacity if needed
 */
static void ensure_int64_capacity(HooInt64ArrayImpl* impl, int64_t needed_capacity) {
    if (needed_capacity <= impl->capacity) {
        return;
    }

    // Double the capacity until it fits
    int64_t new_capacity = impl->capacity;
    while (new_capacity < needed_capacity) {
        new_capacity *= 2;
    }

    size_t header_size = offsetof(HooInt64ArrayImpl, data);
    size_t total_size = header_size + (new_capacity * sizeof(int64_t));

    HooInt64ArrayImpl* new_impl = (HooInt64ArrayImpl*)std::realloc(impl, total_size);
    if (!new_impl) {
        std::fprintf(stderr, "ERROR: Out of memory reallocating HooInt64Array\n");
        std::exit(1);
    }

    new_impl->capacity = new_capacity;
}

// ============================================================================
// Double Array - Internal Utilities
// ============================================================================

static HooDoubleArrayImpl* get_double_impl(HooDoubleArray arr) {
    return (HooDoubleArrayImpl*)arr;
}

static HooDoubleArray from_double_impl(HooDoubleArrayImpl* impl) {
    return (HooDoubleArray)impl;
}

/**
 * Internal allocation - creates a new double array with given capacity
 */
static HooDoubleArray allocate_double_array(int64_t capacity) {
    if (capacity < 0) capacity = 0;
    if (capacity == 0) capacity = 10;  // Default minimum capacity

    // Calculate total size: header + array data
    size_t header_size = offsetof(HooDoubleArrayImpl, data);
    size_t total_size = header_size + (capacity * sizeof(double));

    HooDoubleArrayImpl* impl = (HooDoubleArrayImpl*)std::malloc(total_size);
    if (!impl) {
        std::fprintf(stderr, "ERROR: Out of memory allocating HooDoubleArray (size: %zu bytes)\n", total_size);
        std::exit(1);
    }

    impl->refcount = 1;
    impl->length = 0;
    impl->capacity = capacity;

    return from_double_impl(impl);
}

/**
 * Grow the double array capacity if needed
 */
static void ensure_double_capacity(HooDoubleArrayImpl* impl, int64_t needed_capacity) {
    if (needed_capacity <= impl->capacity) {
        return;
    }

    // Double the capacity until it fits
    int64_t new_capacity = impl->capacity;
    while (new_capacity < needed_capacity) {
        new_capacity *= 2;
    }

    size_t header_size = offsetof(HooDoubleArrayImpl, data);
    size_t total_size = header_size + (new_capacity * sizeof(double));

    HooDoubleArrayImpl* new_impl = (HooDoubleArrayImpl*)std::realloc(impl, total_size);
    if (!new_impl) {
        std::fprintf(stderr, "ERROR: Out of memory reallocating HooDoubleArray\n");
        std::exit(1);
    }

    new_impl->capacity = new_capacity;
}

// ============================================================================
// Int64 Array - Creation and Destruction
// ============================================================================

HooInt64Array hoo_int64_array_new(int64_t capacity) {
    return allocate_int64_array(capacity);
}

HooInt64Array hoo_int64_array_from_buffer(const int64_t* data, int64_t length) {
    if (!data || length <= 0) {
        return hoo_int64_array_new(10);
    }

    HooInt64Array arr = allocate_int64_array(length);
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    std::memcpy(impl->data, data, length * sizeof(int64_t));
    impl->length = length;

    return arr;
}

HooInt64Array hoo_int64_array_repeat(int64_t value, int64_t count) {
    if (count <= 0) {
        return hoo_int64_array_new(10);
    }

    HooInt64Array arr = allocate_int64_array(count);
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    for (int64_t i = 0; i < count; i++) {
        impl->data[i] = value;
    }
    impl->length = count;

    return arr;
}

// ============================================================================
// Int64 Array - Basic Operations
// ============================================================================

int64_t hoo_int64_array_length(HooInt64Array arr) {
    if (!arr) return 0;
    return get_int64_impl(arr)->length;
}

int64_t hoo_int64_array_get(HooInt64Array arr, int64_t index) {
    if (!arr) return 0;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    if (index < 0 || index >= impl->length) return 0;
    return impl->data[index];
}

int64_t hoo_int64_array_set(HooInt64Array arr, int64_t index, int64_t value) {
    if (!arr) return 0;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    if (index < 0 || index >= impl->length) return 0;
    impl->data[index] = value;
    return 1;
}

int64_t hoo_int64_array_push(HooInt64Array arr, int64_t value) {
    if (!arr) return 0;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    ensure_int64_capacity(impl, impl->length + 1);
    impl = get_int64_impl(arr);  // Reacquire in case realloc moved it
    impl->data[impl->length] = value;
    impl->length++;
    return 1;
}

int64_t hoo_int64_array_pop(HooInt64Array arr) {
    if (!arr) return 0;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    if (impl->length <= 0) return 0;
    impl->length--;
    return impl->data[impl->length];
}

void hoo_int64_array_clear(HooInt64Array arr) {
    if (!arr) return;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    impl->length = 0;
}

int64_t hoo_int64_array_contains(HooInt64Array arr, int64_t value) {
    if (!arr) return 0;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    for (int64_t i = 0; i < impl->length; i++) {
        if (impl->data[i] == value) return 1;
    }
    return 0;
}

int64_t hoo_int64_array_index_of(HooInt64Array arr, int64_t value) {
    if (!arr) return -1;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    for (int64_t i = 0; i < impl->length; i++) {
        if (impl->data[i] == value) return i;
    }
    return -1;
}

HooInt64Array hoo_int64_array_concat(HooInt64Array arr1, HooInt64Array arr2) {
    int64_t len1 = hoo_int64_array_length(arr1);
    int64_t len2 = hoo_int64_array_length(arr2);

    HooInt64Array result = hoo_int64_array_new(len1 + len2);
    HooInt64ArrayImpl* result_impl = get_int64_impl(result);

    if (arr1) {
        HooInt64ArrayImpl* impl1 = get_int64_impl(arr1);
        std::memcpy(result_impl->data, impl1->data, len1 * sizeof(int64_t));
    }

    if (arr2) {
        HooInt64ArrayImpl* impl2 = get_int64_impl(arr2);
        std::memcpy(result_impl->data + len1, impl2->data, len2 * sizeof(int64_t));
    }

    result_impl->length = len1 + len2;
    return result;
}

HooInt64Array hoo_int64_array_slice(HooInt64Array arr, int64_t start, int64_t length) {
    if (!arr) {
        return hoo_int64_array_new(10);
    }

    HooInt64ArrayImpl* impl = get_int64_impl(arr);

    if (start < 0) start = 0;
    if (start >= impl->length) {
        return hoo_int64_array_new(10);
    }

    if (length < 0) {
        return hoo_int64_array_new(10);
    }

    int64_t actual_length = std::min(length, impl->length - start);
    if (actual_length <= 0) {
        return hoo_int64_array_new(10);
    }

    HooInt64Array result = hoo_int64_array_new(actual_length);
    HooInt64ArrayImpl* result_impl = get_int64_impl(result);
    std::memcpy(result_impl->data, impl->data + start, actual_length * sizeof(int64_t));
    result_impl->length = actual_length;

    return result;
}

HooInt64Array hoo_int64_array_clone(HooInt64Array arr) {
    if (!arr) return nullptr;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    return hoo_int64_array_from_buffer(impl->data, impl->length);
}

// ============================================================================
// Int64 Array - Reference Counting
// ============================================================================

HooInt64Array hoo_int64_array_retain(HooInt64Array arr) {
    if (!arr) return nullptr;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    impl->refcount++;
    return arr;
}

void hoo_int64_array_release(HooInt64Array arr) {
    if (!arr) return;
    HooInt64ArrayImpl* impl = get_int64_impl(arr);
    impl->refcount--;
    if (impl->refcount <= 0) {
        std::free(impl);
    }
}

int64_t hoo_int64_array_refcount(HooInt64Array arr) {
    if (!arr) return 0;
    return get_int64_impl(arr)->refcount;
}

// ============================================================================
// Double Array - Creation and Destruction
// ============================================================================

HooDoubleArray hoo_double_array_new(int64_t capacity) {
    return allocate_double_array(capacity);
}

HooDoubleArray hoo_double_array_from_buffer(const double* data, int64_t length) {
    if (!data || length <= 0) {
        return hoo_double_array_new(10);
    }

    HooDoubleArray arr = allocate_double_array(length);
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    std::memcpy(impl->data, data, length * sizeof(double));
    impl->length = length;

    return arr;
}

HooDoubleArray hoo_double_array_repeat(double value, int64_t count) {
    if (count <= 0) {
        return hoo_double_array_new(10);
    }

    HooDoubleArray arr = allocate_double_array(count);
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    for (int64_t i = 0; i < count; i++) {
        impl->data[i] = value;
    }
    impl->length = count;

    return arr;
}

// ============================================================================
// Double Array - Basic Operations
// ============================================================================

int64_t hoo_double_array_length(HooDoubleArray arr) {
    if (!arr) return 0;
    return get_double_impl(arr)->length;
}

double hoo_double_array_get(HooDoubleArray arr, int64_t index) {
    if (!arr) return 0.0;
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    if (index < 0 || index >= impl->length) return 0.0;
    return impl->data[index];
}

int64_t hoo_double_array_set(HooDoubleArray arr, int64_t index, double value) {
    if (!arr) return 0;
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    if (index < 0 || index >= impl->length) return 0;
    impl->data[index] = value;
    return 1;
}

int64_t hoo_double_array_push(HooDoubleArray arr, double value) {
    if (!arr) return 0;
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    ensure_double_capacity(impl, impl->length + 1);
    impl = get_double_impl(arr);  // Reacquire in case realloc moved it
    impl->data[impl->length] = value;
    impl->length++;
    return 1;
}

double hoo_double_array_pop(HooDoubleArray arr) {
    if (!arr) return 0.0;
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    if (impl->length <= 0) return 0.0;
    impl->length--;
    return impl->data[impl->length];
}

void hoo_double_array_clear(HooDoubleArray arr) {
    if (!arr) return;
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    impl->length = 0;
}

HooDoubleArray hoo_double_array_concat(HooDoubleArray arr1, HooDoubleArray arr2) {
    int64_t len1 = hoo_double_array_length(arr1);
    int64_t len2 = hoo_double_array_length(arr2);

    HooDoubleArray result = hoo_double_array_new(len1 + len2);
    HooDoubleArrayImpl* result_impl = get_double_impl(result);

    if (arr1) {
        HooDoubleArrayImpl* impl1 = get_double_impl(arr1);
        std::memcpy(result_impl->data, impl1->data, len1 * sizeof(double));
    }

    if (arr2) {
        HooDoubleArrayImpl* impl2 = get_double_impl(arr2);
        std::memcpy(result_impl->data + len1, impl2->data, len2 * sizeof(double));
    }

    result_impl->length = len1 + len2;
    return result;
}

HooDoubleArray hoo_double_array_slice(HooDoubleArray arr, int64_t start, int64_t length) {
    if (!arr) {
        return hoo_double_array_new(10);
    }

    HooDoubleArrayImpl* impl = get_double_impl(arr);

    if (start < 0) start = 0;
    if (start >= impl->length) {
        return hoo_double_array_new(10);
    }

    if (length < 0) {
        return hoo_double_array_new(10);
    }

    int64_t actual_length = std::min(length, impl->length - start);
    if (actual_length <= 0) {
        return hoo_double_array_new(10);
    }

    HooDoubleArray result = hoo_double_array_new(actual_length);
    HooDoubleArrayImpl* result_impl = get_double_impl(result);
    std::memcpy(result_impl->data, impl->data + start, actual_length * sizeof(double));
    result_impl->length = actual_length;

    return result;
}

HooDoubleArray hoo_double_array_clone(HooDoubleArray arr) {
    if (!arr) return nullptr;
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    return hoo_double_array_from_buffer(impl->data, impl->length);
}

// ============================================================================
// Double Array - Reference Counting
// ============================================================================

HooDoubleArray hoo_double_array_retain(HooDoubleArray arr) {
    if (!arr) return nullptr;
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    impl->refcount++;
    return arr;
}

void hoo_double_array_release(HooDoubleArray arr) {
    if (!arr) return;
    HooDoubleArrayImpl* impl = get_double_impl(arr);
    impl->refcount--;
    if (impl->refcount <= 0) {
        std::free(impl);
    }
}

int64_t hoo_double_array_refcount(HooDoubleArray arr) {
    if (!arr) return 0;
    return get_double_impl(arr)->refcount;
}
