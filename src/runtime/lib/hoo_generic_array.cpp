#include "hoo_generic_array.h"
#include "hoo_runtime.h"
#include <new>
#include <cstring>
#include <cassert>

namespace hooc {

// ============================================================================
// HooArrayImpl Implementation
// ============================================================================

HooArrayImpl::HooArrayImpl()
    : element_type(nullptr) {
}

HooArrayImpl::~HooArrayImpl() {
}

}  // namespace hooc

// ============================================================================
// C API Implementations
// ============================================================================

HooArray hoo_array_new(void) {
    try {
        void* mem = hoo_alloc(sizeof(hooc::HooArrayImpl), HOO_TYPE_ARRAY);
        auto* impl = new (mem) hooc::HooArrayImpl();
        return static_cast<HooArray>(impl);
    } catch (...) {
        return nullptr;
    }
}

HooArray hoo_array_from_buffer(const void* data, int64_t length) {
    return hoo_array_new();
}

HooArray hoo_array_repeat(const void* value, int64_t count) {
    return hoo_array_new();
}

int64_t hoo_array_length(HooArray arr) {
    if (!arr) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->length();
}

int64_t hoo_array_get(HooArray arr, int64_t index, void* dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);

    if (index < 0 || index >= impl->length()) {
        return 0;
    }

    // Generic get - not directly supported
    // Use type-specific getters instead
    return 0;
}

int64_t hoo_array_set(HooArray arr, int64_t index, const void* value) {
    if (!arr || !value) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);

    if (index < 0 || index >= impl->length()) {
        return 0;
    }

    // Generic set - not directly supported
    // Use type-specific setters instead
    return 0;
}

int64_t hoo_array_push(HooArray arr, const void* value) {
    if (!arr) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);

    // Generic push - not directly supported
    // Use type-specific push functions
    return impl->length();
}

int64_t hoo_array_pop(HooArray arr, void* dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);

    if (impl->empty()) {
        return 0;
    }

    // Generic pop - not directly supported
    // Use type-specific pop functions
    return 0;
}

void hoo_array_clear(HooArray arr) {
    if (!arr) return;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    impl->clear();
}

int64_t hoo_array_empty(HooArray arr) {
    if (!arr) return 1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->empty() ? 1 : 0;
}

// ============================================================================
// Type-Specific Push Operations
// ============================================================================

int64_t hoo_array_push_int64(HooArray arr, int64_t value) {
    if (!arr) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    try {
        impl->push<int64_t>(value);
        return impl->length();
    } catch (...) {
        return -1;
    }
}

int64_t hoo_array_push_double(HooArray arr, double value) {
    if (!arr) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    try {
        impl->push<double>(value);
        return impl->length();
    } catch (...) {
        return -1;
    }
}

int64_t hoo_array_push_float(HooArray arr, float value) {
    if (!arr) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    try {
        impl->push<float>(value);
        return impl->length();
    } catch (...) {
        return -1;
    }
}

int64_t hoo_array_push_bool(HooArray arr, int64_t value) {
    if (!arr) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    try {
        bool bvalue = (value != 0);
        impl->push<bool>(bvalue);
        return impl->length();
    } catch (...) {
        return -1;
    }
}

int64_t hoo_array_push_char(HooArray arr, char value) {
    if (!arr) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    try {
        impl->push<char>(value);
        return impl->length();
    } catch (...) {
        return -1;
    }
}

int64_t hoo_array_push_string(HooArray arr, const char* value) {
    if (!arr || !value) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    try {
        impl->push<const char*>(value);
        return impl->length();
    } catch (...) {
        return -1;
    }
}

int64_t hoo_array_push_object(HooArray arr, void* value) {
    if (!arr) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    try {
        impl->push<void*>(value);
        return impl->length();
    } catch (...) {
        return -1;
    }
}

int64_t hoo_array_push_array(HooArray arr, HooArray value) {
    if (!arr) return -1;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);

    // Retain the nested array to maintain reference count
    if (value) {
        hoo_array_retain(value);
    }

    try {
        impl->push<HooArray>(value);
        return impl->length();
    } catch (...) {
        return -1;
    }
}

// ============================================================================
// Type-Specific Get Operations
// ============================================================================

int64_t hoo_array_get_int64(HooArray arr, int64_t index, int64_t* dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->get<int64_t>(index, *dest) ? 1 : 0;
}

int64_t hoo_array_get_double(HooArray arr, int64_t index, double* dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->get<double>(index, *dest) ? 1 : 0;
}

int64_t hoo_array_get_float(HooArray arr, int64_t index, float* dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->get<float>(index, *dest) ? 1 : 0;
}

int64_t hoo_array_get_bool(HooArray arr, int64_t index, int64_t* dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    bool bvalue = false;
    int result = impl->get<bool>(index, bvalue) ? 1 : 0;
    *dest = bvalue ? 1 : 0;
    return result;
}

int64_t hoo_array_get_char(HooArray arr, int64_t index, char* dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->get<char>(index, *dest) ? 1 : 0;
}

int64_t hoo_array_get_string(HooArray arr, int64_t index, const char** dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->get<const char*>(index, *dest) ? 1 : 0;
}

int64_t hoo_array_get_object(HooArray arr, int64_t index, void** dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->get<void*>(index, *dest) ? 1 : 0;
}

int64_t hoo_array_get_array(HooArray arr, int64_t index, HooArray* dest) {
    if (!arr || !dest) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    return impl->get<HooArray>(index, *dest) ? 1 : 0;
}

// ============================================================================
// Reference Counting
// ============================================================================

HooArray hoo_array_retain(HooArray arr) {
    return (HooArray)hoo_retain(arr);
}

void hoo_array_release(HooArray arr) {
    if (!arr) return;

    if (hoo_get_refcount(arr) == 1) {
        auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
        impl->~HooArrayImpl();
    }

    hoo_release(arr);
}

int64_t hoo_array_refcount(HooArray arr) {
    return hoo_get_refcount(arr);
}

// ============================================================================
// Type Information
// ============================================================================

const char* hoo_array_element_type(HooArray arr) {
    if (!arr) return nullptr;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    const std::type_info* type = impl->getElementType();
    if (!type) return nullptr;
    return type->name();
}

int64_t hoo_array_is_type(HooArray arr, const char* type_name) {
    if (!arr || !type_name) return 0;
    auto* impl = static_cast<hooc::HooArrayImpl*>(arr);
    const std::type_info* type = impl->getElementType();
    if (!type) return 0;

    // Compare type names as strings
    std::string current_name = type->name();
    std::string check_name = type_name;
    return (current_name == check_name) ? 1 : 0;
}
