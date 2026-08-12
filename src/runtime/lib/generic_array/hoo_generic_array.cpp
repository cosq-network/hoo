#include "runtime/lib/generic_array/hoo_generic_array.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include <cstring>
#include <cstdlib>
#include <random>
#include <algorithm>

// ============================================================================
// Low-level HooArray Implementation (Hardware Ready)
// ============================================================================
// Format in memory:
// [Header: 16 bytes (ARC refcount + Type ID)]
// [Length: 8 bytes (int64_t)]
// [Capacity: 8 bytes (int64_t)]
// [Element Type ID: 8 bytes (int64_t)]
// [Reserved/Padding: 8 bytes (int64_t)]
// [elements (64-bit each)...]

#define ARRAY_HEADER_WORDS 4 // length, capacity, element_type, reserved

// Simple intrinsic type identifiers used for homogeneous arrays
#define TYPE_ID_NONE   0
#define TYPE_ID_INT64  1
#define TYPE_ID_DOUBLE 2
#define TYPE_ID_BOOL   3
#define TYPE_ID_CHAR   4
#define TYPE_ID_STRING 5
#define TYPE_ID_OBJECT 6

// Destructor for HOO_TYPE_ARRAY: releases all managed-typed elements
static void array_destructor(void* obj) {
    int64_t* raw = (int64_t*)obj;
    int64_t len = raw[0];
    int64_t elem_type = raw[2];

    if (elem_type >= 100) {
        for (int64_t i = 0; i < len; i++) {
            void* elem = (void*)raw[i + ARRAY_HEADER_WORDS];
            if (elem) hoo_release(elem);
        }
    }
}

// Register array destructor at library load time
namespace {
    struct ArrayDestructorRegistrar {
        ArrayDestructorRegistrar() {
            hoo_register_destructor(HOO_TYPE_ARRAY, array_destructor);
        }
    } registrar;
}

HooArray hoo_array_new(void) {
    // Allocate with initial capacity for 1024 elements + header
    int64_t* arr = (int64_t*)hoo_alloc(ARRAY_HEADER_WORDS * 8 + 8192, HOO_TYPE_ARRAY);
    arr[0] = 0;                  // Length
    arr[1] = 1024;               // Capacity (elements)
    arr[2] = 0;                  // Default: elements are raw bits (safe for release)
    arr[3] = 0;                  // Reserved
    return (HooArray)arr;
}

HooArray hoo_array_from_buffer(const void* data, int64_t length) {
    if (length < 0) length = 0;
    int64_t* arr = (int64_t*)hoo_alloc(ARRAY_HEADER_WORDS * 8 + (length * 8), HOO_TYPE_ARRAY);
    arr[0] = length;
    arr[1] = length;
    arr[2] = 0;
    arr[3] = 0;
    if (data && length > 0) {
        std::memcpy(arr + ARRAY_HEADER_WORDS, data, (size_t)length * 8);
    }
    return (HooArray)arr;
}

HooArray hoo_array_repeat(const void* value, int64_t count) {
    if (count < 0) count = 0;
    int64_t* arr = (int64_t*)hoo_alloc(ARRAY_HEADER_WORDS * 8 + (count * 8), HOO_TYPE_ARRAY);
    arr[0] = count;
    arr[1] = count;
    arr[2] = 0;
    arr[3] = 0;
    if (value) {
        int64_t val = *(const int64_t*)value;
        for (int64_t i = 0; i < count; i++) {
            arr[i + ARRAY_HEADER_WORDS] = val;
        }
    }
    return (HooArray)arr;
}

int64_t hoo_array_length(HooArray arr) {
    if (!arr) return 0;
    return *(int64_t*)arr;
}

int64_t hoo_array_get(HooArray arr, int64_t index, void* dest) {
    if (!arr || !dest) return 0;
    int64_t length = *(int64_t*)arr;
    if (index < 0 || index >= length) return 0;
    *(int64_t*)dest = ((int64_t*)arr)[index + ARRAY_HEADER_WORDS];
    return 1;
}

int64_t hoo_array_set(HooArray arr, int64_t index, const void* value) {
    if (!arr || !value) return 0;
    int64_t* raw = (int64_t*)arr;
    int64_t length = raw[0];
    if (index < 0 || index >= length) return 0;
    int64_t elem_type = raw[2];
    int64_t new_elem = *(const int64_t*)value;
    if (elem_type >= 100) {
        // Managed-element array: replacing a slot drops the reference held by
        // the array (caller transfers ownership of the new element, matching
        // hoo_array_push).  Skip release when writing the same handle back.
        void* old_elem = (void*)raw[index + ARRAY_HEADER_WORDS];
        if (old_elem != (void*)new_elem) {
            raw[index + ARRAY_HEADER_WORDS] = new_elem;
            if (old_elem) hoo_release(old_elem);
        }
    } else {
        raw[index + ARRAY_HEADER_WORDS] = new_elem;
    }
    return 1;
}

