#include "hvm/HoModuleBase.h"
#include "core/SymbolMangler.h"
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <fstream>

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
        const_cast<bool&>(has_circular_dependency_) = true;
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

bool HoModuleBase::serialize(std::vector<uint8_t>& output) const {
    output.clear();
    output.reserve(256);

    output.resize(16, 0);
    uint32_t magic = 0x484F4F48;
    *reinterpret_cast<uint32_t*>(output.data() + 0x00) = magic;
    *reinterpret_cast<uint32_t*>(output.data() + 0x04) = static_cast<uint32_t>(module_type_);
    *reinterpret_cast<uint32_t*>(output.data() + 0x08) = 1;
    *reinterpret_cast<uint32_t*>(output.data() + 0x0C) = static_cast<uint32_t>(module_name_.size());

    output.insert(output.end(), module_name_.begin(), module_name_.end());

    return true;
}

bool HoModuleBase::deserialize(const std::vector<uint8_t>& input) {
    if (input.size() < 16) {
        error_ = "Input too small for header";
        return false;
    }

    uint32_t magic = *reinterpret_cast<const uint32_t*>(input.data() + 0x00);
    if (magic != 0x484F4F48) {
        error_ = "Invalid magic number";
        return false;
    }

    uint32_t recordedType = *reinterpret_cast<const uint32_t*>(input.data() + 0x04);
    if (recordedType != static_cast<uint32_t>(module_type_)) {
        error_ = "Module type mismatch";
        return false;
    }

    uint32_t nameLen = *reinterpret_cast<const uint32_t*>(input.data() + 0x0C);
    if (input.size() < 16 + nameLen) {
        error_ = "Input too small for name";
        return false;
    }

    module_name_ = std::string(reinterpret_cast<const char*>(input.data() + 16), nameLen);
    return true;
}

bool HoModuleBase::serializeToFile(const std::string& file_path) const {
    std::vector<uint8_t> data;
    if (!serialize(data)) {
        return false;
    }

    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        error_ = "Cannot open file for writing";
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool HoModuleBase::deserializeFromFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        error_ = "Cannot open file for reading";
        return false;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        error_ = "Cannot read file";
        return false;
    }

    return deserialize(data);
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

bool StaticHoModule::serialize(std::vector<uint8_t>& output) const {
    output.clear();
    output.reserve(256);

    output.resize(32, 0);
    uint32_t magic = 0x484F4F48;
    *reinterpret_cast<uint32_t*>(output.data() + 0x00) = magic;
    *reinterpret_cast<uint32_t*>(output.data() + 0x04) = static_cast<uint32_t>(ModuleType::StaticRuntime);
    *reinterpret_cast<uint32_t*>(output.data() + 0x08) = 1;
    *reinterpret_cast<uint32_t*>(output.data() + 0x0C) = static_cast<uint32_t>(module_name_.size());

    uint32_t symCount = static_cast<uint32_t>(symbols_by_name_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x10) = symCount;
    *reinterpret_cast<uint32_t*>(output.data() + 0x14) = static_cast<uint32_t>(dependencies_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x18) = linked_ ? 1 : 0;
    *reinterpret_cast<uint32_t*>(output.data() + 0x1C) = static_cast<uint32_t>(library_path_.size());

    size_t offset = 32;
    output.insert(output.end(), module_name_.begin(), module_name_.end());

    for (const auto& [name, sym] : symbols_by_name_) {
        uint32_t nameLen = static_cast<uint32_t>(name.size());
        output.resize(output.size() + 8 + nameLen + sym.signature.size() + 1, 0);
        *reinterpret_cast<uint32_t*>(output.data() + offset) = nameLen;
        offset += 4;
        std::memcpy(output.data() + offset, name.data(), nameLen);
        offset += nameLen;
        *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(sym.signature.size());
        offset += 4;
        if (!sym.signature.empty()) {
            std::memcpy(output.data() + offset, sym.signature.data(), sym.signature.size());
            offset += sym.signature.size();
        }
    }

    for (const auto& dep : dependencies_) {
        uint32_t nameLen = static_cast<uint32_t>(dep.module_name.size());
        output.resize(output.size() + 12 + nameLen, 0);
        *reinterpret_cast<uint32_t*>(output.data() + offset) = nameLen;
        offset += 4;
        std::memcpy(output.data() + offset, dep.module_name.data(), nameLen);
        offset += nameLen;
        *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(dep.type);
        offset += 4;
        *reinterpret_cast<uint32_t*>(output.data() + offset) = dep.optional ? 1 : 0;
        offset += 4;
    }

    if (!library_path_.empty()) {
        output.insert(output.end(), library_path_.begin(), library_path_.end());
    }

    return true;
}

