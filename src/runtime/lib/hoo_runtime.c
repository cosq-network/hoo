#include "hoo_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
typedef SRWLOCK hoo_mutex_t;
#define HOO_MUTEX_INIT SRWLOCK_INIT
static void hoo_mutex_lock(hoo_mutex_t* m) { AcquireSRWLockExclusive(m); }
static void hoo_mutex_unlock(hoo_mutex_t* m) { ReleaseSRWLockExclusive(m); }
#else
#include <pthread.h>
typedef pthread_mutex_t hoo_mutex_t;
#define HOO_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
static void hoo_mutex_lock(hoo_mutex_t* m) { pthread_mutex_lock(m); }
static void hoo_mutex_unlock(hoo_mutex_t* m) { pthread_mutex_unlock(m); }
#endif

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

#define HOO_TLAB_BLOCK_SIZE (64U * 1024U)
#define HOO_TLAB_MAX_OBJECT_SIZE 2048U

typedef struct TLABBlock {
    struct TLABBlock* next;
    size_t used;
    size_t capacity;
    unsigned char data[];
} TLABBlock;

typedef struct {
    TLABBlock* head;
} ThreadTLAB;

static _Thread_local ThreadTLAB g_thread_tlab = {NULL};

// Memory statistics (for debugging and testing)
static struct {
    int64_t total_allocations;
    int64_t total_deallocations;
    int64_t current_live_objects;
    int64_t total_bytes_allocated;
} memory_stats = {0LL, 0LL, 0LL, 0LL};

static struct {
    _Atomic int64_t hits;
    _Atomic int64_t misses;
    _Atomic int64_t blocks_allocated;
} tlab_stats = {0};

typedef struct {
    _Atomic int64_t refcount;
    int64_t type_id;
    int64_t capacity;
    int64_t reserved; // Padding for 32-byte alignment
} HooObjectHeader;

typedef struct TLABObjNode {
    void* obj;
    struct TLABObjNode* next;
} TLABObjNode;

static _Thread_local TLABObjNode* g_tlab_objects = NULL;

#define HOO_DESTRUCTOR_TABLE_SIZE 256

static HooDestructor g_destructors[HOO_DESTRUCTOR_TABLE_SIZE] = {NULL};

void hoo_register_destructor(int64_t type_id, HooDestructor dtor) {
    if (type_id >= 0 && type_id < HOO_DESTRUCTOR_TABLE_SIZE) {
        g_destructors[type_id] = dtor;
    }
}

typedef struct ManagedObjNode {
    void* obj;
    struct ManagedObjNode* next;
} ManagedObjNode;

static hoo_mutex_t g_managed_objects_mutex = HOO_MUTEX_INIT;
static ManagedObjNode* g_managed_objects = NULL;

static void managed_register(void* obj) {
    ManagedObjNode* node = (ManagedObjNode*)malloc(sizeof(ManagedObjNode));
    if (!node) {
        fprintf(stderr, "FATAL: Out of memory while tracking managed allocation\n");
        exit(1);
    }
    node->obj = obj;
    hoo_mutex_lock(&g_managed_objects_mutex);
    node->next = g_managed_objects;
    g_managed_objects = node;
    hoo_mutex_unlock(&g_managed_objects_mutex);
}

static void managed_unregister(void* obj) {
    hoo_mutex_lock(&g_managed_objects_mutex);
    ManagedObjNode* prev = NULL;
    ManagedObjNode* it = g_managed_objects;
    while (it) {
        if (it->obj == obj) {
            if (prev) {
                prev->next = it->next;
            } else {
                g_managed_objects = it->next;
            }
            hoo_mutex_unlock(&g_managed_objects_mutex);
            free(it);
            return;
        }
        prev = it;
        it = it->next;
    }
    hoo_mutex_unlock(&g_managed_objects_mutex);
}

int64_t hoo_is_managed_object(const void* obj) {
    if (!obj) return 0;
    hoo_mutex_lock(&g_managed_objects_mutex);
    ManagedObjNode* it = g_managed_objects;
    while (it) {
        if (it->obj == obj) {
            hoo_mutex_unlock(&g_managed_objects_mutex);
            return 1;
        }
        it = it->next;
    }
    hoo_mutex_unlock(&g_managed_objects_mutex);
    return 0;
}

