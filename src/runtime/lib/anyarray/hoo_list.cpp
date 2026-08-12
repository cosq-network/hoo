#include "runtime/lib/anyarray/hoo_list.h"
#include "runtime/lib/runtime/hoo_runtime.h"

#include <new>
#include <vector>

namespace {

struct HooListImpl {
    std::vector<HooAnyValue> elements;

    ~HooListImpl() {
        clear();
    }

    void clear() {
        for (const auto& value : elements) {
            hoo_any_release(value);
        }
        elements.clear();
    }
};

bool registerListDestructor() {
    hoo_register_destructor(HOO_TYPE_LIST, [](void* obj) {
        static_cast<HooListImpl*>(obj)->~HooListImpl();
    });
    return true;
}

bool g_listDtorRegistered = registerListDestructor();

HooAnyValue makeValue(int64_t type_id, uint64_t data) {
    return HooAnyValue{type_id, data};
}

}

extern "C" {

HooList hoo_list_new(void) {
    return hoo_list_new_capacity(0);
}

HooList hoo_list_new_capacity(int64_t capacity) {
    try {
        void* mem = hoo_alloc(sizeof(HooListImpl), HOO_TYPE_LIST);
        auto* impl = new (mem) HooListImpl();
        if (capacity > 0) {
            impl->elements.reserve(static_cast<size_t>(capacity));
        }
        return impl;
    } catch (...) {
        return nullptr;
    }
}

HooList hoo_list_retain(HooList array) {
    return hoo_retain(array);
}

void hoo_list_release(HooList array) {
    if (array) hoo_release(array);
}

int64_t hoo_list_refcount(HooList array) {
    return hoo_get_refcount(array);
}

int64_t hoo_list_length(HooList array) {
    if (!array) return 0;
    return static_cast<int64_t>(static_cast<HooListImpl*>(array)->elements.size());
}

int64_t hoo_list_push(HooList array, int64_t type_id, uint64_t data) {
    if (!array) return 0;
    try {
        HooAnyValue value = makeValue(type_id, data);
        hoo_any_retain(value);
        static_cast<HooListImpl*>(array)->elements.push_back(value);
        return 1;
    } catch (...) {
        return 0;
    }
}

int64_t hoo_list_set(HooList array, int64_t index, int64_t type_id, uint64_t data) {
    if (!array || index < 0) return 0;
    auto* impl = static_cast<HooListImpl*>(array);
    const auto idx = static_cast<size_t>(index);
    if (idx >= impl->elements.size()) return 0;

    HooAnyValue value = makeValue(type_id, data);
    hoo_any_retain(value);
    hoo_any_release(impl->elements[idx]);
    impl->elements[idx] = value;
    return 1;
}

int64_t hoo_list_get(HooList array, int64_t index, HooAnyValue* out) {
    if (!array || !out || index < 0) return 0;
    auto* impl = static_cast<HooListImpl*>(array);
    const auto idx = static_cast<size_t>(index);
    if (idx >= impl->elements.size()) return 0;
    *out = impl->elements[idx];
    return 1;
}

int64_t hoo_list_pop(HooList array, HooAnyValue* out) {
    if (!array) return 0;
    auto* impl = static_cast<HooListImpl*>(array);
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

void hoo_list_clear(HooList array) {
    if (!array) return;
    static_cast<HooListImpl*>(array)->clear();
}

}
