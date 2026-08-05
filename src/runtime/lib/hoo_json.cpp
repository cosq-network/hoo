#include "hoo_json.h"

#include "hoo_any.h"
#include "hoo_anyarray.h"
#include "hoo_buffer.h"
#include "hoo_encoding.h"
#include "hoo_exception.h"
#include "hoo_hashmap.h"
#include "hoo_runtime.h"
#include "hoo_string.h"
#include "hoo_tensor.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

enum class JsonKind {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

struct JsonNode {
    JsonKind kind = JsonKind::Null;
    bool boolValue = false;
    std::string numberValue;
    std::string stringValue;
    std::vector<std::unique_ptr<JsonNode>> arrayValues;
    std::vector<std::pair<std::string, std::unique_ptr<JsonNode>>> objectValues;
};

[[noreturn]] void throwJsonRuntime(const std::string& message) {
    HooException exc = hoo_exception_runtime(message.c_str());
    hoo_exception_throw(exc);
    std::abort();
}

[[noreturn]] void rethrowAsJsonRuntime(const char* operation, const std::exception& e) {
    throwJsonRuntime(std::string("JSON ") + operation + " failed: " + e.what());
}

[[noreturn]] void rethrowAsJsonRuntime(const char* operation) {
    throwJsonRuntime(std::string("JSON ") + operation + " failed");
}

bool appendUtf8(uint32_t cp, std::string& out) {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
    if (cp <= 0x7F) {
        out += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return true;
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

std::string formatDouble(double value) {
    if (!std::isfinite(value)) return {};
    std::ostringstream oss;
    oss << std::setprecision(17) << value;
    return oss.str();
}

uint64_t pointerToData(const void* ptr) {
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr));
}

template<typename T>
T* dataToPointer(uint64_t data) {
    return reinterpret_cast<T*>(static_cast<uintptr_t>(data));
}

uint64_t int64ToData(int64_t value) {
    uint64_t data = 0;
    static_assert(sizeof(data) == sizeof(value), "int64_t and uint64_t size mismatch");
    std::memcpy(&data, &value, sizeof(data));
    return data;
}

int64_t dataToInt64(uint64_t data) {
    int64_t value = 0;
    static_assert(sizeof(data) == sizeof(value), "int64_t and uint64_t size mismatch");
    std::memcpy(&value, &data, sizeof(value));
    return value;
}

struct Parser {
    const char* input = nullptr;
    size_t len = 0;
    size_t pos = 0;

    explicit Parser(const char* json) : input(json), len(std::strlen(json)) {}

    void skipWhitespace() {
        while (pos < len) {
            char c = input[pos];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
            ++pos;
        }
    }

    bool consume(char c) {
        skipWhitespace();
        if (pos < len && input[pos] == c) {
            ++pos;
            return true;
        }
        return false;
    }

    bool consumeLiteral(const char* literal) {
        skipWhitespace();
        size_t literalLen = std::strlen(literal);
        if (pos + literalLen > len) return false;
        if (std::strncmp(input + pos, literal, literalLen) != 0) return false;
        pos += literalLen;
        return true;
    }

    bool parseHex4(uint32_t& out) {
        if (pos + 4 > len) return false;
        uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            int nibble = hexValue(input[pos + i]);
            if (nibble < 0) return false;
            value = (value << 4) | static_cast<uint32_t>(nibble);
        }
        pos += 4;
        out = value;
        return true;
    }

