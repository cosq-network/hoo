#include "runtime/lib/event_loop/hoo_event_loop.h"
#include <stdlib.h>
#include <mutex>

static uv_loop_t* global_loop = NULL;
static std::mutex global_loop_mutex;

#ifdef __cplusplus
extern "C" {
#endif

void hoo_event_loop_init(void) {
    std::lock_guard<std::mutex> lock(global_loop_mutex);
    if (global_loop == NULL) {
        global_loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_loop_init(global_loop);
    }
}

uv_loop_t* hoo_event_loop_get(void) {
    std::lock_guard<std::mutex> lock(global_loop_mutex);
    if (global_loop == NULL) {
        global_loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        if (global_loop) uv_loop_init(global_loop);
    }
    return global_loop;
}

void hoo_event_loop_run(void) {
    std::lock_guard<std::mutex> lock(global_loop_mutex);
    if (global_loop != NULL) {
        uv_run(global_loop, UV_RUN_DEFAULT);
    }
}

void hoo_event_loop_run_nowait(void) {
    std::lock_guard<std::mutex> lock(global_loop_mutex);
    if (global_loop != NULL) {
        uv_run(global_loop, UV_RUN_NOWAIT);
    }
}

void hoo_event_loop_destroy(void) {
    std::lock_guard<std::mutex> lock(global_loop_mutex);
    if (global_loop != NULL) {
        if (uv_loop_close(global_loop) == 0) {
            free(global_loop);
            global_loop = NULL;
        }
    }
}

#ifdef __cplusplus
}
#endif
