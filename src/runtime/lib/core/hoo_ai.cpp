#include "runtime/lib/core/hoo_ai.h"

#include <cstring>

namespace {
thread_local HooStatus g_last_status = HOO_STATUS_OK;
thread_local char g_last_error[256] = {};

/*
 * Single source of truth for dtype support. Both the feature strings
 * (via hoo_ai_has_feature) and the tensor module's dtype validation
 * (via hoo_ai_dtype_supported, embedded in valid_element_type) derive
 * from this table, so adding a dtype here enables both at once. Keep
 * the tensor module's own checks aligned to this list.
 *
 * NOTE: legacy element type id 3 (opaque/bool) is deliberately not in
 * this table; hoo_tensor.cpp still accepts it for legacy callers.
 */
constexpr int64_t kSupportedDtypes[] = {
    HOO_TENSOR_DTYPE_INT64, HOO_TENSOR_DTYPE_F64,
    HOO_TENSOR_DTYPE_INT8, HOO_TENSOR_DTYPE_BYTE,
    HOO_TENSOR_DTYPE_BIT, HOO_TENSOR_DTYPE_F8,
    HOO_TENSOR_DTYPE_F32, HOO_TENSOR_DTYPE_INT32,
};

struct AiFeatureEntry {
    const char* name;
    int64_t supported;
};

constexpr AiFeatureEntry kFeatures[] = {
    {"tensor_abi_v2", 1},
    {"tensor_dynamic_rank", 1},
    /* Dtype features resolve through the shared kSupportedDtypes table
       so introspection and tensor validation can never drift. */
    {"tensor_int64", HOO_TENSOR_DTYPE_INT64},
    {"tensor_f64", HOO_TENSOR_DTYPE_F64},
    {"tensor_int8", HOO_TENSOR_DTYPE_INT8},
    {"tensor_byte", HOO_TENSOR_DTYPE_BYTE},
    {"tensor_bit", HOO_TENSOR_DTYPE_BIT},
    {"tensor_f8", HOO_TENSOR_DTYPE_F8},
    {"tensor_f32", HOO_TENSOR_DTYPE_F32},
    {"tensor_int32", HOO_TENSOR_DTYPE_INT32},
    {"tensor_f16", HOO_TENSOR_DTYPE_F16},
    {"tensor_bf16", HOO_TENSOR_DTYPE_BF16},
};
}  // namespace

extern "C" {

int32_t hoo_ai_abi_version(void) {
    return HOO_TENSOR_ABI_VERSION;
}

HooStatus hoo_ai_last_status(void) {
    return g_last_status;
}

const char* hoo_ai_last_error(void) {
    return g_last_error;
}

void hoo_ai_set_last_error(HooStatus status, const char* message) {
    g_last_status = status;
    if (!message) message = "";
    std::strncpy(g_last_error, message, sizeof(g_last_error) - 1);
    g_last_error[sizeof(g_last_error) - 1] = '\0';
}

void hoo_ai_clear_last_error(void) {
    g_last_status = HOO_STATUS_OK;
    g_last_error[0] = '\0';
}

int64_t hoo_ai_dtype_supported(int64_t element_type) {
    for (const int64_t dtype : kSupportedDtypes) {
        if (element_type == dtype) return 1;
    }
    return 0;
}

int64_t hoo_ai_has_feature(const char* feature_name) {
    if (!feature_name) return 0;
    /* Dtype-prefixed features are gate-checked against the dtype table so
       that feature introspection and tensor validation can never drift. */
    for (const auto& feature : kFeatures) {
        if (std::strcmp(feature.name, feature_name) != 0) continue;
        if (feature.supported == 1) return 1;
        if (feature.supported < 0) return 0;
        return hoo_ai_dtype_supported(feature.supported);
    }
    return 0;
}

}  // extern "C"
