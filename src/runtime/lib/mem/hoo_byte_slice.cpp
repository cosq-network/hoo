#include "runtime/lib/mem/hoo_byte_slice.h"
#include "runtime/lib/core/hoo_runtime.h"
#include <cstdlib>

extern "C" {

struct HooByteSliceHandleImpl {
    HooByteSlice view;
    HooBuffer owner; // Retained backing buffer, or NULL for raw-byte slices
};

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

static void hoo_byte_slice_dtor(void* obj) {
    auto* impl = static_cast<HooByteSliceHandleImpl*>(obj);
    if (impl->owner) {
        hoo_release(impl->owner);
    }
}

static void ensure_dtor_registered(void) {
    static int registered = 0;
    if (!registered) {
        hoo_register_destructor(HOO_TYPE_BYTE_SLICE, hoo_byte_slice_dtor);
        registered = 1;
    }
}

static int is_byte_slice_handle(HooByteSliceHandle handle) {
    return handle && hoo_is_managed_object(handle) &&
           hoo_get_type_id(handle) == HOO_TYPE_BYTE_SLICE;
}

HooByteSliceHandle hoo_byte_slice_from_bytes_handle(const uint8_t* data, int64_t length) {
    HooByteSlice slice = hoo_byte_slice_from_bytes(data, length);
    if (!hoo_byte_slice_is_valid(slice)) return nullptr;
    ensure_dtor_registered();
    auto* impl = static_cast<HooByteSliceHandleImpl*>(hoo_alloc(sizeof(HooByteSliceHandleImpl), HOO_TYPE_BYTE_SLICE));
    if (!impl) return nullptr;
    impl->view = slice;
    impl->owner = nullptr;
    return impl;
}

HooByteSliceHandle hoo_byte_slice_from_buffer_handle(HooBuffer buffer) {
    if (!buffer) return nullptr;
    HooByteSlice slice = hoo_byte_slice_from_buffer(buffer);
    if (!hoo_byte_slice_is_valid(slice)) return nullptr;
    ensure_dtor_registered();
    auto* impl = static_cast<HooByteSliceHandleImpl*>(hoo_alloc(sizeof(HooByteSliceHandleImpl), HOO_TYPE_BYTE_SLICE));
    if (!impl) return nullptr;
    impl->view = slice;
    impl->owner = buffer;
    hoo_retain(buffer);
    return impl;
}

HooByteSlice hoo_byte_slice_view(HooByteSliceHandle handle) {
    if (!is_byte_slice_handle(handle)) return hoo_byte_slice_empty();
    return static_cast<const HooByteSliceHandleImpl*>(handle)->view;
}

void hoo_byte_slice_release(HooByteSliceHandle handle) {
    if (!is_byte_slice_handle(handle)) return;
    hoo_release(handle);
}

}