    bool parseString(std::string& out) {
        skipWhitespace();
        if (pos >= len || input[pos] != '"') return false;
        ++pos;
        out.clear();

        while (pos < len) {
            unsigned char c = static_cast<unsigned char>(input[pos++]);
            if (c == '"') return true;
            if (c < 0x20) return false;
            if (c != '\\') {
                out += static_cast<char>(c);
                continue;
            }
            if (pos >= len) return false;
            char esc = input[pos++];
            switch (esc) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    uint32_t cp = 0;
                    if (!parseHex4(cp)) return false;
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (pos + 6 > len || input[pos] != '\\' || input[pos + 1] != 'u') return false;
                        pos += 2;
                        uint32_t low = 0;
                        if (!parseHex4(low) || low < 0xDC00 || low > 0xDFFF) return false;
                        cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                    }
                    if (!appendUtf8(cp, out)) return false;
                    break;
                }
                default:
                    return false;
            }
        }
        return false;
    }

    std::unique_ptr<JsonNode> parseNumber() {
        skipWhitespace();
        size_t start = pos;
        if (pos < len && input[pos] == '-') ++pos;
        if (pos >= len) return nullptr;
        if (input[pos] == '0') {
            ++pos;
        } else if (input[pos] >= '1' && input[pos] <= '9') {
            while (pos < len && input[pos] >= '0' && input[pos] <= '9') ++pos;
        } else {
            return nullptr;
        }
        if (pos < len && input[pos] == '.') {
            ++pos;
            size_t fracStart = pos;
            while (pos < len && input[pos] >= '0' && input[pos] <= '9') ++pos;
            if (pos == fracStart) return nullptr;
        }
        if (pos < len && (input[pos] == 'e' || input[pos] == 'E')) {
            ++pos;
            if (pos < len && (input[pos] == '+' || input[pos] == '-')) ++pos;
            size_t expStart = pos;
            while (pos < len && input[pos] >= '0' && input[pos] <= '9') ++pos;
            if (pos == expStart) return nullptr;
        }

        auto node = std::make_unique<JsonNode>();
        node->kind = JsonKind::Number;
        node->numberValue.assign(input + start, pos - start);
        return node;
    }

    std::unique_ptr<JsonNode> parseArray() {
        if (!consume('[')) return nullptr;
        auto node = std::make_unique<JsonNode>();
        node->kind = JsonKind::Array;
        skipWhitespace();
        if (consume(']')) return node;
        while (true) {
            auto item = parseValue();
            if (!item) return nullptr;
            node->arrayValues.push_back(std::move(item));
            skipWhitespace();
            if (consume(']')) return node;
            if (!consume(',')) return nullptr;
        }
    }

    std::unique_ptr<JsonNode> parseObject() {
        if (!consume('{')) return nullptr;
        auto node = std::make_unique<JsonNode>();
        node->kind = JsonKind::Object;
        skipWhitespace();
        if (consume('}')) return node;
        while (true) {
            std::string key;
            if (!parseString(key)) return nullptr;
            if (!consume(':')) return nullptr;
            auto value = parseValue();
            if (!value) return nullptr;
            node->objectValues.emplace_back(std::move(key), std::move(value));
            skipWhitespace();
            if (consume('}')) return node;
            if (!consume(',')) return nullptr;
        }
    }

    std::unique_ptr<JsonNode> parseValue() {
        skipWhitespace();
        if (pos >= len) return nullptr;
        char c = input[pos];
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') {
            auto node = std::make_unique<JsonNode>();
            node->kind = JsonKind::String;
            if (!parseString(node->stringValue)) return nullptr;
            return node;
        }
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
        if (consumeLiteral("true")) {
            auto node = std::make_unique<JsonNode>();
            node->kind = JsonKind::Bool;
            node->boolValue = true;
            return node;
        }
        if (consumeLiteral("false")) {
            auto node = std::make_unique<JsonNode>();
            node->kind = JsonKind::Bool;
            node->boolValue = false;
            return node;
        }
        if (consumeLiteral("null")) {
            return std::make_unique<JsonNode>();
        }
        return nullptr;
    }

    std::unique_ptr<JsonNode> parseDocument() {
        auto root = parseValue();
        if (!root) return nullptr;
        skipWhitespace();
        return pos == len ? std::move(root) : nullptr;
    }
};

