#include "hoo_tensor.h"
#include "hoo_runtime.h"

#include <cmath>
#include <cstring>

#define HOO_TYPE_TENSOR 113
#define TENSOR_ELEMENT_BIT 8
#define TENSOR_ELEMENT_F8 9
#define TENSOR_ELEMENT_F64 2

struct HooTensorHeader {
    int64_t element_type;
    int64_t rank;
    int64_t dims[3];
    int64_t length;
    int64_t next_index;
    int64_t storage_bytes;
};

static HooTensorHeader* header(HooTensor tensor) {
    return reinterpret_cast<HooTensorHeader*>(tensor);
}

static uint8_t* data(HooTensor tensor) {
    return reinterpret_cast<uint8_t*>(tensor) + sizeof(HooTensorHeader);
}

static bool is_float_element(int64_t element_type) {
    return element_type == TENSOR_ELEMENT_F64 || element_type == TENSOR_ELEMENT_F8;
}

static int64_t checked_length(int64_t rank, int64_t d0, int64_t d1, int64_t d2) {
    if (rank < 1 || rank > 3) return 0;
    int64_t dims[3] = {d0, d1, d2};
    int64_t length = 1;
    for (int64_t i = 0; i < rank; ++i) {
        if (dims[i] <= 0) return 0;
        length *= dims[i];
    }
    return length;
}

static int64_t storage_bytes(int64_t element_type, int64_t length) {
    if (length <= 0) return 0;
    if (element_type == TENSOR_ELEMENT_BIT) return (length + 7) / 8;
    return length * 8;
}

static bool same_shape(HooTensor left, HooTensor right) {
    auto* l = header(left);
    auto* r = header(right);
    if (!l || !r || l->rank != r->rank || l->length != r->length) return false;
    for (int64_t i = 0; i < l->rank; ++i) {
        if (l->dims[i] != r->dims[i]) return false;
    }
    return true;
}

