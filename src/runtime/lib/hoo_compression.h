#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void*   hoo_compression_new(void);
void    hoo_compression_release(void* comp);

int64_t hoo_compression_gzip_compress(const uint8_t* data, int64_t data_len,
                                       uint8_t** out_data, int64_t* out_len);
int64_t hoo_compression_gzip_decompress(const uint8_t* data, int64_t data_len,
                                          uint8_t** out_data, int64_t* out_len);
int64_t hoo_compression_deflate_compress(const uint8_t* data, int64_t data_len,
                                           uint8_t** out_data, int64_t* out_len);
int64_t hoo_compression_deflate_decompress(const uint8_t* data, int64_t data_len,
                                             uint8_t** out_data, int64_t* out_len);
void    hoo_compression_free_bytes(uint8_t* data);

#ifdef __cplusplus
}
#endif
