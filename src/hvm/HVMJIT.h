#ifndef HVM_HVM_JIT_H
#define HVM_HVM_JIT_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <optional>
#include <mutex>
#include <array>
#include <atomic>

#include "core/IOProvider.h"
#include "core/HooCompiler.h"
#include "hvm/HOModule.h"
#include "hvm/HVMModuleBundle.h"

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/LLVMContext.h"

namespace hooc {

class HVMJIT {
public:
    enum class ErrorPhase {
        None,
        Parse,
        Validate,
        Resolve,
        Initialize,
        Execute
    };

    enum class ErrorCode {
        None,
        IoReadFailed,
        ParseFailed,
        InvalidHeader,
        InvalidSection,
        InvalidSymbol,
        InvalidMetadata,
        MissingEntryPoint,
        MissingDependency,
        CircularDependency,
        RuntimeBootstrapFailed,
        UnsupportedInstruction,
        ExecutionFailed
    };

    struct ErrorInfo {
        ErrorPhase phase = ErrorPhase::None;
        ErrorCode code = ErrorCode::None;
        std::string moduleName;
        std::string symbolName;
        std::string path;
        std::string message;
    };

    enum class LoaderState {
        Discovered,
        Parsed,
        Validated,
        Registered,
        DependenciesResolved,
        Ready,
        Failed
    };

    explicit HVMJIT(IOProvider& io);
    ~HVMJIT();

    bool loadInput(const std::string& pathOrModuleName);
    bool loadModule(const std::string& path);
    bool loadModule(std::unique_ptr<hvm::HOModule> module);
    bool loadSource(const std::string& sourcePath);
    bool loadSourceCode(const std::string& moduleName, const std::string& sourceCode);
    bool loadBytecode(const std::string& modulePath);
    int64_t run(const std::string& entryPoint = "_F_main_v");

    void* getSymbolAddress(const std::string& mangledName);
    bool hasModuleJITDylib(const std::string& moduleName) const;
    std::vector<std::string> getModuleLogicalSearchOrder(const std::string& moduleName) const;
    void* createInboundTrampoline(const std::string& moduleName, const std::string& functionName,
                                  size_t arity = 1);
    int64_t invokeInboundCallback(size_t slot, const std::vector<uint64_t>& args);
    bool lastRunUsedJIT() const { return lastRunUsedJIT_; }

    const std::string& getLastError() const { return lastError_; }
    std::optional<ErrorInfo> getLastErrorInfo() const { return lastErrorInfo_; }
    bool hasError() const { return !lastError_.empty(); }
    void clearError() {
        lastError_.clear();
        lastErrorInfo_.reset();
    }

    struct HVMState {
        int64_t regs[32]{};
        uint8_t* memory = nullptr;
        IOProvider* io = nullptr;
        bool trapHit = false;
        int64_t loop_count = 0;
        int64_t loop_backedge = 0;
    };
    struct InspectorSnapshot {
        std::array<int64_t, 32> regs{};
        uint64_t pc = 0;
        std::string moduleName;
        std::string functionName;
        std::string opcode;
        bool halted = false;
    };

    bool buildInspectorTrace(const std::string& entryPoint = "_F_main_v");
    bool inspectorStep();
    std::optional<InspectorSnapshot> getInspectorSnapshot() const;
    void resetInspector();
    void stopExecution();
    std::array<int64_t, 32> getRegisters() const;
    std::vector<uint8_t> readVirtualMemory(uint64_t addr, size_t size) const;
    bool getStopExecutionRequested() const { return stopExecutionRequested_.load(std::memory_order_relaxed); }

private:

