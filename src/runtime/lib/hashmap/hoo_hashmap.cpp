#include "runtime/lib/hashmap/hoo_hashmap.h"
#include "runtime/lib/runtime/hoo_runtime.h"

#include <new>
#include <unordered_map>

namespace {

struct HooHashMapImpl {
    int64_t key_type_id;
    int64_t value_type_id;
    std::unordered_map<int64_t, uint64_t> fixed_values;
    std::unordered_map<int64_t, HooAnyValue> any_values;

    HooHashMapImpl(int64_t keyType, int64_t valueType)
        : key_type_id(keyType), value_type_id(valueType) {}

    ~HooHashMapImpl() {
        clear();
    }

    void clear() {
        if (value_type_id == HOO_TYPE_ANY) {
            for (const auto& [key, value] : any_values) {
                hoo_any_release(value);
            }
            any_values.clear();
        }
        fixed_values.clear();
    }
};

bool isSupportedKeyType(int64_t typeId) {
    return typeId == HOO_TYPE_INT64 || typeId == HOO_TYPE_INT8 || typeId == HOO_TYPE_BYTE;
}

bool registerHashMapDestructor() {
    hoo_register_destructor(HOO_TYPE_HASHMAP, [](void* obj) {
        static_cast<HooHashMapImpl*>(obj)->~HooHashMapImpl();
    });
    return true;
}

bool g_hashMapDtorRegistered = registerHashMapDestructor();

}

extern "C" {

HooHashMap hoo_hashmap_new(int64_t key_type_id, int64_t value_type_id) {
    if (!isSupportedKeyType(key_type_id)) return nullptr;
    try {
        void* mem = hoo_alloc(sizeof(HooHashMapImpl), HOO_TYPE_HASHMAP);
        return new (mem) HooHashMapImpl(key_type_id, value_type_id);
    } catch (...) {
        return nullptr;
    }
}

HooHashMap hoo_hashmap_retain(HooHashMap map) {
    return hoo_retain(map);
}

void hoo_hashmap_release(HooHashMap map) {
    if (map) hoo_release(map);
}

int64_t hoo_hashmap_refcount(HooHashMap map) {
    return hoo_get_refcount(map);
}

int64_t hoo_hashmap_count(HooHashMap map) {
    if (!map) return 0;
    auto* impl = static_cast<HooHashMapImpl*>(map);
    return static_cast<int64_t>(impl->value_type_id == HOO_TYPE_ANY
        ? impl->any_values.size()
        : impl->fixed_values.size());
}

int64_t hoo_hashmap_key_type(HooHashMap map) {
    if (!map) return -1;
    return static_cast<HooHashMapImpl*>(map)->key_type_id;
}

int64_t hoo_hashmap_value_type(HooHashMap map) {
    if (!map) return -1;
    return static_cast<HooHashMapImpl*>(map)->value_type_id;
}

void hoo_hashmap_clear(HooHashMap map) {
    if (!map) return;
    static_cast<HooHashMapImpl*>(map)->clear();
}

int64_t hoo_hashmap_remove_i8(HooHashMap map, int64_t key) {
    if (!map) return 0;
    auto* impl = static_cast<HooHashMapImpl*>(map);
    if (impl->value_type_id == HOO_TYPE_ANY) {
        auto it = impl->any_values.find(key);
        if (it == impl->any_values.end()) return 0;
        hoo_any_release(it->second);
        impl->any_values.erase(it);
        return 1;
    }
    return impl->fixed_values.erase(key) ? 1 : 0;
}

int64_t hoo_hashmap_set_fixed_i8(HooHashMap map, int64_t key, uint64_t data) {
    if (!map) return 0;
    auto* impl = static_cast<HooHashMapImpl*>(map);
    if (impl->value_type_id == HOO_TYPE_ANY) return 0;
    impl->fixed_values[key] = data;
    return 1;
}

int64_t hoo_hashmap_get_fixed_i8(HooHashMap map, int64_t key, uint64_t* out) {
    if (!map || !out) return 0;
    auto* impl = static_cast<HooHashMapImpl*>(map);
    if (impl->value_type_id == HOO_TYPE_ANY) return 0;
    auto it = impl->fixed_values.find(key);
    if (it == impl->fixed_values.end()) return 0;
    *out = it->second;
    return 1;
}

int64_t hoo_hashmap_set_any_i8(HooHashMap map, int64_t key, int64_t type_id, uint64_t data) {
    if (!map) return 0;
    auto* impl = static_cast<HooHashMapImpl*>(map);
    if (impl->value_type_id != HOO_TYPE_ANY) return 0;

    HooAnyValue next{type_id, data};
    hoo_any_retain(next);
    auto it = impl->any_values.find(key);
    if (it != impl->any_values.end()) {
        hoo_any_release(it->second);
        it->second = next;
    } else {
        impl->any_values.emplace(key, next);
    }
    return 1;
}

int64_t hoo_hashmap_get_any_i8(HooHashMap map, int64_t key, HooAnyValue* out) {
    if (!map || !out) return 0;
    auto* impl = static_cast<HooHashMapImpl*>(map);
    if (impl->value_type_id != HOO_TYPE_ANY) return 0;
    auto it = impl->any_values.find(key);
    if (it == impl->any_values.end()) return 0;
    *out = it->second;
    return 1;
}

int64_t hoo_hashmap_get_keys_i8(HooHashMap map, int64_t* keys, int64_t max_count) {
    if (!map || !keys || max_count <= 0) return 0;
    auto* impl = static_cast<HooHashMapImpl*>(map);
    int64_t written = 0;
    if (impl->value_type_id == HOO_TYPE_ANY) {
        for (const auto& [key, value] : impl->any_values) {
            (void)value;
            if (written >= max_count) break;
            keys[written++] = key;
        }
    } else {
        for (const auto& [key, value] : impl->fixed_values) {
            (void)value;
            if (written >= max_count) break;
            keys[written++] = key;
        }
    }
    return written;
}

int64_t hoo_hashmap_get_fixed_at_i8(HooHashMap map, int64_t key, uint64_t* out) {
    return hoo_hashmap_get_fixed_i8(map, key, out);
}

int64_t hoo_hashmap_get_any_at_i8(HooHashMap map, int64_t key, HooAnyValue* out) {
    return hoo_hashmap_get_any_i8(map, key, out);
}

}
