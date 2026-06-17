#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooTensor;

HooTensor hoo_tensor_new(int64_t element_type, int64_t rank, int64_t d0, int64_t d1, int64_t d2);
HooTensor hoo_tensor_new1(int64_t element_type, int64_t d0);
HooTensor hoo_tensor_new2(int64_t element_type, int64_t d0, int64_t d1);
HooTensor hoo_tensor_new3(int64_t element_type, int64_t d0, int64_t d1, int64_t d2);

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
HooTensor hoo_tensor_matmul(HooTensor left, HooTensor right);

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
