/**
 * @file ModuleSystem.cpp
 * @brief Module registry and hierarchical module management
 *
 * RESPONSIBILITY
 *   - Central registry for all compiler modules (hoo, user-defined)
 *   - Resolution of qualified names (hoo.String, hoo.io.File) to exports
 *   - Hierarchical module navigation (hoo -> io -> File)
 *
 * IMPLEMENTATION
 *   - Module hierarchy stored as nested unique_ptr trees
 *   - Exports stored in unordered_map for O(1) lookup
 *   - hoo module initialized eagerly; others on-demand
 *   - Pointer caching (stdModule_) for fast hoo access
 *
 * USAGE
 *   ModuleRegistry registry;
 *
 *   // Resolve qualified name
 *   auto exp = registry.resolveQualifiedName({"hoo", "String"});
 *
 *   // Navigate module path
 *   auto mod = registry.resolveModulePath({"hoo", "io"});
 *
 *   // Add custom module
 *   registry.addModule({"myapp", "utils"}, std::make_unique<HooModule>("utils"));
 *
 * THREAD SAFETY
 *   - Not thread-safe; synchronize externally if needed
 */

#include "ModuleSystem.h"
#include <algorithm>

namespace hooc {

ModuleRegistry::ModuleRegistry() {
    initializeHooModule();
}

const ModuleExport* ModuleRegistry::resolveQualifiedName(const std::vector<std::string>& path) const {
    if (path.empty()) return nullptr;

    // Handle root modules
    auto it = rootModules_.find(path[0]);
    if (it == rootModules_.end()) return nullptr;

    HooModule* current = it->second.get();
    
    // Navigate hierarchy
    for (size_t i = 1; i < path.size() - 1; ++i) {
        current = current->getSubmodule(path[i]);
        if (!current) return nullptr;
    }

    // Get export from the last module in path
    return current->getExport(path.back());
}

HooModule* ModuleRegistry::resolveModulePath(const std::vector<std::string>& path) const {
    if (path.empty()) return nullptr;

    auto it = rootModules_.find(path[0]);
    if (it == rootModules_.end()) return nullptr;

    HooModule* current = it->second.get();
    for (size_t i = 1; i < path.size(); ++i) {
        current = current->getSubmodule(path[i]);
        if (!current) return nullptr;
    }

    return current;
}

void ModuleRegistry::addModule(const std::vector<std::string>& path, std::unique_ptr<HooModule> module) {
    if (path.empty()) return;

    // Handle root module separately
    if (path.size() == 1) {
        rootModules_[path[0]] = std::move(module);
        return;
    }

    // Ensure intermediate modules exist
    HooModule* current = nullptr;
    auto it = rootModules_.find(path[0]);
    if (it == rootModules_.end()) {
        auto newRoot = std::make_unique<HooModule>(path[0]);
        current = newRoot.get();
        rootModules_[path[0]] = std::move(newRoot);
    } else {
        current = it->second.get();
    }

    for (size_t i = 1; i < path.size() - 1; ++i) {
        HooModule* next = current->getSubmodule(path[i]);
        if (!next) {
            auto newSub = std::make_unique<HooModule>(path[i]);
            next = newSub.get();
            current->addSubmodule(std::move(newSub));
        }
        current = next;
    }

    // Add the final module as a submodule
    current->addSubmodule(std::move(module));
}

HooModule* ModuleRegistry::navigateModulePath(const std::vector<std::string>& path) const {
    return resolveModulePath(path);
}

void ModuleRegistry::initializeHooModule() {
    // Create the hoo module
    auto hooModule = std::make_unique<HooModule>("hoo");

    // Add String class to hoo module
    hooModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "String",
        "HooString",  // Runtime class name
        false         // Not generic
    ));

    // Add Array class to hoo module (generic)
    hooModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "Array",
        "HooArray",   // Runtime class name
        true          // Generic: Array<T>
    ));

    // Add Map class to hoo module (generic)
    hooModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "Map",
        "HooMap",     // Runtime class name
        true          // Generic: Map<K, V>
    ));

    // Add Exception class to hoo module
    hooModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "Exception",
        "HooException", // Runtime class name
        false           // Not generic
    ));

    // Cache the pointer before moving the unique_ptr
    stdModule_ = hooModule.get();

    // Add to registry
    rootModules_["hoo"] = std::move(hooModule);
}

} // namespace hooc
