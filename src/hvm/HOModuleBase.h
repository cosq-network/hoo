#ifndef HVM_HVM_MODULE_BASE_H
#define HVM_HVM_MODULE_BASE_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include <tuple>

#include "hvm/HVMInstruction.h"

namespace hooc {
class IOProvider;
}

namespace hvm {

enum class ModuleType : uint8_t {
    Compiled = 0x01,
    StaticRuntime = 0x02,
    DynamicLibrary = 0x03
};

enum class SymbolBinding : uint8_t {
    Local = 0,
    Global = 1,
    Weak = 2
};

enum class SymbolType : uint8_t {
    NoType = 0,
    Function = 1,
    Object = 2,
    Type = 3,
    TLS = 4
};

struct ModuleSymbol {
    std::string name;
    std::string mangled_name;
    SymbolBinding binding;
    SymbolType type;
    uint64_t address;
    uint64_t size;
    std::string signature;

    bool is_function() const { return type == SymbolType::Function; }
    bool is_object() const { return type == SymbolType::Object; }
    bool is_global() const { return binding == SymbolBinding::Global || binding == SymbolBinding::Weak; }
};

struct ModuleDependency {
    std::string module_name;
    std::string module_path;
    ModuleType type;
    bool optional;
    uint32_t version_min;
    uint32_t version_max;
};

class HOModuleBase {
public:
    HOModuleBase(ModuleType type, const std::string& name);
    virtual ~HOModuleBase() = default;

    virtual ModuleType getModuleType() const { return module_type_; }
    virtual const std::string& getName() const { return module_name_; }
    virtual void setName(const std::string& name) { module_name_ = name; }

    virtual const std::string& getSourcePath() const { return source_path_; }
    virtual void setSourcePath(const std::string& path) { source_path_ = path; }

    virtual bool isLoaded() const { return loaded_; }
    virtual void setLoaded(bool loaded) { loaded_ = loaded; }

    virtual const ModuleSymbol* findSymbol(const std::string& name) const;
    virtual const ModuleSymbol* findSymbolMangled(const std::string& mangled_name) const;
    virtual std::vector<const ModuleSymbol*> findSymbolsByPrefix(const std::string& prefix) const;

    virtual void addSymbol(const ModuleSymbol& symbol);
    virtual void addDependency(ModuleDependency dependency);
    virtual void addDependency(const std::string& module_name, ModuleType type,
                              bool optional = false, uint32_t version_min = 0, uint32_t version_max = 0xFFFFFFFF);

    virtual const std::vector<ModuleDependency>& getDependencies() const { return dependencies_; }
    virtual const ModuleDependency* findDependency(const std::string& module_name) const;
    virtual bool hasDependency(const std::string& module_name) const;
    virtual const std::vector<std::string>& getDependencyOrder() const { return dependency_order_; }

    virtual void resolveDependencyOrder(const std::vector<std::shared_ptr<HOModuleBase>>& all_modules);
    virtual bool hasCircularDependency() const { return has_circular_dependency_; }

    virtual std::string getError() const { return error_; }
    virtual bool hasError() const { return !error_.empty(); }
    virtual void clearError() { error_.clear(); }
    virtual void setError(const std::string& error) { error_ = error; }

    virtual std::string getSymbolSignature(const std::string& symbol_name) const;

    virtual bool serialize(std::vector<uint8_t>& output) const;
    virtual bool deserialize(const std::vector<uint8_t>& input);

    virtual bool serializeToFile(const std::string& file_path) const;
    virtual bool deserializeFromFile(const std::string& file_path);

    virtual void setIOProvider(std::shared_ptr<hooc::IOProvider> provider);
    virtual std::shared_ptr<hooc::IOProvider> getIOProvider() const;

