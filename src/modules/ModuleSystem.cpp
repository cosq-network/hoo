/**
 * @file ModuleSystem.cpp
 * @brief Module registry and hierarchical module management
 *
 * RESPONSIBILITY
 *   - Central registry for all compiler modules (std, user-defined)
 *   - Resolution of qualified names (std.String, std.io.File) to exports
 *   - Hierarchical module navigation (std -> io -> File)
 *
 * IMPLEMENTATION
 *   - Module hierarchy stored as nested unique_ptr trees
 *   - Exports stored in unordered_map for O(1) lookup
 *   - std module initialized eagerly; others on-demand
 *   - Pointer caching (stdModule_) for fast std access
 *
 * USAGE
 *   ModuleRegistry registry;
 *
 *   // Resolve qualified name
 *   auto exp = registry.resolveQualifiedName({"std", "String"});
 *
 *   // Navigate module path
 *   auto mod = registry.resolveModulePath({"std", "io"});
 *
 *   // Add custom module
 *   registry.addModule({"myapp", "utils"}, std::make_unique<Module>("utils"));
 *
 * THREAD SAFETY
 *   - Not thread-safe; synchronize externally if needed
 */

#include "ModuleSystem.h"

namespace hooc {

// ============================================================================
// Module Implementation
// ============================================================================

Module::Module(const std::string& name)
    : name_(name) {}

void Module::addExport(const ModuleExport& export_) {
    exports_[export_.name] = export_;
}

const ModuleExport* Module::getExport(const std::string& name) const {
    auto it = exports_.find(name);
    if (it != exports_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Module::hasExport(const std::string& name) const {
    return exports_.find(name) != exports_.end();
}

void Module::addSubmodule(const std::string& name, std::unique_ptr<Module> submodule) {
    submodules_[name] = std::move(submodule);
}

Module* Module::getSubmodule(const std::string& name) const {
    auto it = submodules_.find(name);
    if (it != submodules_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool Module::hasSubmodule(const std::string& name) const {
    return submodules_.find(name) != submodules_.end();
}

// ============================================================================
// ModuleRegistry Implementation
// ============================================================================

ModuleRegistry::ModuleRegistry()
    : stdModule_(nullptr) {
    initializeStdModule();
}

const ModuleExport* ModuleRegistry::resolveQualifiedName(const ast::QualifiedIdentifier& qid) const {
    const auto& components = qid.getComponents();

    if (components.empty() || components.size() == 1) {
        return nullptr;
    }

    // Multi-component identifier - navigate module hierarchy
    Module* currentModule = nullptr;

    // Get the root module (e.g., "std" from "std.String" or "std.io.File")
    auto rootIt = rootModules_.find(components[0]);
    if (rootIt == rootModules_.end()) {
        return nullptr;
    }
    currentModule = rootIt->second.get();

    // Navigate through intermediate modules (e.g., "io" in "std.io.File")
    for (size_t i = 1; i < components.size() - 1; ++i) {
        currentModule = currentModule->getSubmodule(components[i]);
        if (!currentModule) {
            return nullptr;
        }
    }

    // Get the final export (e.g., "File" from "std.io.File")
    return currentModule->getExport(components.back());
}

Module* ModuleRegistry::resolveModulePath(const std::vector<std::string>& path) const {
    if (path.empty()) {
        return nullptr;
    }

    return navigateModulePath(path);
}

const Module* ModuleRegistry::getStdModule() const {
    return stdModule_;
}

void ModuleRegistry::addModule(const std::vector<std::string>& path, std::unique_ptr<Module> module) {
    if (path.empty()) {
        return;
    }

    if (path.size() == 1) {
        // Root module
        rootModules_[path[0]] = std::move(module);
        return;
    }

    // Navigate/create hierarchy for nested modules
    Module* currentModule = nullptr;
    auto rootIt = rootModules_.find(path[0]);
    if (rootIt == rootModules_.end()) {
        // Create root module if it doesn't exist
        auto rootModule = std::make_unique<Module>(path[0]);
        currentModule = rootModule.get();
        rootModules_[path[0]] = std::move(rootModule);
    } else {
        currentModule = rootIt->second.get();
    }

    // Navigate/create intermediate modules
    for (size_t i = 1; i < path.size() - 1; ++i) {
        if (!currentModule->hasSubmodule(path[i])) {
            currentModule->addSubmodule(path[i], std::make_unique<Module>(path[i]));
        }
        currentModule = currentModule->getSubmodule(path[i]);
    }

    // Add the final module
    currentModule->addSubmodule(path.back(), std::move(module));
}

Module* ModuleRegistry::navigateModulePath(const std::vector<std::string>& path) const {
    if (path.empty()) {
        return nullptr;
    }

    // Get root module
    auto it = rootModules_.find(path[0]);
    if (it == rootModules_.end()) {
        return nullptr;
    }

    Module* currentModule = it->second.get();

    // Navigate through submodules
    for (size_t i = 1; i < path.size(); ++i) {
        currentModule = currentModule->getSubmodule(path[i]);
        if (!currentModule) {
            return nullptr;
        }
    }

    return currentModule;
}

void ModuleRegistry::initializeStdModule() {
    // Create the std module
    auto stdModule = std::make_unique<Module>("std");

    // Add String class to std module
    stdModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "String",
        "HooString",  // Runtime class name
        false         // Not generic
    ));

    // Add Array class to std module (generic)
    stdModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "Array",
        "HooArray",   // Runtime class name
        true          // Generic: Array<T>
    ));

    // Cache the pointer before moving the unique_ptr
    stdModule_ = stdModule.get();

    // Add to registry
    rootModules_["std"] = std::move(stdModule);

    // Optionally, you can add nested modules like std.io.File here
    // For now, we'll keep it simple and add them later as needed
}

} // namespace hooc
