#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooBuffer;
typedef void* HooString;

// ── Creation / Destruction ──────────────────────────────────────────────────

HooBuffer hoo_buffer_new(int64_t initial_capacity);
HooBuffer hoo_buffer_from_bytes(const uint8_t* data, int64_t length);
HooBuffer hoo_buffer_copy(HooBuffer buf);
void      hoo_buffer_release(HooBuffer buf);
HooBuffer hoo_buffer_retain(HooBuffer buf);

// ── Properties ──────────────────────────────────────────────────────────────

int64_t       hoo_buffer_length(HooBuffer buf);
int64_t       hoo_buffer_capacity(HooBuffer buf);
const uint8_t* hoo_buffer_data(HooBuffer buf);

// ── Conversion ───────────────────────────────────────────────────────────────

HooString hoo_buffer_to_string(HooBuffer buf);

// ── Read / Write ────────────────────────────────────────────────────────────

int64_t  hoo_buffer_byte_at(HooBuffer buf, int64_t index);
int64_t  hoo_buffer_set_byte(HooBuffer buf, int64_t index, int64_t byte_val);
int64_t  hoo_buffer_write_byte(HooBuffer buf, int64_t byte_val);
int64_t  hoo_buffer_write(HooBuffer buf, HooString str);
HooBuffer hoo_buffer_append(HooBuffer buf, const uint8_t* data, int64_t length);
HooBuffer hoo_buffer_append_buffer(HooBuffer buf, HooBuffer other);
int64_t  hoo_buffer_clear(HooBuffer buf);

// ── Slice ───────────────────────────────────────────────────────────────────

HooBuffer hoo_buffer_slice(HooBuffer buf, int64_t start, int64_t end);

#ifdef __cplusplus
}
#endif
