#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace hooc {
namespace runtime {

// ============================================================================
// Runtime Method Descriptor
// ============================================================================

struct RuntimeMethodDescriptor {
    const char* hoocMethodName;     // Method name in hooc (e.g., "length")
    const char* runtimeFuncName;    // Runtime function name (e.g., "hoo_string_length")
    bool isStatic;                   // Whether this is a static method
};

// ============================================================================
// Runtime Class Descriptor
// ============================================================================

struct RuntimeClassDescriptor {
    const char* className;           // Hooc class name (e.g., "string")
    const char* hoocTypeName;       // Type name used in hooc code (e.g., "string")
    const RuntimeMethodDescriptor* methods;
    size_t methodCount;
};

// ============================================================================
// Runtime Method Registry
// ============================================================================

class RuntimeMethodRegistry {
public:
    static RuntimeMethodRegistry& getInstance();

    void registerClass(const RuntimeClassDescriptor& classDesc);

    const RuntimeClassDescriptor* findClass(const std::string& className) const;

    const RuntimeMethodDescriptor* findMethod(
        const std::string& className,
        const std::string& methodName) const;

    bool isRuntimeClass(const std::string& className) const;

private:
    RuntimeMethodRegistry() = default;
    std::unordered_map<std::string, const RuntimeClassDescriptor*> classes_;
};

// ============================================================================
// Macro for declaring runtime class methods
// ============================================================================

#define BEGIN_RUNTIME_CLASS(ClassName, HoocTypeName) \
    static const ::hooc::runtime::RuntimeMethodDescriptor \
        _runtime_methods_##ClassName[] = {

#define RUNTIME_METHOD(HoocName, RuntimeFunc) \
    { #HoocName, RuntimeFunc, false },

#define END_RUNTIME_CLASS(ClassName, HoocTypeName) \
    }; \
    static ::hooc::runtime::RuntimeClassDescriptor \
        _runtime_class_##ClassName##_desc = { \
            #ClassName, \
            HoocTypeName, \
            _runtime_methods_##ClassName, \
            sizeof(_runtime_methods_##ClassName) / sizeof(_runtime_methods_##ClassName[0]) \
    }; \
    static ::hooc::runtime::RuntimeMethodAutoRegister \
        _runtime_class_##ClassName##_registrar(&_runtime_class_##ClassName##_desc);

#define REGISTER_RUNTIME_CLASS(ClassName, HoocTypeName) \
    ::hooc::runtime::RuntimeMethodRegistry::getInstance().registerClass( \
        _runtime_class_##ClassName##_desc)

// ============================================================================
// Auto-registration helper
// ============================================================================

class RuntimeMethodAutoRegister {
public:
    explicit RuntimeMethodAutoRegister(const RuntimeClassDescriptor* desc) {
        RuntimeMethodRegistry::getInstance().registerClass(*desc);
    }
};

} // namespace runtime
} // namespace hooc
