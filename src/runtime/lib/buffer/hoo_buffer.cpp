#include "runtime/lib/buffer/hoo_buffer.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include <cstring>
#include <cstdlib>
#include <limits>

extern "C" {

// ── Internal layout ─────────────────────────────────────────────────────────
// Memory: [ARC header 16B] [BufferImpl: length(8) + capacity(8) + data(8)]
// The byte storage lives out-of-line on the heap, so growing the buffer never
// runs hoo_realloc over the ARC header (which would lose refcount state) and
// never invalidates the handle.  Matches the HooString pattern otherwise.

struct BufferImpl {
    int64_t length;
    int64_t capacity; // allocated byte capacity of data
    uint8_t* data;
};

static const size_t BUFFER_METADATA_SIZE = sizeof(BufferImpl);

static BufferImpl* to_impl(HooBuffer buf) {
    return (BufferImpl*)buf;
}

static HooBuffer from_impl(BufferImpl* impl) {
    return (HooBuffer)impl;
}

static int64_t max_i64(int64_t a, int64_t b) { return a > b ? a : b; }

static void buffer_destructor(void* obj) {
    BufferImpl* impl = (BufferImpl*)obj;
    if (impl) {
        free(impl->data);
        impl->data = nullptr;
    }
}

// ── Creation / Destruction ──────────────────────────────────────────────────

HooBuffer hoo_buffer_new(int64_t initial_capacity) {
    if (initial_capacity < 0) initial_capacity = 0;
    if (initial_capacity == 0) initial_capacity = 64;
    BufferImpl* impl = (BufferImpl*)hoo_alloc(sizeof(BufferImpl), HOO_TYPE_BUFFER);
    if (!impl) return nullptr;
    impl->data = (uint8_t*)malloc((size_t)initial_capacity);
    if (!impl->data) {
        hoo_release(impl);
        return nullptr;
    }
    impl->length = 0;
    impl->capacity = initial_capacity;
    return from_impl(impl);
}

HooBuffer hoo_buffer_from_bytes(const uint8_t* data, int64_t length) {
    if (length < 0) return nullptr;
    if (length == 0 || !data) return hoo_buffer_new(0);
    BufferImpl* impl = (BufferImpl*)hoo_alloc(sizeof(BufferImpl), HOO_TYPE_BUFFER);
    if (!impl) return nullptr;
    impl->data = (uint8_t*)malloc((size_t)length);
    if (!impl->data) {
        hoo_release(impl);
        return nullptr;
    }
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
    uint8_t* data = impl->data;
    int64_t old = (int64_t)(unsigned char)data[index];
    data[index] = (uint8_t)(byte_val & 0xFF);
    return old;
}

int64_t hoo_buffer_write_byte(HooBuffer buf, int64_t byte_val) {
    if (!buf) return -1;
    uint8_t b = (uint8_t)(byte_val & 0xFF);
    hoo_buffer_append(buf, &b, 1);
    return 0;
}

int64_t hoo_buffer_write(HooBuffer buf, HooString str) {
    if (!buf) return -1;
    if (!str) return 0;
    hoo_buffer_append(buf, reinterpret_cast<const uint8_t*>(hoo_string_data(str)),
                      hoo_string_length(str));
    return 0;
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
        uint8_t* new_data = (uint8_t*)realloc(impl->data, (size_t)new_cap);
        if (!new_data) return buf;
        impl->data = new_data;
        impl->capacity = new_cap;
    }
    // memmove (not memcpy) so self-append with sufficient capacity stays valid.
    std::memmove(impl->data + impl->length, data, (size_t)length);
    impl->length = needed;
    return buf;
}

HooBuffer hoo_buffer_append_buffer(HooBuffer buf, HooBuffer other) {
    if (!buf || !other) return buf;
    BufferImpl* impl = to_impl(buf);
    BufferImpl* other_impl = to_impl(other);
    if (other_impl->length == 0) return buf;
    if (impl == other_impl) {
        // Self-append: the source lives in storage that append may realloc, so
        // snapshot it first.
        size_t cap = (size_t)other_impl->length;
        uint8_t* snapshot = (uint8_t*)malloc(cap ? cap : 1);
        if (!snapshot) return buf;
        std::memcpy(snapshot, other_impl->data, (size_t)other_impl->length);
        HooBuffer result = hoo_buffer_append(buf, snapshot, other_impl->length);
        free(snapshot);
        return result;
    }
    return hoo_buffer_append(buf, other_impl->data, other_impl->length);
}

int64_t hoo_buffer_clear(HooBuffer buf) {
    if (!buf) return -1;
    to_impl(buf)->length = 0;
    return 0;
}

// ── Conversion ───────────────────────────────────────────────────────────────

HooString hoo_buffer_to_string(HooBuffer buf) {
    if (!buf) return hoo_string_new();
    BufferImpl* impl = to_impl(buf);
    return hoo_string_from_bytes(reinterpret_cast<const char*>(impl->data), impl->length);
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

namespace {
struct BufferDestructorRegistrar {
    BufferDestructorRegistrar() {
        hoo_register_destructor(HOO_TYPE_BUFFER, buffer_destructor);
    }
} buffer_destructor_registrar;
}
