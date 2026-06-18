#include "hoo_buffer.h"
#include "hoo_runtime.h"
#include <cstring>
#include <cstdlib>

extern "C" {

// ── Internal layout ─────────────────────────────────────────────────────────
// Memory: [ARC header 16B] [BufferImpl: length(8) + capacity(8) + bytes...]
// HooBuffer handle points to BufferImpl (right after ARC header).
// This matches the HooString pattern (HooString → HooStringImpl).

struct BufferImpl {
    int64_t length;
    int64_t capacity;
    uint8_t data[1];
};

#define BUFFER_METADATA_SIZE offsetof(BufferImpl, data)  // 16 bytes

static BufferImpl* to_impl(HooBuffer buf) {
    return (BufferImpl*)buf;
}

static HooBuffer from_impl(BufferImpl* impl) {
    return (HooBuffer)impl;
}

static int64_t max_i64(int64_t a, int64_t b) { return a > b ? a : b; }

// ── Creation / Destruction ──────────────────────────────────────────────────

HooBuffer hoo_buffer_new(int64_t initial_capacity) {
    if (initial_capacity < 0) initial_capacity = 0;
    if (initial_capacity == 0) initial_capacity = 64;
    size_t alloc_size = BUFFER_METADATA_SIZE + (size_t)initial_capacity;
    BufferImpl* impl = (BufferImpl*)hoo_alloc(alloc_size, HOO_TYPE_BUFFER);
    if (!impl) return nullptr;
    impl->length = 0;
    impl->capacity = initial_capacity;
    return from_impl(impl);
}

HooBuffer hoo_buffer_from_bytes(const uint8_t* data, int64_t length) {
    if (length < 0) return nullptr;
    if (length == 0 || !data) return hoo_buffer_new(0);
    size_t alloc_size = BUFFER_METADATA_SIZE + (size_t)length;
    BufferImpl* impl = (BufferImpl*)hoo_alloc(alloc_size, HOO_TYPE_BUFFER);
    if (!impl) return nullptr;
    impl->length = length;
    impl->capacity = length;
    std::memcpy(impl->data, data, (size_t)length);
    return from_impl(impl);
}

HooBuffer hoo_buffer_copy(HooBuffer buf) {
    if (!buf) return nullptr;
    BufferImpl* impl = to_impl(buf);
    return hoo_buffer_from_bytes(impl->data, impl->length);
}

void hoo_buffer_release(HooBuffer buf) {
    if (!buf) return;
    hoo_release(buf);
}

HooBuffer hoo_buffer_retain(HooBuffer buf) {
    if (!buf) return nullptr;
    return (HooBuffer)hoo_retain(buf);
}

// ── Properties ──────────────────────────────────────────────────────────────

int64_t hoo_buffer_length(HooBuffer buf) {
    if (!buf) return 0;
    return to_impl(buf)->length;
}

int64_t hoo_buffer_capacity(HooBuffer buf) {
    if (!buf) return 0;
    return to_impl(buf)->capacity;
}

const uint8_t* hoo_buffer_data(HooBuffer buf) {
    if (!buf) return nullptr;
    return to_impl(buf)->data;
}

// ── Read / Write ────────────────────────────────────────────────────────────

int64_t hoo_buffer_byte_at(HooBuffer buf, int64_t index) {
    if (!buf) return -1;
    BufferImpl* impl = to_impl(buf);
    if (index < 0 || index >= impl->length) return -1;
    return (int64_t)(unsigned char)impl->data[index];
}

int64_t hoo_buffer_set_byte(HooBuffer buf, int64_t index, int64_t byte_val) {
    if (!buf) return -1;
    BufferImpl* impl = to_impl(buf);
    if (index < 0 || index >= impl->length) return -1;
    int64_t old = (int64_t)(unsigned char)impl->data[index];
    impl->data[index] = (uint8_t)(byte_val & 0xFF);
    return old;
}

HooBuffer hoo_buffer_append(HooBuffer buf, const uint8_t* data, int64_t length) {
    if (!buf || !data || length < 0) return buf;
    if (length == 0) return buf;
    BufferImpl* impl = to_impl(buf);
    int64_t needed = impl->length + length;
    if (needed > impl->capacity) {
        int64_t new_cap = max_i64(impl->capacity * 2, needed);
        if (new_cap < 64) new_cap = 64;
        size_t new_alloc = BUFFER_METADATA_SIZE + (size_t)new_cap;
        BufferImpl* new_impl = (BufferImpl*)hoo_realloc(impl, new_alloc);
        if (!new_impl) return buf;
        new_impl->capacity = new_cap;
        impl = new_impl;
    }
    std::memcpy(impl->data + impl->length, data, (size_t)length);
    impl->length = needed;
    return from_impl(impl);
}

HooBuffer hoo_buffer_append_buffer(HooBuffer buf, HooBuffer other) {
    if (!buf || !other) return buf;
    BufferImpl* other_impl = to_impl(other);
    return hoo_buffer_append(buf, other_impl->data, other_impl->length);
}

int64_t hoo_buffer_clear(HooBuffer buf) {
    if (!buf) return -1;
    to_impl(buf)->length = 0;
    return 0;
}

// ── Slice ───────────────────────────────────────────────────────────────────

HooBuffer hoo_buffer_slice(HooBuffer buf, int64_t start, int64_t end) {
    if (!buf) return nullptr;
    BufferImpl* impl = to_impl(buf);
    if (start < 0 || start > impl->length) return nullptr;
    if (end < start || end > impl->length) return nullptr;
    if (start == end) return hoo_buffer_new(0);
    return hoo_buffer_from_bytes(impl->data + start, end - start);
}

} // extern "C"