void stringifyNode(const JsonNode& node, std::string& out, int indent, bool pretty) {
    const std::string currentIndent(pretty ? indent * 2 : 0, ' ');
    const std::string nextIndent(pretty ? (indent + 1) * 2 : 0, ' ');

    switch (node.kind) {
        case JsonKind::Null:
            out += "null";
            break;
        case JsonKind::Bool:
            out += node.boolValue ? "true" : "false";
            break;
        case JsonKind::Number:
            out += node.numberValue;
            break;
        case JsonKind::String:
            out += '"';
            out += jsonEscape(node.stringValue);
            out += '"';
            break;
        case JsonKind::Array:
            if (node.arrayValues.empty()) {
                out += "[]";
                break;
            }
            out += '[';
            if (pretty) out += '\n';
            for (size_t i = 0; i < node.arrayValues.size(); ++i) {
                if (pretty) out += nextIndent;
                stringifyNode(*node.arrayValues[i], out, indent + 1, pretty);
                if (i + 1 < node.arrayValues.size()) out += ',';
                if (pretty) out += '\n';
            }
            if (pretty) out += currentIndent;
            out += ']';
            break;
        case JsonKind::Object:
            if (node.objectValues.empty()) {
                out += "{}";
                break;
            }
            out += '{';
            if (pretty) out += '\n';
            for (size_t i = 0; i < node.objectValues.size(); ++i) {
                if (pretty) out += nextIndent;
                out += '"';
                out += jsonEscape(node.objectValues[i].first);
                out += pretty ? "\": " : "\":";
                stringifyNode(*node.objectValues[i].second, out, indent + 1, pretty);
                if (i + 1 < node.objectValues.size()) out += ',';
                if (pretty) out += '\n';
            }
            if (pretty) out += currentIndent;
            out += '}';
            break;
    }
}

void serializeAnyValue(HooAnyValue value, std::string& out);

void serializeTensor(HooTensor tensor, std::string& out) {
    if (!tensor) throw std::runtime_error("Tensor is nil");
    int64_t rank = hoo_tensor_rank(tensor);
    if (rank < 0 || rank > 3) throw std::runtime_error("Tensor rank is invalid");
    out += "{\"__hoo_tensor__\":true,\"element_type\":";
    out += std::to_string(hoo_tensor_element_type(tensor));
    out += ",\"dims\":[";
    for (int64_t axis = 0; axis < rank; ++axis) {
        if (axis) out += ',';
        out += std::to_string(hoo_tensor_dim(tensor, axis));
    }
    out += "],\"data\":[";
    int64_t length = hoo_tensor_length(tensor);
    for (int64_t index = 0; index < length; ++index) {
        if (index) out += ',';
        out += std::to_string(hoo_tensor_get_bits(tensor, index));
    }
    out += "]}";
}

void serializeAnyArray(HooAnyArray array, std::string& out) {
    if (!array) throw std::runtime_error("AnyArray is nil");
    int64_t length = hoo_anyarray_length(array);
    if (length < 0) throw std::runtime_error("AnyArray length is invalid");
    out += '[';
    for (int64_t i = 0; i < length; ++i) {
        HooAnyValue value{0, 0};
        if (!hoo_anyarray_get(array, i, &value)) throw std::runtime_error("failed to read AnyArray element");
        if (i > 0) out += ',';
        serializeAnyValue(value, out);
    }
    out += ']';
}

void serializeHashMap(HooHashMap map, std::string& out) {
    if (!map) throw std::runtime_error("HashMap is nil");
    int64_t count = hoo_hashmap_count(map);
    if (count < 0) throw std::runtime_error("HashMap count is invalid");

    std::vector<int64_t> keys(static_cast<size_t>(count));
    int64_t actual = count == 0 ? 0 : hoo_hashmap_get_keys_i8(map, keys.data(), count);
    if (actual < 0) throw std::runtime_error("failed to enumerate HashMap keys");

    int64_t valueType = hoo_hashmap_value_type(map);
    out += '{';
    for (int64_t i = 0; i < actual; ++i) {
        if (i > 0) out += ',';
        out += '"';
        out += std::to_string(keys[static_cast<size_t>(i)]);
        out += "\":";
        if (valueType == HOO_TYPE_ANY) {
            HooAnyValue value{0, 0};
            if (!hoo_hashmap_get_any_at_i8(map, keys[static_cast<size_t>(i)], &value)) {
                throw std::runtime_error("failed to read HashMap any value");
            }
            serializeAnyValue(value, out);
        } else {
            uint64_t data = 0;
            if (!hoo_hashmap_get_fixed_at_i8(map, keys[static_cast<size_t>(i)], &data)) {
                throw std::runtime_error("failed to read HashMap value");
            }
            HooAnyValue value{valueType, data};
            serializeAnyValue(value, out);
        }
    }
    out += '}';
}

