#include "runtime/lib/tensor/hoo_tensor.h"
#include "runtime/lib/runtime/hoo_runtime.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <mutex>

#define TENSOR_ELEMENT_BIT HOO_TENSOR_DTYPE_BIT
#define TENSOR_ELEMENT_F8 HOO_TENSOR_DTYPE_F8
#define TENSOR_ELEMENT_F64 HOO_TENSOR_DTYPE_F64
#define TENSOR_ELEMENT_INT8 HOO_TENSOR_DTYPE_INT8
#define TENSOR_ELEMENT_BYTE HOO_TENSOR_DTYPE_BYTE
#define TENSOR_ELEMENT_INT64 HOO_TENSOR_DTYPE_INT64
#define TENSOR_ELEMENT_F32 HOO_TENSOR_DTYPE_F32
#define TENSOR_ELEMENT_F16 HOO_TENSOR_DTYPE_F16
#define TENSOR_ELEMENT_BF16 HOO_TENSOR_DTYPE_BF16
#define TENSOR_ELEMENT_INT32 HOO_TENSOR_DTYPE_INT32

struct HooTensorHeader {
    int64_t element_type;
    int64_t rank;
    int64_t legacy_dims[3];
    int64_t length;
    int64_t next_index;
    int64_t storage_bytes;
    uint32_t abi_version;
    uint32_t flags;
    int64_t* dims;
    int64_t* strides;
    uint8_t* data_ptr;
    HooTensor base;
};

static constexpr uint32_t TENSOR_OWNS_DATA = 1u << 0;
static constexpr uint32_t TENSOR_OWNS_METADATA = 1u << 1;

static void destroy_tensor(void* object) {
    auto* h = static_cast<HooTensorHeader*>(object);
    if (!h) return;
    if (h->base) hoo_release(h->base);
    if (h->flags & TENSOR_OWNS_DATA) std::free(h->data_ptr);
    if (h->flags & TENSOR_OWNS_METADATA) {
        std::free(h->dims);
        std::free(h->strides);
    }
    h->data_ptr = nullptr;
    h->dims = nullptr;
    h->strides = nullptr;
    h->base = nullptr;
}

static void ensure_tensor_runtime() {
    static std::once_flag once;
    std::call_once(once, [] {
        hoo_register_destructor(HOO_TYPE_TENSOR, &destroy_tensor);
    });
}

static HooTensorHeader* header(HooTensor tensor) {
    return reinterpret_cast<HooTensorHeader*>(tensor);
}

static uint8_t* data(HooTensor tensor) {
    auto* h = header(tensor);
    return h ? h->data_ptr : nullptr;
}

static bool valid_element_type(int64_t element_type) {
    /* Type ID 3 is retained for compatibility with existing opaque/bool
       tensor callers even though it is not an ANN training dtype. */
    return element_type == 3 || element_type == TENSOR_ELEMENT_BIT || element_type == TENSOR_ELEMENT_F8 ||
           element_type == TENSOR_ELEMENT_F64 || element_type == TENSOR_ELEMENT_INT8 ||
           element_type == TENSOR_ELEMENT_BYTE || element_type == TENSOR_ELEMENT_INT64 ||
           element_type == TENSOR_ELEMENT_F32 || element_type == TENSOR_ELEMENT_INT32;
}

static int64_t element_bytes(int64_t element_type) {
    if (element_type == TENSOR_ELEMENT_BIT || element_type == TENSOR_ELEMENT_INT8 ||
        element_type == TENSOR_ELEMENT_BYTE || element_type == TENSOR_ELEMENT_F8) return 1;
    if (element_type == TENSOR_ELEMENT_F16 || element_type == TENSOR_ELEMENT_BF16) return 2;
    if (element_type == TENSOR_ELEMENT_F32 || element_type == TENSOR_ELEMENT_INT32) return 4;
    return 8;
}

static bool checked_shape(int64_t rank, const int64_t* dims, int64_t* length) {
    if (!dims || rank < 1 || rank > 64 || !length) return false;
    int64_t result = 1;
    for (int64_t i = 0; i < rank; ++i) {
        if (dims[i] <= 0 || result > std::numeric_limits<int64_t>::max() / dims[i]) return false;
        result *= dims[i];
    }
    *length = result;
    return true;
}

