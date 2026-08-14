#include "runtime/lib/encoding/hoo_encoding.h"
#include "runtime/lib/buffer/hoo_buffer.h"
#include <cstring>
#include <cstdlib>
#include <string>
#include <cctype>

static const char BASE64_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

char* hoo_encoding_base64_encode(const uint8_t* data, int64_t len) {
    if (len < 0 || (len != 0 && !data)) return nullptr;
    if (len == 0) {
        char* r = (char*)malloc(1);
        if (r) r[0] = '\0';
        return r;
    }

    int64_t out_len = ((len + 2) / 3) * 4;
    char* result = (char*)malloc(out_len + 1);
    if (!result) return nullptr;

    int64_t i = 0, j = 0;
    while (i < len) {
        int remaining = len - i;
        uint8_t b0 = data[i++];
        uint8_t b1 = (remaining >= 2) ? data[i++] : 0;
        uint8_t b2 = (remaining >= 3) ? data[i++] : 0;
        uint32_t triple = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | b2;

        result[j++] = BASE64_ALPHABET[(triple >> 18) & 0x3F];
        result[j++] = BASE64_ALPHABET[(triple >> 12) & 0x3F];
        result[j++] = (remaining >= 2) ? BASE64_ALPHABET[(triple >> 6) & 0x3F] : '=';
        result[j++] = (remaining >= 3) ? BASE64_ALPHABET[triple & 0x3F] : '=';
    }
    result[j] = '\0';
    return result;
}

int64_t hoo_encoding_base64_decode(const char* encoded, uint8_t** out_data) {
    if (!encoded || !out_data) return -1;
    *out_data = nullptr;

    int64_t len = (int64_t)std::strlen(encoded);
    if (len == 0) {
        *out_data = (uint8_t*)malloc(1);
        if (*out_data) **out_data = 0;
        return 0;
    }

    if (len % 4 != 0) return -1;

    int pad = 0;
    if (len >= 1 && encoded[len - 1] == '=') pad++;
    if (len >= 2 && encoded[len - 2] == '=') pad++;
    if (pad > 2) return -1;

    int64_t valid_len = len - pad;

    for (int64_t i = 0; i < valid_len; i++) {
        if (base64_index(encoded[i]) < 0) return -1;
    }
    for (int64_t i = valid_len; i < len; i++) {
        if (encoded[i] != '=') return -1;
    }

    int64_t out_len = (len / 4) * 3 - pad;
    uint8_t* result = (uint8_t*)malloc(out_len + 1);
    if (!result) return -1;

    int64_t j = 0;
    for (int64_t i = 0; i < len; i += 4) {
        uint32_t b0 = (uint32_t)base64_index(encoded[i]);
        uint32_t b1 = (uint32_t)base64_index(encoded[i + 1]);
        bool has_b2 = (i + 2 < len && encoded[i + 2] != '=');
        bool has_b3 = (i + 3 < len && encoded[i + 3] != '=');
        uint32_t b2 = has_b2 ? (uint32_t)base64_index(encoded[i + 2]) : 0;
        uint32_t b3 = has_b3 ? (uint32_t)base64_index(encoded[i + 3]) : 0;
        uint32_t triple = (b0 << 18) | (b1 << 12) | (b2 << 6) | b3;

        result[j++] = (uint8_t)((triple >> 16) & 0xFF);
        if (has_b2) result[j++] = (uint8_t)((triple >> 8) & 0xFF);
        if (has_b3) result[j++] = (uint8_t)(triple & 0xFF);
    }
    result[out_len] = 0;
    *out_data = result;
    return out_len;
}

char* hoo_encoding_hex_encode(const uint8_t* data, int64_t len) {
    if (len < 0 || (len != 0 && !data)) return nullptr;
    if (len == 0) {
        char* r = (char*)malloc(1);
        if (r) r[0] = '\0';
        return r;
    }

    static const char* HEX = "0123456789abcdef";
    char* result = (char*)malloc(len * 2 + 1);
    if (!result) return nullptr;

    for (int64_t i = 0; i < len; i++) {
        result[i * 2] = HEX[(data[i] >> 4) & 0x0F];
        result[i * 2 + 1] = HEX[data[i] & 0x0F];
    }
    result[len * 2] = '\0';
    return result;
}

