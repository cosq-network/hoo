#pragma once

#include <stdint.h>
#include "hoo_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooUUID - UUID Functions
// ============================================================================

typedef void* HooUUID;

// ============================================================================
// Creation
// ============================================================================

/**
 * Generate a random UUID v4
 * @return A new HooUUID (retained), or NULL on failure
 */
HooUUID hoo_uuid_v4(void);

/**
 * Create a nil UUID (00000000-0000-0000-0000-000000000000)
 * @return A new HooUUID (retained)
 */
HooUUID hoo_uuid_nil(void);

/**
 * Parse a UUID from its canonical string representation ("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")
 * @param str Input string
 * @return A new HooUUID (retained), or NULL on parse failure
 */
HooUUID hoo_uuid_from_string(const char* str);

/**
 * Create a UUID from 16 raw bytes
 * @param bytes Pointer to 16 bytes
 * @return A new HooUUID (retained), or NULL on failure
 */
HooUUID hoo_uuid_from_bytes(const uint8_t* bytes);

HooUUID hoo_uuid_from_bytes_buffer(HooBuffer buf);
HooBuffer hoo_uuid_to_bytes_buffer(HooUUID uuid);

// ============================================================================
// Conversion
// ============================================================================

/**
 * Convert a UUID to its canonical string representation
 * @param uuid UUID handle
 * @return Allocated string (caller must free with hoo_uuid_free_string), or NULL on failure
 */
char* hoo_uuid_to_string(HooUUID uuid);

/**
 * Write the 16 raw bytes of a UUID into a buffer
 * @param uuid UUID handle
 * @param out_16 Output buffer (must be at least 16 bytes)
 * @return 1 on success, 0 on failure
 */
int64_t hoo_uuid_to_bytes(HooUUID uuid, uint8_t* out_16);

// ============================================================================
// Properties
// ============================================================================

/**
 * Check if a UUID is the nil UUID
 * @param uuid UUID handle
 * @return 1 if nil, 0 otherwise
 */
int64_t hoo_uuid_is_nil(HooUUID uuid);

/**
 * Compare two UUIDs for equality
 * @param a First UUID handle
 * @param b Second UUID handle
 * @return 1 if equal, 0 otherwise
 */
int64_t hoo_uuid_equals(HooUUID a, HooUUID b);

/**
 * Lexicographically compare two UUIDs
 * @param a First UUID handle
 * @param b Second UUID handle
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int64_t hoo_uuid_compare(HooUUID a, HooUUID b);

// ============================================================================
// Reference Counting
// ============================================================================

/**
 * Retain (increment reference count of) a UUID
 * @param uuid UUID handle
 * @return The same UUID handle
 */
HooUUID hoo_uuid_retain(HooUUID uuid);

/**
 * Release (decrement reference count of) a UUID
 * If the reference count reaches zero, the UUID is freed.
 * @param uuid UUID handle
 */
void hoo_uuid_release(HooUUID uuid);

// ============================================================================
// Memory Management
// ============================================================================

/**
 * Free a string allocated by hoo_uuid_to_string
 * @param str String to free
 */
void hoo_uuid_free_string(char* str);

#ifdef __cplusplus
}
#endif
