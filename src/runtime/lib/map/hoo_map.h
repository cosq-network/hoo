#pragma once

#include <stdint.h>

#ifdef __cplusplus
    #include <any>
    #include <string>
    #include <unordered_map>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooMap - Object-oriented Map API (similar to .NET Dictionary<TKey, TValue>)
// ============================================================================
//
// Key convention for type-erased void* parameters:
//   HOO_MAP_KEY_BYTE / HOO_MAP_KEY_INT8: *(const int8_t*)key
//   HOO_MAP_KEY_INT64:                 *(const int64_t*)key
//   HOO_MAP_KEY_CHAR:                  *(const int32_t*)key
//   HOO_MAP_KEY_STRING:                (const char*)key (directly)
//
// Value convention for type-erased void* parameters:
//   HOO_MAP_VAL_INT64 / VALT_BOOL:     *(const int64_t*)value
//   HOO_MAP_VAL_DOUBLE:                *(const double*)value
//   HOO_MAP_VAL_INT8:                  *(const int8_t*)value
//   HOO_MAP_VAL_CHAR:                  *(const int32_t*)value
//   HOO_MAP_VAL_STRING:                (const char*)value (directly; map strdup's internally)
//   HOO_MAP_VAL_OBJECT:                (void*)value (directly)
//
// For try_get output, the same convention applies in reverse:
//   writes *(int64_t*)value, *(double*)value, *(const char**)value, etc.
//
// Thread safety: a HooMap is NOT internally synchronised.  Concurrent access
// from multiple threads must be externally serialised by the caller.
//

typedef void* HooMap;

// ============================================================================
// Key Types
// ============================================================================

typedef enum {
    HOO_MAP_KEY_BYTE = 0,
    HOO_MAP_KEY_INT8 = 1,
    HOO_MAP_KEY_INT64 = 2,
    HOO_MAP_KEY_CHAR = 3,
    HOO_MAP_KEY_STRING = 4
} HooMapKeyType;

// ============================================================================
// Value Types
// ============================================================================

typedef enum {
    HOO_MAP_VAL_ANY = 0,
    HOO_MAP_VAL_INT64 = 1,
    HOO_MAP_VAL_DOUBLE = 2,
    HOO_MAP_VAL_BOOL = 3,
    HOO_MAP_VAL_STRING = 4,
    HOO_MAP_VAL_OBJECT = 5,
    HOO_MAP_VAL_INT8 = 6,
    HOO_MAP_VAL_CHAR = 7
} HooMapValueType;

// ============================================================================
// Lifetime
// ============================================================================

HooMap   hoo_map_new(int keyType, int valueType);
HooMap   hoo_map_retain(HooMap map);
void     hoo_map_release(HooMap map);
int64_t  hoo_map_refcount(HooMap map);

// ============================================================================
// Introspection  (.NET: Count, IsEmpty, KeyType, ValueType)
// ============================================================================

int64_t  hoo_map_count(HooMap map);
int64_t  hoo_map_is_empty(HooMap map);
int      hoo_map_key_type(HooMap map);
int      hoo_map_value_type(HooMap map);

// ============================================================================
// Core Operations  (.NET: ContainsKey, Add, Remove, Clear, TryGetValue)
// ============================================================================

int64_t  hoo_map_contains_key(HooMap map, const void* key);
int64_t  hoo_map_set(HooMap map, const void* key, const void* value);
int64_t  hoo_map_try_get(HooMap map, const void* key, void* value);
int64_t  hoo_map_remove(HooMap map, const void* key);
void     hoo_map_clear(HooMap map);

// ============================================================================
// Enumeration  (.NET: Keys, Values)
// ============================================================================

int64_t  hoo_map_get_keys(HooMap map, void* keys, int64_t max_count);
int64_t  hoo_map_get_values(HooMap map, void* values, int64_t max_count);

#ifdef __cplusplus
}  // extern "C"
#endif

// ============================================================================
// C++ Implementation Class
// ============================================================================

#ifdef __cplusplus

namespace hooc {

class HooMapImpl {
public:
    HooMapImpl(int keyType, int valueType);
    ~HooMapImpl();

    HooMapImpl(const HooMapImpl&) = delete;
    HooMapImpl& operator=(const HooMapImpl&) = delete;

    // Polymorphic operations (key/value dispatch based on stored types)
    int64_t setByKeyAndValue(const void* key, const void* value);
    int64_t getByKeyAndValue(const void* key, void* value) const;
    int64_t containsByKey(const void* key) const;
    int64_t removeByKey(const void* key);
    void clearAll();
    int64_t length() const;
    bool empty() const;
    int64_t getKeys(void* keys, int64_t max_count) const;
    int64_t getValues(void* values, int64_t max_count) const;

    int getKeyType() const { return keyType_; }
    int getValueType() const { return valueType_; }

    // Value lifetime management
    void releaseValue(std::any& val);
    void releaseAllValues();

private:
    int keyType_;
    int valueType_;

    // Separate storage for each key type
    std::unordered_map<int8_t, std::any> data_int8_;
    std::unordered_map<int64_t, std::any> data_int64_;
    std::unordered_map<int8_t, std::any> data_char_;
    std::unordered_map<std::string, std::any> data_string_;

    // Internal typed helpers (used by dispatch methods)
    static int64_t setValue(std::any& slot, const void* value, int valueType);
    static int64_t getValue(const std::any& slot, void* value, int valueType);

    template<typename MapT, typename KeyT>
    static void releaseOldValue(MapT& map, const KeyT& key, int valueType);

    template<typename MapT, typename KeyT>
    static int64_t containsKey(const MapT& map, const KeyT& key);

    template<typename MapT, typename KeyT>
    static int64_t setTyped(MapT& map, const KeyT& key, const void* value, int valueType);

    template<typename MapT, typename KeyT>
    static int64_t getTyped(const MapT& map, const KeyT& key, void* value, int valueType);

    template<typename MapT, typename KeyT>
    static int64_t removeTyped(MapT& map, const KeyT& key, int valueType);
};

} // namespace hooc

#endif // __cplusplus
