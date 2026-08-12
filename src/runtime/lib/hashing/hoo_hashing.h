#pragma once

#include <stdint.h>
#include <stddef.h>
#include "runtime/lib/buffer/hoo_buffer.h"
#include "runtime/lib/byte_slice/hoo_byte_slice.h"

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
char*    hoo_hashing_sha256_buffer(HooBuffer buf);
char*    hoo_hashing_sha1_buffer(HooBuffer buf);
char*    hoo_hashing_md5_buffer(HooBuffer buf);
uint64_t hoo_hashing_crc32_buffer(HooBuffer buf);
char*    hoo_hashing_sha256_slice(HooByteSliceHandle slice);
char*    hoo_hashing_sha1_slice(HooByteSliceHandle slice);
char*    hoo_hashing_md5_slice(HooByteSliceHandle slice);
uint64_t hoo_hashing_crc32_slice(HooByteSliceHandle slice);
char*    hoo_hashing_hmac_sha256_buffer(HooBuffer key_buf, HooBuffer data_buf);
void     hoo_hashing_free_string(char* str);

#ifdef __cplusplus
}
#endif
