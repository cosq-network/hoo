#include "runtime/lib/mem/hoo_map.h"
#include "runtime/lib/core/hoo_runtime.h"
#include <new>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <unordered_map>

// ============================================================================
// Destructor registration (called once at module load)
// ============================================================================
namespace {
    bool registerMapDestructor() {
        hoo_register_destructor(HOO_TYPE_MAP, [](void* obj) {
            auto* impl = static_cast<hooc::HooMapImpl*>(obj);
            impl->~HooMapImpl();
        });
        return true;
    }
    bool g_mapDtorRegistered = registerMapDestructor();
}

namespace hooc {

// ============================================================================
// HooMapImpl Implementation
// ============================================================================

HooMapImpl::HooMapImpl(int keyType, int valueType)
    : keyType_(keyType), valueType_(valueType) {
}

HooMapImpl::~HooMapImpl() {
    releaseAllValues();
}

void HooMapImpl::releaseValue(std::any& val) {
    if (valueType_ == HOO_MAP_VAL_STRING) {
        const char** p = std::any_cast<const char*>(&val);
        if (p && *p) {
            free(const_cast<char*>(*p));
        }
    }
}

void HooMapImpl::releaseAllValues() {
    if (valueType_ == HOO_MAP_VAL_STRING) {
        for (auto& [k, v] : data_int8_)   releaseValue(v);
        for (auto& [k, v] : data_int64_)  releaseValue(v);
        for (auto& [k, v] : data_char_)   releaseValue(v);
        for (auto& [k, v] : data_string_) releaseValue(v);
    }
}

// ---------------------------------------------------------------------------
// Type-erased value set/get helpers
// ---------------------------------------------------------------------------

int64_t HooMapImpl::setValue(std::any& slot, const void* value, int valueType) {
    switch (valueType) {
        case HOO_MAP_VAL_INT64:
        case HOO_MAP_VAL_BOOL:
            slot = *static_cast<const int64_t*>(value);
            return 1;
        case HOO_MAP_VAL_DOUBLE:
            slot = *static_cast<const double*>(value);
            return 1;
        case HOO_MAP_VAL_INT8:
            slot = *static_cast<const int8_t*>(value);
            return 1;
        case HOO_MAP_VAL_CHAR: {
            int64_t c = *static_cast<const int64_t*>(value);
            slot = static_cast<int8_t>(c);
            return 1;
        }
        case HOO_MAP_VAL_STRING: {
            const char* s = static_cast<const char*>(value);
            if (!s) return -1;
            // Release old value if replacing
            const char** old = std::any_cast<const char*>(&slot);
            if (old && *old) free(const_cast<char*>(*old));
            slot = (const char*)strdup(s);
            return 1;
        }
        case HOO_MAP_VAL_OBJECT:
            slot = const_cast<void*>(static_cast<const void*>(value));
            return 1;
        default:
            slot = const_cast<void*>(static_cast<const void*>(value));
            return 1;
    }
}

int64_t HooMapImpl::getValue(const std::any& slot, void* value, int valueType) {
    switch (valueType) {
        case HOO_MAP_VAL_INT64:
        case HOO_MAP_VAL_BOOL: {
            auto p = std::any_cast<int64_t>(&slot);
            if (!p) return 0;
            *static_cast<int64_t*>(value) = *p;
            return 1;
        }
        case HOO_MAP_VAL_DOUBLE: {
            auto p = std::any_cast<double>(&slot);
            if (!p) return 0;
            *static_cast<double*>(value) = *p;
            return 1;
        }
        case HOO_MAP_VAL_INT8: {
            auto p = std::any_cast<int8_t>(&slot);
            if (!p) return 0;
            *static_cast<int8_t*>(value) = *p;
            return 1;
        }
        case HOO_MAP_VAL_CHAR: {
            auto p = std::any_cast<int8_t>(&slot);
            if (!p) return 0;
            // Return as int64 in output buffer for JIT compatibility
            *static_cast<int64_t*>(value) = static_cast<int64_t>(*p);
            return 1;
        }
        case HOO_MAP_VAL_STRING: {
            auto p = std::any_cast<const char*>(&slot);
            if (!p || !*p) return 0;
            *static_cast<const char**>(value) = *p;
            return 1;
        }
        case HOO_MAP_VAL_OBJECT: {
            auto p = std::any_cast<void*>(&slot);
            if (!p) return 0;
            *static_cast<void**>(value) = *p;
            return 1;
        }
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// Dispatch-based key helpers (templated to work with any map type)
// ---------------------------------------------------------------------------

template<typename MapT, typename KeyT>
void HooMapImpl::releaseOldValue(MapT& map, const KeyT& key, int valueType) {
    if (valueType == HOO_MAP_VAL_STRING) {
        auto it = map.find(key);
        if (it != map.end()) {
            const char** p = std::any_cast<const char*>(&it->second);
            if (p && *p) free(const_cast<char*>(*p));
        }
    }
}

template<typename MapT, typename KeyT>
int64_t HooMapImpl::containsKey(const MapT& map, const KeyT& key) {
    return map.find(key) != map.end() ? 1 : 0;
}

template<typename MapT, typename KeyT>
int64_t HooMapImpl::setTyped(MapT& map, const KeyT& key, const void* value, int valueType) {
    releaseOldValue(map, key, valueType);
    auto& slot = map[key];
    return HooMapImpl::setValue(slot, value, valueType);
}

template<typename MapT, typename KeyT>
int64_t HooMapImpl::getTyped(const MapT& map, const KeyT& key, void* value, int valueType) {
    auto it = map.find(key);
    if (it == map.end()) return 0;
    return HooMapImpl::getValue(it->second, value, valueType);
}

template<typename MapT, typename KeyT>
int64_t HooMapImpl::removeTyped(MapT& map, const KeyT& key, int valueType) {
    auto it = map.find(key);
    if (it == map.end()) return 0;
    releaseOldValue(map, key, valueType);
    map.erase(it);
    return 1;
}

// ---------------------------------------------------------------------------
// Public dispatch methods  (called by C-ABI wrappers)
// ---------------------------------------------------------------------------

int64_t HooMapImpl::length() const {
    switch (keyType_) {
        case HOO_MAP_KEY_BYTE:
        case HOO_MAP_KEY_INT8:
            return static_cast<int64_t>(data_int8_.size());
        case HOO_MAP_KEY_INT64:
            return static_cast<int64_t>(data_int64_.size());
        case HOO_MAP_KEY_CHAR:
            return static_cast<int64_t>(data_char_.size());
        case HOO_MAP_KEY_STRING:
            return static_cast<int64_t>(data_string_.size());
    }
    return 0;
}

bool HooMapImpl::empty() const {
    return length() == 0;
}

void HooMapImpl::clearAll() {
    releaseAllValues();
    data_int8_.clear();
    data_int64_.clear();
    data_char_.clear();
    data_string_.clear();
}

int64_t HooMapImpl::containsByKey(const void* key) const {
    switch (keyType_) {
        case HOO_MAP_KEY_BYTE:
        case HOO_MAP_KEY_INT8:
            return containsKey(data_int8_, *static_cast<const int8_t*>(key));
        case HOO_MAP_KEY_INT64:
            return containsKey(data_int64_, *static_cast<const int64_t*>(key));
        case HOO_MAP_KEY_CHAR:
            return containsKey(data_char_, static_cast<int8_t>(*static_cast<const int64_t*>(key)));
        case HOO_MAP_KEY_STRING:
            return containsKey(data_string_, std::string(static_cast<const char*>(key)));
    }
    return 0;
}

int64_t HooMapImpl::setByKeyAndValue(const void* key, const void* value) {
    switch (keyType_) {
        case HOO_MAP_KEY_BYTE:
        case HOO_MAP_KEY_INT8:
            return setTyped(data_int8_, *static_cast<const int8_t*>(key), value, valueType_);
        case HOO_MAP_KEY_INT64:
            return setTyped(data_int64_, *static_cast<const int64_t*>(key), value, valueType_);
        case HOO_MAP_KEY_CHAR:
            return setTyped(data_char_, static_cast<int8_t>(*static_cast<const int64_t*>(key)), value, valueType_);
        case HOO_MAP_KEY_STRING:
            return setTyped(data_string_, std::string(static_cast<const char*>(key)), value, valueType_);
    }
    return 0;
}

int64_t HooMapImpl::getByKeyAndValue(const void* key, void* value) const {
    switch (keyType_) {
        case HOO_MAP_KEY_BYTE:
        case HOO_MAP_KEY_INT8:
            return getTyped(data_int8_, *static_cast<const int8_t*>(key), value, valueType_);
        case HOO_MAP_KEY_INT64:
            return getTyped(data_int64_, *static_cast<const int64_t*>(key), value, valueType_);
        case HOO_MAP_KEY_CHAR:
            return getTyped(data_char_, static_cast<int8_t>(*static_cast<const int64_t*>(key)), value, valueType_);
        case HOO_MAP_KEY_STRING:
            return getTyped(data_string_, std::string(static_cast<const char*>(key)), value, valueType_);
    }
    return 0;
}

int64_t HooMapImpl::removeByKey(const void* key) {
    switch (keyType_) {
        case HOO_MAP_KEY_BYTE:
        case HOO_MAP_KEY_INT8:
            return removeTyped(data_int8_, *static_cast<const int8_t*>(key), valueType_);
        case HOO_MAP_KEY_INT64:
            return removeTyped(data_int64_, *static_cast<const int64_t*>(key), valueType_);
        case HOO_MAP_KEY_CHAR:
            return removeTyped(data_char_, static_cast<int8_t>(*static_cast<const int64_t*>(key)), valueType_);
        case HOO_MAP_KEY_STRING:
            return removeTyped(data_string_, std::string(static_cast<const char*>(key)), valueType_);
    }
    return 0;
}

int64_t HooMapImpl::getKeys(void* keys, int64_t max_count) const {
    if (!keys || max_count <= 0) return 0;
    int64_t written = 0;
    switch (keyType_) {
        case HOO_MAP_KEY_BYTE:
        case HOO_MAP_KEY_INT8: {
            auto* out = static_cast<int8_t*>(keys);
            for (auto& [k, v] : data_int8_) {
                if (written >= max_count) break;
                out[written++] = k;
            }
            return written;
        }
        case HOO_MAP_KEY_INT64: {
            auto* out = static_cast<int64_t*>(keys);
            for (auto& [k, v] : data_int64_) {
                if (written >= max_count) break;
                out[written++] = k;
            }
            return written;
        }
        case HOO_MAP_KEY_CHAR: {
            auto* out = static_cast<int64_t*>(keys);
            for (auto& [k, v] : data_char_) {
                if (written >= max_count) break;
                out[written++] = static_cast<int64_t>(k);
            }
            return written;
        }
        case HOO_MAP_KEY_STRING: {
            auto* out = static_cast<const char**>(keys);
            for (auto& [k, v] : data_string_) {
                if (written >= max_count) break;
                out[written++] = k.c_str();
            }
            return written;
        }
    }
    return written;
}

int64_t HooMapImpl::getValues(void* values, int64_t max_count) const {
    if (!values || max_count <= 0) return 0;
    int64_t written = 0;
    switch (keyType_) {
        case HOO_MAP_KEY_BYTE:
        case HOO_MAP_KEY_INT8: {
            for (auto& [k, v] : data_int8_) {
                if (written >= max_count) break;
                getValue(v, static_cast<char*>(values) + written * sizeof(int64_t), valueType_);
                written++;
            }
            return written;
        }
        case HOO_MAP_KEY_INT64: {
            for (auto& [k, v] : data_int64_) {
                if (written >= max_count) break;
                getValue(v, static_cast<char*>(values) + written * sizeof(int64_t), valueType_);
                written++;
            }
            return written;
        }
        case HOO_MAP_KEY_CHAR: {
            for (auto& [k, v] : data_char_) {
                if (written >= max_count) break;
                getValue(v, static_cast<char*>(values) + written * sizeof(int64_t), valueType_);
                written++;
            }
            return written;
        }
        case HOO_MAP_KEY_STRING: {
            for (auto& [k, v] : data_string_) {
                if (written >= max_count) break;
                getValue(v, static_cast<char*>(values) + written * sizeof(int64_t), valueType_);
                written++;
            }
            return written;
        }
    }
    return written;
}

}  // namespace hooc

// ============================================================================
// C API Implementations  (15 polymorphic functions)
// ============================================================================

extern "C" {

HooMap hoo_map_new(int keyType, int valueType) {
    try {
        void* mem = hoo_alloc(sizeof(hooc::HooMapImpl), HOO_TYPE_MAP);
        auto* impl = new (mem) hooc::HooMapImpl(keyType, valueType);
        return static_cast<HooMap>(impl);
    } catch (...) {
        return nullptr;
    }
}

HooMap hoo_map_retain(HooMap map) {
    return (HooMap)hoo_retain(map);
}

void hoo_map_release(HooMap map) {
    if (!map) return;
    hoo_release(map);
}

int64_t hoo_map_refcount(HooMap map) {
    return hoo_get_refcount(map);
}

int64_t hoo_map_count(HooMap map) {
    if (!map) return 0;
    return static_cast<hooc::HooMapImpl*>(map)->length();
}

int64_t hoo_map_is_empty(HooMap map) {
    if (!map) return 1;
    return static_cast<hooc::HooMapImpl*>(map)->empty() ? 1 : 0;
}

int hoo_map_key_type(HooMap map) {
    if (!map) return -1;
    return static_cast<hooc::HooMapImpl*>(map)->getKeyType();
}

int hoo_map_value_type(HooMap map) {
    if (!map) return -1;
    return static_cast<hooc::HooMapImpl*>(map)->getValueType();
}

int64_t hoo_map_contains_key(HooMap map, const void* key) {
    if (!map || !key) return 0;
    return static_cast<hooc::HooMapImpl*>(map)->containsByKey(key);
}

int64_t hoo_map_set(HooMap map, const void* key, const void* value) {
    if (!map || !key || !value) return -1;
    return static_cast<hooc::HooMapImpl*>(map)->setByKeyAndValue(key, value);
}

int64_t hoo_map_try_get(HooMap map, const void* key, void* value) {
    if (!map || !key || !value) return 0;
    return static_cast<hooc::HooMapImpl*>(map)->getByKeyAndValue(key, value);
}

int64_t hoo_map_remove(HooMap map, const void* key) {
    if (!map || !key) return 0;
    return static_cast<hooc::HooMapImpl*>(map)->removeByKey(key);
}

void hoo_map_clear(HooMap map) {
    if (!map) return;
    static_cast<hooc::HooMapImpl*>(map)->clearAll();
}

int64_t hoo_map_get_keys(HooMap map, void* keys, int64_t max_count) {
    if (!map || !keys || max_count <= 0) return 0;
    return static_cast<hooc::HooMapImpl*>(map)->getKeys(keys, max_count);
}

int64_t hoo_map_get_values(HooMap map, void* values, int64_t max_count) {
    if (!map || !values || max_count <= 0) return 0;
    return static_cast<hooc::HooMapImpl*>(map)->getValues(values, max_count);
}

}  // extern "C"
