#pragma once

#include <uv.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the global libuv event loop.
 */
void hoo_event_loop_init(void);

/**
 * Get the global libuv event loop.
 */
uv_loop_t* hoo_event_loop_get(void);

/**
 * Run the global event loop until there are no more active handles.
 */
void hoo_event_loop_run(void);

/**
 * Destroy and clean up the global event loop.
 */
void hoo_event_loop_destroy(void);

#ifdef __cplusplus
}
#endif
