#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ast/QualifiedIdentifier.h"

namespace hooc {

/**
 * ModuleExport represents an exported entity from a module
 * (class, function, type, etc.)
 */
struct ModuleExport {
    enum class Kind {
        CLASS,      // Class type (e.g., std.String)
        FUNCTION,   // Standalone function
        TYPE        // Type alias
    };

    Kind kind;
    std::string name;              // Short name (e.g., "String")
    std::string runtimeClassName;  // Maps to runtime class for code generation
    bool isGeneric;                // true for generic types like Array<T>

    // Default constructor (required for use in unordered_map)
    ModuleExport()
        : kind(Kind::CLASS), name(""), runtimeClassName(""), isGeneric(false) {}

    // Parameterized constructor
    ModuleExport(Kind k, const std::string& n, const std::string& rtName, bool generic = false)
        : kind(k), name(n), runtimeClassName(rtName), isGeneric(generic) {}
};

/**
 * Module represents a hierarchical module with exports and submodules
 * Supports structure like: std.io.File, std.collections.Map, etc.
 */
class Module {
public:
    Module(const std::string& name);

    // Export management
    void addExport(const ModuleExport& export_);
    const ModuleExport* getExport(const std::string& name) const;
    bool hasExport(const std::string& name) const;

    // Submodule management (hierarchical)
    void addSubmodule(const std::string& name, std::unique_ptr<Module> submodule);
    Module* getSubmodule(const std::string& name) const;
    bool hasSubmodule(const std::string& name) const;

    // Accessors
    const std::string& getName() const { return name_; }
    const std::unordered_map<std::string, ModuleExport>& getExports() const { return exports_; }

private:
    std::string name_;
    std::unordered_map<std::string, ModuleExport> exports_;
    std::unordered_map<std::string, std::unique_ptr<Module>> submodules_;
};

/**
 * ModuleRegistry is the central registry for all modules in the system
 * Provides resolution of qualified names to exports
 *
 * Example usage:
 *   ModuleRegistry registry;
 *
 *   // Resolve std.String
 *   auto exp = registry.resolveQualifiedName(QualifiedIdentifier({"std", "String"}));
 *
 *   // Resolve module path for std.io
 *   auto ioModule = registry.resolveModulePath({"std", "io"});
 */
class ModuleRegistry {
public:
    ModuleRegistry();

    /**
     * Resolve a qualified identifier to its export metadata
     * Examples:
     *   QualifiedIdentifier({"std", "String"}) -> String class export
     *   QualifiedIdentifier({"std", "Array"}) -> Array class export
     *   QualifiedIdentifier({"std", "io", "File"}) -> io.File class export
     */
    const ModuleExport* resolveQualifiedName(const ast::QualifiedIdentifier& qid) const;

    /**
     * Resolve a module path to a Module pointer
     * Returns nullptr if path doesn't exist
     */
    Module* resolveModulePath(const std::vector<std::string>& path) const;

    /**
     * Get the std module (always exists)
     */
    Module* getStdModule() const;

    /**
     * Add a module to the registry at a specific path
     * Path: {"std", "io"} creates std.io module
     */
    void addModule(const std::vector<std::string>& path, std::unique_ptr<Module> module);

private:
    std::unordered_map<std::string, std::unique_ptr<Module>> rootModules_;
    Module* stdModule_;  // Cached pointer to std module for fast access

    /**
     * Initialize standard library modules (std.String, std.Array, etc.)
     */
    void initializeStdModule();

    /**
     * Helper to navigate module hierarchy
     */
    Module* navigateModulePath(const std::vector<std::string>& path) const;
};

} // namespace hooc