bool StaticHoModule::deserialize(const std::vector<uint8_t>& input) {
    if (input.size() < 32) {
        error_ = "Input too small for header";
        return false;
    }

    uint32_t magic = *reinterpret_cast<const uint32_t*>(input.data() + 0x00);
    if (magic != 0x484F4F48) {
        error_ = "Invalid magic number";
        return false;
    }

    uint32_t recordedType = *reinterpret_cast<const uint32_t*>(input.data() + 0x04);
    if (recordedType != static_cast<uint32_t>(ModuleType::StaticRuntime)) {
        error_ = "Module type mismatch";
        return false;
    }

    uint32_t nameLen = *reinterpret_cast<const uint32_t*>(input.data() + 0x0C);
    if (input.size() < 32 + nameLen) {
        error_ = "Input too small for name";
        return false;
    }

    module_name_ = std::string(reinterpret_cast<const char*>(input.data() + 32), nameLen);

    uint32_t symCount = *reinterpret_cast<const uint32_t*>(input.data() + 0x10);
    uint32_t depCount = *reinterpret_cast<const uint32_t*>(input.data() + 0x14);
    linked_ = *reinterpret_cast<const uint32_t*>(input.data() + 0x18) != 0;
    uint32_t libPathLen = *reinterpret_cast<const uint32_t*>(input.data() + 0x1C);

    size_t offset = 32 + nameLen;

    symbols_by_name_.clear();
    for (uint32_t i = 0; i < symCount; ++i) {
        uint32_t symNameLen = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        if (input.size() < offset + symNameLen) {
            error_ = "Input too small for symbol name";
            return false;
        }

        std::string symName(reinterpret_cast<const char*>(input.data() + offset), symNameLen);
        offset += symNameLen;

        uint32_t sigLen = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        std::string signature;
        if (sigLen > 0) {
            signature = std::string(reinterpret_cast<const char*>(input.data() + offset), sigLen);
            offset += sigLen;
        }

        ModuleSymbol sym;
        sym.name = symName;
        sym.mangled_name = hooc::SymbolMangler::mangleModuleSymbol(
            std::vector<std::string>{module_name_}, symName);
        sym.binding = SymbolBinding::Global;
        sym.type = SymbolType::Function;
        sym.address = 0;
        sym.size = 0;
        sym.signature = signature;
        addSymbol(sym);
    }

    dependencies_.clear();
    dependency_names_.clear();
    for (uint32_t i = 0; i < depCount; ++i) {
        uint32_t depNameLen = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        if (input.size() < offset + depNameLen) {
            error_ = "Input too small for dependency name";
            return false;
        }

        std::string depName(reinterpret_cast<const char*>(input.data() + offset), depNameLen);
        offset += depNameLen;

        uint32_t depType = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;
        uint32_t optional = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        addDependency(depName, static_cast<ModuleType>(depType), optional != 0);
    }

    if (libPathLen > 0 && input.size() >= offset + libPathLen) {
        library_path_ = std::string(reinterpret_cast<const char*>(input.data() + offset), libPathLen);
    }

    return true;
}

