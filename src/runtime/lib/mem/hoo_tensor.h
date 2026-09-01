#pragma once

#include <stdint.h>
#include "runtime/lib/core/hoo_ai.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooTensor;

#define HOO_TENSOR_ELEMENT_INT64 HOO_TENSOR_DTYPE_INT64
#define HOO_TENSOR_ELEMENT_F64  HOO_TENSOR_DTYPE_F64
#define HOO_TENSOR_ELEMENT_INT8 HOO_TENSOR_DTYPE_INT8
#define HOO_TENSOR_ELEMENT_BYTE HOO_TENSOR_DTYPE_BYTE
#define HOO_TENSOR_ELEMENT_BIT  HOO_TENSOR_DTYPE_BIT
#define HOO_TENSOR_ELEMENT_F8   HOO_TENSOR_DTYPE_F8

HooTensor hoo_tensor_new(int64_t element_type, int64_t rank, int64_t d0, int64_t d1, int64_t d2);
HooTensor hoo_tensor_new1(int64_t element_type, int64_t d0);
HooTensor hoo_tensor_new2(int64_t element_type, int64_t d0, int64_t d1);
HooTensor hoo_tensor_new3(int64_t element_type, int64_t d0, int64_t d1, int64_t d2);

/* Versioned extension. Existing constructors and symbols remain unchanged. */
HooStatus hoo_tensor_new_ex(int64_t element_type, int64_t rank,
                            const int64_t* dims, HooTensor* out);
HooStatus hoo_tensor_shape(HooTensor tensor, int64_t capacity,
                           int64_t* dims, int64_t* out_count);
HooStatus hoo_tensor_strides(HooTensor tensor, int64_t capacity,
                             int64_t* strides, int64_t* out_count);
HooStatus hoo_tensor_numel(HooTensor tensor, int64_t* out_numel);
HooStatus hoo_tensor_abi_version(HooTensor tensor, int32_t* out_version);
HooStatus hoo_tensor_copy(HooTensor tensor, HooTensor* out);

int64_t hoo_tensor_length(HooTensor tensor);
int64_t hoo_tensor_rank(HooTensor tensor);
int64_t hoo_tensor_dim(HooTensor tensor, int64_t axis);
int64_t hoo_tensor_element_type(HooTensor tensor);

int64_t hoo_tensor_push_value(HooTensor tensor, int64_t value_bits);
int64_t hoo_tensor_set_value(HooTensor tensor, int64_t index, int64_t value_bits);
int64_t hoo_tensor_get_int64(HooTensor tensor, int64_t index);
double hoo_tensor_get_double(HooTensor tensor, int64_t index);
int64_t hoo_tensor_get_bits(HooTensor tensor, int64_t index);

HooTensor hoo_tensor_add(HooTensor left, HooTensor right);
HooTensor hoo_tensor_sub(HooTensor left, HooTensor right);
HooTensor hoo_tensor_element_mul(HooTensor left, HooTensor right);
HooTensor hoo_tensor_element_div(HooTensor left, HooTensor right);
HooTensor hoo_tensor_add_scalar_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type);
HooTensor hoo_tensor_sub_scalar_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type);
HooTensor hoo_tensor_sub_scalar_left_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type);
HooTensor hoo_tensor_scale_scalar_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type);
HooTensor hoo_tensor_div_scalar_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type);
HooTensor hoo_tensor_div_scalar_left_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type);
HooTensor hoo_tensor_add_scalar(HooTensor tensor, double scalar);
HooTensor hoo_tensor_sub_scalar(HooTensor tensor, double scalar);
HooTensor hoo_tensor_sub_scalar_left(HooTensor tensor, double scalar);
HooTensor hoo_tensor_scale_scalar(HooTensor tensor, double scalar);
HooTensor hoo_tensor_div_scalar(HooTensor tensor, double scalar);
HooTensor hoo_tensor_div_scalar_left(HooTensor tensor, double scalar);
HooTensor hoo_tensor_matmul(HooTensor left, HooTensor right);
HooTensor hoo_tensor_reshape(HooTensor tensor, int64_t rank, int64_t d0, int64_t d1, int64_t d2);
HooTensor hoo_tensor_transpose(HooTensor tensor);
HooTensor hoo_tensor_softmax(HooTensor tensor);

HooTensor hoo_tensor_eq(HooTensor left, HooTensor right);
HooTensor hoo_tensor_ne(HooTensor left, HooTensor right);
HooTensor hoo_tensor_lt(HooTensor left, HooTensor right);
HooTensor hoo_tensor_le(HooTensor left, HooTensor right);
HooTensor hoo_tensor_gt(HooTensor left, HooTensor right);
HooTensor hoo_tensor_ge(HooTensor left, HooTensor right);

HooTensor hoo_tensor_and(HooTensor left, HooTensor right);
HooTensor hoo_tensor_or(HooTensor left, HooTensor right);
HooTensor hoo_tensor_not(HooTensor tensor);

#ifdef __cplusplus
}
#endif