void serializeAnyValue(HooAnyValue value, std::string& out) {
    switch (value.type_id) {
        case HOO_TYPE_INT64:
            out += std::to_string(dataToInt64(value.data));
            return;
        case HOO_TYPE_INT8:
            out += std::to_string(static_cast<int8_t>(value.data));
            return;
        case HOO_TYPE_BYTE:
            out += std::to_string(static_cast<uint8_t>(value.data));
            return;
        case HOO_TYPE_BOOL:
            out += value.data ? "true" : "false";
            return;
        case HOO_TYPE_FLOAT64: {
            double d = 0.0;
            static_assert(sizeof(d) == sizeof(value.data), "double and uint64_t size mismatch");
            std::memcpy(&d, &value.data, sizeof(d));
            std::string formatted = formatDouble(d);
            if (formatted.empty()) throw std::runtime_error("non-finite floating-point value");
            out += formatted;
            return;
        }
        case HOO_TYPE_STRING: {
            if (value.data == 0) {
                out += "null";
                return;
            }
            const char* data = hoo_string_data(dataToPointer<void>(value.data));
            if (!data) throw std::runtime_error("string value is invalid");
            out += '"';
            out += jsonEscape(data);
            out += '"';
            return;
        }
        case HOO_TYPE_BUFFER: {
            if (value.data == 0) {
                out += "null";
                return;
            }
            char* encoded = hoo_encoding_base64_encode_buffer(dataToPointer<void>(value.data));
            if (!encoded) throw std::runtime_error("buffer base64 encoding failed");
            out += "{\"__hoo_buffer__\":true,\"data\":\"";
            out += jsonEscape(encoded);
            out += "\"}";
            hoo_encoding_free_string(encoded);
            return;
        }
        case HOO_TYPE_TENSOR_SERIALIZED:
            serializeTensor(dataToPointer<void>(value.data), out);
            return;
        case HOO_TYPE_HASHMAP:
            serializeHashMap(dataToPointer<void>(value.data), out);
            return;
        case HOO_TYPE_ANYARRAY:
            serializeAnyArray(dataToPointer<void>(value.data), out);
            return;
        case HOO_TYPE_VOID:
            out += "null";
            return;
        default:
            throw std::runtime_error("unsupported value type id " + std::to_string(value.type_id));
    }
}

bool parseInt64Text(const std::string& text, int64_t& out) {
    if (text.empty()) return false;
    errno = 0;
    char* end = nullptr;
    long long value = std::strtoll(text.c_str(), &end, 10);
    if (errno == ERANGE || !end || *end != '\0') return false;
    out = static_cast<int64_t>(value);
    return true;
}

bool numberIsFloat(const std::string& text) {
    return text.find_first_of(".eE") != std::string::npos;
}

HooAnyValue nodeToAnyValue(const JsonNode& node);

HooAnyArray nodeToAnyArray(const JsonNode& node) {
    if (node.kind != JsonKind::Array) throw std::runtime_error("JSON root is not an array");
    HooAnyArray array = hoo_anyarray_new_capacity(static_cast<int64_t>(node.arrayValues.size()));
    if (!array) throw std::runtime_error("failed to allocate AnyArray");
    try {
        for (const auto& item : node.arrayValues) {
            HooAnyValue value = nodeToAnyValue(*item);
            if (!hoo_anyarray_push(array, value.type_id, value.data)) {
                hoo_any_release(value);
                throw std::runtime_error("failed to append AnyArray element");
            }
            hoo_any_release(value);
        }
        return array;
    } catch (...) {
        hoo_anyarray_release(array);
        throw;
    }
}

HooHashMap nodeToHashMap(const JsonNode& node) {
    if (node.kind != JsonKind::Object) throw std::runtime_error("JSON root is not an object");
    HooHashMap map = hoo_hashmap_new(HOO_TYPE_INT64, HOO_TYPE_ANY);
    if (!map) throw std::runtime_error("failed to allocate HashMap");
    try {
        for (const auto& [keyText, valueNode] : node.objectValues) {
            int64_t key = 0;
            if (!parseInt64Text(keyText, key)) {
                throw std::runtime_error("JSON object key '" + keyText + "' is not a valid int64 HashMap key");
            }
            HooAnyValue value = nodeToAnyValue(*valueNode);
            if (!hoo_hashmap_set_any_i8(map, key, value.type_id, value.data)) {
                hoo_any_release(value);
                throw std::runtime_error("failed to set HashMap value");
            }
            hoo_any_release(value);
        }
        return map;
    } catch (...) {
        hoo_hashmap_release(map);
        throw;
    }
}

