#pragma once

#include <stdint.h>
#include "hoo_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

char*   hoo_encoding_base64_encode(const uint8_t* data, int64_t len);
int64_t hoo_encoding_base64_decode(const char* encoded, uint8_t** out_data);

char*   hoo_encoding_hex_encode(const uint8_t* data, int64_t len);
int64_t hoo_encoding_hex_decode(const char* hex, uint8_t** out_data);

char*    hoo_encoding_base64_encode_buffer(HooBuffer buf);
HooBuffer hoo_encoding_base64_decode_buffer(const char* encoded);
char*    hoo_encoding_hex_encode_buffer(HooBuffer buf);
HooBuffer hoo_encoding_hex_decode_buffer(const char* hex);

char*   hoo_encoding_url_encode(const char* str);
char*   hoo_encoding_url_decode(const char* encoded);

void    hoo_encoding_free_string(char* str);
void    hoo_encoding_free_bytes(uint8_t* data);

#ifdef __cplusplus
}
#endif
