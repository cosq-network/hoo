#include "hoo_map.h"
#include "hoo_runtime.h"
#include <new>
#include <cstring>
#include <cassert>
#include <unordered_map>
#include <mutex>

namespace hooc {

// ============================================================================
// HooMapImpl Implementation
// ============================================================================

HooMapImpl::HooMapImpl(int keyType, int valueType)
    : keyType_(keyType), valueType_(valueType) {
}

HooMapImpl::~HooMapImpl() {
}

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

int64_t HooMapImpl::containsInt8(int8_t key) const {
    return data_int8_.find(key) != data_int8_.end() ? 1 : 0;
}

int64_t HooMapImpl::containsInt64(int64_t key) const {
    return data_int64_.find(key) != data_int64_.end() ? 1 : 0;
}

int64_t HooMapImpl::containsChar(char key) const {
    return data_char_.find(key) != data_char_.end() ? 1 : 0;
}

int64_t HooMapImpl::containsString(const char* key) const {
    std::string s(key);
    return data_string_.find(s) != data_string_.end() ? 1 : 0;
}

int64_t HooMapImpl::removeInt8(int8_t key) {
    return data_int8_.erase(key) > 0 ? 1 : 0;
}

int64_t HooMapImpl::removeInt64(int64_t key) {
    return data_int64_.erase(key) > 0 ? 1 : 0;
}

int64_t HooMapImpl::removeChar(char key) {
    return data_char_.erase(key) > 0 ? 1 : 0;
}

int64_t HooMapImpl::removeString(const char* key) {
    std::string s(key);
    return data_string_.erase(s) > 0 ? 1 : 0;
}

void HooMapImpl::clear() {
    data_int8_.clear();
    data_int64_.clear();
    data_char_.clear();
    data_string_.clear();
}

bool HooMapImpl::empty() const {
    switch (keyType_) {
        case HOO_MAP_KEY_BYTE:
        case HOO_MAP_KEY_INT8:
            return data_int8_.empty();
        case HOO_MAP_KEY_INT64:
            return data_int64_.empty();
        case HOO_MAP_KEY_CHAR:
            return data_char_.empty();
        case HOO_MAP_KEY_STRING:
            return data_string_.empty();
    }
    return true;
}

// Int8 key operations
int64_t HooMapImpl::setInt8Int64(int8_t key, int64_t value) {
    data_int8_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt8Int64(int8_t key, int64_t& dest) const {
    auto it = data_int8_.find(key);
    if (it != data_int8_.end()) {
        try {
            dest = std::any_cast<int64_t>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::setInt8Double(int8_t key, double value) {
    data_int8_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt8Double(int8_t key, double& dest) const {
    auto it = data_int8_.find(key);
    if (it != data_int8_.end()) {
        try { dest = std::any_cast<double>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setInt8Bool(int8_t key, int64_t value) {
    data_int8_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt8Bool(int8_t key, int64_t& dest) const {
    auto it = data_int8_.find(key);
    if (it != data_int8_.end()) {
        try { dest = std::any_cast<int64_t>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setInt8String(int8_t key, const char* value) {
    data_int8_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt8String(int8_t key, const char*& dest) const {
    auto it = data_int8_.find(key);
    if (it != data_int8_.end()) {
        try { dest = std::any_cast<const char*>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setInt8Object(int8_t key, void* value) {
    data_int8_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt8Object(int8_t key, void*& dest) const {
    auto it = data_int8_.find(key);
    if (it != data_int8_.end()) {
        try { dest = std::any_cast<void*>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

// Int64 key operations
int64_t HooMapImpl::setInt64Int64(int64_t key, int64_t value) {
    data_int64_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt64Int64(int64_t key, int64_t& dest) const {
    auto it = data_int64_.find(key);
    if (it != data_int64_.end()) {
        try {
            dest = std::any_cast<int64_t>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::setInt64Double(int64_t key, double value) {
    data_int64_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt64Double(int64_t key, double& dest) const {
    auto it = data_int64_.find(key);
    if (it != data_int64_.end()) {
        try { dest = std::any_cast<double>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setInt64Bool(int64_t key, int64_t value) {
    data_int64_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt64Bool(int64_t key, int64_t& dest) const {
    auto it = data_int64_.find(key);
    if (it != data_int64_.end()) {
        try { dest = std::any_cast<int64_t>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setInt64String(int64_t key, const char* value) {
    data_int64_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt64String(int64_t key, const char*& dest) const {
    auto it = data_int64_.find(key);
    if (it != data_int64_.end()) {
        try { dest = std::any_cast<const char*>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setInt64Object(int64_t key, void* value) {
    data_int64_[key] = value;
    return 1;
}

int64_t HooMapImpl::getInt64Object(int64_t key, void*& dest) const {
    auto it = data_int64_.find(key);
    if (it != data_int64_.end()) {
        try { dest = std::any_cast<void*>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

// Char key operations
int64_t HooMapImpl::setCharInt64(char key, int64_t value) {
    data_char_[key] = value;
    return 1;
}

int64_t HooMapImpl::getCharInt64(char key, int64_t& dest) const {
    auto it = data_char_.find(key);
    if (it != data_char_.end()) {
        try { dest = std::any_cast<int64_t>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setCharDouble(char key, double value) {
    data_char_[key] = value;
    return 1;
}

int64_t HooMapImpl::getCharDouble(char key, double& dest) const {
    auto it = data_char_.find(key);
    if (it != data_char_.end()) {
        try {
            dest = std::any_cast<double>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::setCharBool(char key, int64_t value) {
    data_char_[key] = value;
    return 1;
}

int64_t HooMapImpl::getCharBool(char key, int64_t& dest) const {
    auto it = data_char_.find(key);
    if (it != data_char_.end()) {
        try { dest = std::any_cast<int64_t>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setCharString(char key, const char* value) {
    data_char_[key] = value;
    return 1;
}

int64_t HooMapImpl::getCharString(char key, const char*& dest) const {
    auto it = data_char_.find(key);
    if (it != data_char_.end()) {
        try { dest = std::any_cast<const char*>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setCharObject(char key, void* value) {
    data_char_[key] = value;
    return 1;
}

int64_t HooMapImpl::getCharObject(char key, void*& dest) const {
    auto it = data_char_.find(key);
    if (it != data_char_.end()) {
        try { dest = std::any_cast<void*>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

// String key operations
int64_t HooMapImpl::setStringInt64(const char* key, int64_t value) {
    data_string_[std::string(key)] = value;
    return 1;
}

int64_t HooMapImpl::getStringInt64(const char* key, int64_t& dest) const {
    auto it = data_string_.find(std::string(key));
    if (it != data_string_.end()) {
        try {
            dest = std::any_cast<int64_t>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::setStringDouble(const char* key, double value) {
    data_string_[std::string(key)] = value;
    return 1;
}

int64_t HooMapImpl::getStringDouble(const char* key, double& dest) const {
    auto it = data_string_.find(std::string(key));
    if (it != data_string_.end()) {
        try {
            dest = std::any_cast<double>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::setStringString(const char* key, const char* value) {
    data_string_[std::string(key)] = value;
    return 1;
}

int64_t HooMapImpl::getStringString(const char* key, const char*& dest) const {
    auto it = data_string_.find(std::string(key));
    if (it != data_string_.end()) {
        try {
            dest = std::any_cast<const char*>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::setStringBool(const char* key, int64_t value) {
    data_string_[std::string(key)] = value;
    return 1;
}

int64_t HooMapImpl::getStringBool(const char* key, int64_t& dest) const {
    auto it = data_string_.find(std::string(key));
    if (it != data_string_.end()) {
        try { dest = std::any_cast<int64_t>(it->second); return 1; }
        catch (...) { return 0; }
    }
    return 0;
}

int64_t HooMapImpl::setStringObject(const char* key, void* value) {
    data_string_[std::string(key)] = value;
    return 1;
}

int64_t HooMapImpl::getStringObject(const char* key, void*& dest) const {
    auto it = data_string_.find(std::string(key));
    if (it != data_string_.end()) {
        try {
            dest = std::any_cast<void*>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

// Generic value operations
int64_t HooMapImpl::setInt8Value(int8_t key, void* value) {
    data_int8_[key] = value;
    return 1;
}

int64_t HooMapImpl::setInt64Value(int64_t key, void* value) {
    data_int64_[key] = value;
    return 1;
}

int64_t HooMapImpl::setCharValue(char key, void* value) {
    data_char_[key] = value;
    return 1;
}

int64_t HooMapImpl::setStringValue(const char* key, void* value) {
    data_string_[std::string(key)] = value;
    return 1;
}

int64_t HooMapImpl::getInt8Value(int8_t key, void* dest) const {
    auto it = data_int8_.find(key);
    if (it != data_int8_.end()) {
        try {
            *static_cast<void**>(dest) = std::any_cast<void*>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::getInt64Value(int64_t key, void* dest) const {
    auto it = data_int64_.find(key);
    if (it != data_int64_.end()) {
        try {
            *static_cast<void**>(dest) = std::any_cast<void*>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::getCharValue(char key, void* dest) const {
    auto it = data_char_.find(key);
    if (it != data_char_.end()) {
        try {
            *static_cast<void**>(dest) = std::any_cast<void*>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

int64_t HooMapImpl::getStringValue(const char* key, void* dest) const {
    auto it = data_string_.find(std::string(key));
    if (it != data_string_.end()) {
        try {
            *static_cast<void**>(dest) = std::any_cast<void*>(it->second);
            return 1;
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

}  // namespace hooc

// ============================================================================
// C API Implementations
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

HooMap hoo_map_new_with_keytype(int keyType) {
    return hoo_map_new(keyType, HOO_MAP_VAL_ANY);
}

HooMap hoo_map_from_pairs(int keyType, int valueType, const void* keys, const void** values, int64_t count) {
    HooMap map = hoo_map_new(keyType, valueType);
    if (!map) return nullptr;

    // TODO: full implementation would iterate over keys/values and insert them.
    return map;
}

int64_t hoo_map_length(HooMap map) {
    if (!map) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->length();
}

int64_t hoo_map_contains_int8(HooMap map, int8_t key) {
    if (!map) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->containsInt8(key);
}

int64_t hoo_map_contains_int64(HooMap map, int64_t key) {
    if (!map) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->containsInt64(key);
}

int64_t hoo_map_contains_char(HooMap map, char key) {
    if (!map) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->containsChar(key);
}

int64_t hoo_map_contains_string(HooMap map, const char* key) {
    if (!map || !key) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->containsString(key);
}

int64_t hoo_map_remove_int8(HooMap map, int8_t key) {
    if (!map) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->removeInt8(key);
}

int64_t hoo_map_remove_int64(HooMap map, int64_t key) {
    if (!map) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->removeInt64(key);
}

int64_t hoo_map_remove_char(HooMap map, char key) {
    if (!map) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->removeChar(key);
}

int64_t hoo_map_remove_string(HooMap map, const char* key) {
    if (!map || !key) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->removeString(key);
}

void hoo_map_clear(HooMap map) {
    if (!map) return;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    impl->clear();
}

int64_t hoo_map_empty(HooMap map) {
    if (!map) return 1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->empty() ? 1 : 0;
}

// Int8 key operations
int64_t hoo_map_set_int8_int64(HooMap map, int8_t key, int64_t value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt8Int64(key, value);
}

int64_t hoo_map_get_int8_int64(HooMap map, int8_t key, int64_t* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt8Int64(key, *dest);
}

int64_t hoo_map_set_int8_double(HooMap map, int8_t key, double value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt8Double(key, value);
}

int64_t hoo_map_get_int8_double(HooMap map, int8_t key, double* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt8Double(key, *dest);
}

int64_t hoo_map_set_int8_bool(HooMap map, int8_t key, int64_t value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt8Bool(key, value);
}

int64_t hoo_map_get_int8_bool(HooMap map, int8_t key, int64_t* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt8Bool(key, *dest);
}

int64_t hoo_map_set_int8_string(HooMap map, int8_t key, const char* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt8String(key, value);
}

int64_t hoo_map_get_int8_string(HooMap map, int8_t key, const char** dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt8String(key, *dest);
}

int64_t hoo_map_set_int8_object(HooMap map, int8_t key, void* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt8Object(key, value);
}

int64_t hoo_map_get_int8_object(HooMap map, int8_t key, void** dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt8Object(key, *dest);
}

// Int64 key operations
int64_t hoo_map_set_int64_int64(HooMap map, int64_t key, int64_t value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt64Int64(key, value);
}

int64_t hoo_map_get_int64_int64(HooMap map, int64_t key, int64_t* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt64Int64(key, *dest);
}

int64_t hoo_map_set_int64_double(HooMap map, int64_t key, double value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt64Double(key, value);
}

int64_t hoo_map_get_int64_double(HooMap map, int64_t key, double* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt64Double(key, *dest);
}

int64_t hoo_map_set_int64_bool(HooMap map, int64_t key, int64_t value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt64Bool(key, value);
}

int64_t hoo_map_get_int64_bool(HooMap map, int64_t key, int64_t* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt64Bool(key, *dest);
}

int64_t hoo_map_set_int64_string(HooMap map, int64_t key, const char* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt64String(key, value);
}

int64_t hoo_map_get_int64_string(HooMap map, int64_t key, const char** dest) {
    if (!map || !key || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt64String(key, *dest);
}

int64_t hoo_map_set_int64_object(HooMap map, int64_t key, void* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt64Object(key, value);
}

int64_t hoo_map_get_int64_object(HooMap map, int64_t key, void** dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt64Object(key, *dest);
}

// Char key operations
int64_t hoo_map_set_char_int64(HooMap map, char key, int64_t value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setCharInt64(key, value);
}

int64_t hoo_map_get_char_int64(HooMap map, char key, int64_t* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getCharInt64(key, *dest);
}

int64_t hoo_map_set_char_double(HooMap map, char key, double value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setCharDouble(key, value);
}

int64_t hoo_map_get_char_double(HooMap map, char key, double* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getCharDouble(key, *dest);
}

int64_t hoo_map_set_char_bool(HooMap map, char key, int64_t value) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setCharBool(key, value);
}

int64_t hoo_map_get_char_bool(HooMap map, char key, int64_t* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getCharBool(key, *dest);
}

int64_t hoo_map_set_char_string(HooMap map, char key, const char* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setCharString(key, value);
}

int64_t hoo_map_get_char_string(HooMap map, char key, const char** dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getCharString(key, *dest);
}

int64_t hoo_map_set_char_object(HooMap map, char key, void* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setCharObject(key, value);
}

int64_t hoo_map_get_char_object(HooMap map, char key, void** dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getCharObject(key, *dest);
}

// String key operations
int64_t hoo_map_set_string_int64(HooMap map, const char* key, int64_t value) {
    if (!map || !key) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setStringInt64(key, value);
}

int64_t hoo_map_get_string_int64(HooMap map, const char* key, int64_t* dest) {
    if (!map || !key || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getStringInt64(key, *dest);
}

int64_t hoo_map_set_string_double(HooMap map, const char* key, double value) {
    if (!map || !key) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setStringDouble(key, value);
}

int64_t hoo_map_get_string_double(HooMap map, const char* key, double* dest) {
    if (!map || !key || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getStringDouble(key, *dest);
}

int64_t hoo_map_set_string_string(HooMap map, const char* key, const char* value) {
    if (!map || !key || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setStringString(key, value);
}

int64_t hoo_map_get_string_string(HooMap map, const char* key, const char** dest) {
    if (!map || !key || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getStringString(key, *dest);
}

int64_t hoo_map_set_string_bool(HooMap map, const char* key, int64_t value) {
    if (!map || !key) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setStringBool(key, value);
}

int64_t hoo_map_get_string_bool(HooMap map, const char* key, int64_t* dest) {
    if (!map || !key || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getStringBool(key, *dest);
}

int64_t hoo_map_set_string_object(HooMap map, const char* key, void* value) {
    if (!map || !key || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setStringObject(key, value);
}

int64_t hoo_map_get_string_object(HooMap map, const char* key, void** dest) {
    if (!map || !key || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getStringObject(key, *dest);
}

// Generic value operations
int64_t hoo_map_set_int8_value(HooMap map, int8_t key, void* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt8Value(key, value);
}

int64_t hoo_map_get_int8_value(HooMap map, int8_t key, void* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt8Value(key, dest);
}

int64_t hoo_map_set_int64_value(HooMap map, int64_t key, void* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setInt64Value(key, value);
}

int64_t hoo_map_get_int64_value(HooMap map, int64_t key, void* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getInt64Value(key, dest);
}

int64_t hoo_map_set_char_value(HooMap map, char key, void* value) {
    if (!map || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setCharValue(key, value);
}

int64_t hoo_map_get_char_value(HooMap map, char key, void* dest) {
    if (!map || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getCharValue(key, dest);
}

int64_t hoo_map_set_string_value(HooMap map, const char* key, void* value) {
    if (!map || !key || !value) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->setStringValue(key, value);
}

int64_t hoo_map_get_string_value(HooMap map, const char* key, void* dest) {
    if (!map || !key || !dest) return 0;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getStringValue(key, dest);
}

static std::mutex gMapReleaseMu;

// Reference counting
HooMap hoo_map_retain(HooMap map) {
    return (HooMap)hoo_retain(map);
}

void hoo_map_release(HooMap map) {
    if (!map) return;

    bool doCleanup = false;
    {
        std::lock_guard<std::mutex> lk(gMapReleaseMu);
        if (hoo_get_refcount(map) == 1) {
            doCleanup = true;
        }
    }

    if (doCleanup) {
        auto* impl = static_cast<hooc::HooMapImpl*>(map);
        impl->~HooMapImpl();
    }

    hoo_release(map);
}

int64_t hoo_map_refcount(HooMap map) {
    return hoo_get_refcount(map);
}

// Utility
int hoo_map_key_type(HooMap map) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getKeyType();
}

int hoo_map_value_type(HooMap map) {
    if (!map) return -1;
    auto* impl = static_cast<hooc::HooMapImpl*>(map);
    return impl->getValueType();
}

}  // extern "C"
