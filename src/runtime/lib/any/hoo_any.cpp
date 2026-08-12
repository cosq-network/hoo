#include "runtime/lib/any/hoo_any.h"
#include "runtime/lib/runtime/hoo_runtime.h"

extern "C" {

int64_t hoo_any_is_managed(int64_t type_id) {
    return type_id >= HOO_TYPE_OBJECT ? 1 : 0;
}

void hoo_any_retain(HooAnyValue value) {
    if (hoo_any_is_managed(value.type_id) && value.data != 0) {
        hoo_retain(reinterpret_cast<void*>(static_cast<uintptr_t>(value.data)));
    }
}

void hoo_any_release(HooAnyValue value) {
    if (hoo_any_is_managed(value.type_id) && value.data != 0) {
        hoo_release(reinterpret_cast<void*>(static_cast<uintptr_t>(value.data)));
    }
}

}
