#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

char*    hoo_hashing_sha256(const uint8_t* data, int64_t len);
char*    hoo_hashing_sha256_file(const char* path);
char*    hoo_hashing_sha1(const uint8_t* data, int64_t len);
char*    hoo_hashing_md5(const uint8_t* data, int64_t len);
uint64_t hoo_hashing_crc32(const uint8_t* data, int64_t len);
char*    hoo_hashing_hmac_sha256(const uint8_t* key, int64_t key_len,
                                 const uint8_t* data, int64_t data_len);
void     hoo_hashing_free_string(char* str);

#ifdef __cplusplus
}
#endif