HooArray hoo_array_push(HooArray arr_handle, const void* value) {
    if (!arr_handle || !value) return nullptr;
    int64_t* raw = (int64_t*)arr_handle;
    int64_t len = raw[0];
    int64_t cap = raw[1];
    int64_t elem_type = raw[2]; // preserve element type
    if (len >= cap) {
        int64_t new_cap = (cap <= 0) ? 8 : (cap > 0x7FFFFFFFFFFFFFFFLL / 2) ? 0x7FFFFFFFFFFFFFFFLL : cap * 2;
        if (elem_type >= 100) {
            // Managed-element array: hoo_realloc would release the old block,
            // running array_destructor over elements the new block still
            // references.  Grow by copying, retaining each element for the new
            // block, then releasing the old block.
            size_t new_size = ARRAY_HEADER_WORDS * 8 + (size_t)new_cap * 8;
            int64_t* new_raw = (int64_t*)hoo_alloc(new_size, HOO_TYPE_ARRAY);
            std::memcpy(new_raw, raw, ARRAY_HEADER_WORDS * 8);
            new_raw[1] = new_cap;
            if (len > 0) {
                std::memcpy(new_raw + ARRAY_HEADER_WORDS, raw + ARRAY_HEADER_WORDS, (size_t)len * 8);
                for (int64_t i = 0; i < len; i++) {
                    void* elem = (void*)new_raw[i + ARRAY_HEADER_WORDS];
                    if (elem) hoo_retain(elem);
                }
            }
            hoo_release(arr_handle);
            raw = new_raw;
        } else {
            size_t new_size = ARRAY_HEADER_WORDS * 8 + (size_t)new_cap * 8;
            int64_t* new_raw = (int64_t*)hoo_realloc(arr_handle, new_size);
            if (!new_raw) return nullptr;
            raw = new_raw;
            raw[1] = new_cap;
            raw[2] = elem_type; // restore element type
        }
    }
    raw[len + ARRAY_HEADER_WORDS] = *(const int64_t*)value;
    raw[0] = len + 1;
    return (HooArray)raw;
}

int64_t hoo_array_push_h(HooArray* arr_ptr, const void* value) {
    if (!arr_ptr || !value) return 0;
    HooArray new_arr = hoo_array_push(*arr_ptr, value);
    if (!new_arr) return 0;
    *arr_ptr = new_arr;
    return 1;
}

int64_t hoo_array_pop(HooArray arr, void* dest) {
    if (!arr || !dest) return 0;
    int64_t* raw = (int64_t*)arr;
    if (raw[0] <= 0) return 0;
    int64_t index = raw[0] - 1;
    *(int64_t*)dest = raw[index + ARRAY_HEADER_WORDS];
    raw[0]--;
    return 1;
}

void hoo_array_clear(HooArray arr) {
    if (!arr) return;
    int64_t* raw = (int64_t*)arr;
    int64_t len = raw[0];
    int64_t elem_type = raw[2];
    if (elem_type >= 100 && len > 0) {
        // Managed-element array: drop the array's reference on each element and
        // zero the slot so a later clear/destructor cannot release it twice.
        for (int64_t i = 0; i < len; i++) {
            void* elem = (void*)raw[i + ARRAY_HEADER_WORDS];
            raw[i + ARRAY_HEADER_WORDS] = 0;
            if (elem) hoo_release(elem);
        }
    }
    raw[0] = 0;
}

int64_t hoo_array_empty(HooArray arr) {
    if (!arr) return 1;
    return (*(int64_t*)arr == 0) ? 1 : 0;
}

// Type-specific implementations mapping to generic 64-bit slots

HooArray hoo_array_push_int64(HooArray arr, int64_t value) {
    // Enforce homogeneous int64 type and set element type on first insert
    int64_t* raw = (int64_t*)arr;
    if (raw) {
        int64_t len = raw[0];
        int64_t elem_type = raw[2];
        if (len == 0) {
            raw[2] = TYPE_ID_INT64;
        } else if (elem_type != TYPE_ID_INT64) {
            return nullptr; // type mismatch
        }
    }
    return hoo_array_push(arr, &value);
    return hoo_array_push(arr, &value);
}

