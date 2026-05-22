#include "hvm/ModuleBundle.h"

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

ModuleBundle* ModuleBundle::getModules() {
    static auto instance = std::make_unique<ModuleBundle>();
    return instance.get();
}

void ModuleBundle::shutdown() {
    getModules()->clear();
}

void ModuleBundle::addModule(std::shared_ptr<HoModuleBase> module) {
    if (!module) return;

    const std::string& name = module->getName();
    if (modules_by_name_.count(name)) {
        return;
    }

    module_set_.insert(module);
    modules_by_name_[name] = module;

    for (const auto& sym : module->findSymbolsByPrefix("")) {
        if (sym) {
            symbols_to_modules_[sym->name].insert(name);
            if (!sym->mangled_name.empty()) {
                mangled_symbols_to_modules_[sym->mangled_name].insert(name);
            }
        }
    }
}

bool ModuleBundle::removeModule(const std::string& name) {
    auto it = modules_by_name_.find(name);
    if (it == modules_by_name_.end()) {
        return false;
    }

    auto module = it->second;
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

    modules_by_name_.erase(it);

    for (auto mod_it = module_set_.begin(); mod_it != module_set_.end(); ++mod_it) {
        if ((*mod_it)->getName() == name) {
            module_set_.erase(mod_it);
            break;
        }
    }

    return true;
}

bool ModuleBundle::hasModule(const std::string& name) const {
    return modules_by_name_.count(name) > 0;
}