static bool checked_storage_bytes(int64_t element_type, int64_t length, int64_t* bytes) {
    if (!bytes || length <= 0 || !valid_element_type(element_type)) return false;
    if (element_type == TENSOR_ELEMENT_BIT) {
        if (length > std::numeric_limits<int64_t>::max() - 7) return false;
        *bytes = (length + 7) / 8;
        return true;
    }
    const int64_t width = element_bytes(element_type);
    if (length > std::numeric_limits<int64_t>::max() / width) return false;
    *bytes = length * width;
    return true;
}

static bool set_error(HooStatus status, const char* message) {
    hoo_ai_set_last_error(status, message);
    return false;
}

static HooTensorHeader* allocate_tensor(int64_t element_type, int64_t rank,
                                         const int64_t* dims, HooTensor base,
                                         int64_t offset_bytes) {
    ensure_tensor_runtime();
    int64_t length = 0;
    int64_t bytes = 0;
    if (!checked_shape(rank, dims, &length) ||
        !checked_storage_bytes(element_type, length, &bytes)) return nullptr;

    auto* h = static_cast<HooTensorHeader*>(hoo_alloc(sizeof(HooTensorHeader), HOO_TYPE_TENSOR));
    if (!h) return nullptr;
    std::memset(h, 0, sizeof(*h));
    h->abi_version = HOO_TENSOR_ABI_VERSION;
    h->element_type = element_type;
    h->rank = rank;
    h->length = length;
    h->storage_bytes = bytes;

    h->dims = static_cast<int64_t*>(std::calloc(static_cast<size_t>(rank), sizeof(int64_t)));
    h->strides = static_cast<int64_t*>(std::calloc(static_cast<size_t>(rank), sizeof(int64_t)));
    if (!h->dims || !h->strides) {
        std::free(h->dims);
        std::free(h->strides);
        hoo_release(h);
        return nullptr;
    }
    h->flags |= TENSOR_OWNS_METADATA;
    std::memcpy(h->dims, dims, static_cast<size_t>(rank) * sizeof(int64_t));
    // Packed bit tensors report strides in bits; all other tensors report
    // byte strides. This keeps the metadata meaningful without pretending
    // that each bit occupies a byte.
    int64_t stride = element_type == TENSOR_ELEMENT_BIT ? 1 : element_bytes(element_type);
    for (int64_t axis = rank - 1; axis >= 0; --axis) {
        h->strides[axis] = stride;
        if (h->dims[axis] > 0 && stride > std::numeric_limits<int64_t>::max() / h->dims[axis]) {
            hoo_release(h);
            return nullptr;
        }
        stride *= h->dims[axis];
    }

    if (base) {
        h->base = static_cast<HooTensor>(hoo_retain(base));
        h->data_ptr = data(base) + offset_bytes;
    } else {
        h->data_ptr = static_cast<uint8_t*>(std::calloc(1, static_cast<size_t>(bytes)));
        if (!h->data_ptr) {
            hoo_release(h);
            return nullptr;
        }
        h->flags |= TENSOR_OWNS_DATA;
    }
    return h;
}

static uint8_t fp8_encode_e4m3(double value) {
    if (std::isnan(value)) return 0x7F;
    if (std::isinf(value)) return static_cast<uint8_t>(std::signbit(value) ? 0xF8 : 0x78);
    if (value == 0.0) return static_cast<uint8_t>(std::signbit(value) ? 0x80 : 0x00);

    const bool negative = std::signbit(value);
    const double magnitude = std::fabs(value);
    int exponent = 0;
    std::frexp(magnitude, &exponent);
    exponent -= 1;

    if (exponent < -6) {
        const int mantissa = static_cast<int>(std::round(std::ldexp(magnitude, 9)));
        return static_cast<uint8_t>((negative ? 0x80 : 0) | std::min(mantissa, 7));
    }
    if (exponent > 7) return static_cast<uint8_t>(negative ? 0xF8 : 0x78);

    const int exponent_bits = exponent + 7;
    const double normalized = std::ldexp(magnitude, -exponent) - 1.0;
    int mantissa = static_cast<int>(std::round(normalized * 8.0));
    int adjusted_exponent = exponent_bits;
    if (mantissa == 8) {
        mantissa = 0;
        ++adjusted_exponent;
    }
    if (adjusted_exponent >= 15) return static_cast<uint8_t>(negative ? 0xF8 : 0x78);
    return static_cast<uint8_t>((negative ? 0x80 : 0) |
                                ((adjusted_exponent & 0x0F) << 3) |
                                (mantissa & 0x07));
}

