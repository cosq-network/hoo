#include "hvm/HoModuleBase.h"
#include "core/SymbolMangler.h"
#include <algorithm>
#include <filesystem>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#include <mach-o/dyld.h>
#else
#include <dlfcn.h>
#include <link.h>
#endif

namespace hvm {

HoModuleBase::HoModuleBase(ModuleType type, const std::string& name)
    : module_type_(type)
    , module_name_(name)
    , source_path_("")
    , loaded_(false)
    , has_circular_dependency_(false)
{
}

const ModuleSymbol* HoModuleBase::findSymbol(const std::string& name) const {
    auto it = symbols_by_name_.find(name);
    if (it != symbols_by_name_.end()) {
        return &it->second;
    }

    auto mangled = hooc::SymbolMangler::mangleModuleSymbol(
        std::vector<std::string>{module_name_}, name);
    it = symbols_by_name_.find(mangled);
    if (it != symbols_by_name_.end()) {
        return &it->second;
    }

    return findSymbolInternal(name);
}

const ModuleSymbol* HoModuleBase::findSymbolMangled(const std::string& mangled_name) const {
    auto it = symbols_by_name_.find(mangled_name);
    if (it != symbols_by_name_.end()) {
        return &it->second;
    }
    return nullptr;
}

const ModuleSymbol* HoModuleBase::findSymbolInternal(const std::string& name) const {
    auto it = symbols_by_name_.find(name);
    if (it != symbols_by_name_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const ModuleSymbol*> HoModuleBase::findSymbolsByPrefix(const std::string& prefix) const {
    std::vector<const ModuleSymbol*> results;
    for (const auto& pair : symbols_by_name_) {
        if (pair.first.substr(0, prefix.size()) == prefix) {
            results.push_back(&pair.second);
        }
    }
    return results;
}

void HoModuleBase::addSymbol(const ModuleSymbol& symbol) {
    auto mangled = hooc::SymbolMangler::mangleModuleSymbol(
        std::vector<std::string>{module_name_}, symbol.name);
    ModuleSymbol copy = symbol;
    copy.mangled_name = mangled;
    addSymbolInternal(copy);
}

void HoModuleBase::addSymbolInternal(const ModuleSymbol& symbol) {
    symbols_by_name_[symbol.name] = symbol;
    if (!symbol.mangled_name.empty()) {
        symbols_by_name_[symbol.mangled_name] = symbol;
    }
}

void HoModuleBase::addDependency(ModuleDependency dependency) {
    if (dependency_names_.find(dependency.module_name) == dependency_names_.end()) {
        dependencies_.push_back(dependency);
        dependency_names_.insert(dependency.module_name);
    }
}

void HoModuleBase::addDependency(const std::string& module_name, ModuleType type,
                                bool optional, uint32_t version_min, uint32_t version_max) {
    ModuleDependency dep;
    dep.module_name = module_name;
    dep.type = type;
    dep.optional = optional;
    dep.version_min = version_min;
    dep.version_max = version_max;
    addDependency(dep);
}

const ModuleDependency* HoModuleBase::findDependency(const std::string& module_name) const {
    for (const auto& dep : dependencies_) {
        if (dep.module_name == module_name) {
            return &dep;
        }
    }
    return nullptr;
}

bool HoModuleBase::hasDependency(const std::string& module_name) const {
    return dependency_names_.find(module_name) != dependency_names_.end();
}

void HoModuleBase::resolveDependencyOrder(const std::vector<std::shared_ptr<HoModuleBase>>& all_modules) {
    dependency_order_.clear();
    has_circular_dependency_ = false;

    std::unordered_map<std::string, std::shared_ptr<HoModuleBase>> module_map;
    for (const auto& mod : all_modules) {
        module_map[mod->getName()] = mod;
    }

    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> temp_visited;

    std::function<void(const std::string&)> visit = [&](const std::string& name) {
        if (temp_visited.find(name) != temp_visited.end()) {
            has_circular_dependency_ = true;
            return;
        }

        if (visited.find(name) != visited.end()) {
            return;
        }

        temp_visited.insert(name);

        auto dep = findDependency(name);
        if (dep) {
            visit(dep->module_name);
        }

        temp_visited.erase(name);
        visited.insert(name);
        dependency_order_.push_back(name);
    };

    for (const auto& dep : dependencies_) {
        if (module_map.find(dep.module_name) != module_map.end()) {
            visit(dep.module_name);
        }
    }

    dependency_order_.insert(dependency_order_.begin(), module_name_);
}

void HoModuleBase::checkCircularDependencies(const std::string& module_name,
                                            std::unordered_set<std::string>& visited,
                                            std::unordered_set<std::string>& recursion_stack) const {
    if (recursion_stack.find(module_name) != recursion_stack.end()) {
        has_circular_dependency_ = true;
        return;
    }

    if (visited.find(module_name) != visited.end()) {
        return;
    }

    visited.insert(module_name);
    recursion_stack.insert(module_name);

    for (const auto& dep : dependencies_) {
        checkCircularDependencies(dep.module_name, visited, recursion_stack);
    }

    recursion_stack.erase(module_name);
}

std::string HoModuleBase::getSymbolSignature(const std::string& symbol_name) const {
    auto it = symbols_by_name_.find(symbol_name);
    if (it != symbols_by_name_.end()) {
        return it->second.signature;
    }
    return "";
}

std::string HoModuleBase::getModuleTypeName(ModuleType type) {
    switch (type) {
        case ModuleType::Compiled: return "Compiled";
        case ModuleType::StaticRuntime: return "StaticRuntime";
        case ModuleType::DynamicLibrary: return "DynamicLibrary";
        default: return "Unknown";
    }
}

std::string HoModuleBase::mangleSymbol(const std::string& symbol_name, SymbolType sym_type) {
    return hooc::SymbolMangler::mangleModuleSymbol(
        std::vector<std::string>{}, symbol_name);
}

StaticHoModule::StaticHoModule(const std::string& name)
    : HoModuleBase(ModuleType::StaticRuntime, name)
    , linked_(true)
{
}

StaticHoModule::~StaticHoModule() = default;

std::shared_ptr<StaticHoModule> StaticHoModule::create(const std::string& name) {
    return std::shared_ptr<StaticHoModule>(new StaticHoModule(name));
}

void StaticHoModule::registerFunction(const std::string& name, void* address,
                                    const std::string& signature,
                                    SymbolBinding binding) {
    function_addresses_[name] = address;

    ModuleSymbol sym;
    sym.name = name;
    sym.mangled_name = hooc::SymbolMangler::mangleModuleSymbol(
        std::vector<std::string>{module_name_}, name);
    sym.binding = binding;
    sym.type = SymbolType::Function;
    sym.address = reinterpret_cast<uint64_t>(address);
    sym.signature = signature;

    addSymbol(sym);
}

void StaticHoModule::registerObject(const std::string& name, void* address, size_t size,
                                   const std::string& type_name,
                                   SymbolBinding binding) {
    object_addresses_[name] = address;

    ModuleSymbol sym;
    sym.name = name;
    sym.mangled_name = hooc::SymbolMangler::mangleModuleSymbol(
        std::vector<std::string>{module_name_}, name);
    sym.binding = binding;
    sym.type = SymbolType::Object;
    sym.address = reinterpret_cast<uint64_t>(address);
    sym.size = size;
    sym.signature = type_name;

    addSymbol(sym);
}

void StaticHoModule::registerFunctions(const std::vector<std::tuple<std::string, void*, std::string>>& funcs) {
    for (const auto& [name, addr, sig] : funcs) {
        registerFunction(name, addr, sig);
    }
}

void StaticHoModule::registerObjects(const std::vector<std::tuple<std::string, void*, size_t, std::string>>& objs) {
    for (const auto& [name, addr, size, type] : objs) {
        registerObject(name, addr, size, type);
    }
}

void* StaticHoModule::resolveFunction(const std::string& name) const {
    auto it = function_addresses_.find(name);
    if (it != function_addresses_.end()) {
        return it->second;
    }
    return nullptr;
}

void* StaticHoModule::resolveObject(const std::string& name) const {
    auto it = object_addresses_.find(name);
    if (it != object_addresses_.end()) {
        return it->second;
    }
    return nullptr;
}

DynamicHoModule::DynamicHoModule(const std::string& name)
    : HoModuleBase(ModuleType::DynamicLibrary, name)
    , library_loaded_(false)
    , library_handle_(nullptr)
{
}

DynamicHoModule::~DynamicHoModule() {
    unloadLibrary();
}

std::shared_ptr<DynamicHoModule> DynamicHoModule::create(const std::string& name) {
    return std::shared_ptr<DynamicHoModule>(new DynamicHoModule(name));
}

std::shared_ptr<DynamicHoModule> DynamicHoModule::load(const std::string& library_path,
                                                       const std::string& module_name) {
    auto module = std::shared_ptr<DynamicHoModule>(new DynamicHoModule(
        module_name.empty() ? std::filesystem::path(library_path).stem().string() : module_name));
    module->library_path_ = library_path;

    if (module->loadLibrary()) {
        return module;
    }
    return nullptr;
}

std::shared_ptr<DynamicHoModule> DynamicHoModule::load(const std::vector<std::string>& search_paths,
                                                       const std::string& library_name,
                                                       const std::string& module_name) {
    std::string full_path;

#ifdef _WIN32
    std::string lib_name = library_name + ".dll";
#elif defined(__APPLE__)
    std::string lib_name = "lib" + library_name + ".dylib";
#else
    std::string lib_name = "lib" + library_name + ".so";
#endif

    for (const auto& path : search_paths) {
        auto full = std::filesystem::path(path) / lib_name;
        if (std::filesystem::exists(full)) {
            full_path = full.string();
            break;
        }
    }

    if (full_path.empty()) {
        return nullptr;
    }

    return load(full_path, module_name);
}

bool DynamicHoModule::loadLibrary() {
    if (library_loaded_) {
        return true;
    }

#ifdef _WIN32
    library_handle_ = LoadLibraryA(library_path_.c_str());
    if (!library_handle_) {
        setError("Failed to load library: " + library_path_);
        return false;
    }
#else
    library_handle_ = dlopen(library_path_.c_str(), RTLD_NOW);
    if (!library_handle_) {
        setError("Failed to load library: " + std::string(dlerror()));
        return false;
    }
#endif

    library_loaded_ = true;
    loaded_ = true;

    if (!loadExportedSymbols()) {
        return false;
    }

    return true;
}

bool DynamicHoModule::loadLibrary(const std::string& library_path) {
    library_path_ = library_path;
    return loadLibrary();
}

bool DynamicHoModule::unloadLibrary() {
    if (!library_loaded_ || !library_handle_) {
        return true;
    }

#ifdef _WIN32
    bool result = FreeLibrary(static_cast<HMODULE>(library_handle_)) != 0;
#else
    bool result = dlclose(library_handle_) == 0;
#endif

    if (result) {
        library_loaded_ = false;
        library_handle_ = nullptr;
        loaded_ = false;
        exported_symbols_.clear();
        resolved_symbols_.clear();
    }

    return result;
}

void* DynamicHoModule::resolveSymbol(const std::string& symbol_name) const {
    auto it = resolved_symbols_.find(symbol_name);
    if (it != resolved_symbols_.end()) {
        return it->second;
    }

    if (!library_loaded_ || !library_handle_) {
        return nullptr;
    }

#ifdef _WIN32
    void* addr = GetProcAddress(static_cast<HMODULE>(library_handle_), symbol_name.c_str());
#else
    void* addr = dlsym(library_handle_, symbol_name.c_str());
#endif

    if (addr) {
        resolved_symbols_[symbol_name] = addr;
    }

    return addr;
}

void* DynamicHoModule::resolveSymbolMangled(const std::string& mangled_name) const {
    auto it = resolved_symbols_.find(mangled_name);
    if (it != resolved_symbols_.end()) {
        return it->second;
    }

    auto demangled = hooc::SymbolMangler::demangleSymbol(mangled_name);
    if (!demangled.functionName.empty()) {
        void* addr = resolveSymbol(demangled.functionName);
        if (addr) {
            resolved_symbols_[mangled_name] = addr;
            return addr;
        }
    }

    return resolveSymbol(mangled_name);
}

bool DynamicHoModule::loadExportedSymbols() {
#ifdef _WIN32
    HMODULE mod = static_cast<HMODULE>(library_handle_);
    DWORD size = 0;
    EnumProcessModules(mod, nullptr, 0, &size);

    if (size == 0) {
        return true;
    }

    std::vector<HMODULE> modules(size / sizeof(HMODULE));
    EnumProcessModules(mod, modules.data(), size, &size);

    for (HMODULE module : modules) {
        char name[MAX_PATH];
        GetModuleFileNameA(module, name, MAX_PATH);

        unsigned char* buffer = nullptr;
        DWORD export_size = 0;
        PIMAGE_EXPORT_DIRECTORY export_dir = nullptr;

        PIMAGE_NT_HEADERS nt_headers = ImageNtHeader(module);
        if (!nt_headers) continue;

        DWORD export_rva = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        if (!export_rva) continue;

        export_dir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
            ImageRvaToVa(nt_headers, module, export_rva, nullptr));
        if (!export_dir) continue;

        DWORD* name_rva = reinterpret_cast<DWORD*>(
            ImageRvaToVa(nt_headers, module, export_dir->AddressOfNames, nullptr));

        for (DWORD i = 0; i < export_dir->NumberOfNames; ++i) {
            const char* sym_name = reinterpret_cast<const char*>(
                ImageRvaToVa(nt_headers, module, name_rva[i], nullptr));
            if (sym_name) {
                exported_symbols_.push_back(sym_name);

                ModuleSymbol sym;
                sym.name = sym_name;
                sym.mangled_name = hooc::SymbolMangler::mangleModuleSymbol(
                    std::vector<std::string>{module_name_}, sym_name);
                sym.binding = SymbolBinding::Global;
                sym.type = SymbolType::NoType;
                sym.address = 0;
                addSymbolInternal(sym);
            }
        }
    }
#elif defined(__APPLE__)
    size_t count = _dyld_image_count();
    for (size_t i = 0; i < count; ++i) {
        if (_dyld_get_image_header(i) == library_handle_) {
            const char* name = _dyld_get_image_name(i);
            if (name == library_path_) {
                struct mach_header_64* header = (struct mach_header_64*)_dyld_get_image_header(i);
                uintptr_t slide = _dyld_get_image_vmaddr_slide(i);

                auto cmd = (struct load_command*)(header + 1);
                for (uint32_t j = 0; j < header->ncmds; ++j) {
                    if (cmd->cmd == LC_DYLD_INFO_ONLY) {
                        auto info = (struct dyld_info_command*)cmd;
                        if (info->export_size > 0) {
                            const uint8_t* export_data = (const uint8_t*)header + info->export_off;
                        }
                    }
                    cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
                }
            }
        }
    }
#else
    if (library_handle_) {
        struct link_map* map = nullptr;

        if (dlinfo(library_handle_, RTLD_DI_LINKMAP, &map) == 0 && map) {
            for (auto* sym = map->l_symtab; sym < map->l_symtab + map->l_nsyms; ++sym) {
                const char* sym_name = map->l_strings + sym->st_name;
                if (sym_name && sym_name[0]) {
                    exported_symbols_.push_back(sym_name);

                    ModuleSymbol msym;
                    msym.name = sym_name;
                    msym.mangled_name = hooc::SymbolMangler::mangleModuleSymbol(
                        std::vector<std::string>{module_name_}, sym_name);
                    msym.binding = (sym->st_shndx == SHN_UNDEF) ? SymbolBinding::Weak : SymbolBinding::Global;
                    msym.type = (sym->st_info & STT_FUNC) ? SymbolType::Function : SymbolType::Object;
                    msym.address = sym->st_value;
                    msym.size = sym->st_size;
                    addSymbolInternal(msym);
                }
            }
        }
    }
#endif

    return true;
}

void DynamicHoModule::addLoadedLibrary(const std::string& library_path) {
    auto it = std::find(loaded_libraries_.begin(), loaded_libraries_.end(), library_path);
    if (it == loaded_libraries_.end()) {
        loaded_libraries_.push_back(library_path);
    }
}

}