static double get_numeric(HooTensor tensor, int64_t index) {
    auto* h = header(tensor);
    if (!h || index < 0 || index >= h->length) return 0.0;
    if (h->element_type == TENSOR_ELEMENT_BIT) {
        uint8_t byte = data(tensor)[index / 8];
        return (byte >> (index % 8)) & 1;
    }
    int64_t bits = 0;
    std::memcpy(&bits, data(tensor) + index * 8, sizeof(bits));
    if (is_float_element(h->element_type)) {
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    return static_cast<double>(bits);
}

static int64_t pack_numeric(int64_t element_type, double value) {
    if (is_float_element(element_type)) {
        int64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        return bits;
    }
    return static_cast<int64_t>(value);
}

static int64_t result_element_type(HooTensor left, HooTensor right) {
    int64_t lt = header(left)->element_type;
    int64_t rt = header(right)->element_type;
    if (lt == TENSOR_ELEMENT_F64 || rt == TENSOR_ELEMENT_F64) return TENSOR_ELEMENT_F64;
    if (lt == TENSOR_ELEMENT_F8 || rt == TENSOR_ELEMENT_F8) return TENSOR_ELEMENT_F8;
    if (lt == TENSOR_ELEMENT_BIT && rt == TENSOR_ELEMENT_BIT) return TENSOR_ELEMENT_BIT;
    return lt;
}

static HooTensor make_like(HooTensor source, int64_t element_type) {
    auto* h = header(source);
    return hoo_tensor_new(element_type, h->rank, h->dims[0], h->dims[1], h->dims[2]);
}

static HooTensor binary_numeric(HooTensor left, HooTensor right, int op) {
    if (!left || !right || !same_shape(left, right)) return nullptr;
    int64_t elem_type = result_element_type(left, right);
    HooTensor result = make_like(left, elem_type);
    auto* h = header(left);
    for (int64_t i = 0; i < h->length; ++i) {
        double l = get_numeric(left, i);
        double r = get_numeric(right, i);
        double value = 0.0;
        switch (op) {
            case 0: value = l + r; break;
            case 1: value = l - r; break;
            case 2: value = l * r; break;
            case 3: value = r == 0.0 ? 0.0 : l / r; break;
        }
        hoo_tensor_set_value(result, i, pack_numeric(elem_type, value));
    }
    return result;
}

static HooTensor binary_compare(HooTensor left, HooTensor right, int op) {
    if (!left || !right || !same_shape(left, right)) return nullptr;
    HooTensor result = make_like(left, TENSOR_ELEMENT_BIT);
    auto* h = header(left);
    for (int64_t i = 0; i < h->length; ++i) {
        double l = get_numeric(left, i);
        double r = get_numeric(right, i);
        int64_t value = 0;
        switch (op) {
            case 0: value = l == r; break;
            case 1: value = l != r; break;
            case 2: value = l < r; break;
            case 3: value = l <= r; break;
            case 4: value = l > r; break;
            case 5: value = l >= r; break;
        }
        hoo_tensor_set_value(result, i, value);
    }
    return result;
}

HooTensor hoo_tensor_new(int64_t element_type, int64_t rank, int64_t d0, int64_t d1, int64_t d2) {
    int64_t length = checked_length(rank, d0, d1, d2);
    int64_t bytes = storage_bytes(element_type, length);
    auto* h = reinterpret_cast<HooTensorHeader*>(hoo_alloc(sizeof(HooTensorHeader) + bytes, HOO_TYPE_TENSOR));
    if (!h) return nullptr;
    h->element_type = element_type;
    h->rank = rank;
    h->dims[0] = d0;
    h->dims[1] = rank >= 2 ? d1 : 1;
    h->dims[2] = rank >= 3 ? d2 : 1;
    h->length = length;
    h->next_index = 0;
    h->storage_bytes = bytes;
    std::memset(reinterpret_cast<uint8_t*>(h) + sizeof(HooTensorHeader), 0, static_cast<size_t>(bytes));
    return reinterpret_cast<HooTensor>(h);
}

HooTensor hoo_tensor_new1(int64_t element_type, int64_t d0) {
    return hoo_tensor_new(element_type, 1, d0, 1, 1);
}

HooTensor hoo_tensor_new2(int64_t element_type, int64_t d0, int64_t d1) {
    return hoo_tensor_new(element_type, 2, d0, d1, 1);
}

HooTensor hoo_tensor_new3(int64_t element_type, int64_t d0, int64_t d1, int64_t d2) {
    return hoo_tensor_new(element_type, 3, d0, d1, d2);
}

int64_t hoo_tensor_length(HooTensor tensor) {
    return tensor ? header(tensor)->length : 0;
}

int64_t hoo_tensor_rank(HooTensor tensor) {
    return tensor ? header(tensor)->rank : 0;
}

int64_t hoo_tensor_dim(HooTensor tensor, int64_t axis) {
    if (!tensor || axis < 0 || axis >= header(tensor)->rank) return 0;
    return header(tensor)->dims[axis];
}

int64_t hoo_tensor_element_type(HooTensor tensor) {
    return tensor ? header(tensor)->element_type : 0;
}

int64_t hoo_tensor_set_value(HooTensor tensor, int64_t index, int64_t value_bits) {
    auto* h = header(tensor);
    if (!h || index < 0 || index >= h->length) return 0;
    if (h->element_type == TENSOR_ELEMENT_BIT) {
        uint8_t* bytes = data(tensor);
        uint8_t mask = static_cast<uint8_t>(1u << (index % 8));
        if (value_bits & 1) bytes[index / 8] |= mask;
        else bytes[index / 8] &= static_cast<uint8_t>(~mask);
        return 1;
    }
    std::memcpy(data(tensor) + index * 8, &value_bits, sizeof(value_bits));
    return 1;
}

int64_t hoo_tensor_push_value(HooTensor tensor, int64_t value_bits) {
    auto* h = header(tensor);
    if (!h || h->next_index >= h->length) return 0;
    return hoo_tensor_set_value(tensor, h->next_index++, value_bits);
}

int64_t hoo_tensor_get_int64(HooTensor tensor, int64_t index) {
    return static_cast<int64_t>(get_numeric(tensor, index));
}

double hoo_tensor_get_double(HooTensor tensor, int64_t index) {
    return get_numeric(tensor, index);
}

int64_t hoo_tensor_get_bits(HooTensor tensor, int64_t index) {
    auto* h = header(tensor);
    if (!h || index < 0 || index >= h->length) return 0;
    if (h->element_type == TENSOR_ELEMENT_BIT) return static_cast<int64_t>(get_numeric(tensor, index));
    int64_t bits = 0;
    std::memcpy(&bits, data(tensor) + index * 8, sizeof(bits));
    return bits;
}

HooTensor hoo_tensor_add(HooTensor left, HooTensor right) { return binary_numeric(left, right, 0); }
HooTensor hoo_tensor_sub(HooTensor left, HooTensor right) { return binary_numeric(left, right, 1); }
HooTensor hoo_tensor_element_mul(HooTensor left, HooTensor right) { return binary_numeric(left, right, 2); }
HooTensor hoo_tensor_element_div(HooTensor left, HooTensor right) { return binary_numeric(left, right, 3); }

HooTensor hoo_tensor_matmul(HooTensor left, HooTensor right) {
    if (!left || !right) return nullptr;
    auto* l = header(left);
    auto* r = header(right);
    if (l->rank != 2 || r->rank != 2 || l->dims[1] != r->dims[0]) return nullptr;
    int64_t elem_type = result_element_type(left, right);
    HooTensor result = hoo_tensor_new2(elem_type, l->dims[0], r->dims[1]);
    for (int64_t i = 0; i < l->dims[0]; ++i) {
        for (int64_t j = 0; j < r->dims[1]; ++j) {
            double sum = 0.0;
            for (int64_t k = 0; k < l->dims[1]; ++k) {
                sum += get_numeric(left, i * l->dims[1] + k) * get_numeric(right, k * r->dims[1] + j);
            }
            hoo_tensor_set_value(result, i * r->dims[1] + j, pack_numeric(elem_type, sum));
        }
    }
    return result;
}

HooTensor hoo_tensor_eq(HooTensor left, HooTensor right) { return binary_compare(left, right, 0); }
HooTensor hoo_tensor_ne(HooTensor left, HooTensor right) { return binary_compare(left, right, 1); }
HooTensor hoo_tensor_lt(HooTensor left, HooTensor right) { return binary_compare(left, right, 2); }
HooTensor hoo_tensor_le(HooTensor left, HooTensor right) { return binary_compare(left, right, 3); }
HooTensor hoo_tensor_gt(HooTensor left, HooTensor right) { return binary_compare(left, right, 4); }
HooTensor hoo_tensor_ge(HooTensor left, HooTensor right) { return binary_compare(left, right, 5); }

HooTensor hoo_tensor_and(HooTensor left, HooTensor right) {
    if (!left || !right || !same_shape(left, right)) return nullptr;
    HooTensor result = make_like(left, TENSOR_ELEMENT_BIT);
    for (int64_t i = 0; i < header(left)->length; ++i) {
        hoo_tensor_set_value(result, i, get_numeric(left, i) != 0.0 && get_numeric(right, i) != 0.0);
    }
    return result;
}

HooTensor hoo_tensor_or(HooTensor left, HooTensor right) {
    if (!left || !right || !same_shape(left, right)) return nullptr;
    HooTensor result = make_like(left, TENSOR_ELEMENT_BIT);
    for (int64_t i = 0; i < header(left)->length; ++i) {
        hoo_tensor_set_value(result, i, get_numeric(left, i) != 0.0 || get_numeric(right, i) != 0.0);
    }
    return result;
}

HooTensor hoo_tensor_not(HooTensor tensor) {
    if (!tensor) return nullptr;
    HooTensor result = make_like(tensor, TENSOR_ELEMENT_BIT);
    for (int64_t i = 0; i < header(tensor)->length; ++i) {
        hoo_tensor_set_value(result, i, get_numeric(tensor, i) == 0.0);
    }
    return result;
}