int64_t hoo_encoding_hex_decode(const char* hex, uint8_t** out_data) {
    if (!hex || !out_data) return -1;
    *out_data = nullptr;

    int64_t len = (int64_t)std::strlen(hex);
    if (len % 2 != 0) return -1;
    if (len == 0) {
        *out_data = (uint8_t*)malloc(1);
        if (*out_data) **out_data = 0;
        return 0;
    }

    int64_t out_len = len / 2;
    uint8_t* result = (uint8_t*)malloc(out_len + 1);
    if (!result) return -1;

    for (int64_t i = 0; i < out_len; i++) {
        int h = hex_val(hex[i * 2]);
        int l = hex_val(hex[i * 2 + 1]);
        if (h < 0 || l < 0) {
            free(result);
            return -1;
        }
        result[i] = (uint8_t)((h << 4) | l);
    }
    result[out_len] = 0;
    *out_data = result;
    return out_len;
}

char* hoo_encoding_url_encode(const char* str) {
    if (!str) return nullptr;
    if (*str == '\0') {
        char* r = (char*)malloc(1);
        if (r) r[0] = '\0';
        return r;
    }

    int64_t len = (int64_t)std::strlen(str);
    char* result = (char*)malloc(len * 3 + 1);
    if (!result) return nullptr;

    static const char* HEX = "0123456789ABCDEF";
    int64_t j = 0;
    for (int64_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result[j++] = (char)c;
        } else {
            result[j++] = '%';
            result[j++] = HEX[(c >> 4) & 0x0F];
            result[j++] = HEX[c & 0x0F];
        }
    }
    result[j] = '\0';
    return result;
}

char* hoo_encoding_url_decode(const char* encoded) {
    if (!encoded) return nullptr;
    if (*encoded == '\0') {
        char* r = (char*)malloc(1);
        if (r) r[0] = '\0';
        return r;
    }

    int64_t len = (int64_t)std::strlen(encoded);
    char* result = (char*)malloc(len + 1);
    if (!result) return nullptr;

    int64_t j = 0;
    for (int64_t i = 0; i < len; i++) {
        if (encoded[i] == '+') {
            result[j++] = ' ';
        } else if (encoded[i] == '%') {
            if (i + 2 >= len) {
                free(result);
                return nullptr;
            }
            int h = hex_val((unsigned char)encoded[i + 1]);
            int l = hex_val((unsigned char)encoded[i + 2]);
            if (h < 0 || l < 0) {
                free(result);
                return nullptr;
            }
            result[j++] = (char)((h << 4) | l);
            i += 2;
        } else {
            result[j++] = encoded[i];
        }
    }
    result[j] = '\0';
    return result;
}

void hoo_encoding_free_string(char* str) {
    free(str);
}

void hoo_encoding_free_bytes(uint8_t* data) {
    free(data);
}

char* hoo_encoding_base64_encode_buffer(HooBuffer buf) {
    return hoo_encoding_base64_encode(hoo_buffer_data(buf), hoo_buffer_length(buf));
}

HooBuffer hoo_encoding_base64_decode_buffer(const char* encoded) {
    uint8_t* data = nullptr;
    int64_t len = hoo_encoding_base64_decode(encoded, &data);
    if (len < 0 || !data) return nullptr;
    HooBuffer buf = hoo_buffer_from_bytes(data, len);
    free(data);
    return buf;
}

char* hoo_encoding_hex_encode_buffer(HooBuffer buf) {
    return hoo_encoding_hex_encode(hoo_buffer_data(buf), hoo_buffer_length(buf));
}

HooBuffer hoo_encoding_hex_decode_buffer(const char* hex) {
    uint8_t* data = nullptr;
    int64_t len = hoo_encoding_hex_decode(hex, &data);
    if (len < 0 || !data) return nullptr;
    HooBuffer buf = hoo_buffer_from_bytes(data, len);
    free(data);
    return buf;
}

char* hoo_encoding_base64_encode_slice(HooByteSliceHandle slice) {
    HooByteSlice view = hoo_byte_slice_view(slice);
    return hoo_encoding_base64_encode(view.data, view.length);
}

char* hoo_encoding_hex_encode_slice(HooByteSliceHandle slice) {
    HooByteSlice view = hoo_byte_slice_view(slice);
    return hoo_encoding_hex_encode(view.data, view.length);
}
