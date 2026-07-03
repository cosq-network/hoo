#include "hoo_exception.h"
#include "hoo_runtime.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <atomic>
#include <new>
#include <mutex>

#ifdef _WIN32
#include <malloc.h>
static char* strndup(const char* s, size_t n) {
    size_t len = strnlen(s, n);
    char* p = (char*)malloc(len + 1);
    if (p) {
        memcpy(p, s, len);
        p[len] = '\0';
    }
    return p;
}
#endif

// ============================================================================
// Internal Structure (Hidden from hoo Code)
// ============================================================================

struct HooExceptionImpl {
    int64_t typeId;
    const char* typeName;
    const char* message;
    int64_t frameCount;
    char** frames;
    HooException cause;
};

static const char* type_names[] = {
    "RuntimeException",
    "NullPointerException",
    "IndexOutOfBoundsException",
    "DivisionByZeroException",
    "InvalidCastException",
    "CustomException"
};

#define FRAME_BUFFER_SIZE 64

#ifdef __cplusplus
#include <exception>
#include <typeinfo>
#include <stack>

static thread_local HooException currentException = nullptr;
static thread_local std::stack<void*> handlerStack;
static std::mutex gExceptionReleaseMu;

class HooStdException : public std::exception {
public:
    HooStdException(HooException exc) : exc_(exc) {}
    ~HooStdException() noexcept override {}

    const char* what() const noexcept override {
        return hoo_exception_get_message(exc_);
    }

    HooException getHooException() const { return exc_; }

private:
    HooException exc_;
};

