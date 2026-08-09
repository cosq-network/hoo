#pragma once

#include <stdint.h>
#include "hoo_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// A borrowed, read-only view over contiguous bytes. The view never retains,
// releases, reallocates, or frees its backing storage.
typedef struct HooByteSlice {
    const uint8_t* data;
    int64_t length;
} HooByteSlice;
typedef void* HooByteSliceHandle;

HooByteSlice hoo_byte_slice_empty(void);
HooByteSlice hoo_byte_slice_from_bytes(const uint8_t* data, int64_t length);
HooByteSlice hoo_byte_slice_from_buffer(HooBuffer buffer);
int64_t hoo_byte_slice_is_valid(HooByteSlice slice);
HooByteSliceHandle hoo_byte_slice_from_buffer_handle(HooBuffer buffer);
HooByteSlice hoo_byte_slice_view(HooByteSliceHandle handle);
void hoo_byte_slice_release(HooByteSliceHandle handle);

#ifdef __cplusplus
}
#endif
