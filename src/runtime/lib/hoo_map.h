#pragma once

#include <stdint.h>

#ifdef __cplusplus
    #include <memory>
    #include <vector>
    #include <any>
    #include <typeinfo>
    #include <string>
    #include <unordered_map>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooMap - Generic Map with typed keys and values
// ============================================================================
//
// A type-safe map that stores key-value pairs. Keys are restricted to:
// - byte (int8)
// - int8 (int8)
// - int64 (int64)
// - char (int32)
// - string (const char*)
//
// Values can be any type including primitives, strings, objects, and arrays.
// Internally managed with automatic reference counting (ARC).
//

typedef void* HooMap;

// ============================================================================
// Key Types (for compile-time key type checking)
// ============================================================================

typedef enum {
    HOO_MAP_KEY_BYTE = 0,
    HOO_MAP_KEY_INT8 = 1,
    HOO_MAP_KEY_INT64 = 2,
    HOO_MAP_KEY_CHAR = 3,
    HOO_MAP_KEY_STRING = 4
} HooMapKeyType;

// ============================================================================
// Value Types (for compile-time value type checking)
// ============================================================================

typedef enum {
    HOO_MAP_VAL_ANY = 0,      // no type enforcement
    HOO_MAP_VAL_INT64 = 1,
    HOO_MAP_VAL_DOUBLE = 2,
    HOO_MAP_VAL_BOOL = 3,
    HOO_MAP_VAL_STRING = 4,
    HOO_MAP_VAL_OBJECT = 5,
    HOO_MAP_VAL_INT8 = 6,
    HOO_MAP_VAL_CHAR = 7
} HooMapValueType;

// ============================================================================
// Creation and Destruction
// ============================================================================

/**
 * Create a new empty map with specified key type and value type
 * @param keyType The type of keys this map will store (HooMapKeyType)
 * @param valueType The type of values this map will store (HooMapValueType)
 * @return New HooMap with refcount=1, or NULL on allocation failure
 */
HooMap hoo_map_new(int keyType, int valueType);

/**
 * Create a new empty map with specified key type (value type defaults to ANY)
 * @param keyType The type of keys this map will store (HooMapKeyType)
 * @return New HooMap with refcount=1, or NULL on allocation failure
 */
HooMap hoo_map_new_with_keytype(int keyType);

/**
 * Create a map from key-value pairs
 * @param keyType The type of keys
 * @param valueType The type of values
 * @param keys Array of keys
 * @param values Array of values (as void pointers)
 * @param count Number of pairs
 * @return New HooMap
 */
HooMap hoo_map_from_pairs(int keyType, int valueType, const void* keys, const void** values, int64_t count);

// ============================================================================
// Basic Operations
// ============================================================================

/**
 * Get number of entries in map
 * @param map Map (may be NULL)
 * @return Number of entries, or 0 if NULL
 */
int64_t hoo_map_length(HooMap map);

/**
 * Check if map contains a key
 * @param map Map
 * @param key Pointer to key value
 * @return 1 if key exists, 0 otherwise
 */
int64_t hoo_map_contains_int8(HooMap map, int8_t key);
int64_t hoo_map_contains_int64(HooMap map, int64_t key);
int64_t hoo_map_contains_char(HooMap map, char key);
int64_t hoo_map_contains_string(HooMap map, const char* key);

/**
 * Remove entry by key
 * @param map Map
 * @param key Pointer to key value
 * @return 1 if removed, 0 if not found
 */
int64_t hoo_map_remove_int8(HooMap map, int8_t key);
int64_t hoo_map_remove_int64(HooMap map, int64_t key);
int64_t hoo_map_remove_char(HooMap map, char key);
int64_t hoo_map_remove_string(HooMap map, const char* key);

/**
 * Clear all entries
 * @param map Map
 */
void hoo_map_clear(HooMap map);

/**
 * Check if map is empty
 * @param map Map (may be NULL)
 * @return 1 if empty or NULL, 0 otherwise
 */
int64_t hoo_map_empty(HooMap map);

// ============================================================================
// Key-Specific Set Operations
// ============================================================================

// Int8 key operations
int64_t hoo_map_set_int8_int64(HooMap map, int8_t key, int64_t value);
int64_t hoo_map_set_int8_double(HooMap map, int8_t key, double value);
int64_t hoo_map_set_int8_bool(HooMap map, int8_t key, int64_t value);
int64_t hoo_map_set_int8_string(HooMap map, int8_t key, const char* value);
int64_t hoo_map_set_int8_object(HooMap map, int8_t key, void* value);

