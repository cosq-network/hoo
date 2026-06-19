#include "hoo_buffer.h"
#include "hoo_runtime.h"
#include <cstring>
#include <cstdlib>
#include <limits>

extern "C" {

// ── Internal layout ─────────────────────────────────────────────────────────
// Memory: [ARC header 16B] [BufferImpl: length(8) + capacity(8) + bytes...]
// HooBuffer handle points to BufferImpl (right after ARC header).
// This matches the HooString pattern (HooString → HooStringImpl).

struct BufferImpl {
    int64_t length;
    int64_t capacity;
};

static const size_t BUFFER_METADATA_SIZE = sizeof(BufferImpl);

static BufferImpl* to_impl(HooBuffer buf) {
    return (BufferImpl*)buf;
}

static HooBuffer from_impl(BufferImpl* impl) {
    return (HooBuffer)impl;
}

static int64_t max_i64(int64_t a, int64_t b) { return a > b ? a : b; }

static uint8_t* buffer_data_ptr(BufferImpl* impl) {
    return reinterpret_cast<uint8_t*>(impl) + BUFFER_METADATA_SIZE;
}

static const uint8_t* buffer_data_ptr_const(const BufferImpl* impl) {
    return reinterpret_cast<const uint8_t*>(impl) + BUFFER_METADATA_SIZE;
}

static bool checked_alloc_size(int64_t capacity, size_t* out_size) {
    if (capacity < 0) return false;
    if (capacity > std::numeric_limits<int64_t>::max() - static_cast<int64_t>(BUFFER_METADATA_SIZE)) {
        return false;
    }
    const uint64_t cap = static_cast<uint64_t>(capacity);
    if (cap > static_cast<uint64_t>(std::numeric_limits<size_t>::max() - BUFFER_METADATA_SIZE)) {
        return false;
    }
    *out_size = BUFFER_METADATA_SIZE + static_cast<size_t>(cap);
    return true;
}

// ── Creation / Destruction ──────────────────────────────────────────────────

HooBuffer hoo_buffer_new(int64_t initial_capacity) {
    if (initial_capacity < 0) initial_capacity = 0;
    if (initial_capacity == 0) initial_capacity = 64;
    size_t alloc_size = 0;
    if (!checked_alloc_size(initial_capacity, &alloc_size)) return nullptr;
    BufferImpl* impl = (BufferImpl*)hoo_alloc(alloc_size, HOO_TYPE_BUFFER);
    if (!impl) return nullptr;
    impl->length = 0;
    impl->capacity = initial_capacity;
    return from_impl(impl);
}

HooBuffer hoo_buffer_from_bytes(const uint8_t* data, int64_t length) {
    if (length < 0) return nullptr;
    if (length == 0 || !data) return hoo_buffer_new(0);
    size_t alloc_size = 0;
    if (!checked_alloc_size(length, &alloc_size)) return nullptr;
    BufferImpl* impl = (BufferImpl*)hoo_alloc(alloc_size, HOO_TYPE_BUFFER);
    if (!impl) return nullptr;
    impl->length = length;
    impl->capacity = length;
    std::memcpy(buffer_data_ptr(impl), data, (size_t)length);
    return from_impl(impl);
}

HooBuffer hoo_buffer_copy(HooBuffer buf) {
    if (!buf) return nullptr;
    BufferImpl* impl = to_impl(buf);
    return hoo_buffer_from_bytes(buffer_data_ptr(impl), impl->length);
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
    return buffer_data_ptr(to_impl(buf));
}

// ── Read / Write ────────────────────────────────────────────────────────────

int64_t hoo_buffer_byte_at(HooBuffer buf, int64_t index) {
    if (!buf) return -1;
    BufferImpl* impl = to_impl(buf);
    if (index < 0 || index >= impl->length) return -1;
    return (int64_t)(unsigned char)buffer_data_ptr(impl)[index];
}

int64_t hoo_buffer_set_byte(HooBuffer buf, int64_t index, int64_t byte_val) {
    if (!buf) return -1;
    BufferImpl* impl = to_impl(buf);
    if (index < 0 || index >= impl->length) return -1;
    uint8_t* data = buffer_data_ptr(impl);
    int64_t old = (int64_t)(unsigned char)data[index];
    data[index] = (uint8_t)(byte_val & 0xFF);
    return old;
}

HooBuffer hoo_buffer_append(HooBuffer buf, const uint8_t* data, int64_t length) {
    if (!buf || !data || length < 0) return buf;
    if (length == 0) return buf;
    BufferImpl* impl = to_impl(buf);
    if (impl->length > std::numeric_limits<int64_t>::max() - length) return buf;
    int64_t needed = impl->length + length;
    if (needed > impl->capacity) {
        int64_t doubled = impl->capacity;
        if (doubled > std::numeric_limits<int64_t>::max() / 2) {
            doubled = std::numeric_limits<int64_t>::max();
        } else {
            doubled *= 2;
        }
        int64_t new_cap = max_i64(doubled, needed);
        if (new_cap < 64) new_cap = 64;
        size_t new_alloc = 0;
        if (!checked_alloc_size(new_cap, &new_alloc)) return buf;
        BufferImpl* new_impl = (BufferImpl*)hoo_realloc(impl, new_alloc);
        if (!new_impl) return buf;
        new_impl->capacity = new_cap;
        impl = new_impl;
    }
    std::memcpy(buffer_data_ptr(impl) + impl->length, data, (size_t)length);
    impl->length = needed;
    return from_impl(impl);
}

HooBuffer hoo_buffer_append_buffer(HooBuffer buf, HooBuffer other) {
    if (!buf || !other) return buf;
    BufferImpl* other_impl = to_impl(other);
    return hoo_buffer_append(buf, buffer_data_ptr(other_impl), other_impl->length);
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
    return hoo_buffer_from_bytes(buffer_data_ptr(impl) + start, end - start);
}

} // extern "C"
