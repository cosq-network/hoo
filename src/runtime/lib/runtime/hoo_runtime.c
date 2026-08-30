#include "runtime/lib/runtime/hoo_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
// Provide OpenSSL's OPENSSL_Applink symbol so libcrypto can interoperate with
// the MSVC C runtime (avoids the "OPENSSL_Uplink ... no OPENSSL_Applink" abort).
// applink.c is only meant to be compiled into the application once; guard with
// a macro of our own so this translation unit is the single provider.
#ifndef HOO_OPENSSL_APPLINK_PROVIDED
#define HOO_OPENSSL_APPLINK_PROVIDED
#include <openssl/applink.c>
#endif
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

static pthread_key_t g_tlab_cleanup_key;
static pthread_once_t g_tlab_cleanup_once = PTHREAD_ONCE_INIT;

static void tlab_reset_thread_cache_impl(void);
void hoo_tlab_reset_thread_cache(void);

static void tlab_cleanup_destructor(void* arg) {
    (void)arg;
    hoo_tlab_reset_thread_cache();
}

static void tlab_cleanup_key_create(void) {
    pthread_key_create(&g_tlab_cleanup_key, tlab_cleanup_destructor);
}

static void tlab_register_cleanup(void) {
    pthread_once(&g_tlab_cleanup_once, tlab_cleanup_key_create);
    pthread_setspecific(g_tlab_cleanup_key, (void*)1);
}
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
    _Atomic int64_t live_objects;
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

typedef struct {
    int64_t type_id;
    HooDestructor destructor;
} HooDestructorEntry;

static hoo_mutex_t g_destructors_mutex = HOO_MUTEX_INIT;
static HooDestructorEntry* g_destructors = NULL;
static size_t g_destructor_count = 0;
static size_t g_destructor_capacity = 0;

void hoo_register_destructor(int64_t type_id, HooDestructor dtor) {
    if (type_id < 0) {
        fprintf(stderr, "FATAL: Invalid negative destructor type ID: %lld\n", (long long)type_id);
        abort();
    }

    hoo_mutex_lock(&g_destructors_mutex);
    for (size_t i = 0; i < g_destructor_count; ++i) {
        if (g_destructors[i].type_id != type_id) continue;

        if (dtor) {
            g_destructors[i].destructor = dtor;
        } else {
            g_destructors[i] = g_destructors[g_destructor_count - 1];
            --g_destructor_count;
        }
        hoo_mutex_unlock(&g_destructors_mutex);
        return;
    }

    if (!dtor) {
        hoo_mutex_unlock(&g_destructors_mutex);
        return;
    }

    if (g_destructor_count == g_destructor_capacity) {
        size_t new_capacity = g_destructor_capacity ? g_destructor_capacity * 2 : 16;
        HooDestructorEntry* entries = (HooDestructorEntry*)realloc(
            g_destructors, new_capacity * sizeof(HooDestructorEntry));
        if (!entries) {
            hoo_mutex_unlock(&g_destructors_mutex);
            fprintf(stderr, "FATAL: Out of memory while registering destructor for type ID %lld\n",
                    (long long)type_id);
            abort();
        }
        g_destructors = entries;
        g_destructor_capacity = new_capacity;
    }

    g_destructors[g_destructor_count++] = (HooDestructorEntry){type_id, dtor};
    hoo_mutex_unlock(&g_destructors_mutex);
}

static HooDestructor hoo_find_destructor(int64_t type_id) {
    HooDestructor result = NULL;
    hoo_mutex_lock(&g_destructors_mutex);
    for (size_t i = 0; i < g_destructor_count; ++i) {
        if (g_destructors[i].type_id == type_id) {
            result = g_destructors[i].destructor;
            break;
        }
    }
    hoo_mutex_unlock(&g_destructors_mutex);
    return result;
}

#define HOO_MANAGED_HASH_BITS 10
#define HOO_MANAGED_HASH_SIZE (1U << HOO_MANAGED_HASH_BITS)