bool StaticHoModule::serializeToFile(const std::string& file_path) const {
    std::vector<uint8_t> data;
    if (!serialize(data)) {
        return false;
    }

    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        error_ = "Cannot open file for writing";
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool StaticHoModule::deserializeFromFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        error_ = "Cannot open file for reading";
        return false;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        error_ = "Cannot read file";
        return false;
    }

    return deserialize(data);
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
        const_cast<std::unordered_map<std::string, void*>&>(resolved_symbols_)[symbol_name] = addr;
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
            const_cast<std::unordered_map<std::string, void*>&>(resolved_symbols_)[mangled_name] = addr;
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

bool DynamicHoModule::serialize(std::vector<uint8_t>& output) const {
    output.clear();
    output.reserve(256);

    output.resize(40, 0);
    uint32_t magic = 0x484F4F48;
    *reinterpret_cast<uint32_t*>(output.data() + 0x00) = magic;
    *reinterpret_cast<uint32_t*>(output.data() + 0x04) = static_cast<uint32_t>(ModuleType::DynamicLibrary);
    *reinterpret_cast<uint32_t*>(output.data() + 0x08) = 1;
    *reinterpret_cast<uint32_t*>(output.data() + 0x0C) = static_cast<uint32_t>(module_name_.size());

    uint32_t symCount = static_cast<uint32_t>(symbols_by_name_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x10) = symCount;
    *reinterpret_cast<uint32_t*>(output.data() + 0x14) = static_cast<uint32_t>(dependencies_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x18) = library_loaded_ ? 1 : 0;
    *reinterpret_cast<uint32_t*>(output.data() + 0x1C) = static_cast<uint32_t>(library_path_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x20) = static_cast<uint32_t>(exported_symbols_.size());
    *reinterpret_cast<uint32_t*>(output.data() + 0x24) = static_cast<uint32_t>(loaded_libraries_.size());

    size_t offset = 40;
    output.insert(output.end(), module_name_.begin(), module_name_.end());

    for (const auto& [name, sym] : symbols_by_name_) {
        uint32_t nameLen = static_cast<uint32_t>(name.size());
        output.resize(output.size() + 4 + nameLen, 0);
        *reinterpret_cast<uint32_t*>(output.data() + offset) = nameLen;
        offset += 4;
        std::memcpy(output.data() + offset, name.data(), nameLen);
        offset += nameLen;
    }

    for (const auto& dep : dependencies_) {
        uint32_t nameLen = static_cast<uint32_t>(dep.module_name.size());
        output.resize(output.size() + 12 + nameLen, 0);
        *reinterpret_cast<uint32_t*>(output.data() + offset) = nameLen;
        offset += 4;
        std::memcpy(output.data() + offset, dep.module_name.data(), nameLen);
        offset += nameLen;
        *reinterpret_cast<uint32_t*>(output.data() + offset) = static_cast<uint32_t>(dep.type);
        offset += 4;
        *reinterpret_cast<uint32_t*>(output.data() + offset) = dep.optional ? 1 : 0;
        offset += 4;
    }

    if (!library_path_.empty()) {
        output.insert(output.end(), library_path_.begin(), library_path_.end());
    }

    for (const auto& expSym : exported_symbols_) {
        uint32_t nameLen = static_cast<uint32_t>(expSym.size());
        output.resize(output.size() + 4 + nameLen, 0);
        *reinterpret_cast<uint32_t*>(output.data() + offset) = nameLen;
        offset += 4;
        std::memcpy(output.data() + offset, expSym.data(), nameLen);
        offset += nameLen;
    }

    for (const auto& loadedLib : loaded_libraries_) {
        uint32_t nameLen = static_cast<uint32_t>(loadedLib.size());
        output.resize(output.size() + 4 + nameLen, 0);
        *reinterpret_cast<uint32_t*>(output.data() + offset) = nameLen;
        offset += 4;
        std::memcpy(output.data() + offset, loadedLib.data(), nameLen);
        offset += nameLen;
    }

    return true;
}

