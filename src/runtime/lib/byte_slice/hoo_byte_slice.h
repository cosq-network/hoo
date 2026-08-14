#pragma once

#include <stdint.h>
#include "runtime/lib/buffer/hoo_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// A borrowed, read-only view over contiguous bytes. A HooByteSlice value never
// retains, releases, reallocates, or frees its backing storage; the caller must
// keep the backing bytes alive for as long as the slice is used.
typedef struct HooByteSlice {
    const uint8_t* data;
    int64_t length;
} HooByteSlice;

// An ARC-managed handle that owns a HooByteSlice value. Unlike the raw value,
// a handle keeps its backing storage alive: when created from a buffer the
// handle retains the buffer, and releasing the handle (hoo_byte_slice_release)
// releases the buffer. Handles are exclusively owned by the caller and MUST be
// released with hoo_byte_slice_release exactly once. The codegen does not
// auto-release byte_slice locals; forgetting to call hoo_byte_slice_release
// leaks the handle and its backing buffer.
typedef void* HooByteSliceHandle;

HooByteSlice hoo_byte_slice_empty(void);
HooByteSlice hoo_byte_slice_from_bytes(const uint8_t* data, int64_t length);
HooByteSlice hoo_byte_slice_from_buffer(HooBuffer buffer);
int64_t hoo_byte_slice_is_valid(HooByteSlice slice);
HooByteSliceHandle hoo_byte_slice_from_bytes_handle(const uint8_t* data, int64_t length);
HooByteSliceHandle hoo_byte_slice_from_buffer_handle(HooBuffer buffer);
HooByteSlice hoo_byte_slice_view(HooByteSliceHandle handle);
void hoo_byte_slice_release(HooByteSliceHandle handle);

#ifdef __cplusplus
}
#endif