static double fp8_decode_e4m3(uint8_t encoded) {
    const bool negative = (encoded & 0x80) != 0;
    const int exponent_bits = (encoded >> 3) & 0x0F;
    const int mantissa = encoded & 0x07;
    double value = 0.0;
    if (exponent_bits == 0) {
        value = std::ldexp(static_cast<double>(mantissa), -9);
    } else if (exponent_bits == 15) {
        value = mantissa == 0 ? std::numeric_limits<double>::infinity()
                              : std::numeric_limits<double>::quiet_NaN();
    } else {
        value = std::ldexp(1.0 + static_cast<double>(mantissa) / 8.0, exponent_bits - 7);
    }
    return negative ? -value : value;
}

static int64_t checked_length(int64_t rank, int64_t d0, int64_t d1, int64_t d2) {
    if (rank < 1 || rank > 3) return 0;
    int64_t dims[3] = {d0, d1, d2};
    int64_t length = 1;
    for (int64_t i = 0; i < rank; ++i) {
        if (dims[i] <= 0) return 0;
        if (length > std::numeric_limits<int64_t>::max() / dims[i]) return 0;
        length *= dims[i];
    }
    return length;
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
    if (h->element_type == TENSOR_ELEMENT_INT8)
        return static_cast<int8_t>(data(tensor)[index]);
    if (h->element_type == TENSOR_ELEMENT_BYTE)
        return data(tensor)[index];
    if (h->element_type == TENSOR_ELEMENT_F8)
        return fp8_decode_e4m3(data(tensor)[index]);
    if (h->element_type == TENSOR_ELEMENT_F32) {
        float value = 0.0f;
        std::memcpy(&value, data(tensor) + index * sizeof(float), sizeof(value));
        return static_cast<double>(value);
    }
    if (h->element_type == TENSOR_ELEMENT_INT32) {
        int32_t value = 0;
        std::memcpy(&value, data(tensor) + index * sizeof(value), sizeof(value));
        return static_cast<double>(value);
    }
    int64_t bits = 0;
    std::memcpy(&bits, data(tensor) + index * 8, sizeof(bits));
    if (h->element_type == TENSOR_ELEMENT_F64) {
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    return static_cast<double>(bits);
}

static int64_t pack_numeric(int64_t element_type, double value) {
    if (element_type == TENSOR_ELEMENT_F8 || element_type == TENSOR_ELEMENT_F64) {
        int64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        return bits;
    }
    if (element_type == TENSOR_ELEMENT_F32) {
        float narrowed = static_cast<float>(value);
        int64_t bits = 0;
        std::memcpy(&bits, &narrowed, sizeof(narrowed));
        return bits;
    }
    if (element_type == TENSOR_ELEMENT_INT8)
        return static_cast<int8_t>(static_cast<int64_t>(value));
    if (element_type == TENSOR_ELEMENT_BYTE)
        return static_cast<uint8_t>(static_cast<int64_t>(value));
    if (element_type == TENSOR_ELEMENT_BIT)
        return value != 0.0;
    if (element_type == TENSOR_ELEMENT_INT32) return static_cast<int32_t>(value);
    return static_cast<int64_t>(value);
}

static int64_t result_element_type(HooTensor left, HooTensor right) {
    int64_t lt = header(left)->element_type;
    int64_t rt = header(right)->element_type;
    if (lt == TENSOR_ELEMENT_F64 || rt == TENSOR_ELEMENT_F64) return TENSOR_ELEMENT_F64;
    if (lt == TENSOR_ELEMENT_F32 || rt == TENSOR_ELEMENT_F32) return TENSOR_ELEMENT_F32;
    if (lt == TENSOR_ELEMENT_F8 || rt == TENSOR_ELEMENT_F8) return TENSOR_ELEMENT_F8;
    if (lt == TENSOR_ELEMENT_INT64 || rt == TENSOR_ELEMENT_INT64) return TENSOR_ELEMENT_INT64;
    if (lt == TENSOR_ELEMENT_INT32 || rt == TENSOR_ELEMENT_INT32) return TENSOR_ELEMENT_INT32;
    if (lt == TENSOR_ELEMENT_BIT && rt == TENSOR_ELEMENT_BIT) return TENSOR_ELEMENT_BIT;
    if (lt == rt) return lt;
    if ((lt == TENSOR_ELEMENT_BYTE && rt == TENSOR_ELEMENT_INT8) ||
        (lt == TENSOR_ELEMENT_INT8 && rt == TENSOR_ELEMENT_BYTE)) return TENSOR_ELEMENT_INT64;
    return lt;
}

static double scalar_numeric(int64_t scalar_bits, int64_t scalar_type) {
    if (scalar_type == TENSOR_ELEMENT_F64 || scalar_type == TENSOR_ELEMENT_F8) {
        double value = 0.0;
        std::memcpy(&value, &scalar_bits, sizeof(value));
        return value;
    }
    if (scalar_type == TENSOR_ELEMENT_INT8)
        return static_cast<int8_t>(scalar_bits);
    if (scalar_type == TENSOR_ELEMENT_BYTE)
        return static_cast<uint8_t>(scalar_bits);
    if (scalar_type == TENSOR_ELEMENT_F32) {
        float value = 0.0f;
        std::memcpy(&value, &scalar_bits, sizeof(value));
        return static_cast<double>(value);
    }
    if (scalar_type == TENSOR_ELEMENT_INT32)
        return static_cast<int32_t>(scalar_bits);
    if (scalar_type == TENSOR_ELEMENT_BIT)
        return scalar_bits != 0 ? 1.0 : 0.0;
    return static_cast<double>(scalar_bits);
}

static int64_t scalar_result_element_type(HooTensor tensor, int64_t scalar_type) {
    const int64_t tensor_type = header(tensor)->element_type;
    if (tensor_type == TENSOR_ELEMENT_F64 || scalar_type == TENSOR_ELEMENT_F64)
        return TENSOR_ELEMENT_F64;
    if (tensor_type == TENSOR_ELEMENT_F32 || scalar_type == TENSOR_ELEMENT_F32)
        return TENSOR_ELEMENT_F32;
    if (tensor_type == TENSOR_ELEMENT_F8 || scalar_type == TENSOR_ELEMENT_F8)
        return TENSOR_ELEMENT_F8;
    if (tensor_type == TENSOR_ELEMENT_INT64 || scalar_type == TENSOR_ELEMENT_INT64)
        return TENSOR_ELEMENT_INT64;
    if (tensor_type == TENSOR_ELEMENT_INT32 || scalar_type == TENSOR_ELEMENT_INT32)
        return TENSOR_ELEMENT_INT32;
    if (tensor_type == TENSOR_ELEMENT_BIT && scalar_type != TENSOR_ELEMENT_BIT)
        return TENSOR_ELEMENT_INT64;
    return tensor_type;
}

static HooTensor make_like(HooTensor source, int64_t element_type);

static HooTensor scalar_binary(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type,
                               int op, bool scalar_left) {
    if (!tensor || !header(tensor)) return nullptr;
    const double scalar = scalar_numeric(scalar_bits, scalar_type);
    const int64_t element_type = scalar_result_element_type(tensor, scalar_type);
    HooTensor result = make_like(tensor, element_type);
    if (!result) return nullptr;
    for (int64_t i = 0; i < header(tensor)->length; ++i) {
        const double value = get_numeric(tensor, i);
        const double left = scalar_left ? scalar : value;
        const double right = scalar_left ? value : scalar;
        double computed = 0.0;
        switch (op) {
            case 0: computed = left + right; break;
            case 1: computed = left - right; break;
            case 2: computed = left * right; break;
            case 3: computed = right == 0.0 ? 0.0 : left / right; break;
            default: return nullptr;
        }
        hoo_tensor_set_value(result, i, pack_numeric(element_type, computed));
    }
    return result;
}

static HooTensor make_like(HooTensor source, int64_t element_type) {
    auto* h = header(source);
    HooTensor result = nullptr;
    if (hoo_tensor_new_ex(element_type, h->rank, h->dims, &result) != HOO_STATUS_OK) return nullptr;
    return result;
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
    if (rank < 1 || rank > 3) return nullptr;
    const int64_t dims[3] = {d0, d1, d2};
    HooTensor result = nullptr;
    return hoo_tensor_new_ex(element_type, rank, dims, &result) == HOO_STATUS_OK ? result : nullptr;
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

HooStatus hoo_tensor_new_ex(int64_t element_type, int64_t rank,
                            const int64_t* dims, HooTensor* out) {
    if (!out) {
        set_error(HOO_STATUS_INVALID_ARGUMENT, "tensor output is null");
        return HOO_STATUS_INVALID_ARGUMENT;
    }
    *out = nullptr;
    if (!valid_element_type(element_type)) {
        set_error(HOO_STATUS_INVALID_DTYPE, "unsupported tensor element type");
        return HOO_STATUS_INVALID_DTYPE;
    }
    int64_t length = 0;
    if (!checked_shape(rank, dims, &length)) {
        set_error(HOO_STATUS_INVALID_SHAPE, "invalid tensor rank or dimensions");
        return HOO_STATUS_INVALID_SHAPE;
    }
    *out = allocate_tensor(element_type, rank, dims, nullptr, 0);
    if (!*out) {
        set_error(HOO_STATUS_OUT_OF_MEMORY, "tensor allocation failed");
        return HOO_STATUS_OUT_OF_MEMORY;
    }
    (void)length;
    hoo_ai_set_last_error(HOO_STATUS_OK, "");
    return HOO_STATUS_OK;
}

HooStatus hoo_tensor_shape(HooTensor tensor, int64_t capacity,
                           int64_t* dims, int64_t* out_count) {
    auto* h = header(tensor);
    if (!h || !out_count || capacity < 0 || (capacity > 0 && !dims)) {
        set_error(HOO_STATUS_INVALID_ARGUMENT, "invalid tensor shape query");
        return HOO_STATUS_INVALID_ARGUMENT;
    }
    *out_count = h->rank;
    if (capacity < h->rank) {
        set_error(HOO_STATUS_OUT_OF_BOUNDS, "shape output capacity is too small");
        return HOO_STATUS_OUT_OF_BOUNDS;
    }
    std::memcpy(dims, h->dims, static_cast<size_t>(h->rank) * sizeof(int64_t));
    hoo_ai_set_last_error(HOO_STATUS_OK, "");
    return HOO_STATUS_OK;
}

HooStatus hoo_tensor_strides(HooTensor tensor, int64_t capacity,
                             int64_t* strides, int64_t* out_count) {
    auto* h = header(tensor);
    if (!h || !out_count || capacity < 0 || (capacity > 0 && !strides)) {
        set_error(HOO_STATUS_INVALID_ARGUMENT, "invalid tensor stride query");
        return HOO_STATUS_INVALID_ARGUMENT;
    }
    *out_count = h->rank;
    if (capacity < h->rank) {
        set_error(HOO_STATUS_OUT_OF_BOUNDS, "stride output capacity is too small");
        return HOO_STATUS_OUT_OF_BOUNDS;
    }
    std::memcpy(strides, h->strides, static_cast<size_t>(h->rank) * sizeof(int64_t));
    hoo_ai_set_last_error(HOO_STATUS_OK, "");
    return HOO_STATUS_OK;
}

HooStatus hoo_tensor_numel(HooTensor tensor, int64_t* out_numel) {
    auto* h = header(tensor);
    if (!h || !out_numel) {
        set_error(HOO_STATUS_INVALID_ARGUMENT, "invalid tensor numel query");
        return HOO_STATUS_INVALID_ARGUMENT;
    }
    *out_numel = h->length;
    hoo_ai_set_last_error(HOO_STATUS_OK, "");
    return HOO_STATUS_OK;
}

HooStatus hoo_tensor_abi_version(HooTensor tensor, int32_t* out_version) {
    auto* h = header(tensor);
    if (!h || !out_version) {
        set_error(HOO_STATUS_INVALID_ARGUMENT, "invalid tensor ABI query");
        return HOO_STATUS_INVALID_ARGUMENT;
    }
    *out_version = static_cast<int32_t>(h->abi_version);
    hoo_ai_set_last_error(HOO_STATUS_OK, "");
    return HOO_STATUS_OK;
}

HooStatus hoo_tensor_copy(HooTensor tensor, HooTensor* out) {
    auto* h = header(tensor);
    if (!h || !out) {
        set_error(HOO_STATUS_INVALID_ARGUMENT, "invalid tensor copy argument");
        return HOO_STATUS_INVALID_ARGUMENT;
    }
    *out = nullptr;
    HooTensor result = nullptr;
    HooStatus status = hoo_tensor_new_ex(h->element_type, h->rank, h->dims, &result);
    if (status != HOO_STATUS_OK) return status;
    std::memcpy(data(result), data(tensor), static_cast<size_t>(h->storage_bytes));
    *out = result;
    hoo_ai_set_last_error(HOO_STATUS_OK, "");
    return HOO_STATUS_OK;
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
    if (h->element_type == TENSOR_ELEMENT_INT8 || h->element_type == TENSOR_ELEMENT_BYTE) {
        data(tensor)[index] = static_cast<uint8_t>(value_bits);
        return 1;
    }
    if (h->element_type == TENSOR_ELEMENT_F32) {
        float value = 0.0f;
        std::memcpy(&value, &value_bits, sizeof(value));
        std::memcpy(data(tensor) + index * sizeof(value), &value, sizeof(value));
        return 1;
    }
    if (h->element_type == TENSOR_ELEMENT_INT32) {
        int32_t value = static_cast<int32_t>(value_bits);
        std::memcpy(data(tensor) + index * sizeof(value), &value, sizeof(value));
        return 1;
    }
    if (h->element_type == TENSOR_ELEMENT_F8) {
        double value = 0.0;
        std::memcpy(&value, &value_bits, sizeof(value));
        data(tensor)[index] = fp8_encode_e4m3(value);
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
    if (h->element_type == TENSOR_ELEMENT_INT8 || h->element_type == TENSOR_ELEMENT_BYTE)
        return data(tensor)[index];
    if (h->element_type == TENSOR_ELEMENT_F8)
        return data(tensor)[index];
    if (h->element_type == TENSOR_ELEMENT_F32) {
        int64_t bits = 0;
        std::memcpy(&bits, data(tensor) + index * sizeof(float), sizeof(float));
        return bits;
    }
    if (h->element_type == TENSOR_ELEMENT_INT32) {
        int32_t value = 0;
        std::memcpy(&value, data(tensor) + index * sizeof(value), sizeof(value));
        return value;
    }
    int64_t bits = 0;
    std::memcpy(&bits, data(tensor) + index * 8, sizeof(bits));
    return bits;
}

HooTensor hoo_tensor_add(HooTensor left, HooTensor right) { return binary_numeric(left, right, 0); }
HooTensor hoo_tensor_sub(HooTensor left, HooTensor right) { return binary_numeric(left, right, 1); }
HooTensor hoo_tensor_element_mul(HooTensor left, HooTensor right) { return binary_numeric(left, right, 2); }
HooTensor hoo_tensor_element_div(HooTensor left, HooTensor right) { return binary_numeric(left, right, 3); }
HooTensor hoo_tensor_add_scalar_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type) {
    return scalar_binary(tensor, scalar_bits, scalar_type, 0, false);
}
HooTensor hoo_tensor_sub_scalar_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type) {
    return scalar_binary(tensor, scalar_bits, scalar_type, 1, false);
}
HooTensor hoo_tensor_sub_scalar_left_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type) {
    return scalar_binary(tensor, scalar_bits, scalar_type, 1, true);
}
HooTensor hoo_tensor_scale_scalar_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type) {
    return scalar_binary(tensor, scalar_bits, scalar_type, 2, false);
}
HooTensor hoo_tensor_div_scalar_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type) {
    return scalar_binary(tensor, scalar_bits, scalar_type, 3, false);
}
HooTensor hoo_tensor_div_scalar_left_bits(HooTensor tensor, int64_t scalar_bits, int64_t scalar_type) {
    return scalar_binary(tensor, scalar_bits, scalar_type, 3, true);
}
HooTensor hoo_tensor_add_scalar(HooTensor tensor, double scalar) {
    int64_t bits = 0; std::memcpy(&bits, &scalar, sizeof(bits));
    return hoo_tensor_add_scalar_bits(tensor, bits, TENSOR_ELEMENT_F64);
}
HooTensor hoo_tensor_sub_scalar(HooTensor tensor, double scalar) {
    int64_t bits = 0; std::memcpy(&bits, &scalar, sizeof(bits));
    return hoo_tensor_sub_scalar_bits(tensor, bits, TENSOR_ELEMENT_F64);
}
HooTensor hoo_tensor_sub_scalar_left(HooTensor tensor, double scalar) {
    int64_t bits = 0; std::memcpy(&bits, &scalar, sizeof(bits));
    return hoo_tensor_sub_scalar_left_bits(tensor, bits, TENSOR_ELEMENT_F64);
}
HooTensor hoo_tensor_scale_scalar(HooTensor tensor, double scalar) {
    int64_t bits = 0; std::memcpy(&bits, &scalar, sizeof(bits));
    return hoo_tensor_scale_scalar_bits(tensor, bits, TENSOR_ELEMENT_F64);
}
HooTensor hoo_tensor_div_scalar(HooTensor tensor, double scalar) {
    int64_t bits = 0; std::memcpy(&bits, &scalar, sizeof(bits));
    return hoo_tensor_div_scalar_bits(tensor, bits, TENSOR_ELEMENT_F64);
}
HooTensor hoo_tensor_div_scalar_left(HooTensor tensor, double scalar) {
    int64_t bits = 0; std::memcpy(&bits, &scalar, sizeof(bits));
    return hoo_tensor_div_scalar_left_bits(tensor, bits, TENSOR_ELEMENT_F64);
}

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

