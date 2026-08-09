#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace hooc {

struct ExternalFunctionInfo {
    std::string modulePath;
    std::string returnType;
    std::string returnClass;
    std::vector<std::string> parameterTypes;
};

using ExternalFunctionMetadataSets =
    std::unordered_map<std::string, std::vector<ExternalFunctionInfo>>;

/**
 * Opaque types for code generator output.
 * These hide the implementation details and allow different backends.
 */

// Forward declarations for opaque types
class GeneratedModule;
class GeneratedFunction;
class GeneratedValue;
class GeneratedType;

/**
 * Base class for generated modules (compilation units).
 * Concrete implementations will inherit from this.
 */
class GeneratedModule {
public:
    virtual ~GeneratedModule() = default;

    // Get the underlying implementation-specific module
    virtual void* getImplementation() = 0;
    virtual const void* getImplementation() const = 0;
};

/**
 * Base class for generated functions.
 * Concrete implementations will inherit from this.
 */
class GeneratedFunction {
public:
    virtual ~GeneratedFunction() = default;

    virtual void* getImplementation() = 0;
    virtual const void* getImplementation() const = 0;
};

/**
 * Base class for generated values/expressions.
 * Concrete implementations will inherit from this.
 */
class GeneratedValue {
public:
    virtual ~GeneratedValue() = default;

    virtual void* getImplementation() = 0;
    virtual const void* getImplementation() const = 0;
};

/**
 * Base class for generated types.
 * Concrete implementations will inherit from this.
 */
class GeneratedType {
public:
    virtual ~GeneratedType() = default;

    virtual void* getImplementation() = 0;
    virtual const void* getImplementation() const = 0;
};

} // namespace hooc
