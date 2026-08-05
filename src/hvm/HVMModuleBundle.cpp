#include "hvm/HVMModuleBundle.h"

#include <algorithm>

namespace hvm {
namespace {
std::string symbolTypeTag(SymbolType kind) {
    switch (kind) {
        case SymbolType::NoType: return "nt";
        case SymbolType::Function: return "fn";
        case SymbolType::Object: return "ob";
        case SymbolType::Type: return "ty";
        case SymbolType::TLS: return "tls";
        default: return "uk";
    }
}
} // namespace

HVMModuleBundle& HVMModuleBundle::getModules() {
    static HVMModuleBundle instance;
    return instance;
}

void HVMModuleBundle::shutdown() {
    getModules().clear();
}

void HVMModuleBundle::addModule(std::shared_ptr<HOModuleBase> module) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!module) return;

    const std::string& name = module->getName();
    if (modules_by_name_.count(name)) {
        return;
    }

    auto [set_it, inserted] = module_set_.insert(module);
    modules_by_name_[name] = {module, set_it};

    for (const auto& sym : module->findSymbolsByPrefix("")) {
        if (sym) {
            symbols_to_modules_[sym->name].insert(name);
            if (!sym->mangled_name.empty()) {
                mangled_symbols_to_modules_[sym->mangled_name].insert(name);
            }
        }
    }
}

bool HVMModuleBundle::removeModule(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = modules_by_name_.find(name);
    if (it == modules_by_name_.end()) {
        return false;
    }

    auto module = it->second.module;
    for (const auto& sym : module->findSymbolsByPrefix("")) {
        if (sym) {
            auto sym_it = symbols_to_modules_.find(sym->name);
            if (sym_it != symbols_to_modules_.end()) {
                sym_it->second.erase(name);
                if (sym_it->second.empty()) {
                    symbols_to_modules_.erase(sym_it);
                }
            }
            if (!sym->mangled_name.empty()) {
                auto mangled_it = mangled_symbols_to_modules_.find(sym->mangled_name);
                if (mangled_it != mangled_symbols_to_modules_.end()) {
                    mangled_it->second.erase(name);
                    if (mangled_it->second.empty()) {
                        mangled_symbols_to_modules_.erase(mangled_it);
                    }
                }
            }
        }
    }

    module_set_.erase(it->second.set_iterator);
    modules_by_name_.erase(it);

    return true;
}

bool HVMModuleBundle::hasModule(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return modules_by_name_.count(name) > 0;
}

std::shared_ptr<HOModuleBase> HVMModuleBundle::getModule(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = modules_by_name_.find(name);
    if (it != modules_by_name_.end()) {
        return it->second.module;
    }
    return nullptr;
}

std::shared_ptr<HOModuleBase> HVMModuleBundle::findModuleBySymbol(const std::string& symbol_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = symbols_to_modules_.find(symbol_name);
    if (it != symbols_to_modules_.end() && !it->second.empty()) {
        const std::string& module_name = *it->second.begin();
        return getModule(module_name);
    }
    return nullptr;
}

std::shared_ptr<HOModuleBase> HVMModuleBundle::findModuleBySymbolMangled(const std::string& mangled_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = mangled_symbols_to_modules_.find(mangled_name);
    if (it != mangled_symbols_to_modules_.end() && !it->second.empty()) {
        const std::string& module_name = *it->second.begin();
        return getModule(module_name);
    }
    return nullptr;
}

std::vector<std::shared_ptr<HOModuleBase>> HVMModuleBundle::getAllModules() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return std::vector<std::shared_ptr<HOModuleBase>>(module_set_.begin(), module_set_.end());
}

std::vector<std::string> HVMModuleBundle::getModuleNames() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(modules_by_name_.size());
    for (const auto& pair : modules_by_name_) {
        names.push_back(pair.first);
    }
    return names;
}

void HVMModuleBundle::clear() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    module_set_.clear();
    modules_by_name_.clear();
    symbols_to_modules_.clear();
    mangled_symbols_to_modules_.clear();
    exports_by_name_.clear();
    nested_exports_to_modules_.clear();
    namespace_exports_.clear();
    mangled_to_original_.clear();
}