typedef struct ManagedObjNode {
    void* obj;
    struct ManagedObjNode* next;
} ManagedObjNode;

#define HOO_MANAGED_MUTEX_COUNT 64
static hoo_mutex_t g_managed_mutexes[HOO_MANAGED_MUTEX_COUNT];
static _Atomic int g_managed_mutexes_ready = 0;

static void managed_mutexes_ensure_init(void) {
    if (!atomic_load_explicit(&g_managed_mutexes_ready, memory_order_acquire)) {
#ifndef _WIN32
        for (int i = 0; i < HOO_MANAGED_MUTEX_COUNT; i++) {
            pthread_mutex_init(&g_managed_mutexes[i], NULL);
        }
#endif
        atomic_store_explicit(&g_managed_mutexes_ready, 1, memory_order_release);
    }
}
static ManagedObjNode* g_managed_hash[HOO_MANAGED_HASH_SIZE] = {NULL};

static inline uint32_t managed_hash(const void* obj) {
    uintptr_t p = (uintptr_t)obj;
    return (uint32_t)((p >> 3) & (HOO_MANAGED_HASH_SIZE - 1));
}

static void managed_register(void* obj) {
    managed_mutexes_ensure_init();
    ManagedObjNode* node = (ManagedObjNode*)malloc(sizeof(ManagedObjNode));
    if (!node) {
        fprintf(stderr, "FATAL: Out of memory while tracking managed allocation\n");
        exit(1);
    }
    node->obj = obj;
    uint32_t idx = managed_hash(obj);
    uint32_t lock_idx = idx % HOO_MANAGED_MUTEX_COUNT;
    hoo_mutex_lock(&g_managed_mutexes[lock_idx]);
    node->next = g_managed_hash[idx];
    g_managed_hash[idx] = node;
    hoo_mutex_unlock(&g_managed_mutexes[lock_idx]);
}

static void managed_unregister(void* obj) {
    managed_mutexes_ensure_init();
    uint32_t idx = managed_hash(obj);
    uint32_t lock_idx = idx % HOO_MANAGED_MUTEX_COUNT;
    hoo_mutex_lock(&g_managed_mutexes[lock_idx]);
    ManagedObjNode* prev = NULL;
    ManagedObjNode* it = g_managed_hash[idx];
    while (it) {
        if (it->obj == obj) {
            if (prev) {
                prev->next = it->next;
            } else {
                g_managed_hash[idx] = it->next;
            }
            hoo_mutex_unlock(&g_managed_mutexes[lock_idx]);
            free(it);
            return;
        }
        prev = it;
        it = it->next;
    }
    hoo_mutex_unlock(&g_managed_mutexes[lock_idx]);
}

int64_t hoo_is_managed_object(const void* obj) {
    if (!obj) return 0;
    managed_mutexes_ensure_init();
    uint32_t idx = managed_hash(obj);
    uint32_t lock_idx = idx % HOO_MANAGED_MUTEX_COUNT;
    hoo_mutex_lock(&g_managed_mutexes[lock_idx]);
    ManagedObjNode* it = g_managed_hash[idx];
    while (it) {
        if (it->obj == obj) {
            hoo_mutex_unlock(&g_managed_mutexes[lock_idx]);
            return 1;
        }
        it = it->next;
    }
    hoo_mutex_unlock(&g_managed_mutexes[lock_idx]);
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

#ifndef _WIN32
    tlab_register_cleanup();
#endif

    TLABBlock* block = (TLABBlock*)malloc(sizeof(TLABBlock) + block_cap);
    if (!block) {
        return NULL;
    }
    block->next = g_thread_tlab.head;
    block->used = 0;
    block->capacity = block_cap;
    atomic_store_explicit(&block->live_objects, 0, memory_order_relaxed);
    g_thread_tlab.head = block;
    atomic_fetch_add_explicit(&tlab_stats.blocks_allocated, 1, memory_order_relaxed);
    return block;
}