std::shared_ptr<HoModuleBase> ModuleBundle::getModule(const std::string& name) const {
    auto it = modules_by_name_.find(name);
    if (it != modules_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<HoModuleBase> ModuleBundle::findModuleBySymbol(const std::string& symbol_name) const {
    auto it = symbols_to_modules_.find(symbol_name);
    if (it != symbols_to_modules_.end() && !it->second.empty()) {
        const std::string& module_name = *it->second.begin();
        return getModule(module_name);
    }
    return nullptr;
}

std::shared_ptr<HoModuleBase> ModuleBundle::findModuleBySymbolMangled(const std::string& mangled_name) const {
    auto it = mangled_symbols_to_modules_.find(mangled_name);
    if (it != mangled_symbols_to_modules_.end() && !it->second.empty()) {
        const std::string& module_name = *it->second.begin();
        return getModule(module_name);
    }
    return nullptr;
}

std::vector<std::shared_ptr<HoModuleBase>> ModuleBundle::getAllModules() const {
    return std::vector<std::shared_ptr<HoModuleBase>>(module_set_.begin(), module_set_.end());
}

std::vector<std::string> ModuleBundle::getModuleNames() const {
    std::vector<std::string> names;
    names.reserve(modules_by_name_.size());
    for (const auto& pair : modules_by_name_) {
        names.push_back(pair.first);
    }
    return names;
}

void ModuleBundle::clear() {
    module_set_.clear();
    modules_by_name_.clear();
    symbols_to_modules_.clear();
    mangled_symbols_to_modules_.clear();
    exports_by_name_.clear();
    nested_exports_to_modules_.clear();
    namespace_exports_.clear();
    mangled_to_original_.clear();
}

std::vector<std::shared_ptr<HoModuleBase>> ModuleBundle::resolveDependencyOrder() const {
    std::vector<std::shared_ptr<HoModuleBase>> result;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursion_stack;

    std::function<void(const std::shared_ptr<HoModuleBase>&)> visit =
        [&](const std::shared_ptr<HoModuleBase>& module) {
            const std::string& name = module->getName();
            if (visited.count(name)) return;
            if (recursion_stack.count(name)) return;

            recursion_stack.insert(name);

            for (const auto& dep_name : module->getDependencyOrder()) {
                auto dep_module = getModule(dep_name);
                if (dep_module) {
                    visit(dep_module);
                }
            }

            recursion_stack.erase(name);
            visited.insert(name);
            result.push_back(module);
        };

    for (const auto& pair : modules_by_name_) {
        visit(pair.second);
    }

    return result;
}

std::vector<std::string> ModuleBundle::getModuleDependencyOrder(const std::string& module_name) const {
    auto module = getModule(module_name);
    if (!module) return {};

    std::vector<std::shared_ptr<HoModuleBase>> all_modules;
    for (const auto& pair : modules_by_name_) {
        all_modules.push_back(pair.second);
    }

    module->resolveDependencyOrder(all_modules);
    return module->getDependencyOrder();
}

std::vector<std::string> ModuleBundle::getAllDependentModules(const std::string& module_name) const {
    std::vector<std::string> dependents;

    std::function<void(const std::string&, std::unordered_set<std::string>&)> find_deps =
        [&](const std::string& name, std::unordered_set<std::string>& visited) {
            if (visited.count(name)) return;
            visited.insert(name);

            for (const auto& pair : modules_by_name_) {
                if (pair.second->hasDependency(name)) {
                    find_deps(pair.first, visited);
                    dependents.push_back(pair.first);
                }
            }
        };

    std::unordered_set<std::string> visited;
    find_deps(module_name, visited);

    return dependents;
}

bool ModuleBundle::hasCircularDependency(const std::string& module_name) const {
    auto module = getModule(module_name);
    if (!module) return false;

    std::vector<std::shared_ptr<HoModuleBase>> all_modules;
    for (const auto& pair : modules_by_name_) {
        all_modules.push_back(pair.second);
    }

    module->resolveDependencyOrder(all_modules);
    return module->hasCircularDependency();
}

bool ModuleBundle::hasCircularDependency() const {
    for (const auto& pair : modules_by_name_) {
        if (hasCircularDependency(pair.first)) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<HoModuleBase> ModuleBundle::findModuleByNestedSymbol(const std::vector<std::string>& module_path,
                                                            const std::string& member_name) const {
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

std::shared_ptr<HoModuleBase> ModuleBundle::findModuleByExport(const std::string& export_name) const {
    auto it = exports_by_name_.find(export_name);
    if (it != exports_by_name_.end()) {
        auto nested_it = nested_exports_to_modules_.find(export_name);
        if (nested_it != nested_exports_to_modules_.end() && !nested_it->second.empty()) {
            return getModule(*nested_it->second.begin());
        }
    }
    return findModuleBySymbol(export_name);
}

void ModuleBundle::registerExport(const std::string& module_name,
                                const std::string& symbol_name,
                                const std::string& mangled_name,
                                SymbolType kind) {
    if (!hasModule(module_name)) return;
    exports_by_name_[symbol_name] = kind;
    mangled_to_original_[mangled_name] = symbol_name;
    symbols_to_modules_[symbol_name].insert(module_name);
    if (!mangled_name.empty()) {
        mangled_symbols_to_modules_[mangled_name].insert(module_name);
    }
}

void ModuleBundle::registerNestedExport(const std::vector<std::string>& module_path,
                                        const std::string& member_name,
                                        const std::string& mangled_name,
                                        SymbolType kind) {
    std::string key;
    for (size_t i = 0; i < module_path.size(); ++i) {
        if (i > 0) key += ".";
        key += module_path[i];
    }
    key += "." + member_name;

    auto module = findModuleBySymbol(module_path.empty() ? "" : module_path.front());
    if (module) {
        nested_exports_to_modules_[key].insert(module->getName());
        mangled_to_original_[mangled_name] = key;
    }
}

void ModuleBundle::registerNamespaceExport(const std::string& namespace_name,
                                      const std::string& member_name,
                                      const std::string& mangled_name,
                                      SymbolType kind) {
    std::string key = namespace_name + "." + member_name;
    namespace_exports_[namespace_name].insert(member_name);
    exports_by_name_[key] = kind;
    mangled_to_original_[mangled_name] = key;
}

std::vector<std::string> ModuleBundle::findExportsByKind(SymbolType kind) const {
    std::vector<std::string> result;
    for (const auto& pair : exports_by_name_) {
        if (pair.second == kind) {
            result.push_back(pair.first);
        }
    }
    return result;
}

std::vector<std::string> ModuleBundle::findExportsInNamespace(const std::string& namespace_name) const {
    auto it = namespace_exports_.find(namespace_name);
    if (it != namespace_exports_.end()) {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

bool ModuleBundle::hasExport(const std::string& symbol_name) const {
    return exports_by_name_.count(symbol_name) > 0;
}

bool ModuleBundle::hasNestedExport(const std::vector<std::string>& module_path,
                                 const std::string& member_name) const {
    std::string key;
    for (size_t i = 0; i < module_path.size(); ++i) {
        if (i > 0) key += ".";
        key += module_path[i];
    }
    key += "." + member_name;
    return nested_exports_to_modules_.count(key) > 0;
}

std::string ModuleBundle::mangleExport(const std::vector<std::string>& module_path,
                                     const std::string& symbol_name,
                                     SymbolType kind) const {
    const std::string kindTag = symbolTypeTag(kind);
    return hooc::SymbolMangler::mangleModuleSymbol(module_path, symbol_name + "_" + kindTag);
}

std::string ModuleBundle::mangleNestedMember(const std::vector<std::string>& module_path,
                                         const std::string& member_name,
                                         SymbolType kind) const {
    std::vector<std::string> path = module_path;
    path.push_back("nested");
    return mangleExport(path, member_name, kind);
}

std::string ModuleBundle::mangleNamespaceMember(const std::string& namespace_name,
                                          const std::string& member_name,
                                          SymbolType kind) const {
    std::vector<std::string> path = {"ns", namespace_name};
    return mangleExport(path, member_name, kind);
}

hooc::DemangledSymbol ModuleBundle::demangleExport(const std::string& mangled_name) const {
    return hooc::SymbolMangler::demangleSymbol(mangled_name);
}

std::vector<std::string> ModuleBundle::getAllExportedSymbols() const {
    std::vector<std::string> result;
    for (const auto& pair : exports_by_name_) {
        result.push_back(pair.first);
    }
    return result;
}

std::vector<std::string> ModuleBundle::getAllMangledExports() const {
    std::vector<std::string> result;
    for (const auto& pair : mangled_to_original_) {
        result.push_back(pair.first);
    }
    return result;
}

}
