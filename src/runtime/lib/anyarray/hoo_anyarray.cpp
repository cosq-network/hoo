#include "runtime/lib/anyarray/hoo_anyarray.h"
#include "runtime/lib/runtime/hoo_runtime.h"

#include <new>
#include <vector>

namespace {

struct HooAnyArrayImpl {
    std::vector<HooAnyValue> elements;

    ~HooAnyArrayImpl() {
        clear();
    }

    void clear() {
        for (const auto& value : elements) {
            hoo_any_release(value);
        }
        elements.clear();
    }
};

bool registerAnyArrayDestructor() {
    hoo_register_destructor(HOO_TYPE_ANYARRAY, [](void* obj) {
        static_cast<HooAnyArrayImpl*>(obj)->~HooAnyArrayImpl();
    });
    return true;
}

bool g_anyArrayDtorRegistered = registerAnyArrayDestructor();

HooAnyValue makeValue(int64_t type_id, uint64_t data) {
    return HooAnyValue{type_id, data};
}

}

extern "C" {

HooAnyArray hoo_anyarray_new(void) {
    return hoo_anyarray_new_capacity(0);
}

HooAnyArray hoo_anyarray_new_capacity(int64_t capacity) {
    try {
        void* mem = hoo_alloc(sizeof(HooAnyArrayImpl), HOO_TYPE_ANYARRAY);
        auto* impl = new (mem) HooAnyArrayImpl();
        if (capacity > 0) {
            impl->elements.reserve(static_cast<size_t>(capacity));
        }
        return impl;
    } catch (...) {
        return nullptr;
    }
}

HooAnyArray hoo_anyarray_retain(HooAnyArray array) {
    return hoo_retain(array);
}

void hoo_anyarray_release(HooAnyArray array) {
    if (array) hoo_release(array);
}

int64_t hoo_anyarray_refcount(HooAnyArray array) {
    return hoo_get_refcount(array);
}

int64_t hoo_anyarray_length(HooAnyArray array) {
    if (!array) return 0;
    return static_cast<int64_t>(static_cast<HooAnyArrayImpl*>(array)->elements.size());
}

int64_t hoo_anyarray_push(HooAnyArray array, int64_t type_id, uint64_t data) {
    if (!array) return 0;
    try {
        HooAnyValue value = makeValue(type_id, data);
        hoo_any_retain(value);
        static_cast<HooAnyArrayImpl*>(array)->elements.push_back(value);
        return 1;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_anyarray_set(HooAnyArray array, int64_t index, int64_t type_id, uint64_t data) {
    if (!array || index < 0) return 0;
    auto* impl = static_cast<HooAnyArrayImpl*>(array);
    const auto idx = static_cast<size_t>(index);
    if (idx >= impl->elements.size()) return 0;

    HooAnyValue value = makeValue(type_id, data);
    hoo_any_retain(value);
    hoo_any_release(impl->elements[idx]);
    impl->elements[idx] = value;
    return 1;
}

int64_t hoo_anyarray_get(HooAnyArray array, int64_t index, HooAnyValue* out) {
    if (!array || !out || index < 0) return 0;
    auto* impl = static_cast<HooAnyArrayImpl*>(array);
    const auto idx = static_cast<size_t>(index);
    if (idx >= impl->elements.size()) return 0;
    *out = impl->elements[idx];
    return 1;
}

int64_t hoo_anyarray_pop(HooAnyArray array, HooAnyValue* out) {
    if (!array) return 0;
    auto* impl = static_cast<HooAnyArrayImpl*>(array);
    if (impl->elements.empty()) return 0;

    HooAnyValue value = impl->elements.back();
    impl->elements.pop_back();
    if (out) {
        *out = value;
    } else {
        hoo_any_release(value);
    }
    return 1;
}

void hoo_anyarray_clear(HooAnyArray array) {
    if (!array) return;
    static_cast<HooAnyArrayImpl*>(array)->clear();
}

}
