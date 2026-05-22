#include "hoo_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>

/**
 * Object Header Layout (hidden from Hooc code):
 *
 * +-------------------+  <- header pointer
 * | refcount (int64)  |  8 bytes
 * +-------------------+
 * | type_id (int64)   |  8 bytes
 * +-------------------+  <- returned pointer (object data starts here)
 * | field1            |
 * | field2            |
 * | ...               |
 * +-------------------+
 */

typedef struct {
    _Atomic int64_t refcount;
    int64_t type_id;
} HooObjectHeader;

#define HEADER_SIZE sizeof(HooObjectHeader)

// Memory statistics (for debugging and testing)
static struct {
    int64_t total_allocations;
    int64_t total_deallocations;
    int64_t current_live_objects;
    int64_t total_bytes_allocated;
} memory_stats = {0LL, 0LL, 0LL, 0LL};

// Helper: Get header from object pointer
static inline HooObjectHeader* get_header(void* obj) {
    return (HooObjectHeader*)((char*)obj - HEADER_SIZE);
}

void* hoo_alloc(size_t size, int64_t type_id) {
    // Allocate header + object data
    HooObjectHeader* header = (HooObjectHeader*)malloc(HEADER_SIZE + size);

    if (!header) {
        fprintf(stderr, "FATAL: Out of memory (tried to allocate %zu bytes)\n", size);
        exit(1);
    }

    // Initialize header
    header->refcount = 1;  // Start with refcount = 1 (caller owns it)
    header->type_id = type_id;

    // Zero-initialize object data for safety
    memset((char*)header + HEADER_SIZE, 0, size);

    // Update statistics
    memory_stats.total_allocations++;
    memory_stats.current_live_objects++;
    memory_stats.total_bytes_allocated += (int64_t)(HEADER_SIZE + size);

    // Return pointer to object data (skip header)
    void* obj = (char*)header + HEADER_SIZE;

#ifdef HOO_DEBUG_MEMORY
    fprintf(stderr, "[ALLOC] obj=%p type=%lld size=%zu refcount=1\n",
            obj, (long long)type_id, size);
#endif

    return obj;
}

void* hoo_retain(void* obj) {
    if (!obj) {
        return NULL;
    }

    HooObjectHeader* header = get_header(obj);

    atomic_fetch_add_explicit(&header->refcount, 1, memory_order_relaxed);

#ifdef HOO_DEBUG_MEMORY
    fprintf(stderr, "[RETAIN] obj=%p type=%lld refcount=%lld\n",
            obj, (long long)header->type_id, (long long)atomic_load(&header->refcount));
#endif

    return obj;
}

void hoo_release(void* obj) {
    if (!obj) {
        return;
    }

    HooObjectHeader* header = get_header(obj);

    int64_t old_count = atomic_fetch_sub_explicit(&header->refcount, 1, memory_order_release);

#ifdef HOO_DEBUG_MEMORY
    fprintf(stderr, "[RELEASE] obj=%p type=%lld refcount=%lld\n",
            obj, (long long)header->type_id, (long long)old_count - 1);
#endif

    if (old_count <= 0) {
        fprintf(stderr, "FATAL: Double-release or negative refcount for object %p (type %lld)\n",
                obj, (long long)header->type_id);
        exit(1);
    }

    if (old_count == 1) {
#ifdef HOO_DEBUG_MEMORY
        fprintf(stderr, "[FREE] obj=%p type=%lld\n",
                obj, (long long)header->type_id);
#endif

        atomic_thread_fence(memory_order_acquire);

        memory_stats.total_deallocations++;
        memory_stats.current_live_objects--;

        free(header);
    }
}

int64_t hoo_get_refcount(void* obj) {
    if (!obj) {
        return 0;
    }

    HooObjectHeader* header = get_header(obj);
    return atomic_load(&header->refcount);
}

int64_t hoo_get_type_id(void* obj) {
    if (!obj) {
        fprintf(stderr, "ERROR: hoo_get_type_id called with NULL\n");
        return -1;
    }

    HooObjectHeader* header = get_header(obj);
    return header->type_id;
}

void hoo_print_memory_stats(void) {
    fprintf(stderr, "\n=== Hooc Memory Statistics ===\n");
    fprintf(stderr, "Total allocations:     %lld\n", (long long)memory_stats.total_allocations);
    fprintf(stderr, "Total deallocations:   %lld\n", (long long)memory_stats.total_deallocations);
    fprintf(stderr, "Current live objects:  %lld\n", (long long)memory_stats.current_live_objects);
    fprintf(stderr, "Total bytes allocated: %lld\n", (long long)memory_stats.total_bytes_allocated);

    if (memory_stats.current_live_objects > 0) {
        fprintf(stderr, "\nWARNING: %lld objects still alive (potential memory leak!)\n",
                (long long)memory_stats.current_live_objects);
    } else {
        fprintf(stderr, "\nAll objects properly deallocated. No leaks detected.\n");
    }
    fprintf(stderr, "==============================\n\n");
}

void hoo_reset_memory_stats(void) {
    memory_stats.total_allocations = 0;
    memory_stats.total_deallocations = 0;
    memory_stats.current_live_objects = 0;
    memory_stats.total_bytes_allocated = 0;
}