    llvm::Expected<llvm::orc::ThreadSafeModule> translateModule(hvm::HOModule& hvmModule);
    bool resolveAndLoadDependencies(const hvm::HOModule& root);
    bool initializeDependencyGraphPostOrder();
    bool bootstrapRuntimeModules();
    bool registerModuleInBundle(const std::shared_ptr<hvm::HOModule>& module);
    bool parseAndLoadModuleFromPath(const std::string& path, std::shared_ptr<hvm::HOModule>& outModule);
    std::string moduleNameToPath(const std::string& moduleName) const;
    bool isSourcePath(const std::string& path) const;
    bool isBytecodePath(const std::string& path) const;
    bool ensureJIT();
    void setError(ErrorPhase phase, ErrorCode code, const std::string& message,
                  const std::string& moduleName = "", const std::string& symbolName = "",
                  const std::string& path = "");
    bool validateModule(const hvm::HOModule& module, const std::string& sourcePath);
    std::optional<std::string> resolveImportModulePath(const std::shared_ptr<hvm::HOModule>& importer,
                                                       const std::string& importModuleName) const;
    void buildModuleRegistryEntry(const std::shared_ptr<hvm::HOModule>& module);
    std::string canonicalizePath(const std::string& path) const;
    bool setLoaderState(const std::string& moduleName, LoaderState state);
    bool validateImportsAgainstDependencies();
    void rollbackModuleLoad(const std::string& moduleName);
    void buildLogicalSearchOrder();
    bool hasExportedOrDefinedSymbol(const std::string& moduleName, const std::string& symbolName) const;
    bool configureJITDylibs();
    bool preloadNativeLibrariesFromImports();
    bool resolveNativeImportSymbol(const hvm::ImportEntry& imp, const std::string& importerModuleName,
                                   uint64_t* outAddr = nullptr);
    bool isNativeImport(const hvm::ImportEntry& imp) const;
    bool registerRuntimeSymbolsInJITDylib();
    bool materializeModulesToJIT();
    bool ensureJITFunctionTable(const std::shared_ptr<hvm::HOModule>& module);
    bool isSupportedForIRLowering(hvm::Opcode op, uint16_t func) const;
    int64_t runViaJIT(const std::string& entryPoint);
    bool mapModuleSections(const std::shared_ptr<hvm::HOModule>& module);
    int64_t executeFunction(const std::shared_ptr<hvm::HOModule>& module, const std::string& functionName, HVMState& state);
    bool initializeModules();
    const hvm::Symbol* findFunctionSymbol(const hvm::HOModule& module, const std::string& functionName) const;
    bool loadU64(uint64_t addr, uint64_t& out) const;
    bool storeU64(uint64_t addr, uint64_t value);
    bool invokeStateAbiSymbol(const std::string& symbolName, HVMState& state, uint64_t& outValue);
    void captureInspectorSnapshot(const HVMState& state, uint64_t pc, const std::string& moduleName,
                                  const std::string& functionName, const std::string& opcode, bool halted);
    bool runModuleInitializer(const std::shared_ptr<hvm::HOModule>& module);
    bool runModuleVTableInitializers(const std::shared_ptr<hvm::HOModule>& module);
    std::shared_ptr<std::once_flag> getOrCreateModuleInitOnceFlag(const std::string& moduleName);
    std::shared_ptr<std::once_flag> getOrCreateModuleVTableOnceFlag(const std::string& moduleName);
    bool runPostLoadInitializers();

    struct ModuleMemoryLayout {
        uint64_t textBase = 0;
        uint64_t rodataBase = 0;
        uint64_t dataBase = 0;
        uint64_t bssBase = 0;
        uint64_t textSize = 0;
        uint64_t rodataSize = 0;
        uint64_t dataSize = 0;
        uint64_t bssSize = 0;
    };

    std::unique_ptr<llvm::orc::LLJIT> jit_;
    std::unique_ptr<llvm::JITEventListener> jitDebugListener_;
    llvm::orc::ThreadSafeContext tsc_;
    IOProvider& io_;
    std::unique_ptr<HooCompiler> sourceCompiler_;
    hvm::HVMModuleBundle bundle_;
    struct ModuleRegistryEntry {
        std::unordered_map<std::string, hvm::Symbol> symbolsByName;
        std::unordered_map<std::string, hvm::ExportEntry> exportsByName;
        std::vector<hvm::ImportEntry> imports;
        std::unordered_map<std::string, hvm::FunctionMetadata> functionMetaByName;
    };
    std::unordered_map<std::string, ModuleRegistryEntry> moduleRegistry_;
    std::unordered_map<std::string, std::shared_ptr<hvm::HOModule>> loadedModules_;
    std::unordered_map<std::string, std::string> moduleCanonicalPathByName_;
    std::unordered_map<std::string, LoaderState> moduleStates_;
    std::unordered_map<std::string, std::vector<std::string>> moduleDependencies_;
    std::unordered_map<std::string, std::vector<std::string>> moduleSearchOrder_;
    std::unordered_map<std::string, llvm::orc::JITDylib*> moduleDylibs_;
    std::unordered_set<std::string> loadedNativeLibraries_;
    std::unordered_map<std::string, std::unordered_map<uint64_t, std::string>> functionNameByOffset_;
    bool modulesMaterialized_ = false;
    std::unordered_map<std::string, ModuleMemoryLayout> moduleLayouts_;
    std::unordered_set<std::string> initializedModules_;
    std::string primaryModuleName_;
    std::string lastError_;
    std::vector<uint8_t> memory_;
    uint64_t memoryTop_ = 0;
    bool modulesInitialized_ = false;
    std::optional<ErrorInfo> lastErrorInfo_;
    bool lastRunUsedJIT_ = false;
    std::mutex moduleInitOnceMu_;
    std::unordered_map<std::string, std::shared_ptr<std::once_flag>> moduleInitOnceFlags_;
    std::mutex moduleVTableOnceMu_;
    std::unordered_map<std::string, std::shared_ptr<std::once_flag>> moduleVTableOnceFlags_;
    std::unordered_set<std::string> initializedVTableClasses_;
    std::mutex inboundTrampolineMu_;
    struct InboundTarget {
        std::string moduleName;
        std::string functionName;
        size_t arity = 1;
    };
    std::unordered_map<size_t, InboundTarget> inboundTrampolineTargets_;
    std::unordered_map<std::string, size_t> inboundTrampolineIndexByTarget_;
    bool inspectorCaptureEnabled_ = false;
    std::vector<InspectorSnapshot> inspectorTrace_;
    size_t inspectorCursor_ = 0;
    std::atomic<bool> stopExecutionRequested_{false};
    mutable std::mutex lastRegistersMu_;
    std::array<int64_t, 32> lastRegisters_{};
};

} // namespace hooc

#endif // HVM_HVM_JIT_H