bool DynamicHoModule::deserialize(const std::vector<uint8_t>& input) {
    if (input.size() < 40) {
        error_ = "Input too small for header";
        return false;
    }

    uint32_t magic = *reinterpret_cast<const uint32_t*>(input.data() + 0x00);
    if (magic != 0x484F4F48) {
        error_ = "Invalid magic number";
        return false;
    }

    uint32_t recordedType = *reinterpret_cast<const uint32_t*>(input.data() + 0x04);
    if (recordedType != static_cast<uint32_t>(ModuleType::DynamicLibrary)) {
        error_ = "Module type mismatch";
        return false;
    }

    uint32_t nameLen = *reinterpret_cast<const uint32_t*>(input.data() + 0x0C);
    if (input.size() < 40 + nameLen) {
        error_ = "Input too small for name";
        return false;
    }

    module_name_ = std::string(reinterpret_cast<const char*>(input.data() + 40), nameLen);

    uint32_t symCount = *reinterpret_cast<const uint32_t*>(input.data() + 0x10);
    uint32_t depCount = *reinterpret_cast<const uint32_t*>(input.data() + 0x14);
    library_loaded_ = *reinterpret_cast<const uint32_t*>(input.data() + 0x18) != 0;
    uint32_t libPathLen = *reinterpret_cast<const uint32_t*>(input.data() + 0x1C);
    uint32_t expSymCount = *reinterpret_cast<const uint32_t*>(input.data() + 0x20);
    uint32_t loadedLibCount = *reinterpret_cast<const uint32_t*>(input.data() + 0x24);

    size_t offset = 40 + nameLen;

    symbols_by_name_.clear();
    for (uint32_t i = 0; i < symCount; ++i) {
        uint32_t symNameLen = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        if (input.size() < offset + symNameLen) {
            error_ = "Input too small for symbol name";
            return false;
        }

        std::string symName(reinterpret_cast<const char*>(input.data() + offset), symNameLen);
        offset += symNameLen;

        ModuleSymbol sym;
        sym.name = symName;
        sym.mangled_name = hooc::SymbolMangler::mangleModuleSymbol(
            std::vector<std::string>{module_name_}, symName);
        sym.binding = SymbolBinding::Global;
        sym.type = SymbolType::NoType;
        sym.address = 0;
        sym.size = 0;
        addSymbolInternal(sym);
    }

    dependencies_.clear();
    dependency_names_.clear();
    for (uint32_t i = 0; i < depCount; ++i) {
        uint32_t depNameLen = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        if (input.size() < offset + depNameLen) {
            error_ = "Input too small for dependency name";
            return false;
        }

        std::string depName(reinterpret_cast<const char*>(input.data() + offset), depNameLen);
        offset += depNameLen;

        uint32_t depType = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;
        uint32_t optional = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        addDependency(depName, static_cast<ModuleType>(depType), optional != 0);
    }

    if (libPathLen > 0 && input.size() >= offset + libPathLen) {
        library_path_ = std::string(reinterpret_cast<const char*>(input.data() + offset), libPathLen);
        offset += libPathLen;
    }

    exported_symbols_.clear();
    for (uint32_t i = 0; i < expSymCount; ++i) {
        uint32_t symLen = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        if (input.size() < offset + symLen) {
            error_ = "Input too small for exported symbol";
            return false;
        }

        exported_symbols_.push_back(std::string(reinterpret_cast<const char*>(input.data() + offset), symLen));
        offset += symLen;
    }

    loaded_libraries_.clear();
    for (uint32_t i = 0; i < loadedLibCount; ++i) {
        uint32_t libLen = *reinterpret_cast<const uint32_t*>(input.data() + offset);
        offset += 4;

        if (input.size() < offset + libLen) {
            error_ = "Input too small for loaded library";
            return false;
        }

        loaded_libraries_.push_back(std::string(reinterpret_cast<const char*>(input.data() + offset), libLen));
        offset += libLen;
    }

    return true;
}

bool DynamicHoModule::serializeToFile(const std::string& file_path) const {
    std::vector<uint8_t> data;
    if (!serialize(data)) {
        return false;
    }

    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        error_ = "Cannot open file for writing";
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool DynamicHoModule::deserializeFromFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        error_ = "Cannot open file for reading";
        return false;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        error_ = "Cannot read file";
        return false;
    }

    return deserialize(data);
}

}