std::vector<std::shared_ptr<HOModuleBase>> HVMModuleBundle::resolveDependencyOrder() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::shared_ptr<HOModuleBase>> result;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursion_stack;
    bool cycle_found = false;

    std::function<void(const std::shared_ptr<HOModuleBase>&)> visit =
        [&](const std::shared_ptr<HOModuleBase>& module) {
            const std::string& name = module->getName();
            if (visited.count(name)) return;
            if (recursion_stack.count(name)) {
                cycle_found = true;
                return;
            }

            recursion_stack.insert(name);

            for (const auto& dep : module->getDependencies()) {
                auto dep_it = modules_by_name_.find(dep.module_name);
                if (dep_it != modules_by_name_.end() && dep_it->second.module) {
                    visit(dep_it->second.module);
                }
            }

            recursion_stack.erase(name);
            visited.insert(name);
            result.push_back(module);
        };

    for (const auto& pair : modules_by_name_) {
        visit(pair.second.module);
    }

    // Keep the per-module cycle state consistent with the bundle-level walk.
    // This also ensures hasCircularDependency(name) and the bundle traversal
    // agree for transitive cycles.
    if (cycle_found) {
        std::vector<std::shared_ptr<HOModuleBase>> all_modules;
        all_modules.reserve(modules_by_name_.size());
        for (const auto& pair : modules_by_name_) {
            all_modules.push_back(pair.second.module);
        }
        for (const auto& pair : modules_by_name_) {
            pair.second.module->resolveDependencyOrder(all_modules);
        }
    }

    return result;
}

std::vector<std::string> HVMModuleBundle::getModuleDependencyOrder(const std::string& module_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto module = getModule(module_name);
    if (!module) return {};

    std::vector<std::shared_ptr<HOModuleBase>> all_modules;
    all_modules.reserve(modules_by_name_.size());
    for (const auto& pair : modules_by_name_) {
        all_modules.push_back(pair.second.module);
    }

    module->resolveDependencyOrder(all_modules);
    return module->getDependencyOrder();
}

std::vector<std::string> HVMModuleBundle::getAllModulesThatDependOn(const std::string& module_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> dependents;

    std::function<void(const std::string&, std::unordered_set<std::string>&)> find_deps =
        [&](const std::string& name, std::unordered_set<std::string>& visited) {
            if (visited.count(name)) return;
            visited.insert(name);

            for (const auto& pair : modules_by_name_) {
                if (pair.second.module->hasDependency(name)) {
                    find_deps(pair.first, visited);
                    dependents.push_back(pair.first);
                }
            }
        };

    std::unordered_set<std::string> visited;
    find_deps(module_name, visited);

    return dependents;
}

bool HVMModuleBundle::hasCircularDependency(const std::string& module_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto module = getModule(module_name);
    if (!module) return false;

    std::vector<std::shared_ptr<HOModuleBase>> all_modules;
    all_modules.reserve(modules_by_name_.size());
    for (const auto& pair : modules_by_name_) {
        all_modules.push_back(pair.second.module);
    }

    module->resolveDependencyOrder(all_modules);
    return module->hasCircularDependency();
}

bool HVMModuleBundle::hasCircularDependency() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const auto& pair : modules_by_name_) {
        auto module = pair.second.module;
        std::vector<std::shared_ptr<HOModuleBase>> all_modules;
        all_modules.reserve(modules_by_name_.size());
        for (const auto& p : modules_by_name_) {
            all_modules.push_back(p.second.module);
        }
        module->resolveDependencyOrder(all_modules);
        if (module->hasCircularDependency()) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<HOModuleBase> HVMModuleBundle::findModuleByNestedSymbol(const std::vector<std::string>& module_path,
                                                            const std::string& member_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::string key;
    for (size_t i = 0; i < module_path.size(); ++i) {
        if (i > 0) key += ".";
        key += module_path[i];
    }
    if (!member_name.empty()) {
        key += "." + member_name;
    }

    auto it = nested_exports_to_modules_.find(key);
    if (it != nested_exports_to_modules_.end() && !it->second.empty()) {
        const std::string& module_name = *it->second.begin();
        return getModule(module_name);
    }
    return nullptr;
}

std::shared_ptr<HOModuleBase> HVMModuleBundle::findModuleByExport(const std::string& export_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = exports_by_name_.find(export_name);
    if (it != exports_by_name_.end()) {
        auto nested_it = nested_exports_to_modules_.find(export_name);
        if (nested_it != nested_exports_to_modules_.end() && !nested_it->second.empty()) {
            return getModule(*nested_it->second.begin());
        }
    }
    return findModuleBySymbol(export_name);
}

void HVMModuleBundle::registerExport(const std::string& module_name,
                                const std::string& symbol_name,
                                const std::string& mangled_name,
                                SymbolType kind) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!hasModule(module_name)) return;
    exports_by_name_[symbol_name] = kind;
    mangled_to_original_[mangled_name] = symbol_name;
    symbols_to_modules_[symbol_name].insert(module_name);
    if (!mangled_name.empty()) {
        mangled_symbols_to_modules_[mangled_name].insert(module_name);
    }
}

