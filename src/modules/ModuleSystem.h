#pragma once

/**
 * @file ModuleSystem.h
 * @brief Module registry and hierarchical module management
 *
 * PURPOSE
 *   Central registry for resolving qualified names (hoo.String, hoo.io.File)
 *   to their corresponding exports. Used by HVMCodeGenerator to identify
 *   standard library classes and generate appropriate runtime calls.
 *
 * ARCHITECTURE
 *   ModuleRegistry (rootModules_) --> HooModule (exports_ + submodules_)
 *                                           |
 *                                    ModuleExport (kind, name, runtimeClassName)
 *
 *   hoo module is initialized on construction with String and Array exports.
 *   User modules can be added via addModule().
 *
 * USAGE IN CODE GENERATION
 *   HVMCodeGenerator holds a ModuleRegistry member. When generating
 *   constructor calls, it calls resolveQualifiedName() to check if a type
 *   is a standard library class (hoo.String, hoo.Array, etc.) and uses
 *   the returned ModuleExport to generate the appropriate runtime constructor.
 *
 * EXAMPLE
 *   ModuleRegistry registry;
 *   auto exp = registry.resolveQualifiedName({"hoo", "String"});
 *   if (exp) { // generate runtime constructor for HooString }
 *
 * THREAD SAFETY
 *   Not thread-safe; synchronize externally if needed.
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace hooc {

/**
 * @brief Represents an exported item in a module (class, function, variable).
 */
struct ModuleExport {
    enum class Kind {
        FUNCTION,
        CLASS,
        VARIABLE
    };

    Kind kind;
    std::string name;
    std::string runtimeClassName; // For CLASS kind
    bool isGeneric;

    ModuleExport() : kind(Kind::VARIABLE), isGeneric(false) {}

    ModuleExport(Kind k, const std::string& n, const std::string& rcn = "", bool g = false)
        : kind(k), name(n), runtimeClassName(rcn), isGeneric(g) {}
};

/**
 * @brief Represents a single module in the hierarchy.
 */
class HooModule {
public:
    explicit HooModule(const std::string& name) : name_(name) {}

    const std::string& getName() const { return name_; }

    void addExport(const ModuleExport& exportItem) {
        exports_[exportItem.name] = exportItem;
    }

    const ModuleExport* getExport(const std::string& name) const {
        auto it = exports_.find(name);
        if (it != exports_.end()) return &it->second;
        return nullptr;
    }

    void addSubmodule(std::unique_ptr<HooModule> submodule) {
        submodules_[submodule->getName()] = std::move(submodule);
    }

    HooModule* getSubmodule(const std::string& name) const {
        auto it = submodules_.find(name);
        if (it != submodules_.end()) return it->second.get();
        return nullptr;
    }

    const std::unordered_map<std::string, ModuleExport>& getExports() const {
        return exports_;
    }

private:
    std::string name_;
    std::unordered_map<std::string, ModuleExport> exports_;
    std::unordered_map<std::string, std::unique_ptr<HooModule>> submodules_;
};

/**
 * @brief Registry for resolving qualified names to exports.
 */
class ModuleRegistry {
public:
    ModuleRegistry();
    ~ModuleRegistry() = default;

    /**
     * Resolve a qualified name (e.g., {"std", "io", "File"}) to its export.
     */
    const ModuleExport* resolveQualifiedName(const std::vector<std::string>& path) const;

    /**
     * Resolve a module path to a Module object.
     */
    HooModule* resolveModulePath(const std::vector<std::string>& path) const;

    /**
     * Add a module to the registry.
     */
    void addModule(const std::vector<std::string>& path, std::unique_ptr<HooModule> module);

private:
    std::unordered_map<std::string, std::unique_ptr<HooModule>> rootModules_;
    HooModule* stdModule_ = nullptr;

    /**
     * Initialize standard modules (hoo.String, hoo.Array, etc.)
     */
    void initializeHooModule();

    /**
     * Helper to navigate module hierarchy
     */
    HooModule* navigateModulePath(const std::vector<std::string>& path) const;
};

} // namespace hooc
