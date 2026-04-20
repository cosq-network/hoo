#include "hoo_exception.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <atomic>
#include <new>

// ============================================================================
// Internal Structure (Hidden from hoo Code)
// ============================================================================

struct HooExceptionImpl {
    std::atomic<int64_t> refcount;
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

static thread_local HooException currentException = nullptr;

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
    HooExceptionImpl* impl = (HooExceptionImpl*)std::malloc(sizeof(HooExceptionImpl));
    if (!impl) {
        std::fprintf(stderr, "ERROR: Out of memory allocating HooException\n");
        std::exit(1);
    }

    impl->refcount.store(1, std::memory_order_relaxed);
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
    impl->cause = nullptr;

    if (cause) {
        impl->cause = hoo_exception_retain(cause);
    }

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

    HooExceptionImpl* impl = (HooExceptionImpl*)std::malloc(sizeof(HooExceptionImpl));
    if (!impl) {
        std::fprintf(stderr, "ERROR: Out of memory allocating HooException\n");
        std::exit(1);
    }

    impl->refcount.store(1, std::memory_order_relaxed);
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
    if (!exc) return nullptr;
    get_impl(exc)->refcount.fetch_add(1, std::memory_order_relaxed);
    return exc;
}

void hoo_exception_release(HooException exc) {
    if (!exc) return;

    HooExceptionImpl* impl = get_impl(exc);
    int64_t oldCount = impl->refcount.fetch_sub(1, std::memory_order_release);

    if (oldCount <= 0) {
        std::fprintf(stderr, "ERROR: Exception refcount went negative! Value: %lld\n", (long long)oldCount);
        std::_Exit(1);
    }

    if (oldCount == 1) {
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
        if (impl->cause) hoo_exception_release(impl->cause);
        std::free(impl);
    }
}

int64_t hoo_exception_refcount(HooException exc) {
    if (!exc) return 0;
    return get_impl(exc)->refcount.load(std::memory_order_relaxed);
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

#ifdef __cplusplus
    throw HooStdException(exc);
#else
    std::fprintf(stderr, "ERROR: hoo_exception_throw requires C++ exception handling\n");
    std::_Exit(1);
#endif
}

HooException hoo_exception_current(void) {
    return currentException;
}

void hoo_exception_clear(void) {
    if (currentException) {
        hoo_exception_release(currentException);
        currentException = nullptr;
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
        return "<null>";
    }

    HooExceptionImpl* impl = get_impl(exc);

    char buffer[512];
    std::snprintf(buffer, sizeof(buffer),
        "HooException { typeId=%lld, type=\"%s\", message=\"%s\", refcount=%lld, hasCause=%s }",
        (long long)impl->typeId,
        impl->typeName,
        impl->message,
        (long long)impl->refcount.load(std::memory_order_relaxed),
        impl->cause ? "true" : "false"
    );

    return buffer;
}

#ifdef __cplusplus
}
#endif