#ifndef HOO_IO_H
#define HOO_IO_H

#include <stdint.h>
#include <stddef.h>
#include "runtime/lib/character/hoo_character.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hooc I/O Runtime Library
 *
 * Provides basic input/output functions for the Hooc language.
 * These functions can be called from JIT-compiled code as if they
 * were module-level functions from the "hoo" module.
 */

/**
 * Print a string to stdout.
 *
 * @param str Pointer to HooString to print (can be NULL - prints "null")
 */
void hoo_print(void* str);

/**
 * Print a string followed by newline to stdout.
 *
 * @param str Pointer to HooString to print (can be NULL - prints "null\n")
 */
void hoo_println(void* str);

/**
 * Read a line of text from stdin.
 * Reads until newline or EOF, returns as HooString.
 *
 * @return Pointer to HooString containing the line (must be released with hoo_release)
 *         Returns empty string if EOF is reached immediately.
 */
void* hoo_readline(void);

/**
 * Read a single character from stdin (non-blocking).
 * Returns NULL immediately if no character is available.
 *
 * @return HooCharacter instance for the character, or NULL if no input is available or on EOF
 */
HooCharacter hoo_readchar(void);

#ifdef __cplusplus
}
#endif

#endif // HOO_IO_H