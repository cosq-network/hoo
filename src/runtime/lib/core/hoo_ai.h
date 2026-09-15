#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t HooStatus;

enum {
    HOO_STATUS_OK = 0,
    HOO_STATUS_INVALID_ARGUMENT = 1,
    HOO_STATUS_INVALID_SHAPE = 2,
    HOO_STATUS_INVALID_DTYPE = 3,
    HOO_STATUS_UNSUPPORTED = 4,
    HOO_STATUS_OUT_OF_MEMORY = 5,
    HOO_STATUS_OUT_OF_BOUNDS = 6,
    HOO_STATUS_DEVICE_UNAVAILABLE = 7,
    HOO_STATUS_IO_ERROR = 8,
    HOO_STATUS_FORMAT_ERROR = 9,
    HOO_STATUS_NUMERICAL_ERROR = 10,
    HOO_STATUS_CANCELLED = 11
};

/*
 * Dtype identifiers for the versioned tensor C ABI (ABI v2, rank 1-64).
 *
 * VALUE COMPATIBILITY: these ids intentionally mirror the HOO_DTYPE_*
 * constants proposed in docs/issues/ISSUE-026_native_ann_support.md
 * section 3.4.0. If that proposal lands, it must reuse these values.
 * Existing dtype ids are appended to and are never reused.
 */
enum {
    HOO_TENSOR_ABI_VERSION = 2,
    HOO_TENSOR_DTYPE_INT64 = 1,
    HOO_TENSOR_DTYPE_F64 = 2,
    HOO_TENSOR_DTYPE_INT8 = 5,
    HOO_TENSOR_DTYPE_BYTE = 6,
    HOO_TENSOR_DTYPE_BIT = 8,
    HOO_TENSOR_DTYPE_F8 = 9,
    HOO_TENSOR_DTYPE_F32 = 16,
    HOO_TENSOR_DTYPE_F16 = 17,
    HOO_TENSOR_DTYPE_BF16 = 18,
    HOO_TENSOR_DTYPE_INT32 = 19
};

/*
 * Public ANN foundation surface (implemented ABI v2 foundation):
 *
 *   hoo_ai_abi_version   - compile-time ABI version of the runtime.
 *   hoo_ai_last_status   - status of the most recent hoo_tensor_* call on
 *                          the CURRENT THREAD (see thread-local note below).
 *   hoo_ai_last_error    - human-readable diagnostic for that status.
 *   hoo_ai_has_feature   - capability query by stable feature name
 *                          ("tensor_abi_v2", "tensor_dynamic_rank",
 *                          "tensor_f32", "tensor_int32", "tensor_f16",
 *                          "tensor_bf16", ...). Unknown names return 0.
 *   hoo_ai_dtype_supported - per-dtype capability query backing the
 *                          feature strings; keeps hoo_ai_has_feature and
 *                          the tensor dtype validation driven by one source.
 *   hoo_ai_clear_last_error - convenience for the success-path contract.
 *
 * NOT YET IMPLEMENTED (planned extensions, see ISSUE-026; listed here so
 * callers do not assume them): hoo_ai_get_capabilities(HooBuffer*), the
 * HooTensorND/HooTensorDesc handle family, HooDevice and the device
 * transfer ABI, and the parameter/tape/dataset/model-format ABIs.
 *
 * THREAD-LOCAL ERROR STATE: last_status/last_error are per-thread. A
 * status set on one thread is never visible on another. Any future
 * parallel tensor kernel must re-capture status on the caller thread.
 * The error string is diagnostic only; programs must branch on
 * HooStatus values or capability queries. The C ABI never throws C++
 * exceptions across Hoo/HVM calls.
 *
 * SUCCESS-PATH CONTRACT: every public hoo_tensor_* function (and any
 * future function of this ABI) must leave the thread's error state
 * consistent: successful completion clears it
 * (hoo_ai_clear_last_error or set_last_ok), failure sets a specific
 * HOO_STATUS. Nothing is reset implicitly; a stale error means the
 * previous call did not honour this contract.
 */

int32_t hoo_ai_abi_version(void);
HooStatus hoo_ai_last_status(void);
const char* hoo_ai_last_error(void);
int64_t hoo_ai_has_feature(const char* feature_name);
int64_t hoo_ai_dtype_supported(int64_t element_type);
void hoo_ai_clear_last_error(void);

/*
 * PRIVATE-BUT-EXPORTED ABI EXTENSION. Used by the tensor module of the
 * same runtime to report typed errors; it is exported like the rest of
 * the hoort C surface, but hoort-external callers must not rely on it:
 * treat it as reserved (existing name, no stability guarantee beyond
 * "same thread, diagnostic-only, bounded copy").
 */
void hoo_ai_set_last_error(HooStatus status, const char* message);

#ifdef __cplusplus
}
#endif