HooAnyValue nodeToAnyValue(const JsonNode& node) {
    switch (node.kind) {
        case JsonKind::Null:
            return HooAnyValue{HOO_TYPE_VOID, 0};
        case JsonKind::Bool:
            return HooAnyValue{HOO_TYPE_BOOL, node.boolValue ? 1ULL : 0ULL};
        case JsonKind::Number:
            if (numberIsFloat(node.numberValue)) {
                char* end = nullptr;
                errno = 0;
                double d = std::strtod(node.numberValue.c_str(), &end);
                if (errno == ERANGE || !end || *end != '\0' || !std::isfinite(d)) {
                    throw std::runtime_error("JSON number is outside supported f64 range");
                }
                uint64_t bits = 0;
                std::memcpy(&bits, &d, sizeof(bits));
                return HooAnyValue{HOO_TYPE_FLOAT64, bits};
            } else {
                int64_t value = 0;
                if (!parseInt64Text(node.numberValue, value)) {
                    throw std::runtime_error("JSON integer is outside int64 range");
                }
                return HooAnyValue{HOO_TYPE_INT64, int64ToData(value)};
            }
        case JsonKind::String: {
            HooString str = hoo_string_from_cstr(node.stringValue.c_str());
            if (!str) throw std::runtime_error("failed to allocate string");
            return HooAnyValue{HOO_TYPE_STRING, pointerToData(str)};
        }
        case JsonKind::Array: {
            HooAnyArray array = nodeToAnyArray(node);
            return HooAnyValue{HOO_TYPE_ANYARRAY, pointerToData(array)};
        }
        case JsonKind::Object: {
            const JsonNode* bufferMarker = nullptr;
            const JsonNode* bufferData = nullptr;
            for (const auto& [key, value] : node.objectValues) {
                if (key == "__hoo_buffer__") bufferMarker = value.get();
                else if (key == "data") bufferData = value.get();
            }
            if (bufferMarker && bufferMarker->kind == JsonKind::Bool &&
                bufferMarker->boolValue && bufferData &&
                bufferData->kind == JsonKind::String) {
                HooBuffer buffer = hoo_encoding_base64_decode_buffer(bufferData->stringValue.c_str());
                if (!buffer) throw std::runtime_error("invalid buffer base64 payload");
                return HooAnyValue{HOO_TYPE_BUFFER, pointerToData(buffer)};
            }
            const JsonNode* tensorMarker = nullptr;
            const JsonNode* elementTypeNode = nullptr;
            const JsonNode* dimsNode = nullptr;
            const JsonNode* dataNode = nullptr;
            for (const auto& [key, value] : node.objectValues) {
                if (key == "__hoo_tensor__") tensorMarker = value.get();
                else if (key == "element_type") elementTypeNode = value.get();
                else if (key == "dims") dimsNode = value.get();
                else if (key == "data") dataNode = value.get();
            }
            if (tensorMarker && tensorMarker->kind == JsonKind::Bool &&
                tensorMarker->boolValue && elementTypeNode && dimsNode && dataNode &&
                elementTypeNode->kind == JsonKind::Number && dimsNode->kind == JsonKind::Array &&
                dataNode->kind == JsonKind::Array && dimsNode->arrayValues.size() <= 3) {
                int64_t elementType = 0;
                if (!parseInt64Text(elementTypeNode->numberValue, elementType)) {
                    throw std::runtime_error("invalid tensor element type");
                }
                int64_t dims[3] = {1, 1, 1};
                for (size_t axis = 0; axis < dimsNode->arrayValues.size(); ++axis) {
                    const auto& dim = *dimsNode->arrayValues[axis];
                    if (dim.kind != JsonKind::Number || !parseInt64Text(dim.numberValue, dims[axis]) || dims[axis] < 0) {
                        throw std::runtime_error("invalid tensor dimension");
                    }
                }
                HooTensor tensor = hoo_tensor_new(elementType,
                    static_cast<int64_t>(dimsNode->arrayValues.size()), dims[0], dims[1], dims[2]);
                if (!tensor) throw std::runtime_error("failed to allocate tensor");
                try {
                    if (static_cast<int64_t>(dataNode->arrayValues.size()) != hoo_tensor_length(tensor)) {
                        throw std::runtime_error("tensor data length does not match dimensions");
                    }
                    for (size_t index = 0; index < dataNode->arrayValues.size(); ++index) {
                        int64_t bits = 0;
                        const auto& item = *dataNode->arrayValues[index];
                        if (item.kind != JsonKind::Number || !parseInt64Text(item.numberValue, bits) ||
                            !hoo_tensor_set_value(tensor, static_cast<int64_t>(index), bits)) {
                            throw std::runtime_error("invalid tensor data");
                        }
                    }
                    return HooAnyValue{HOO_TYPE_TENSOR_SERIALIZED, pointerToData(tensor)};
                } catch (...) {
                    hoo_release(tensor);
                    throw;
                }
            }
            HooHashMap map = nodeToHashMap(node);
            return HooAnyValue{HOO_TYPE_HASHMAP, pointerToData(map)};
        }
    }
    throw std::runtime_error("unsupported JSON value");
}

