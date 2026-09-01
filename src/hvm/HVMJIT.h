#pragma once

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
        UnsupportedFeature,
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
        int64_t vregs[32][8]{};
        int64_t vl = 0;
        int64_t vtype = 0;
        uint64_t reservationAddr = UINT64_MAX;
        uint64_t tlabStart = 0;
        uint64_t tlabEnd = 0;
        uint64_t csrs[12]{}; // HVM system-profile CSR window 0x000..0x00B
    };

    // HVM system-profile CSR addresses (see docs/hvm/hvm-spec.md section 9.2).
    static constexpr uint64_t kCsrSstatus = 0x000;
    static constexpr uint64_t kCsrStvec = 0x001;
    static constexpr uint64_t kCsrSepc = 0x002;
    static constexpr uint64_t kCsrScause = 0x003;
    static constexpr uint64_t kCsrStval = 0x004;
    static constexpr uint64_t kCsrSatp = 0x005;
    static constexpr uint64_t kCsrStime = 0x006;
    static constexpr uint64_t kCsrStimecmp = 0x007;
    static constexpr uint64_t kCsrFeature0 = 0x008;
    static constexpr uint64_t kCsrBadInstruction = 0x009;
    static constexpr uint64_t kCsrSip = 0x00A;
    static constexpr uint64_t kCsrSie = 0x00B;
    static constexpr uint64_t kCsrCount = 0x00C;

    // sip/sie interrupt pending/enable bits (HVM system profile).
    static constexpr uint64_t kSipStip = 1ULL << 0; // supervisor timer
    static constexpr uint64_t kSipSsip = 1ULL << 1; // supervisor software
    static constexpr uint64_t kSipSeip = 1ULL << 2; // supervisor external

    // Synchronous exception / interrupt cause codes (scause).
    static constexpr uint64_t kCauseIllegalInstruction = 2;
    static constexpr uint64_t kCauseBreakpoint = 3;
    static constexpr uint64_t kCauseEcallU = 8;
    static constexpr uint64_t kCauseEcallS = 9;
    static constexpr uint64_t kCauseSyscallU = 16;
    static constexpr uint64_t kCauseSyscallS = 17;
    static constexpr uint64_t kCauseArithmeticOverflow = 18;
    static constexpr uint64_t kCauseDivisionByZero = 19;
    static constexpr uint64_t kCauseNullOrBounds = 20;
    static constexpr uint64_t kCauseInterruptBit = 1ULL << 63;
    static constexpr uint64_t kCauseTimerInterrupt = kCauseInterruptBit | 0;
    static constexpr uint64_t kCauseSoftwareInterrupt = kCauseInterruptBit | 1;
    static constexpr uint64_t kCauseExternalInterrupt = kCauseInterruptBit | 9;

    // feature0 (CSR 0x008) is read-only and reports the implemented HVM
    // feature set. See docs/hvm/hvm-spec.md section 9.2 for the bit layout.
    static constexpr uint64_t kFeatureBaseCore = 1ULL << 0;   // hvm64-core-system
    static constexpr uint64_t kFeatureGreenCompute = 1ULL << 1; // RETAIN/RELEASE/ICACHE.RNG/LD.P/ST.P
    static constexpr uint64_t kFeatureSubWord = 1ULL << 2;    // HVM 1.6 scalar sub-word profile
    static constexpr uint64_t kFeatureVector = 1ULL << 3;     // HVM-V
    static constexpr uint64_t kFeatureHardwareLoop = 1ULL << 4; // HVM-L
    static constexpr uint64_t kFeatureAdvisory = 1ULL << 5;   // PREFETCH.*/MEMZERO.HINT/BR.HINT
    static constexpr uint64_t kFeatureAlloc = 1ULL << 6;      // HVM-Alloc
    static constexpr uint64_t kFeatureProf = 1ULL << 7;       // HVM-Prof
    static constexpr uint64_t kFeatureCap = 1ULL << 8;        // HVM-Cap
    static constexpr uint64_t kFeatureNz = 1ULL << 9;         // HVM-NZ
    static constexpr uint64_t kFeatureAccel = 1ULL << 10;     // HVM-A (not implemented in hosted profile)
    // Bit 11 marks a silicon-MVP-capable core (Bare + green-compute contract).
    static constexpr uint64_t kFeatureSiliconMvp = 1ULL << 11;

    // Feature set reported by the hosted interpreter/JIT profile. Bits 0..9 and
    // SiliconMvp are set; HVM-A (bit 10) is not.
    static constexpr uint64_t kHostedFeature0 =
        kFeatureBaseCore | kFeatureGreenCompute | kFeatureSubWord |
        kFeatureVector | kFeatureHardwareLoop | kFeatureAdvisory |
        kFeatureAlloc | kFeatureProf | kFeatureCap | kFeatureNz |
        kFeatureSiliconMvp;

    // Silicon MVP feature0 subset for first FPGA/ASIC cores (see hvm-spec §10).
    static constexpr uint64_t kSiliconMvpFeature0 =
        kFeatureBaseCore | kFeatureGreenCompute | kFeatureSiliconMvp;

    static void initResetState(HVMState& state) {
        state.csrs[kCsrFeature0] = kHostedFeature0;
    }

    // Record a precise synchronous trap into the architectural CSR window.
    // Hosted execution still returns -1 for unhandled traps; the CSRs remain
    // readable so silicon and simulator profiles share one observation contract.
    static void recordSynchronousTrap(HVMState& state, uint64_t faultPc, uint64_t cause,
                                      uint64_t stval, uint64_t badInstruction) {
        state.csrs[kCsrSepc] = faultPc;
        state.csrs[kCsrScause] = cause;
        state.csrs[kCsrStval] = stval;
        state.csrs[kCsrBadInstruction] = badInstruction;
        state.reservationAddr = UINT64_MAX;
        state.trapHit = true;
    }

    static uint64_t encodeInstructionWord(const hvm::HVMInstruction& ins) {
        const auto bytes = ins.encode();
        uint64_t word = 0;
        for (size_t i = 0; i < bytes.size() && i < 8; ++i) {
            word |= static_cast<uint64_t>(bytes[i]) << (8U * static_cast<unsigned>(i));
        }
        return word;
    }

    static bool isReadOnlyCsr(uint64_t csr) {
        return csr == kCsrStime || csr == kCsrFeature0 || csr == kCsrBadInstruction;
    }
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
    std::array<uint64_t, kCsrCount> getCsrs() const;
    std::vector<uint8_t> readVirtualMemory(uint64_t addr, size_t size) const;
    bool getStopExecutionRequested() const { return stopExecutionRequested_.load(std::memory_order_relaxed); }

    void setTLAB(uint64_t start, uint64_t end) {
        tlabStart_ = start;
        tlabEnd_ = end;
    }

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
    void captureLastArchitecturalState(const HVMState& state);
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
    uint32_t callDepth_ = 0;
    static constexpr uint32_t kMaxCallDepth = 1024;
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
    std::array<uint64_t, kCsrCount> lastCsrs_{};
    uint64_t tlabStart_ = 0;
    uint64_t tlabEnd_ = 0;
};

} // namespace hooc

