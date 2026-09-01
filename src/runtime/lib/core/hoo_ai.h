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

int32_t hoo_ai_abi_version(void);
HooStatus hoo_ai_last_status(void);
const char* hoo_ai_last_error(void);
int64_t hoo_ai_has_feature(const char* feature_name);

/* Internal runtime helper used by new C-ABI implementations. */
void hoo_ai_set_last_error(HooStatus status, const char* message);

#ifdef __cplusplus
}
#endif