extern "C" {
#endif

// ============================================================================
// Utility Functions
// ============================================================================

static HooExceptionImpl* get_impl(HooException exc) {
    return (HooExceptionImpl*)exc;
}

static HooException from_impl(HooExceptionImpl* impl) {
    return (HooException)impl;
}

static const char* empty_str(const char* s) {
    return s ? s : "";
}

// ============================================================================
// Creation and Destruction
// ============================================================================

HooException hoo_exception_create(int64_t typeId, const char* message) {
    return hoo_exception_create_with_cause(typeId, message, nullptr);
}

HooException hoo_exception_create_with_cause(int64_t typeId, const char* message, HooException cause) {
    HooExceptionImpl* impl = (HooExceptionImpl*)hoo_alloc(sizeof(HooExceptionImpl), HOO_TYPE_EXCEPTION);

    impl->typeId = typeId;
    impl->typeName = (typeId >= 0 && typeId < 5) ? type_names[typeId] : "CustomException";

    if (message) {
        size_t len = std::strlen(message);
        impl->message = (const char*)std::malloc(len + 1);
        std::memcpy((void*)impl->message, message, len + 1);
    } else {
        impl->message = "";
    }

    impl->frameCount = 0;
    impl->frames = nullptr;
    impl->cause = cause ? hoo_exception_retain(cause) : nullptr;

    return from_impl(impl);
}

HooException hoo_exception_runtime(const char* message) {
    return hoo_exception_create(HOO_EXCEPTION_RUNTIME, message);
}

HooException hoo_exception_null_pointer(const char* message) {
    const char* msg = message ? message : "Null reference accessed";
    return hoo_exception_create(HOO_EXCEPTION_NULL_POINTER, msg);
}

HooException hoo_exception_index_out_of_bounds(const char* message) {
    const char* msg = message ? message : "Index out of bounds";
    return hoo_exception_create(HOO_EXCEPTION_INDEX_OUT_OF_BOUNDS, msg);
}

HooException hoo_exception_division_by_zero(const char* message) {
    const char* msg = message ? message : "Division by zero";
    return hoo_exception_create(HOO_EXCEPTION_DIVISION_BY_ZERO, msg);
}

HooException hoo_exception_invalid_cast(const char* message) {
    const char* msg = message ? message : "Invalid cast";
    return hoo_exception_create(HOO_EXCEPTION_INVALID_CAST, msg);
}

HooException hoo_exception_custom(const char* exceptionType, const char* message) {
    if (!exceptionType) exceptionType = "CustomException";

    HooExceptionImpl* impl = (HooExceptionImpl*)hoo_alloc(sizeof(HooExceptionImpl), HOO_TYPE_EXCEPTION);

    impl->typeId = HOO_EXCEPTION_CUSTOM;

    size_t typeLen = std::strlen(exceptionType);
    impl->typeName = (const char*)std::malloc(typeLen + 1);
    std::memcpy((void*)impl->typeName, exceptionType, typeLen + 1);

    if (message) {
        size_t msgLen = std::strlen(message);
        impl->message = (const char*)std::malloc(msgLen + 1);
        std::memcpy((void*)impl->message, message, msgLen + 1);
    } else {
        impl->message = "";
    }

    impl->frameCount = 0;
    impl->frames = nullptr;
    impl->cause = nullptr;

    return from_impl(impl);
}

// ============================================================================
// Exception Properties
// ============================================================================

int64_t hoo_exception_get_type_id(HooException exc) {
    if (!exc) return 0;
    return get_impl(exc)->typeId;
}

const char* hoo_exception_get_type_name(HooException exc) {
    if (!exc) return "";
    return empty_str(get_impl(exc)->typeName);
}

const char* hoo_exception_get_message(HooException exc) {
    if (!exc) return "";
    return empty_str(get_impl(exc)->message);
}

int64_t hoo_exception_has_cause(HooException exc) {
    if (!exc) return 0;
    return get_impl(exc)->cause != nullptr;
}

HooException hoo_exception_get_cause(HooException exc) {
    if (!exc) return nullptr;
    return get_impl(exc)->cause;
}

const char* hoo_exception_get_stack_trace(HooException exc) {
    if (!exc) return "";

    HooExceptionImpl* impl = get_impl(exc);

    size_t totalSize = 256;
    for (int64_t i = 0; i < impl->frameCount; i++) {
        totalSize += std::strlen(impl->frames[i]) + 64;
    }

    char* buffer = (char*)std::malloc(totalSize);
    if (!buffer) return "";

    buffer[0] = '\0';

    std::snprintf(buffer, totalSize, "%s: %s",
        impl->typeName, impl->message);

    for (int64_t i = 0; i < impl->frameCount; i++) {
        std::snprintf(buffer + std::strlen(buffer), totalSize - std::strlen(buffer),
            "\n  at %s", impl->frames[i]);
    }

    HooException cause = impl->cause;
    while (cause) {
        HooExceptionImpl* causeImpl = get_impl(cause);
        std::snprintf(buffer + std::strlen(buffer), totalSize - std::strlen(buffer),
            "\nCaused by: %s: %s", causeImpl->typeName, causeImpl->message);
        cause = causeImpl->cause;
    }

    return buffer;
}

int64_t hoo_exception_get_frame_count(HooException exc) {
    if (!exc) return 0;
    return get_impl(exc)->frameCount;
}

const char* hoo_exception_get_frame(HooException exc, int64_t index) {
    if (!exc) return nullptr;

    HooExceptionImpl* impl = get_impl(exc);
    if (index < 0 || index >= impl->frameCount) return nullptr;

    return impl->frames[index];
}

// ============================================================================
// Reference Counting
// ============================================================================

HooException hoo_exception_retain(HooException exc) {
    return (HooException)hoo_retain(exc);
}

void hoo_exception_release(HooException exc) {
    hoo_release(exc);
}

int64_t hoo_exception_refcount(HooException exc) {
    return hoo_get_refcount(exc);
}

// ============================================================================
// Runtime Throwing
// ============================================================================

void hoo_exception_throw(HooException exc) {
    if (!exc) {
        std::fprintf(stderr, "ERROR: Cannot throw NULL exception\n");
        std::_Exit(1);
    }

    currentException = exc;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4297)
#endif
    throw HooStdException(exc);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

HooException hoo_exception_current(void) {
    return currentException;
}

void hoo_exception_set_current(HooException exc) {
    if (currentException && currentException != exc) {
        hoo_exception_release(currentException);
    }
    currentException = exc;
}

void hoo_exception_clear(void) {
    if (currentException) {
        hoo_exception_release(currentException);
        currentException = nullptr;
    }
}

void hoo_push_handler(void* handler_pc) {
    handlerStack.push(handler_pc);
}

void hoo_pop_handler(void) {
    if (!handlerStack.empty()) {
        handlerStack.pop();
    }
}

void hoo_exception_rethrow(void) {
    if (currentException) {
        hoo_exception_throw(currentException);
    } else {
        std::fprintf(stderr, "ERROR: No exception to rethrow\n");
        std::_Exit(1);
    }
}

// ============================================================================
// Comparison
// ============================================================================

int64_t hoo_exception_equals(HooException a, HooException b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;

    HooExceptionImpl* implA = get_impl(a);
    HooExceptionImpl* implB = get_impl(b);

    if (implA->typeId != implB->typeId) return 0;
    if (std::strcmp(implA->message, implB->message) != 0) return 0;

    return 1;
}

// ============================================================================
// Debugging
// ============================================================================

void hoo_exception_print(HooException exc) {
    if (!exc) {
        std::fprintf(stderr, "<null exception>");
        return;
    }

    HooExceptionImpl* impl = get_impl(exc);
    std::fprintf(stderr, "%s: %s", impl->typeName, impl->message);

    if (impl->frameCount > 0) {
        std::fprintf(stderr, "\nStack trace:");
        for (int64_t i = 0; i < impl->frameCount; i++) {
            std::fprintf(stderr, "\n  at %s", impl->frames[i]);
        }
    }

    HooException cause = impl->cause;
    while (cause) {
        HooExceptionImpl* causeImpl = get_impl(cause);
        std::fprintf(stderr, "\nCaused by: %s: %s", causeImpl->typeName, causeImpl->message);
        cause = causeImpl->cause;
    }

    std::fflush(stderr);
}

void hoo_exception_println(HooException exc) {
    hoo_exception_print(exc);
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
}

const char* hoo_exception_debug(HooException exc) {
    if (!exc) {
        return strdup("<null>");
    }

    HooExceptionImpl* impl = get_impl(exc);

    char buffer[512];
    int written = std::snprintf(buffer, sizeof(buffer),
        "HooException { typeId=%lld, type=\"%s\", message=\"%s\", refcount=%lld, hasCause=%s }",
        (long long)impl->typeId,
        impl->typeName,
        impl->message,
        (long long)hoo_get_refcount(exc),
        impl->cause ? "true" : "false"
    );

    if (written < 0) return strdup("");
    return strndup(buffer, (size_t)written);
}

static void exception_destructor(void* obj) {
    HooExceptionImpl* impl = (HooExceptionImpl*)obj;
    if (impl->message && impl->message[0] != '\0') {
        std::free((void*)impl->message);
    }
    if (impl->typeName && impl->typeId >= 5) {
        std::free((void*)impl->typeName);
    }
    for (int64_t i = 0; i < impl->frameCount; i++) {
        std::free(impl->frames[i]);
    }
    if (impl->frames) std::free(impl->frames);
    if (impl->cause) hoo_release(impl->cause);
}

namespace {
    struct ExceptionDestructorRegistrar {
        ExceptionDestructorRegistrar() {
            hoo_register_destructor(HOO_TYPE_EXCEPTION, exception_destructor);
        }
    } exc_registrar;
}

#ifdef __cplusplus
}
#endif