void HVMModuleBundle::registerNestedExport(const std::vector<std::string>& module_path,
                                        const std::string& member_name,
                                        const std::string& mangled_name,
                                        SymbolType kind) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (module_path.empty()) {
        return;
    }

    std::string key;
    for (size_t i = 0; i < module_path.size(); ++i) {
        if (i > 0) key += ".";
        key += module_path[i];
    }
    if (!member_name.empty()) {
        key += "." + member_name;
    }

    auto module = getModule(module_path.back());
    if (module) {
        nested_exports_to_modules_[key].insert(module->getName());
        mangled_to_original_[mangled_name] = key;
        exports_by_name_[key] = kind;
    }
}

void HVMModuleBundle::registerNamespaceExport(const std::string& namespace_name,
                                      const std::string& member_name,
                                      const std::string& mangled_name,
                                      SymbolType kind) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::string key = namespace_name + "." + member_name;
    namespace_exports_[namespace_name].insert(member_name);
    exports_by_name_[key] = kind;
    mangled_to_original_[mangled_name] = key;
}

std::vector<std::string> HVMModuleBundle::findExportsByKind(SymbolType kind) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& pair : exports_by_name_) {
        if (pair.second == kind) {
            result.push_back(pair.first);
        }
    }
    return result;
}

std::vector<std::string> HVMModuleBundle::findExportsInNamespace(const std::string& namespace_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = namespace_exports_.find(namespace_name);
    if (it != namespace_exports_.end()) {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

bool HVMModuleBundle::hasExport(const std::string& symbol_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return exports_by_name_.count(symbol_name) > 0;
}

bool HVMModuleBundle::hasNestedExport(const std::vector<std::string>& module_path,
                                 const std::string& member_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::string key;
    for (size_t i = 0; i < module_path.size(); ++i) {
        if (i > 0) key += ".";
        key += module_path[i];
    }
    if (!member_name.empty()) {
        key += "." + member_name;
    }
    return nested_exports_to_modules_.count(key) > 0;
}

std::string HVMModuleBundle::mangleExport(const std::vector<std::string>& module_path,
                                     const std::string& symbol_name,
                                     SymbolType kind) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const std::string kindTag = symbolTypeTag(kind);
    return hooc::SymbolMangler::mangleModuleSymbol(module_path, symbol_name, kindTag);
}

std::string HVMModuleBundle::mangleNestedMember(const std::vector<std::string>& module_path,
                                         const std::string& member_name,
                                         SymbolType kind) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> path = module_path;
    path.push_back("nested");
    return mangleExport(path, member_name, kind);
}

std::string HVMModuleBundle::mangleNamespaceMember(const std::string& namespace_name,
                                          const std::string& member_name,
                                          SymbolType kind) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> path = {"ns", namespace_name};
    return mangleExport(path, member_name, kind);
}

hooc::DemangledSymbol HVMModuleBundle::demangleExport(const std::string& mangled_name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return hooc::SymbolMangler::demangleSymbol(mangled_name);
}

std::vector<std::string> HVMModuleBundle::getAllExportedSymbols() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& pair : exports_by_name_) {
        result.push_back(pair.first);
    }
    return result;
}

std::vector<std::string> HVMModuleBundle::getAllMangledExports() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& pair : mangled_to_original_) {
        result.push_back(pair.first);
    }
    return result;
}

}
