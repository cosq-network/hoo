#include "hoo_generic_array.h"
#include "hoo_runtime.h"
#include <cstring>
#include <cstdlib>

// ============================================================================
// Low-level HooArray Implementation (Hardware Ready)
// ============================================================================
// Format: [length (8 bytes)][element 0 (8 bytes)][element 1 (8 bytes)]...
// Allocated via hoo_alloc with HOO_TYPE_ARRAY.

HooArray hoo_array_new(void) {
    int64_t* arr = (int64_t*)hoo_alloc(8, HOO_TYPE_ARRAY);
    arr[0] = 0;
    return (HooArray)arr;
}

HooArray hoo_array_from_buffer(const void* data, int64_t length) {
    if (length < 0) length = 0;
    int64_t* arr = (int64_t*)hoo_alloc(8 + (length * 8), HOO_TYPE_ARRAY);
    arr[0] = length;
    if (data && length > 0) {
        std::memcpy(arr + 1, data, length * 8);
    }
    return (HooArray)arr;
}

HooArray hoo_array_repeat(const void* value, int64_t count) {
    if (count < 0) count = 0;
    int64_t* arr = (int64_t*)hoo_alloc(8 + (count * 8), HOO_TYPE_ARRAY);
    arr[0] = count;
    if (value) {
        int64_t val = *(const int64_t*)value;
        for (int64_t i = 0; i < count; i++) {
            arr[i + 1] = val;
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
    *(int64_t*)dest = ((int64_t*)arr)[index + 1];
    return 1;
}

int64_t hoo_array_set(HooArray arr, int64_t index, const void* value) {
    if (!arr || !value) return 0;
    int64_t length = *(int64_t*)arr;
    if (index < 0 || index >= length) return 0;
    ((int64_t*)arr)[index + 1] = *(const int64_t*)value;
    return 1;
}

int64_t hoo_array_push(HooArray arr, const void* value) {
    // Current implementation doesn't support dynamic resizing easily because
    // hoo_alloc doesn't have a realloc yet.
    // For now, return -1 or implement a basic resize if needed.
    // However, the CodeGen currently only uses fixed-size arrays from literals.
    return -1; 
}

int64_t hoo_array_pop(HooArray arr, void* dest) {
    if (!arr || !dest) return 0;
    int64_t* raw = (int64_t*)arr;
    if (raw[0] <= 0) return 0;
    *(int64_t*)dest = raw[raw[0]];
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

int64_t hoo_array_push_int64(HooArray arr, int64_t value) {
    return hoo_array_push(arr, &value);
}

int64_t hoo_array_push_double(HooArray arr, double value) {
    return hoo_array_push(arr, &value);
}

int64_t hoo_array_push_float(HooArray arr, float value) {
    double dval = value;
    return hoo_array_push(arr, &dval);
}

int64_t hoo_array_push_bool(HooArray arr, int64_t value) {
    int64_t bval = value ? 1 : 0;
    return hoo_array_push(arr, &bval);
}

int64_t hoo_array_push_char(HooArray arr, char value) {
    int64_t cval = value;
    return hoo_array_push(arr, &cval);
}

int64_t hoo_array_push_string(HooArray arr, const char* value) {
    return hoo_array_push(arr, &value);
}

int64_t hoo_array_push_object(HooArray arr, void* value) {
    return hoo_array_push(arr, &value);
}

int64_t hoo_array_push_array(HooArray arr, HooArray value) {
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
    // If releasing arrays, we should ideally release their elements too if they are managed.
    // But currently the runtime doesn't have metadata about element types in raw arrays.
    // This is a known gap in the current low-level ARC.
    hoo_release(arr);
}

int64_t hoo_array_refcount(HooArray arr) {
    return hoo_get_refcount(arr);
}

const char* hoo_array_element_type(HooArray arr) {
    return "raw64";
}

int64_t hoo_array_is_type(HooArray arr, const char* type_name) {
    return 0;
}
