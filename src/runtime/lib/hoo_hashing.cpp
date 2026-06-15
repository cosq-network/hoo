#include "hoo_hashing.h"

#ifdef __APPLE__
#include <CommonCrypto/CommonCrypto.h>
#else
#include <openssl/hmac.h>
#include <openssl/sha.h>
#endif

#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>

static void hex_encode(const uint8_t* in, int64_t in_len, char* out) {
    static const char* HEX = "0123456789abcdef";
    for (int64_t i = 0; i < in_len; i++) {
        out[i * 2] = HEX[(in[i] >> 4) & 0x0F];
        out[i * 2 + 1] = HEX[in[i] & 0x0F];
    }
    out[in_len * 2] = '\0';
}

static char* alloc_and_hex(const uint8_t* hash, int64_t hash_len) {
    char* result = (char*)malloc(hash_len * 2 + 1);
    if (!result) return nullptr;
    hex_encode(hash, hash_len, result);
    return result;
}

char* hoo_hashing_sha256(const uint8_t* data, int64_t len) {
    if (!data || len < 0) return nullptr;
#ifdef __APPLE__
    uint8_t hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(data, (CC_LONG)(len < 0 ? 0 : len), hash);
    return alloc_and_hex(hash, CC_SHA256_DIGEST_LENGTH);
#else
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(data, (size_t)len, hash);
    return alloc_and_hex(hash, SHA256_DIGEST_LENGTH);
#endif
}

char* hoo_hashing_sha256_file(const char* path) {
    if (!path) return nullptr;
    std::ifstream file(path, std::ios::binary);
    if (!file) return nullptr;
#ifdef __APPLE__
    CC_SHA256_CTX ctx;
    CC_SHA256_Init(&ctx);
    std::vector<char> buf(8192);
    while (file) {
        file.read(buf.data(), buf.size());
        std::streamsize n = file.gcount();
        if (n > 0) CC_SHA256_Update(&ctx, buf.data(), (CC_LONG)n);
    }
    uint8_t hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(hash, &ctx);
    return alloc_and_hex(hash, CC_SHA256_DIGEST_LENGTH);
#else
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    std::vector<char> buf(8192);
    while (file) {
        file.read(buf.data(), buf.size());
        std::streamsize n = file.gcount();
        if (n > 0) SHA256_Update(&ctx, buf.data(), (size_t)n);
    }
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);
    return alloc_and_hex(hash, SHA256_DIGEST_LENGTH);
#endif
}

char* hoo_hashing_sha1(const uint8_t* data, int64_t len) {
    if (!data || len < 0) return nullptr;
#ifdef __APPLE__
    uint8_t hash[CC_SHA1_DIGEST_LENGTH];
    CC_SHA1(data, (CC_LONG)(len < 0 ? 0 : len), hash);
    return alloc_and_hex(hash, CC_SHA1_DIGEST_LENGTH);
#else
    uint8_t hash[SHA_DIGEST_LENGTH];
    SHA1(data, (size_t)len, hash);
    return alloc_and_hex(hash, SHA_DIGEST_LENGTH);
#endif
}

char* hoo_hashing_md5(const uint8_t* data, int64_t len) {
    if (!data || len < 0) return nullptr;
#ifdef __APPLE__
    uint8_t hash[CC_MD5_DIGEST_LENGTH];
    CC_MD5(data, (CC_LONG)(len < 0 ? 0 : len), hash);
    return alloc_and_hex(hash, CC_MD5_DIGEST_LENGTH);
#else
    uint8_t hash[MD5_DIGEST_LENGTH];
    MD5(data, (size_t)len, hash);
    return alloc_and_hex(hash, MD5_DIGEST_LENGTH);
#endif
}

uint64_t hoo_hashing_crc32(const uint8_t* data, int64_t len) {
    if (!data || len < 0) return 0;
    static uint32_t table[256];
    static bool table_init = false;
    if (!table_init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (int j = 0; j < 8; j++) {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320;
                else
                    crc >>= 1;
            }
            table[i] = crc;
        }
        table_init = true;
    }
    uint32_t crc = 0xFFFFFFFF;
    for (int64_t i = 0; i < len; i++)
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return (uint64_t)(crc ^ 0xFFFFFFFF);
}

char* hoo_hashing_hmac_sha256(const uint8_t* key, int64_t key_len,
                               const uint8_t* data, int64_t data_len) {
    if (!key || !data || key_len < 0 || data_len < 0) return nullptr;
#ifdef __APPLE__
    uint8_t mac[CC_SHA256_DIGEST_LENGTH];
    CCHmac(kCCHmacAlgSHA256, key, (size_t)key_len, data, (size_t)data_len, mac);
    return alloc_and_hex(mac, CC_SHA256_DIGEST_LENGTH);
#else
    unsigned int mac_len = 0;
    uint8_t mac[SHA256_DIGEST_LENGTH];
    HMAC(EVP_sha256(), key, (int)key_len, data, (size_t)data_len, mac, &mac_len);
    return alloc_and_hex(mac, SHA256_DIGEST_LENGTH);
#endif
}

void hoo_hashing_free_string(char* str) {
    free(str);
}