HooString parseAndFormat(const char* json, bool pretty) {
    if (!json) throw std::runtime_error("input string is nil");
    Parser parser(json);
    auto root = parser.parseDocument();
    if (!root) throw std::runtime_error("invalid JSON input");
    std::string out;
    stringifyNode(*root, out, 0, pretty);
    HooString result = hoo_string_from_cstr(out.c_str());
    if (!result) throw std::runtime_error("failed to allocate output string");
    return result;
}

} // namespace

extern "C" {

HooString hoo_json_serialize_hashmap(HooHashMap map) {
    try {
        std::string out;
        serializeHashMap(map, out);
        HooString result = hoo_string_from_cstr(out.c_str());
        if (!result) throw std::runtime_error("failed to allocate output string");
        return result;
    } catch (const std::exception& e) {
        rethrowAsJsonRuntime("serialization", e);
    } catch (...) {
        rethrowAsJsonRuntime("serialization");
    }
}

HooString hoo_json_serialize_anyarray(HooAnyArray array) {
    try {
        std::string out;
        serializeAnyArray(array, out);
        HooString result = hoo_string_from_cstr(out.c_str());
        if (!result) throw std::runtime_error("failed to allocate output string");
        return result;
    } catch (const std::exception& e) {
        rethrowAsJsonRuntime("serialization", e);
    } catch (...) {
        rethrowAsJsonRuntime("serialization");
    }
}

HooHashMap hoo_json_deserialize_hashmap(const char* json) {
    try {
        if (!json) throw std::runtime_error("input string is nil");
        Parser parser(json);
        auto root = parser.parseDocument();
        if (!root) throw std::runtime_error("invalid JSON input");
        return nodeToHashMap(*root);
    } catch (const std::exception& e) {
        rethrowAsJsonRuntime("deserialization", e);
    } catch (...) {
        rethrowAsJsonRuntime("deserialization");
    }
}

HooAnyArray hoo_json_deserialize_anyarray(const char* json) {
    try {
        if (!json) throw std::runtime_error("input string is nil");
        Parser parser(json);
        auto root = parser.parseDocument();
        if (!root) throw std::runtime_error("invalid JSON input");
        return nodeToAnyArray(*root);
    } catch (const std::exception& e) {
        rethrowAsJsonRuntime("deserialization", e);
    } catch (...) {
        rethrowAsJsonRuntime("deserialization");
    }
}

HooString hoo_json_minify(const char* json) {
    try {
        return parseAndFormat(json, false);
    } catch (const std::exception& e) {
        rethrowAsJsonRuntime("minification", e);
    } catch (...) {
        rethrowAsJsonRuntime("minification");
    }
}

HooString hoo_json_beautify(const char* json) {
    try {
        return parseAndFormat(json, true);
    } catch (const std::exception& e) {
        rethrowAsJsonRuntime("beautification", e);
    } catch (...) {
        rethrowAsJsonRuntime("beautification");
    }
}

} // extern "C"
