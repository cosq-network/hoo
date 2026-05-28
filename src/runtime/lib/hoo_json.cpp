#include "hoo_json.h"
#include "hoo_runtime.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <cstdio>

extern "C" {

// ============================================================================
// Internal structures
// ============================================================================

enum HooJsonType {
    JSON_NULL = 0,
    JSON_BOOL = 1,
    JSON_INT = 2,
    JSON_STRING = 3,
    JSON_ARRAY = 4,
    JSON_OBJECT = 5
};

struct JsonBase {
    int64_t refcount;
    int64_t type;
};

struct JsonNull : JsonBase {};
struct JsonBool : JsonBase { int64_t value; };
struct JsonInt : JsonBase { int64_t value; };
struct JsonString : JsonBase { std::string value; };
struct JsonArray : JsonBase { std::vector<JsonBase*> items; };
struct JsonObject : JsonBase { std::map<std::string, JsonBase*> fields; };

// ============================================================================
// Helpers
// ============================================================================

static JsonBase* json_alloc(int64_t type, size_t size) {
    void* mem = hoo_alloc(size, HOO_TYPE_JSON);
    auto* base = static_cast<JsonBase*>(mem);
    base->refcount = 1;
    base->type = type;
    return base;
}

static JsonBase* json_retain(JsonBase* json) {
    if (json) {
        hoo_retain(json);
        json->refcount++;
    }
    return json;
}

static void json_release(JsonBase* json) {
    if (!json) return;
    json->refcount--;
    if (json->refcount > 0) {
        hoo_release(json);
        return;
    }
    if (json->type == JSON_ARRAY) {
        auto* arr = static_cast<JsonArray*>(json);
        for (auto* item : arr->items) {
            json_release(item);
        }
        arr->items.~vector();
    } else if (json->type == JSON_OBJECT) {
        auto* obj = static_cast<JsonObject*>(json);
        for (auto& [_, val] : obj->fields) {
            json_release(val);
        }
        obj->fields.~map();
    } else if (json->type == JSON_STRING) {
        static_cast<JsonString*>(json)->value.~basic_string();
    }
    hoo_release(json);
}

// ============================================================================
// Parser
// ============================================================================

struct Parser {
    const char* p;
    size_t len;
    size_t pos;

    Parser(const char* s) : p(s), len(std::strlen(s)), pos(0) {}

    void skip_ws() {
        while (pos < len && (p[pos] == ' ' || p[pos] == '\t' || p[pos] == '\n' || p[pos] == '\r'))
            pos++;
    }

    char peek() {
        skip_ws();
        return pos < len ? p[pos] : '\0';
    }

    char advance() {
        return pos < len ? p[pos++] : '\0';
    }

    bool expect(char c) {
        skip_ws();
        if (pos < len && p[pos] == c) {
            pos++;
            return true;
        }
        return false;
    }

    JsonBase* parse_value();

    JsonBase* parse_object() {
        auto* obj = static_cast<JsonObject*>(json_alloc(JSON_OBJECT, sizeof(JsonObject)));
        new (&obj->fields) std::map<std::string, JsonBase*>();
        advance();
        if (peek() == '}') { advance(); return obj; }
        while (true) {
            skip_ws();
            if (pos >= len) { json_release(obj); return nullptr; }
            std::string key = parse_string_raw();
            if (!expect(':')) { json_release(obj); return nullptr; }
            auto* val = parse_value();
            if (!val) { json_release(obj); return nullptr; }
            obj->fields[key] = val;
            skip_ws();
            if (peek() == '}') { advance(); break; }
            if (!expect(',')) { json_release(obj); return nullptr; }
        }
        return obj;
    }

    JsonBase* parse_array() {
        auto* arr = static_cast<JsonArray*>(json_alloc(JSON_ARRAY, sizeof(JsonArray)));
        new (&arr->items) std::vector<JsonBase*>();
        advance();
        if (peek() == ']') { advance(); return arr; }
        while (true) {
            auto* val = parse_value();
            if (!val) { json_release(arr); return nullptr; }
            arr->items.push_back(val);
            skip_ws();
            if (peek() == ']') { advance(); break; }
            if (!expect(',')) { json_release(arr); return nullptr; }
        }
        return arr;
    }

    std::string parse_string_raw() {
        skip_ws();
        if (pos >= len || p[pos] != '"') return "";
        pos++;
        std::string result;
        while (pos < len) {
            char c = p[pos++];
            if (c == '"') break;
            if (c == '\\') {
                if (pos >= len) break;
                char esc = p[pos++];
                switch (esc) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        if (pos + 4 > len) break;
                        char hex[5] = {p[pos], p[pos+1], p[pos+2], p[pos+3], 0};
                        pos += 4;
                        unsigned int cp;
                        std::sscanf(hex, "%x", &cp);
                        if (cp <= 0x7F) {
                            result += static_cast<char>(cp);
                        } else if (cp <= 0x7FF) {
                            result += static_cast<char>(0xC0 | (cp >> 6));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (cp >> 12));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        break;
                    }
                    default: result += esc; break;
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    JsonBase* parse_string() {
        std::string s = parse_string_raw();
        auto* js = static_cast<JsonString*>(json_alloc(JSON_STRING, sizeof(JsonString)));
        new (&js->value) std::string(std::move(s));
        return js;
    }

    JsonBase* parse_number() {
        skip_ws();
        size_t start = pos;
        if (pos < len && p[pos] == '-') pos++;
        while (pos < len && p[pos] >= '0' && p[pos] <= '9') pos++;
        if (pos < len && p[pos] == '.') {
            pos++;
            while (pos < len && p[pos] >= '0' && p[pos] <= '9') pos++;
        }
        if (pos < len && (p[pos] == 'e' || p[pos] == 'E')) {
            pos++;
            if (pos < len && (p[pos] == '+' || p[pos] == '-')) pos++;
            while (pos < len && p[pos] >= '0' && p[pos] <= '9') pos++;
        }
        std::string num(p + start, pos - start);
        auto* js = static_cast<JsonInt*>(json_alloc(JSON_INT, sizeof(JsonInt)));
        js->value = std::stoll(num);
        return js;
    }

    JsonBase* parse_keyword(const char* kw, int64_t type_val) {
        size_t kwlen = std::strlen(kw);
        skip_ws();
        if (pos + kwlen <= len && std::strncmp(p + pos, kw, kwlen) == 0) {
            pos += kwlen;
            auto* js = static_cast<JsonInt*>(json_alloc(type_val, sizeof(JsonInt)));
            js->value = type_val == JSON_BOOL ? 1 : 0;
            return js;
        }
        return nullptr;
    }
};

JsonBase* Parser::parse_value() {
    char c = peek();
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == '"') return parse_string();
    if (c == '-' || (c >= '0' && c <= '9')) return parse_number();
    if (c == 't' || c == 'f') return parse_keyword("true", JSON_BOOL);
    if (c == 'n') {
        auto* result = parse_keyword("null", JSON_NULL);
        if (result) {
            result->type = JSON_NULL;
            static_cast<JsonInt*>(result)->value = 0;
        }
        return result;
    }
    return nullptr;
}

// ============================================================================
// Stringification
// ============================================================================

static void stringify_value(JsonBase* json, std::string& out) {
    if (!json) { out += "null"; return; }
    switch (json->type) {
        case JSON_NULL: out += "null"; break;
        case JSON_BOOL: out += static_cast<JsonBool*>(json)->value ? "true" : "false"; break;
        case JSON_INT: out += std::to_string(static_cast<JsonInt*>(json)->value); break;
        case JSON_STRING: {
            out += '"';
            const auto& s = static_cast<JsonString*>(json)->value;
            for (char c : s) {
                switch (c) {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\b': out += "\\b"; break;
                    case '\f': out += "\\f"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default: out += c; break;
                }
            }
            out += '"';
            break;
        }
        case JSON_ARRAY: {
            out += '[';
            auto& items = static_cast<JsonArray*>(json)->items;
            for (size_t i = 0; i < items.size(); i++) {
                if (i > 0) out += ',';
                stringify_value(items[i], out);
            }
            out += ']';
            break;
        }
        case JSON_OBJECT: {
            out += '{';
            auto& fields = static_cast<JsonObject*>(json)->fields;
            bool first = true;
            for (auto& [key, val] : fields) {
                if (!first) out += ',';
                first = false;
                out += '"' + key + '"' + ':';
                stringify_value(val, out);
            }
            out += '}';
            break;
        }
    }
}

// ============================================================================
// Public API
// ============================================================================

HooJson hoo_json_parse(const char* json) {
    if (!json) return nullptr;
    Parser parser(json);
    auto* result = parser.parse_value();
    if (!result || parser.peek() != '\0') {
        if (result) json_release(result);
        return nullptr;
    }
    return result;
}

char* hoo_json_stringify(HooJson json) {
    if (!json) return strdup("null");
    std::string out;
    stringify_value(static_cast<JsonBase*>(json), out);
    return strdup(out.c_str());
}

HooJson hoo_json_get(HooJson obj, const char* key) {
    if (!obj || !key) return nullptr;
    auto* base = static_cast<JsonBase*>(obj);
    if (base->type != JSON_OBJECT) return nullptr;
    auto& fields = static_cast<JsonObject*>(base)->fields;
    auto it = fields.find(key);
    if (it == fields.end()) return nullptr;
    return json_retain(it->second);
}

int64_t hoo_json_get_int(HooJson obj, const char* key) {
    HooJson val = hoo_json_get(obj, key);
    if (!val) return 0;
    auto* base = static_cast<JsonBase*>(val);
    int64_t result = 0;
    if (base->type == JSON_INT) result = static_cast<JsonInt*>(val)->value;
    json_release(static_cast<JsonBase*>(val));
    return result;
}

char* hoo_json_get_string(HooJson obj, const char* key) {
    HooJson val = hoo_json_get(obj, key);
    if (!val) return nullptr;
    auto* base = static_cast<JsonBase*>(val);
    char* result = nullptr;
    if (base->type == JSON_STRING) {
        result = strdup(static_cast<JsonString*>(val)->value.c_str());
    }
    json_release(static_cast<JsonBase*>(val));
    return result;
}

int64_t hoo_json_set(HooJson obj, const char* key, HooJson val) {
    if (!obj || !key) return 0;
    auto* base = static_cast<JsonBase*>(obj);
    if (base->type != JSON_OBJECT) return 0;
    auto& fields = static_cast<JsonObject*>(base)->fields;
    auto it = fields.find(key);
    if (it != fields.end()) {
        json_release(it->second);
    }
    fields[key] = static_cast<JsonBase*>(val);
    json_retain(static_cast<JsonBase*>(val));
    return 1;
}

HooJson hoo_json_array_get(HooJson arr, int64_t index) {
    if (!arr) return nullptr;
    auto* base = static_cast<JsonBase*>(arr);
    if (base->type != JSON_ARRAY) return nullptr;
    auto& items = static_cast<JsonArray*>(base)->items;
    if (index < 0 || static_cast<size_t>(index) >= items.size()) return nullptr;
    return json_retain(items[index]);
}

int64_t hoo_json_array_push(HooJson arr, HooJson val) {
    if (!arr || !val) return 0;
    auto* base = static_cast<JsonBase*>(arr);
    if (base->type != JSON_ARRAY) return 0;
    static_cast<JsonArray*>(base)->items.push_back(static_cast<JsonBase*>(val));
    json_retain(static_cast<JsonBase*>(val));
    return 1;
}

int64_t hoo_json_array_length(HooJson arr) {
    if (!arr) return 0;
    auto* base = static_cast<JsonBase*>(arr);
    if (base->type != JSON_ARRAY) return 0;
    return static_cast<int64_t>(static_cast<JsonArray*>(base)->items.size());
}

int64_t hoo_json_type(HooJson json) {
    if (!json) return HOO_JSON_NULL;
    return static_cast<JsonBase*>(json)->type;
}

HooJson hoo_json_new_object(void) {
    auto* obj = static_cast<JsonObject*>(json_alloc(JSON_OBJECT, sizeof(JsonObject)));
    new (&obj->fields) std::map<std::string, JsonBase*>();
    return obj;
}

HooJson hoo_json_new_array(void) {
    auto* arr = static_cast<JsonArray*>(json_alloc(JSON_ARRAY, sizeof(JsonArray)));
    new (&arr->items) std::vector<JsonBase*>();
    return arr;
}

HooJson hoo_json_new_string(const char* s) {
    auto* js = static_cast<JsonString*>(json_alloc(JSON_STRING, sizeof(JsonString)));
    new (&js->value) std::string(s ? s : "");
    return js;
}

HooJson hoo_json_new_int(int64_t n) {
    auto* js = static_cast<JsonInt*>(json_alloc(JSON_INT, sizeof(JsonInt)));
    js->value = n;
    return js;
}

HooJson hoo_json_new_bool(int64_t b) {
    auto* js = static_cast<JsonBool*>(json_alloc(JSON_BOOL, sizeof(JsonBool)));
    js->value = b;
    return js;
}

HooJson hoo_json_new_null(void) {
    return json_alloc(JSON_NULL, sizeof(JsonNull));
}

void hoo_json_retain(HooJson json) {
    json_retain(static_cast<JsonBase*>(json));
}

void hoo_json_release(HooJson json) {
    json_release(static_cast<JsonBase*>(json));
}

void hoo_json_free_string(char* str) {
    std::free(str);
}

} // extern "C"