static void tlab_reset_thread_cache_impl(void) {
    TLABBlock* it = g_thread_tlab.head;
    while (it) {
        TLABBlock* next = it->next;
        if (atomic_load_explicit(&it->live_objects, memory_order_acquire) == 0) {
            free(it);
        }
        it = next;
    }
    g_thread_tlab.head = NULL;
}

void* hoo_alloc(size_t size, int64_t type_id) {
    const size_t align = sizeof(void*);
    const size_t header_size = sizeof(HooObjectHeader);
    const size_t total_bytes = align_up(header_size + size, align);

    HooObjectHeader* header = NULL;
    TLABBlock* block = NULL;
    bool used_tlab = false;
    if (size <= HOO_TLAB_MAX_OBJECT_SIZE) {
        block = g_thread_tlab.head;
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
    header->reserved = used_tlab ? (int64_t)(intptr_t)block : 0;
    memset((char*)header + header_size, 0, size);

    // Update statistics
    memory_stats.total_allocations++;
    memory_stats.current_live_objects++;
    memory_stats.total_bytes_allocated += (int64_t)(total_bytes);

    // Return pointer to object data (skip header)
    void* obj = (char*)header + header_size;
    if (used_tlab) {
        atomic_fetch_add_explicit(&block->live_objects, 1, memory_order_relaxed);
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

    if (getenv("HOO_TRACE_CALLS")) {
        fprintf(stderr, "[TRACE-RC] hoo_retain(obj=%p type=%lld rc=%lld\n", obj, (long long)header->type_id, (long long)atomic_load(&header->refcount));
    }

#ifdef HOO_DEBUG_MEMORY
    fprintf(stderr, "[RETAIN] obj=%p type=%lld refcount=%lld\n",
            obj, (long long)header->type_id, (long long)atomic_load(&header->refcount));
#endif

    return obj;
}

static void hoo_release_finalize(void* obj, HooObjectHeader* header) {
#ifdef HOO_DEBUG_MEMORY
    fprintf(stderr, "[FREE] obj=%p type=%lld\n",
            obj, (long long)header->type_id);
#endif

    atomic_thread_fence(memory_order_acquire);

    // Call registered destructor before freeing
    HooDestructor dtor = hoo_find_destructor(header->type_id);
    if (dtor) {
        dtor(obj);
    }

    managed_unregister(obj);

    memory_stats.total_deallocations++;
    memory_stats.current_live_objects--;

    TLABBlock* owner = (TLABBlock*)(intptr_t)header->reserved;
    if (owner) {
        atomic_fetch_sub_explicit(&owner->live_objects, 1, memory_order_relaxed);
    } else {
        free(header);
    }
}

void hoo_release(void* obj) {
    if (!obj) {
        return;
    }

    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));

    int64_t old_count = atomic_fetch_sub_explicit(&header->refcount, 1, memory_order_release);

    if (getenv("HOO_TRACE_CALLS")) {
        fprintf(stderr, "[TRACE-RC] hoo_release(obj=%p type=%lld old=%lld\n", obj, (long long)header->type_id, (long long)old_count);
    }

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
        hoo_release_finalize(obj, header);
    }
}

int64_t hoo_release_zero_flag(void* obj) {
    if (!obj) {
        return 0;
    }

    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - sizeof(HooObjectHeader));

    int64_t old_count = atomic_fetch_sub_explicit(&header->refcount, 1, memory_order_release);

#ifdef HOO_DEBUG_MEMORY
    fprintf(stderr, "[RELEASE_ZF] obj=%p type=%lld refcount=%lld\n",
            obj, (long long)header->type_id, (long long)old_count - 1);
#endif

    if (old_count <= 0) {
        fprintf(stderr, "FATAL: Double-release or negative refcount for object %p (type %lld)\n",
                obj, (long long)header->type_id);
        exit(1);
    }

    if (old_count == 1) {
        hoo_release_finalize(obj, header);
        return 1;
    }
    return 0;
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
    tlab_reset_thread_cache_impl();
}
