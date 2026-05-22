#ifndef HVM_MODULE_BUNDLE_H
#define HVM_MODULE_BUNDLE_H

#include <set>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>

#include "hvm/HoModuleBase.h"
#include "core/SymbolMangler.h"

namespace hvm {

class ModuleBundle {
public:
    ModuleBundle() = default;
    ~ModuleBundle() = default;

    void addModule(std::shared_ptr<HoModuleBase> module);
    bool removeModule(const std::string& name);
    bool hasModule(const std::string& name) const;
    std::shared_ptr<HoModuleBase> getModule(const std::string& name) const;
    std::shared_ptr<HoModuleBase> findModuleBySymbol(const std::string& symbol_name) const;
    std::shared_ptr<HoModuleBase> findModuleBySymbolMangled(const std::string& mangled_name) const;

    std::vector<std::shared_ptr<HoModuleBase>> getAllModules() const;
    std::vector<std::string> getModuleNames() const;

    void clear();

    size_t size() const { return modules_by_name_.size(); }
    bool empty() const { return modules_by_name_.empty(); }

    std::vector<std::shared_ptr<HoModuleBase>> resolveDependencyOrder() const;

    std::vector<std::string> getModuleDependencyOrder(const std::string& module_name) const;
    std::vector<std::string> getAllDependentModules(const std::string& module_name) const;

    bool hasCircularDependency(const std::string& module_name) const;
    bool hasCircularDependency() const;

    static ModuleBundle* getModules();
    static void shutdown();

    std::shared_ptr<HoModuleBase> findModuleByNestedSymbol(const std::vector<std::string>& module_path,
                                                            const std::string& member_name) const;
    std::shared_ptr<HoModuleBase> findModuleByExport(const std::string& export_name) const;

    void registerExport(const std::string& module_name,
                       const std::string& symbol_name,
                       const std::string& mangled_name,
                       SymbolType kind);

    void registerNestedExport(const std::vector<std::string>& module_path,
                             const std::string& member_name,
                             const std::string& mangled_name,
                             SymbolType kind);

    void registerNamespaceExport(const std::string& namespace_name,
                               const std::string& member_name,
                               const std::string& mangled_name,
                               SymbolType kind);

    std::vector<std::string> findExportsByKind(SymbolType kind) const;
    std::vector<std::string> findExportsInNamespace(const std::string& namespace_name) const;

    bool hasExport(const std::string& symbol_name) const;
    bool hasNestedExport(const std::vector<std::string>& module_path,
                        const std::string& member_name) const;

    std::string mangleExport(const std::vector<std::string>& module_path,
                            const std::string& symbol_name,
                            SymbolType kind) const;

    std::string mangleNestedMember(const std::vector<std::string>& module_path,
                                  const std::string& member_name,
                                  SymbolType kind) const;

    std::string mangleNamespaceMember(const std::string& namespace_name,
                                     const std::string& member_name,
                                     SymbolType kind) const;

    hooc::DemangledSymbol demangleExport(const std::string& mangled_name) const;

    std::vector<std::string> getAllExportedSymbols() const;
    std::vector<std::string> getAllMangledExports() const;

private:
    struct ModuleComparator {
        bool operator()(const std::shared_ptr<HoModuleBase>& a,
                        const std::shared_ptr<HoModuleBase>& b) const {
            return a->getName() < b->getName();
        }
    };

    std::set<std::shared_ptr<HoModuleBase>, ModuleComparator> module_set_;
    std::unordered_map<std::string, std::shared_ptr<HoModuleBase>> modules_by_name_;
    std::unordered_map<std::string, std::unordered_set<std::string>> symbols_to_modules_;
    std::unordered_map<std::string, std::unordered_set<std::string>> mangled_symbols_to_modules_;

    std::unordered_map<std::string, SymbolType> exports_by_name_;
    std::unordered_map<std::string, std::unordered_set<std::string>> nested_exports_to_modules_;
    std::unordered_map<std::string, std::unordered_set<std::string>> namespace_exports_;
    std::unordered_map<std::string, std::string> mangled_to_original_;
};

}

#endif