HooTensor hoo_tensor_reshape(HooTensor tensor, int64_t rank, int64_t d0, int64_t d1, int64_t d2) {
    if (!tensor || checked_length(rank, d0, d1, d2) != header(tensor)->length) return nullptr;
    HooTensor result = hoo_tensor_new(tensor ? header(tensor)->element_type : 0, rank, d0, d1, d2);
    if (!result) return nullptr;
    for (int64_t i = 0; i < header(tensor)->length; ++i) {
        hoo_tensor_set_value(result, i, pack_numeric(header(tensor)->element_type, get_numeric(tensor, i)));
    }
    return result;
}

HooTensor hoo_tensor_transpose(HooTensor tensor) {
    if (!tensor || header(tensor)->rank != 2) return nullptr;
    auto* source = header(tensor);
    HooTensor result = hoo_tensor_new2(source->element_type, source->dims[1], source->dims[0]);
    if (!result) return nullptr;
    for (int64_t row = 0; row < source->dims[0]; ++row) {
        for (int64_t col = 0; col < source->dims[1]; ++col) {
            const int64_t source_index = row * source->dims[1] + col;
            const int64_t result_index = col * source->dims[0] + row;
            hoo_tensor_set_value(result, result_index,
                                 pack_numeric(source->element_type, get_numeric(tensor, source_index)));
        }
    }
    return result;
}

HooTensor hoo_tensor_softmax(HooTensor tensor) {
    if (!tensor || header(tensor)->length <= 0) return nullptr;
    HooTensor result = make_like(tensor, TENSOR_ELEMENT_F64);
    if (!result) return nullptr;
    const int64_t length = header(tensor)->length;
    double maximum = get_numeric(tensor, 0);
    for (int64_t i = 1; i < length; ++i) maximum = std::max(maximum, get_numeric(tensor, i));
    double total = 0.0;
    for (int64_t i = 0; i < length; ++i) total += std::exp(get_numeric(tensor, i) - maximum);
    if (total == 0.0 || !std::isfinite(total)) return result;
    for (int64_t i = 0; i < length; ++i) {
        const double value = std::exp(get_numeric(tensor, i) - maximum) / total;
        hoo_tensor_set_value(result, i, pack_numeric(TENSOR_ELEMENT_F64, value));
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
