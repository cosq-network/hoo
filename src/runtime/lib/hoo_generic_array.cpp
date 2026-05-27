#include "hoo_generic_array.h"
#include "hoo_runtime.h"
#include <cstring>
#include <cstdlib>

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
    int64_t length = *(int64_t*)arr;
    if (index < 0 || index >= length) return 0;
    ((int64_t*)arr)[index + ARRAY_HEADER_WORDS] = *(const int64_t*)value;
    return 1;
}

HooArray hoo_array_push(HooArray arr_handle, const void* value) {
    if (!arr_handle || !value) return nullptr;
    int64_t* raw = (int64_t*)arr_handle;
    int64_t len = raw[0];
    int64_t cap = raw[1];
    
    if (len >= cap) {
        int64_t new_cap = (cap <= 0) ? 8 : cap * 2;
        size_t new_size = ARRAY_HEADER_WORDS * 8 + (size_t)new_cap * 8;
        int64_t* new_raw = (int64_t*)hoo_realloc(arr_handle, new_size);
        if (!new_raw) return nullptr;
        
        raw = new_raw;
        raw[1] = new_cap;
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
    *(int64_t*)arr = 0;
}

int64_t hoo_array_empty(HooArray arr) {
    if (!arr) return 1;
    return (*(int64_t*)arr == 0) ? 1 : 0;
}

// Type-specific implementations mapping to generic 64-bit slots

HooArray hoo_array_push_int64(HooArray arr, int64_t value) {
    return hoo_array_push(arr, &value);
}

HooArray hoo_array_push_double(HooArray arr, double value) {
    return hoo_array_push(arr, &value);
}

HooArray hoo_array_push_float(HooArray arr, float value) {
    double dval = value;
    return hoo_array_push(arr, &dval);
}

HooArray hoo_array_push_bool(HooArray arr, int64_t value) {
    int64_t bval = value ? 1 : 0;
    return hoo_array_push(arr, &bval);
}

HooArray hoo_array_push_char(HooArray arr, char value) {
    int64_t cval = value;
    return hoo_array_push(arr, &cval);
}

HooArray hoo_array_push_string(HooArray arr, const char* value) {
    return hoo_array_push(arr, &value);
}

HooArray hoo_array_push_object(HooArray arr, void* value) {
    return hoo_array_push(arr, &value);
}

HooArray hoo_array_push_array(HooArray arr, HooArray value) {
    if (value) hoo_retain(value);
    return hoo_array_push(arr, &value);
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
    if (!arr) return;
    int64_t* raw = (int64_t*)arr;
    int64_t len = raw[0];
    int64_t elem_type = raw[2];

    // If it's an object array, release all elements
    if (elem_type >= 100) {
        for (int64_t i = 0; i < len; i++) {
            void* obj = (void*)raw[i + ARRAY_HEADER_WORDS];
            if (obj) hoo_release(obj);
        }
    }
    hoo_release(arr);
}

int64_t hoo_array_refcount(HooArray arr) {
    return hoo_get_refcount(arr);
}

const char* hoo_array_element_type(HooArray arr) {
    if (hoo_array_length(arr) == 0) return nullptr;
    return "raw64";
}

int64_t hoo_array_is_type(HooArray arr, const char* type_name) {
    return 0;
}