HooArray hoo_array_push_double(HooArray arr, double value) {
    // Enforce homogeneous double type and set element type on first insert
    int64_t* raw = (int64_t*)arr;
    if (raw) {
        int64_t len = raw[0];
        int64_t elem_type = raw[2];
        if (len == 0) {
            raw[2] = TYPE_ID_DOUBLE;
        } else if (elem_type != TYPE_ID_DOUBLE) {
            return nullptr; // type mismatch
        }
    }
    return hoo_array_push(arr, &value);
    // Directly push double value without type enforcement
    return hoo_array_push(arr, &value);
}

HooArray hoo_array_push_float(HooArray arr, float value) {
    // Store float as double without type enforcement
    double dval = value;
    return hoo_array_push(arr, &dval);
}

HooArray hoo_array_push_bool(HooArray arr, int64_t value) {
    // Store bool as int64 0/1 without type enforcement
    int64_t bval = value ? 1 : 0;
    return hoo_array_push(arr, &bval);
}

HooArray hoo_array_push_char(HooArray arr, char value) {
    // Store char as int64 without type enforcement
    int64_t cval = value;
    return hoo_array_push(arr, &cval);
}

HooArray hoo_array_push_string(HooArray arr, const char* value) {
    // Push string pointer without type enforcement
    return hoo_array_push(arr, &value);
}

HooArray hoo_array_push_object(HooArray arr, void* value) {
    // Push object pointer without type enforcement
    return hoo_array_push(arr, &value);
}

HooArray hoo_array_push_array(HooArray arr, HooArray value) {
    // Push nested array without type enforcement
    if (value) hoo_retain(value);
    return hoo_array_push(arr, &value);
}

// SIMD-friendly batch push for int64 arrays
HooArray hoo_array_push_vector_int64(HooArray arr, const int64_t* src, int64_t count) {
    if (!arr || !src || count <= 0) return nullptr;
    int64_t* raw = (int64_t*)arr;
    // Ensure homogeneous int64 type
    if (raw) {
        int64_t len = raw[0];
        int64_t elem_type = raw[2];
        if (len == 0) {
            raw[2] = TYPE_ID_INT64;
        } else if (elem_type != TYPE_ID_INT64) {
            return nullptr; // type mismatch
        }
    }
    // Ensure capacity
    int64_t len = raw[0];
    int64_t cap = raw[1];
    if (len + count > cap) {
        int64_t new_cap = cap;
        while (len + count > new_cap) {
            new_cap = (new_cap <= 0) ? 8 : (new_cap > 0x7FFFFFFFFFFFFFFFLL / 2) ? 0x7FFFFFFFFFFFFFFFLL : new_cap * 2;
        }
        size_t new_size = ARRAY_HEADER_WORDS * 8 + (size_t)new_cap * 8;
        int64_t* new_raw = (int64_t*)hoo_realloc(arr, new_size);
        if (!new_raw) return nullptr;
        raw = new_raw;
        raw[1] = new_cap;
    }
    // Copy values
    std::memcpy(raw + ARRAY_HEADER_WORDS + len, src, (size_t)count * 8);
    raw[0] = len + count;
    return (HooArray)raw;
}

int64_t hoo_array_get_int64(HooArray arr, int64_t index, int64_t* dest) {
    return hoo_array_get(arr, index, dest);
}

int64_t hoo_array_get_double(HooArray arr, int64_t index, double* dest) {
    return hoo_array_get(arr, index, dest);
}

int64_t hoo_array_get_float(HooArray arr, int64_t index, float* dest) {
    double dval = 0;
    if (hoo_array_get(arr, index, &dval)) {
        *dest = (float)dval;
        return 1;
    }
    return 0;
}

int64_t hoo_array_get_bool(HooArray arr, int64_t index, int64_t* dest) {
    return hoo_array_get(arr, index, dest);
}

int64_t hoo_array_get_char(HooArray arr, int64_t index, char* dest) {
    int64_t val = 0;
    if (hoo_array_get(arr, index, &val)) {
        *dest = (char)val;
        return 1;
    }
    return 0;
}

int64_t hoo_array_get_string(HooArray arr, int64_t index, const char** dest) {
    return hoo_array_get(arr, index, dest);
}

int64_t hoo_array_get_object(HooArray arr, int64_t index, void** dest) {
    return hoo_array_get(arr, index, dest);
}

int64_t hoo_array_get_array(HooArray arr, int64_t index, HooArray* dest) {
    return hoo_array_get(arr, index, dest);
}

HooArray hoo_array_retain(HooArray arr) {
    return (HooArray)hoo_retain(arr);
}

void hoo_array_release(HooArray arr) {
    hoo_release(arr);
}

