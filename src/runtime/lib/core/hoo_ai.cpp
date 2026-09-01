#include "runtime/lib/core/hoo_ai.h"

#include <cstring>

namespace {
thread_local HooStatus g_last_status = HOO_STATUS_OK;
thread_local char g_last_error[256] = {};
}

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

int64_t hoo_ai_has_feature(const char* feature_name) {
    if (!feature_name) return 0;
    if (std::strcmp(feature_name, "tensor_abi_v2") == 0) return 1;
    if (std::strcmp(feature_name, "tensor_dynamic_rank") == 0) return 1;
    if (std::strcmp(feature_name, "tensor_f32") == 0) return 1;
    if (std::strcmp(feature_name, "tensor_int32") == 0) return 1;
    if (std::strcmp(feature_name, "tensor_f16") == 0) return 0;
    if (std::strcmp(feature_name, "tensor_bf16") == 0) return 0;
    return 0;
}

}
