#include "runtime/lib/text/hoo_character.h"
#include "runtime/lib/core/hoo_runtime.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

struct HooCharacterImpl {
    int64_t length;
    char data[5]; // Up to 4 bytes + null terminator
};

static HooCharacterImpl* get_impl(HooCharacter ch) {
    return (HooCharacterImpl*)ch;
}

static HooCharacter from_impl(HooCharacterImpl* impl) {
    return (HooCharacter)impl;
}

HooCharacter hoo_character_from_utf8(const char* bytes, int64_t length) {
    if (!bytes || length <= 0) return nullptr;

    const unsigned char* b = (const unsigned char*)bytes;

    // Determine the UTF-8 sequence length from the lead byte.
    int64_t seq_len = 1;
    if (b[0] <= 0x7F) {
        seq_len = 1;
    } else if ((b[0] & 0xE0) == 0xC0) {
        seq_len = 2;
    } else if ((b[0] & 0xF0) == 0xE0) {
        seq_len = 3;
    } else if ((b[0] & 0xF8) == 0xF0) {
        seq_len = 4;
    } else {
        return nullptr; // Invalid UTF-8 lead byte
    }

    if (seq_len > length) return nullptr; // Truncated UTF-8 sequence

    // Validate continuation bytes.
    for (int64_t i = 1; i < seq_len; ++i) {
        if ((b[i] & 0xC0) != 0x80) return nullptr;
    }

    HooCharacterImpl* impl = (HooCharacterImpl*)hoo_alloc(sizeof(HooCharacterImpl), HOO_TYPE_CHARACTER);
    impl->length = seq_len;
    std::memcpy(impl->data, bytes, (size_t)seq_len);
    impl->data[seq_len] = '\0';

    return from_impl(impl);
}

HooCharacter hoo_character_from_codepoint(int64_t cp) {
    // Negative values are not scalar values, and UTF-16 surrogate codepoints
    // (0xD800-0xDFFF) are not valid Unicode scalar values either. Map both to
    // the replacement character U+FFFD.
    if (cp < 0 || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return hoo_character_from_codepoint(0xFFFD);
    }

    char bytes[4];
    int64_t len = 0;

    if (cp <= 0x7F) {
        bytes[0] = (char)cp;
        len = 1;
    } else if (cp <= 0x7FF) {
        bytes[0] = (char)(0xC0 | (cp >> 6));
        bytes[1] = (char)(0x80 | (cp & 0x3F));
        len = 2;
    } else if (cp <= 0xFFFF) {
        bytes[0] = (char)(0xE0 | (cp >> 12));
        bytes[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        bytes[2] = (char)(0x80 | (cp & 0x3F));
        len = 3;
    } else if (cp <= 0x10FFFF) {
        bytes[0] = (char)(0xF0 | (cp >> 18));
        bytes[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        bytes[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        bytes[3] = (char)(0x80 | (cp & 0x3F));
        len = 4;
    } else {
        // Invalid codepoint, return replacement character U+FFFD
        return hoo_character_from_codepoint(0xFFFD);
    }

    return hoo_character_from_utf8(bytes, len);
}

int64_t hoo_character_length(HooCharacter ch) {
    if (!ch) return 0;
    return get_impl(ch)->length;
}

const char* hoo_character_data(HooCharacter ch) {
    if (!ch) return "";
    return get_impl(ch)->data;
}

int64_t hoo_character_codepoint(HooCharacter ch) {
    if (!ch) return 0;
    HooCharacterImpl* impl = get_impl(ch);
    const unsigned char* b = (const unsigned char*)impl->data;
    
    if (impl->length == 1) return b[0];
    if (impl->length == 2) return ((b[0] & 0x1F) << 6) | (b[1] & 0x3F);
    if (impl->length == 3) return ((b[0] & 0x0F) << 12) | ((b[1] & 0x3F) << 6) | (b[2] & 0x3F);
    if (impl->length == 4) return ((b[0] & 0x07) << 18) | ((b[1] & 0x3F) << 12) | ((b[2] & 0x3F) << 6) | (b[3] & 0x3F);
    
    return 0;
}

HooCharacter hoo_character_retain(HooCharacter ch) {
    return (HooCharacter)hoo_retain(ch);
}

void hoo_character_release(HooCharacter ch) {
    hoo_release(ch);
}

int64_t hoo_character_refcount(HooCharacter ch) {
    return hoo_get_refcount(ch);
}

void hoo_character_print(HooCharacter ch) {
    if (!ch) return;
    std::printf("%s", get_impl(ch)->data);
}
