#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Component Extraction
// ============================================================================

/**
 * Get the directory portion of a path.
 *
 * Returns the parent directory path. For a file at "a/b/c.txt",
 * returns "a/b". The returned string is dynamically allocated and must
 * be freed with hoo_path_free_string when no longer needed.
 *
 * @param path Null-terminated C string path
 * @return Allocated C string with the directory portion, or NULL on failure
 */
char* hoo_path_dirname(const char* path);

/**
 * Get the filename portion of a path.
 *
 * Returns the last component of the path. For "a/b/c.txt",
 * returns "c.txt". The returned string is dynamically allocated and must
 * be freed with hoo_path_free_string when no longer needed.
 *
 * @param path Null-terminated C string path
 * @return Allocated C string with the filename, or NULL on failure
 */
char* hoo_path_basename(const char* path);

/**
 * Get the file extension from a path.
 *
 * Returns the extension including the leading dot. For "archive.tar.gz",
 * returns ".gz". Returns an empty string if there is no extension.
 * The returned string is dynamically allocated and must be freed with
 * hoo_path_free_string when no longer needed.
 *
 * @param path Null-terminated C string path
 * @return Allocated C string with the extension, or NULL on failure
 */
char* hoo_path_extension(const char* path);

/**
 * Get the filename without its extension.
 *
 * For "a/b/resume.pdf" returns "resume". For ".hidden" returns
 * ".hidden". The returned string is dynamically allocated and must
 * be freed with hoo_path_free_string when no longer needed.
 *
 * @param path Null-terminated C string path
 * @return Allocated C string with the stem, or NULL on failure
 */
char* hoo_path_stem(const char* path);

/**
 * Get the root component of a path.
 *
 * For Unix returns "/" for absolute paths or "" for relative paths.
 * For Windows returns "C:\" for drive-absolute paths, "\\server\share\"
 * for UNC paths, or "" otherwise.
 * The returned string is dynamically allocated and must be freed with
 * hoo_path_free_string when no longer needed.
 *
 * @param path Null-terminated C string path
 * @return Allocated C string with the root component, or NULL on failure
 */
char* hoo_path_root(const char* path);

// ============================================================================
// Construction
// ============================================================================

/**
 * Join two path components with the platform separator.
 *
 * Handles edge cases: empty components, trailing/leading separators,
 * and absolute paths in the second argument.
 * The returned string is dynamically allocated and must be freed with
 * hoo_path_free_string when no longer needed.
 *
 * @param a First path component
 * @param b Second path component
 * @return Allocated C string with the joined path, or NULL on failure
 */
char* hoo_path_join(const char* a, const char* b);

/**
 * Join multiple path components with the platform separator.
 *
 * Behaves like repeated calls to hoo_path_join. All parts are joined
 * in order. The returned string is dynamically allocated and must be
 * freed with hoo_path_free_string when no longer needed.
 *
 * @param parts Array of C strings to join
 * @param count Number of elements in parts
 * @return Allocated C string with the joined path, or NULL on failure
 */
char* hoo_path_join_multi(const char** parts, int64_t count);

// ============================================================================
// Normalization
// ============================================================================

/**
 * Normalize a path, resolving "." and ".." components.
 *
 * Collapses redundant separators and resolves parent directory
 * references. Does not resolve symlinks. The returned string is
 * dynamically allocated and must be freed with hoo_path_free_string
 * when no longer needed.
 *
 * @param path Null-terminated C string path
 * @return Allocated C string with the normalized path, or NULL on failure
 */
char* hoo_path_normalize(const char* path);

/**
 * Convert a path to an absolute path.
 *
 * Resolves the path relative to the current working directory if it
 * is relative. The returned string is dynamically allocated and must
 * be freed with hoo_path_free_string when no longer needed.
 *
 * @param path Null-terminated C string path
 * @return Allocated C string with the absolute path, or NULL on failure
 */
char* hoo_path_absolute(const char* path);

/**
 * Compute a relative path from base to path.
 *
 * Returns the relative path that, when joined to base, yields path.
 * The returned string is dynamically allocated and must be freed with
 * hoo_path_free_string when no longer needed.
 *
 * @param path Target absolute path
 * @param base Base absolute path
 * @return Allocated C string with the relative path, or NULL on failure
 */
char* hoo_path_relative(const char* path, const char* base);

// ============================================================================
// Properties
// ============================================================================

/**
 * Check if a path is absolute.
 *
 * @param path Null-terminated C string path
 * @return 1 if absolute, 0 otherwise
 */
int64_t hoo_path_is_absolute(const char* path);

/**
 * Check if a path is relative.
 *
 * @param path Null-terminated C string path
 * @return 1 if relative, 0 otherwise
 */
int64_t hoo_path_is_relative(const char* path);

/**
 * Check if a path has a file extension.
 *
 * @param path Null-terminated C string path
 * @return 1 if the path has an extension, 0 otherwise
 */
int64_t hoo_path_has_extension(const char* path);

/**
 * Check if a path has a root component.
 *
 * @param path Null-terminated C string path
 * @return 1 if the path has a root, 0 otherwise
 */
int64_t hoo_path_has_root(const char* path);

// ============================================================================
// Split
// ============================================================================

/**
 * Split a path into its individual components.
 *
 * For "a/b/c.txt" returns ["a", "b", "c.txt"]. Root components
 * (e.g. "/", "C:\") are included. The returned array and all strings
 * are dynamically allocated and must be freed with hoo_path_free_parts
 * when no longer needed.
 *
 * @param path     Null-terminated C string path
 * @param out_count Pointer to receive the number of components
 * @return Array of C strings, or NULL on failure
 */
char** hoo_path_split(const char* path, int64_t* out_count);

/**
 * Free a parts array returned by hoo_path_split.
 *
 * @param parts Array of strings to free
 * @param count Number of elements in the array
 */
void hoo_path_free_parts(char** parts, int64_t count);

// ============================================================================
// Platform-Specific
// ============================================================================

/**
 * Get the platform-specific path separator character.
 *
 * @return '/' on Unix, '\\' on Windows
 */
char hoo_path_separator(void);

/**
 * Get the platform-specific path list separator character.
 *
 * @return ':' on Unix, ';' on Windows
 */
char hoo_path_list_separator(void);

// ============================================================================
// Memory Management
// ============================================================================

/**
 * Free a string allocated by any hoo_path_* function.
 *
 * @param str Pointer to string to free (may be NULL)
 */
void hoo_path_free_string(char* str);

#ifdef __cplusplus
}
#endif