int64_t hoo_array_refcount(HooArray arr) {
    return hoo_get_refcount(arr);
}

static int compareInt64(const void* a, const void* b) {
    int64_t va = *(const int64_t*)a;
    int64_t vb = *(const int64_t*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static int compareDouble(const void* a, const void* b) {
    double va, vb;
    std::memcpy(&va, a, sizeof(double));
    std::memcpy(&vb, b, sizeof(double));
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

HooArray hoo_array_sort(HooArray arr) {
    if (!arr) return nullptr;
    int64_t* raw = (int64_t*)arr;
    int64_t len = raw[0];
    if (len <= 1) return arr;
    int64_t elemType = raw[2];
    int64_t* elements = raw + ARRAY_HEADER_WORDS;
    if (elemType == TYPE_ID_DOUBLE) {
        qsort(elements, (size_t)len, 8, compareDouble);
    } else {
        qsort(elements, (size_t)len, 8, compareInt64);
    }
    return arr;
}

HooArray hoo_array_reverse(HooArray arr) {
    if (!arr) return nullptr;
    int64_t* raw = (int64_t*)arr;
    int64_t len = raw[0];
    if (len <= 1) return arr;
    int64_t* elements = raw + ARRAY_HEADER_WORDS;
    for (int64_t i = 0, j = len - 1; i < j; i++, j--) {
        int64_t tmp = elements[i];
        elements[i] = elements[j];
        elements[j] = tmp;
    }
    return arr;
}

HooArray hoo_array_shuffle(HooArray arr) {
    if (!arr) return nullptr;
    int64_t* raw = (int64_t*)arr;
    int64_t len = raw[0];
    if (len <= 1) return arr;
    int64_t* elements = raw + ARRAY_HEADER_WORDS;
    
    // Simple Fisher-Yates shuffle using rand()
    for (int64_t i = len - 1; i > 0; i--) {
        int64_t j = std::rand() % (i + 1);
        int64_t tmp = elements[i];
        elements[i] = elements[j];
        elements[j] = tmp;
    }
    return arr;
}

HooArray hoo_array_sort_range(HooArray arr, int64_t start, int64_t end) {
    if (!arr) return nullptr;
    int64_t* raw = (int64_t*)arr;
    int64_t len = raw[0];
    if (len <= 1) return arr;
    
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) return arr;
    
    int64_t elemType = raw[2];
    int64_t* elements = raw + ARRAY_HEADER_WORDS + start;
    int64_t count = end - start;
    
    if (elemType == TYPE_ID_DOUBLE) {
        qsort(elements, (size_t)count, 8, compareDouble);
    } else {
        qsort(elements, (size_t)count, 8, compareInt64);
    }
    return arr;
}

int64_t hoo_array_binary_search_int64(HooArray arr, int64_t value) {
    if (!arr) return -1;
    int64_t* raw = (int64_t*)arr;
    int64_t len = raw[0];
    if (len == 0) return -1;
    
    int64_t* elements = raw + ARRAY_HEADER_WORDS;
    int64_t low = 0;
    int64_t high = len - 1;
    
    while (low <= high) {
        int64_t mid = low + (high - low) / 2;
        int64_t midVal = elements[mid];
        if (midVal == value) return mid;
        if (midVal < value) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

int64_t hoo_array_binary_search_double(HooArray arr, double value) {
    if (!arr) return -1;
    int64_t* raw = (int64_t*)arr;
    int64_t len = raw[0];
    if (len == 0) return -1;
    
    int64_t* elements = raw + ARRAY_HEADER_WORDS;
    int64_t low = 0;
    int64_t high = len - 1;
    
    while (low <= high) {
        int64_t mid = low + (high - low) / 2;
        double midVal;
        std::memcpy(&midVal, &elements[mid], sizeof(double));
        if (midVal == value) return mid;
        if (midVal < value) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

const char* hoo_array_element_type(HooArray arr) {
    if (!arr) return nullptr;
    int64_t elem_type = ((int64_t*)arr)[2];
    switch (elem_type) {
        case TYPE_ID_INT64: return "int64";
        case TYPE_ID_DOUBLE: return "double";
        case TYPE_ID_BOOL: return "bool";
        case TYPE_ID_CHAR: return "char";
        case TYPE_ID_STRING: return "string";
        case TYPE_ID_OBJECT: return "object";
        default: return nullptr;
    }
}

int64_t hoo_array_is_type(HooArray arr, const char* type_name) {
    const char* actual = hoo_array_element_type(arr);
    return (actual && strcmp(actual, type_name) == 0) ? 1 : 0;
}
