#include "hoo_event_loop.h"
#include <stdlib.h>

static uv_loop_t* global_loop = NULL;

#ifdef __cplusplus
extern "C" {
#endif

void hoo_event_loop_init(void) {
    if (global_loop == NULL) {
        global_loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_loop_init(global_loop);
    }
}

uv_loop_t* hoo_event_loop_get(void) {
    if (global_loop == NULL) {
        hoo_event_loop_init();
    }
    return global_loop;
}

void hoo_event_loop_run(void) {
    if (global_loop != NULL) {
        uv_run(global_loop, UV_RUN_DEFAULT);
    }
}

void hoo_event_loop_destroy(void) {
    if (global_loop != NULL) {
        uv_loop_close(global_loop);
        free(global_loop);
        global_loop = NULL;
    }
}

#ifdef __cplusplus
}
#endif