// Int64 key operations
int64_t hoo_map_set_int64_int64(HooMap map, int64_t key, int64_t value);
int64_t hoo_map_set_int64_double(HooMap map, int64_t key, double value);
int64_t hoo_map_set_int64_bool(HooMap map, int64_t key, int64_t value);
int64_t hoo_map_set_int64_string(HooMap map, int64_t key, const char* value);
int64_t hoo_map_set_int64_object(HooMap map, int64_t key, void* value);

// Char key operations
int64_t hoo_map_set_char_int64(HooMap map, char key, int64_t value);
int64_t hoo_map_set_char_double(HooMap map, char key, double value);
int64_t hoo_map_set_char_bool(HooMap map, char key, int64_t value);
int64_t hoo_map_set_char_string(HooMap map, char key, const char* value);
int64_t hoo_map_set_char_object(HooMap map, char key, void* value);

// String key operations
int64_t hoo_map_set_string_int64(HooMap map, const char* key, int64_t value);
int64_t hoo_map_set_string_double(HooMap map, const char* key, double value);
int64_t hoo_map_set_string_bool(HooMap map, const char* key, int64_t value);
int64_t hoo_map_set_string_string(HooMap map, const char* key, const char* value);
int64_t hoo_map_set_string_object(HooMap map, const char* key, void* value);

// ============================================================================
// Key-Specific Get Operations
// ============================================================================

// Int8 key operations
int64_t hoo_map_get_int8_int64(HooMap map, int8_t key, int64_t* dest);
int64_t hoo_map_get_int8_double(HooMap map, int8_t key, double* dest);
int64_t hoo_map_get_int8_bool(HooMap map, int8_t key, int64_t* dest);
int64_t hoo_map_get_int8_string(HooMap map, int8_t key, const char** dest);
int64_t hoo_map_get_int8_object(HooMap map, int8_t key, void** dest);

// Int64 key operations
int64_t hoo_map_get_int64_int64(HooMap map, int64_t key, int64_t* dest);
int64_t hoo_map_get_int64_double(HooMap map, int64_t key, double* dest);
int64_t hoo_map_get_int64_bool(HooMap map, int64_t key, int64_t* dest);
int64_t hoo_map_get_int64_string(HooMap map, int64_t key, const char** dest);
int64_t hoo_map_get_int64_object(HooMap map, int64_t key, void** dest);

// Char key operations
int64_t hoo_map_get_char_int64(HooMap map, char key, int64_t* dest);
int64_t hoo_map_get_char_double(HooMap map, char key, double* dest);
int64_t hoo_map_get_char_bool(HooMap map, char key, int64_t* dest);
int64_t hoo_map_get_char_string(HooMap map, char key, const char** dest);
int64_t hoo_map_get_char_object(HooMap map, char key, void** dest);

// String key operations
int64_t hoo_map_get_string_int64(HooMap map, const char* key, int64_t* dest);
int64_t hoo_map_get_string_double(HooMap map, const char* key, double* dest);
int64_t hoo_map_get_string_bool(HooMap map, const char* key, int64_t* dest);
int64_t hoo_map_get_string_string(HooMap map, const char* key, const char** dest);
int64_t hoo_map_get_string_object(HooMap map, const char* key, void** dest);

// ============================================================================
// Generic set/get for void* values
// ============================================================================

int64_t hoo_map_set_int8_value(HooMap map, int8_t key, void* value);
int64_t hoo_map_set_int64_value(HooMap map, int64_t key, void* value);
int64_t hoo_map_set_char_value(HooMap map, char key, void* value);
int64_t hoo_map_set_string_value(HooMap map, const char* key, void* value);

int64_t hoo_map_get_int8_value(HooMap map, int8_t key, void* dest);
int64_t hoo_map_get_int64_value(HooMap map, int64_t key, void* dest);
int64_t hoo_map_get_char_value(HooMap map, char key, void* dest);
int64_t hoo_map_get_string_value(HooMap map, const char* key, void* dest);

// ============================================================================
// Reference Counting
// ============================================================================

/**
 * Increment reference count
 * @param map Map
 * @return Map handle (same as input)
 */
HooMap hoo_map_retain(HooMap map);

/**
 * Decrement reference count, free if zero
 * @param map Map
 */
void hoo_map_release(HooMap map);

