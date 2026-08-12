#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooCharacter - UTF-8 Character Type with Reference Counting
// ============================================================================
//
// Represents a single Unicode scalar value encoded as UTF-8.
// Internally managed with automatic reference counting.
//

typedef void* HooCharacter;

/**
 * Create a Character from a UTF-8 byte sequence.
 * 
 * @param bytes Pointer to UTF-8 data (must be at least 1-4 bytes depending on sequence)
 * @param length Number of bytes in the UTF-8 sequence
 * @return New HooCharacter with refcount=1, or NULL on failure
 */
HooCharacter hoo_character_from_utf8(const char* bytes, int64_t length);

/**
 * Create a Character from a Unicode codepoint.
 * 
 * @param codepoint Unicode scalar value
 * @return New HooCharacter with refcount=1
 */
HooCharacter hoo_character_from_codepoint(int64_t codepoint);

/**
 * Get the UTF-8 byte length of the character.
 * 
 * @param ch Character to measure
 * @return Length in bytes (1-4)
 */
int64_t hoo_character_length(HooCharacter ch);

/**
 * Get pointer to internal UTF-8 data.
 * 
 * @param ch Character to get data from
 * @return Pointer to UTF-8 data (null-terminated for safety)
 */
const char* hoo_character_data(HooCharacter ch);

/**
 * Get the Unicode codepoint of the character.
 * 
 * @param ch Character to query
 * @return Unicode scalar value
 */
int64_t hoo_character_codepoint(HooCharacter ch);

/**
 * Increment reference count.
 */
HooCharacter hoo_character_retain(HooCharacter ch);

/**
 * Decrement reference count.
 */
void hoo_character_release(HooCharacter ch);

/**
 * Get current reference count.
 */
int64_t hoo_character_refcount(HooCharacter ch);

/**
 * Print character to stdout.
 */
void hoo_character_print(HooCharacter ch);

#ifdef __cplusplus
}
#endif