    static std::string getModuleTypeName(ModuleType type);
    static std::string mangleSymbol(const std::string& symbol_name, SymbolType sym_type);

protected:
    ModuleType module_type_;
    std::string module_name_;
    std::string source_path_;
    bool loaded_;
    bool has_circular_dependency_;
    std::shared_ptr<hooc::IOProvider> io_provider_;

    std::unordered_map<std::string, ModuleSymbol> symbols_by_name_;
    std::vector<ModuleDependency> dependencies_;
    std::unordered_set<std::string> dependency_names_;
    std::vector<std::string> dependency_order_;

    mutable std::string error_;

    virtual const ModuleSymbol* findSymbolInternal(const std::string& name) const;

    virtual void addSymbolInternal(const ModuleSymbol& symbol);
};

class StaticHOModule : public HOModuleBase {
public:
    StaticHOModule(const std::string& name);
    virtual ~StaticHOModule() override;

    static std::shared_ptr<StaticHOModule> create(const std::string& name);

    void registerFunction(const std::string& name, void* address,
                         const std::string& signature = "",
                         SymbolBinding binding = SymbolBinding::Global);

    void registerObject(const std::string& name, void* address, size_t size,
                       const std::string& type_name = "",
                       SymbolBinding binding = SymbolBinding::Global);

    void registerFunctions(const std::vector<std::tuple<std::string, void*, std::string>>& funcs);
    void registerObjects(const std::vector<std::tuple<std::string, void*, size_t, std::string>>& objs);

    void* resolveFunction(const std::string& name) const;
    void* resolveObject(const std::string& name) const;

    bool serialize(std::vector<uint8_t>& output) const override;
    bool deserialize(const std::vector<uint8_t>& input) override;

    bool serializeToFile(const std::string& file_path) const override;
    bool deserializeFromFile(const std::string& file_path) override;

    bool isLinked() const { return linked_; }
    void setLinked(bool linked) { linked_ = linked; }

    const std::string& getLibraryPath() const { return library_path_; }
    void setLibraryPath(const std::string& path) { library_path_ = path; }

private:
    bool linked_;
    std::string library_path_;
    std::unordered_map<std::string, void*> function_addresses_;
    std::unordered_map<std::string, void*> object_addresses_;
};

class DynamicHOModule : public HOModuleBase {
public:
    DynamicHOModule(const std::string& name);
    virtual ~DynamicHOModule() override;

    static std::shared_ptr<DynamicHOModule> create(const std::string& name);
    static std::shared_ptr<DynamicHOModule> load(const std::string& library_path,
                                                const std::string& module_name = "");
    static std::shared_ptr<DynamicHOModule> load(const std::vector<std::string>& search_paths,
                                               const std::string& library_name,
                                               const std::string& module_name = "");

    bool loadLibrary();
    bool loadLibrary(const std::string& library_path);
    bool unloadLibrary();

    void* resolveSymbol(const std::string& symbol_name) const;
    void* resolveSymbolMangled(const std::string& mangled_name) const;

    bool serialize(std::vector<uint8_t>& output) const override;
    bool deserialize(const std::vector<uint8_t>& input) override;

    bool serializeToFile(const std::string& file_path) const override;
    bool deserializeFromFile(const std::string& file_path) override;

    bool isLibraryLoaded() const { return library_loaded_; }
    void* getLibraryHandle() const { return library_handle_; }
    const std::string& getLibraryPath() const { return library_path_; }

    const std::vector<std::string>& getExportedSymbols() const { return exported_symbols_; }
    const std::vector<std::string>& getLoadedLibraries() const { return loaded_libraries_; }

    void addLoadedLibrary(const std::string& library_path);
    void setLibraryLoaded(bool loaded) { library_loaded_ = loaded; }

private:
    bool library_loaded_;
    void* library_handle_;
    std::string library_path_;
    std::vector<std::string> exported_symbols_;
    std::vector<std::string> loaded_libraries_;
    std::unordered_map<std::string, void*> resolved_symbols_;

    bool loadExportedSymbols();
};

}

#endif