/**
 * Get reference count
 * @param map Map
 * @return Current refcount, or 0 if NULL
 */
int64_t hoo_map_refcount(HooMap map);

// ============================================================================
// Utility
// ============================================================================

/**
 * Get the key type of this map
 * @param map Map
 * @return Key type enum value
 */
int hoo_map_key_type(HooMap map);

/**
 * Get the value type of this map
 * @param map Map
 * @return Value type enum value (HooMapValueType)
 */
int hoo_map_value_type(HooMap map);

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

    // Basic operations
    int64_t length() const;
    int64_t removeInt8(int8_t key);
    int64_t removeInt64(int64_t key);
    int64_t removeChar(char key);
    int64_t removeString(const char* key);
    void clear();
    bool empty() const;

    // Int8 key operations
    int64_t containsInt8(int8_t key) const;
    int64_t setInt8Int64(int8_t key, int64_t value);
    int64_t getInt8Int64(int8_t key, int64_t& dest) const;
    int64_t setInt8Double(int8_t key, double value);
    int64_t getInt8Double(int8_t key, double& dest) const;
    int64_t setInt8Bool(int8_t key, int64_t value);
    int64_t getInt8Bool(int8_t key, int64_t& dest) const;
    int64_t setInt8String(int8_t key, const char* value);
    int64_t getInt8String(int8_t key, const char*& dest) const;
    int64_t setInt8Object(int8_t key, void* value);
    int64_t getInt8Object(int8_t key, void*& dest) const;

    // Int64 key operations
    int64_t containsInt64(int64_t key) const;
    int64_t setInt64Int64(int64_t key, int64_t value);
    int64_t getInt64Int64(int64_t key, int64_t& dest) const;
    int64_t setInt64Double(int64_t key, double value);
    int64_t getInt64Double(int64_t key, double& dest) const;
    int64_t setInt64Bool(int64_t key, int64_t value);
    int64_t getInt64Bool(int64_t key, int64_t& dest) const;
    int64_t setInt64String(int64_t key, const char* value);
    int64_t getInt64String(int64_t key, const char*& dest) const;
    int64_t setInt64Object(int64_t key, void* value);
    int64_t getInt64Object(int64_t key, void*& dest) const;

    // Char key operations
    int64_t containsChar(char key) const;
    int64_t setCharInt64(char key, int64_t value);
    int64_t getCharInt64(char key, int64_t& dest) const;
    int64_t setCharDouble(char key, double value);
    int64_t getCharDouble(char key, double& dest) const;
    int64_t setCharBool(char key, int64_t value);
    int64_t getCharBool(char key, int64_t& dest) const;
    int64_t setCharString(char key, const char* value);
    int64_t getCharString(char key, const char*& dest) const;
    int64_t setCharObject(char key, void* value);
    int64_t getCharObject(char key, void*& dest) const;

    // String key operations
    int64_t containsString(const char* key) const;
    int64_t setStringInt64(const char* key, int64_t value);
    int64_t getStringInt64(const char* key, int64_t& dest) const;
    int64_t setStringDouble(const char* key, double value);
    int64_t getStringDouble(const char* key, double& dest) const;
    int64_t setStringBool(const char* key, int64_t value);
    int64_t getStringBool(const char* key, int64_t& dest) const;
    int64_t setStringString(const char* key, const char* value);
    int64_t getStringString(const char* key, const char*& dest) const;
    int64_t setStringObject(const char* key, void* value);
    int64_t getStringObject(const char* key, void*& dest) const;

    // Generic value operations
    int64_t setInt8Value(int8_t key, void* value);
    int64_t setInt64Value(int64_t key, void* value);
    int64_t setCharValue(char key, void* value);
    int64_t setStringValue(const char* key, void* value);
    int64_t getInt8Value(int8_t key, void* dest) const;
    int64_t getInt64Value(int64_t key, void* dest) const;
    int64_t getCharValue(char key, void* dest) const;
    int64_t getStringValue(const char* key, void* dest) const;

    // Utility
    int getKeyType() const { return keyType_; }
    int getValueType() const { return valueType_; }

private:
    int keyType_;
    int valueType_;

    // Separate storage for each key type
    std::unordered_map<int8_t, std::any> data_int8_;
    std::unordered_map<int64_t, std::any> data_int64_;
    std::unordered_map<char, std::any> data_char_;
    std::unordered_map<std::string, std::any> data_string_;
};

} // namespace hooc

#endif // __cplusplus
