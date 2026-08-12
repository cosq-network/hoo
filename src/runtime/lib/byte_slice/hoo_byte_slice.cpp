#include "runtime/lib/byte_slice/hoo_byte_slice.h"
#include "runtime/lib/runtime/hoo_runtime.h"

extern "C" {

HooByteSlice hoo_byte_slice_empty(void) {
    return HooByteSlice{nullptr, 0};
}

HooByteSlice hoo_byte_slice_from_bytes(const uint8_t* data, int64_t length) {
    if (!data || length <= 0) return hoo_byte_slice_empty();
    return HooByteSlice{data, length};
}

HooByteSlice hoo_byte_slice_from_buffer(HooBuffer buffer) {
    if (!buffer) return hoo_byte_slice_empty();
    return hoo_byte_slice_from_bytes(hoo_buffer_data(buffer), hoo_buffer_length(buffer));
}

int64_t hoo_byte_slice_is_valid(HooByteSlice slice) {
    return slice.length >= 0 && (slice.length == 0 || slice.data != nullptr) ? 1 : 0;
}

HooByteSliceHandle hoo_byte_slice_from_buffer_handle(HooBuffer buffer) {
    HooByteSlice slice = hoo_byte_slice_from_buffer(buffer);
    if (!hoo_byte_slice_is_valid(slice)) return nullptr;
    auto* stored = static_cast<HooByteSlice*>(hoo_alloc(sizeof(HooByteSlice), HOO_TYPE_BYTE_SLICE));
    if (!stored) return nullptr;
    *stored = slice;
    return stored;
}

HooByteSlice hoo_byte_slice_view(HooByteSliceHandle handle) {
    if (!handle) return hoo_byte_slice_empty();
    return *static_cast<const HooByteSlice*>(handle);
}

void hoo_byte_slice_release(HooByteSliceHandle handle) {
    hoo_release(handle);
}

}
