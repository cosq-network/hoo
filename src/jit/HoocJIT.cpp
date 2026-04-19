#include "HoocJIT.h"

#include "../core/HooCompiler.h"
#include "runtime/llvm/RuntimeRegistry.h"

#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::orc;
using namespace hooc;

// ============================================================================
// Lifecycle
// ============================================================================

HoocJIT::HoocJIT() {
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize JIT: " + lastError_);
    }
}

HoocJIT::~HoocJIT() = default;

HoocJIT::HoocJIT(HoocJIT&& other) noexcept
    : jit_(std::move(other.jit_))
    , tsc_(std::move(other.tsc_))
    , compiler_(std::move(other.compiler_))
    , lastError_(std::move(other.lastError_)) {
}

HoocJIT& HoocJIT::operator=(HoocJIT&& other) noexcept {
    if (this != &other) {
        jit_       = std::move(other.jit_);
        tsc_       = std::move(other.tsc_);
        compiler_  = std::move(other.compiler_);
        lastError_ = std::move(other.lastError_);
    }
    return *this;
}

// ============================================================================
// Compilation
// ============================================================================

CompileResult HoocJIT::compile(const std::string& moduleName,
                               const std::string& sourceCode) {
    lastError_.clear();

    auto module = compiler_->compile(moduleName, sourceCode);

    if (!module) {
        return CompileResult::fail(compiler_->getLastError());
    }

    std::string ir;
    if (!verifyAndAddModule(std::move(module), ir)) {
        return CompileResult::fail(lastError_);
    }

    return CompileResult::ok(ir);
}

// ============================================================================
// Execution
// ============================================================================

std::optional<ExecutionResult> HoocJIT::execute(const std::string& functionName) {
    auto addrOpt = lookupAddress(functionName);
    if (!addrOpt) {
        return std::nullopt;
    }

    using FuncPtr = void(*)();
    auto funcPtr = reinterpret_cast<FuncPtr>(*addrOpt);

    try {
        funcPtr();
        return ExecutionResult::ok();
    }
    catch (const std::exception& e) {
        setError(std::string("Exception during execution: ") + e.what());
        return std::nullopt;
    }
    catch (...) {
        setError("Unknown exception during execution");
        return std::nullopt;
    }
}

// ============================================================================
// Lookup
// ============================================================================

std::optional<Symbol> HoocJIT::lookup(const std::string& symbolName) {
    auto addrOpt = lookupAddress(symbolName);
    if (!addrOpt) {
        return std::nullopt;
    }

    return Symbol(ExecutorAddr(*addrOpt), JITSymbolFlags::Exported);
}

std::optional<llvm::JITTargetAddress> HoocJIT::lookupAddress(const std::string& symbolName) {
    auto symbolOrError = jit_->lookup(symbolName);

    if (!symbolOrError) {
        std::ostringstream oss;
        oss << "Symbol lookup failed for '" << symbolName << "': "
            << toString(symbolOrError.takeError());
        setError(oss.str());
        return std::nullopt;
    }

    return symbolOrError->getValue();
}

// ============================================================================
// Internal Helpers
// ============================================================================

bool HoocJIT::initialize() {
    lastError_.clear();

    // Initialize LLVM targets (safe to call multiple times)
    // These are idempotent operations - LLVM internally guards against
    // multiple initialization, but we call them here for completeness
    static bool llvmInitialized = []() {
        auto nativeTargetResult = InitializeNativeTarget();
        auto asmPrinterResult = InitializeNativeTargetAsmPrinter();
        auto asmParserResult = InitializeNativeTargetAsmParser();

        #ifdef DEBUG_JIT_INIT
        std::cerr << "LLVM Initialization Results:" << std::endl;
        std::cerr << "  InitializeNativeTarget: " << nativeTargetResult << std::endl;
        std::cerr << "  InitializeNativeTargetAsmPrinter: " << asmPrinterResult << std::endl;
        std::cerr << "  InitializeNativeTargetAsmParser: " << asmParserResult << std::endl;
        #endif

        return true;
    }();
    (void)llvmInitialized; // Suppress unused variable warning

    auto jitExpected = LLJITBuilder().create();

    if (!jitExpected) {
        std::ostringstream oss;
        oss << "Failed to create JIT: " << toString(jitExpected.takeError());
        setError(oss.str());
        return false;
    }

    jit_ = std::move(*jitExpected);

    auto& registry = runtime::RuntimeRegistry::getInstance();
    registry.registerAllWithJIT(*jit_, jit_->getMainJITDylib());

    tsc_ = ThreadSafeContext(std::make_unique<LLVMContext>());
    
    llvm::LLVMContext* contextPtr = nullptr;
    tsc_.withContextDo([&contextPtr](llvm::LLVMContext* ctx) {
        contextPtr = ctx;
    });
    
    compiler_ = std::make_unique<HooCompiler>(contextPtr);

    return true;
}

bool HoocJIT::verifyAndAddModule(std::unique_ptr<Module> module,
                                 std::string& outIR) {
    if (!module) {
        setError("Cannot verify null module");
        return false;
    }

    if (verifyModule(*module, &errs())) {
        setError("LLVM module verification failed");
        return false;
    }

    outIR = getIRFromModule(*module);

    return addModuleToJIT(std::move(module));
}

bool HoocJIT::addModuleToJIT(std::unique_ptr<Module> module) {
    if (!module) {
        setError("Cannot add null module to JIT");
        return false;
    }

    ThreadSafeModule tsm(std::move(module), tsc_);
    auto error = jit_->addIRModule(std::move(tsm));

    if (error) {
        std::ostringstream oss;
        oss << "Failed to add module to JIT: " << toString(std::move(error));
        setError(oss.str());
        return false;
    }

    return true;
}

std::string HoocJIT::getIRFromModule(const Module& module) const {
    std::string ir;
    raw_string_ostream stream(ir);
    module.print(stream, nullptr);
    stream.flush();
    return ir;
}