static size_t align_up(size_t value, size_t alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static TLABBlock* tlab_alloc_block(size_t min_payload_size) {
    size_t block_cap = HOO_TLAB_BLOCK_SIZE;
    if (min_payload_size > block_cap) {
        block_cap = align_up(min_payload_size, sizeof(void*));
    }

    TLABBlock* block = (TLABBlock*)malloc(sizeof(TLABBlock) + block_cap);
    if (!block) {
        return NULL;
    }
    block->next = g_thread_tlab.head;
    block->used = 0;
    block->capacity = block_cap;
    g_thread_tlab.head = block;
    atomic_fetch_add_explicit(&tlab_stats.blocks_allocated, 1, memory_order_relaxed);
    return block;
}

static void tlab_reset_thread_cache_impl(void) {
    TLABBlock* it = g_thread_tlab.head;
    while (it) {
        TLABBlock* next = it->next;
        free(it);
        it = next;
    }
    g_thread_tlab.head = NULL;
}

void* hoo_alloc(size_t size, int64_t type_id) {
    const size_t align = sizeof(void*);
    const size_t header_size = sizeof(HooObjectHeader);
    const size_t total_bytes = align_up(header_size + size, align);

    HooObjectHeader* header = NULL;
    bool used_tlab = false;
    if (size <= HOO_TLAB_MAX_OBJECT_SIZE) {
        TLABBlock* block = g_thread_tlab.head;
        if (!block || (block->used + total_bytes) > block->capacity) {
            block = tlab_alloc_block(total_bytes);
        }
        if (block) {
            header = (HooObjectHeader*)(void*)(block->data + block->used);
            block->used += total_bytes;
            used_tlab = true;
            atomic_fetch_add_explicit(&tlab_stats.hits, 1, memory_order_relaxed);
        } else {
            atomic_fetch_add_explicit(&tlab_stats.misses, 1, memory_order_relaxed);
        }
    } else {
        atomic_fetch_add_explicit(&tlab_stats.misses, 1, memory_order_relaxed);
    }
    if (!header) {
        header = (HooObjectHeader*)malloc(total_bytes);
    }
    if (!header) {
        fprintf(stderr, "FATAL: Out of memory (tried to allocate %zu bytes)\n", size);
        exit(1);
    }

    header->refcount = 1;
    header->type_id = type_id;
    header->capacity = (int64_t)size;
    memset((char*)header + header_size, 0, size);

    // Update statistics
    memory_stats.total_allocations++;
    memory_stats.current_live_objects++;
    memory_stats.total_bytes_allocated += (int64_t)(total_bytes);

    // Return pointer to object data (skip header)
    void* obj = (char*)header + header_size;
    if (used_tlab) {
        TLABObjNode* node = (TLABObjNode*)malloc(sizeof(TLABObjNode));
        if (!node) {
            fprintf(stderr, "FATAL: Out of memory while tracking TLAB allocation\n");
            exit(1);
        }
        node->obj = obj;
        node->next = g_tlab_objects;
        g_tlab_objects = node;
    }
    managed_register(obj);

#ifdef HOO_DEBUG_MEMORY
    fprintf(stderr, "[ALLOC] obj=%p type=%lld size=%zu refcount=1\n",
            obj, (long long)type_id, size);
#endif

    return obj;
}

void* hoo_realloc(void* obj, size_t new_size) {
    if (!obj) return hoo_alloc(new_size, HOO_TYPE_OBJECT);

    HooObjectHeader* old_header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));
    int64_t type_id = old_header->type_id;

    // Check if current capacity is enough
    if ((int64_t)new_size <= old_header->capacity) {
        return obj;
    }

    // Allocate new block
    void* new_obj = hoo_alloc(new_size, type_id);
    if (!new_obj) return NULL;

    // Copy old data
    size_t copy_size = (size_t)old_header->capacity;
    memcpy(new_obj, obj, copy_size);

    // Release old object
    hoo_release(obj);

    return new_obj;
}

void* hoo_retain(void* obj) {
    if (!obj) {
        return NULL;
    }

    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));

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

    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));

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

        // Call registered destructor before freeing
        int64_t type_id = header->type_id;
        if (type_id >= 0 && type_id < HOO_DESTRUCTOR_TABLE_SIZE) {
            HooDestructor dtor = g_destructors[type_id];
            if (dtor) {
                dtor(obj);
            }
        }

        managed_unregister(obj);

        memory_stats.total_deallocations++;
        memory_stats.current_live_objects--;

        bool is_tlab_obj = false;
        TLABObjNode* prev = NULL;
        TLABObjNode* it = g_tlab_objects;
        while (it) {
            if (it->obj == obj) {
                is_tlab_obj = true;
                if (prev) {
                    prev->next = it->next;
                } else {
                    g_tlab_objects = it->next;
                }
                free(it);
                break;
            }
            prev = it;
            it = it->next;
        }

        if (!is_tlab_obj) {
            free(header);
        }
    }
}

int64_t hoo_get_refcount(void* obj) {
    if (!obj) {
        return 0;
    }

    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));
    return atomic_load(&header->refcount);
}

int64_t hoo_get_type_id(void* obj) {
    if (!obj) {
        fprintf(stderr, "ERROR: hoo_get_type_id called with NULL\n");
        return -1;
    }

    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));
    return header->type_id;
}

void hoo_object_set_field(void* obj, int64_t offset, int64_t value) {
    if (!obj) return;
    *(int64_t*)((char*)obj + offset) = value;
}

int64_t hoo_object_get_field(void* obj, int64_t offset) {
    if (!obj) return 0;
    return *(int64_t*)((char*)obj + offset);
}

int64_t hoo_get_capacity(void* obj) {
    if (!obj) return 0;
    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));
    return header->capacity;
}

void hoo_set_capacity(void* obj, int64_t capacity) {
    if (!obj) return;
    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));
    header->capacity = capacity;
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

int32_t hoo_tlab_enabled(void) {
    return 1;
}

HooTLABStats hoo_get_tlab_stats(void) {
    HooTLABStats out;
    out.tlab_hits = atomic_load_explicit(&tlab_stats.hits, memory_order_relaxed);
    out.tlab_misses = atomic_load_explicit(&tlab_stats.misses, memory_order_relaxed);
    out.tlab_blocks_allocated = atomic_load_explicit(&tlab_stats.blocks_allocated, memory_order_relaxed);
    return out;
}

void hoo_reset_tlab_stats(void) {
    atomic_store_explicit(&tlab_stats.hits, 0, memory_order_relaxed);
    atomic_store_explicit(&tlab_stats.misses, 0, memory_order_relaxed);
    atomic_store_explicit(&tlab_stats.blocks_allocated, 0, memory_order_relaxed);
}

void hoo_tlab_reset_thread_cache(void) {
    TLABObjNode* it = g_tlab_objects;
    while (it) {
        TLABObjNode* next = it->next;
        managed_unregister(it->obj);
        free(it);
        it = next;
    }
    g_tlab_objects = NULL;
    tlab_reset_thread_cache_impl();
}
