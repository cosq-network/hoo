#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooFileHandle - Opaque File Handle
// ============================================================================

typedef void* HooFileHandle;

// ============================================================================
// File Operations
// ============================================================================

/**
 * Check if a path exists in the filesystem.
 *
 * @param path Null-terminated C string path
 * @return 1 if exists, 0 if not found or error
 */
int64_t hoo_fs_exists(const char* path);

/**
 * Check if path points to a regular file.
 *
 * @param path Null-terminated C string path
 * @return 1 if regular file, 0 otherwise
 */
int64_t hoo_fs_is_file(const char* path);

/**
 * Check if path points to a directory.
 *
 * @param path Null-terminated C string path
 * @return 1 if directory, 0 otherwise
 */
int64_t hoo_fs_is_dir(const char* path);

/**
 * Get the size of a file in bytes.
 *
 * @param path Null-terminated C string path
 * @return File size in bytes, or -1 on error
 */
int64_t hoo_fs_size(const char* path);

/**
 * Get the last modification time of a file as a Unix timestamp.
 *
 * @param path Null-terminated C string path
 * @return Unix timestamp (seconds since epoch), or -1 on error
 */
int64_t hoo_fs_last_modified(const char* path);

/**
 * Delete a file from the filesystem.
 *
 * @param path Null-terminated C string path
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_delete(const char* path);

/**
 * Rename (move) a file or directory.
 *
 * @param old_path Current path
 * @param new_path Destination path
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_rename(const char* old_path, const char* new_path);

/**
 * Copy a file from source to destination.
 *
 * @param src Source path
 * @param dst Destination path
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_copy(const char* src, const char* dst);

// ============================================================================
// Read/Write Text Files
// ============================================================================

/**
 * Read the entire contents of a text file.
 *
 * The returned string is dynamically allocated and must be freed
 * with hoo_fs_free_string when no longer needed.
 *
 * @param path Null-terminated C string path
 * @return Allocated C string with file contents, or NULL on failure
 */
char* hoo_fs_read_text(const char* path);

/**
 * Write a string to a text file, overwriting existing content.
 *
 * Creates the file if it does not exist.
 *
 * @param path Null-terminated C string path
 * @param content Null-terminated C string to write
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_write_text(const char* path, const char* content);

/**
 * Append a string to the end of a text file.
 *
 * Creates the file if it does not exist.
 *
 * @param path Null-terminated C string path
 * @param content Null-terminated C string to append
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_append_text(const char* path, const char* content);

// ============================================================================
// Read/Write Binary Files
// ============================================================================

/**
 * Read the entire contents of a binary file.
 *
 * The output buffer is dynamically allocated and must be freed
 * with hoo_free_string when no longer needed.
 *
 * @param path    Null-terminated C string path
 * @param out_data Pointer to receive allocated buffer (caller must free)
 * @param out_len  Pointer to receive number of bytes read
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_read_bytes(const char* path, uint8_t** out_data, int64_t* out_len);

/**
 * Write raw bytes to a binary file, overwriting existing content.
 *
 * @param path Null-terminated C string path
 * @param data Pointer to data buffer
 * @param len  Number of bytes to write
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_write_bytes(const char* path, const uint8_t* data, int64_t len);

// ============================================================================
// Directory Operations
// ============================================================================

/**
 * Create a single directory.
 *
 * The parent directory must already exist.
 *
 * @param path Null-terminated C string path
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_mkdir(const char* path);

/**
 * Create a directory and all missing parent directories (mkdir -p).
 *
 * @param path Null-terminated C string path
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_mkdirs(const char* path);

/**
 * Remove an empty directory.
 *
 * @param path Null-terminated C string path
 * @return 1 on success, 0 on failure
 */
int64_t hoo_fs_rmdir(const char* path);

/**
 * List the contents of a directory.
 *
 * Returns an array of dynamically allocated C strings. The array and all
 * strings must be freed with hoo_fs_free_list when no longer needed.
 *
 * @param path      Null-terminated C string path
 * @param out_count  Pointer to receive number of entries
 * @return Array of C strings, or NULL on failure
 */
char** hoo_fs_list_dir(const char* path, int64_t* out_count);

/**
 * Free a directory listing returned by hoo_fs_list_dir.
 *
 * @param list  Array of strings to free
 * @param count Number of entries in the array
 */
void hoo_fs_free_list(char** list, int64_t count);

// ============================================================================
// Temp Files
// ============================================================================

/**
 * Get the system's temporary directory path.
 *
 * The returned string is dynamically allocated and must be freed
 * with hoo_fs_free_string when no longer needed.
 *
 * @return Allocated C string with temp directory path, or NULL on failure
 */
char* hoo_fs_temp_dir(void);

/**
 * Create a temporary file with the given prefix.
 *
 * The file is created in the system's temporary directory. The returned
 * path string is dynamically allocated and must be freed with
 * hoo_fs_free_string when no longer needed.
 *
 * @param prefix Null-terminated C string prefix for the filename
 * @return Allocated C string with the path to the new temp file, or NULL on failure
 */
char* hoo_fs_create_temp_file(const char* prefix);

// ============================================================================
// Memory Management
// ============================================================================

/**
 * Free a string allocated by any hoo_fs_* function.
 *
 * @param str Pointer to string to free (may be NULL)
 */
void hoo_fs_free_string(char* str);

#ifdef __cplusplus
}
#endif
