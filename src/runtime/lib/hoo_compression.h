#pragma once

#include <stdint.h>
#include <stddef.h>
#include "hoo_buffer.h"

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
HooBuffer hoo_compression_gzip_compress_buffer(HooBuffer buf);
HooBuffer hoo_compression_gzip_decompress_buffer(HooBuffer buf);
HooBuffer hoo_compression_deflate_compress_buffer(HooBuffer buf);
HooBuffer hoo_compression_deflate_decompress_buffer(HooBuffer buf);
void    hoo_compression_free_bytes(uint8_t* data);

#ifdef __cplusplus
}
#endif
