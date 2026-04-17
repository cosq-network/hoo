#include "HoocJIT.h"

#include "HooCompiler.h"
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
    , compiler_(std::move(other.compiler_))
    , lastError_(std::move(other.lastError_)) {
}

HoocJIT& HoocJIT::operator=(HoocJIT&& other) noexcept {
    if (this != &other) {
        jit_       = std::move(other.jit_);
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
    return executeVoidFunction(functionName);
}

// ============================================================================
// Lookup
// ============================================================================

std::optional<Symbol> HoocJIT::lookup(const std::string& symbolName) {
    auto symbolOrError = jit_->lookup(symbolName);

    if (!symbolOrError) {
        std::ostringstream oss;
        oss << "Symbol lookup failed for '" << symbolName << "': "
            << toString(symbolOrError.takeError());
        setError(oss.str());
        return std::nullopt;
    }

    return Symbol(*symbolOrError, JITSymbolFlags::Exported);
}

// ============================================================================
// Internal Helpers
// ============================================================================

bool HoocJIT::initialize() {
    lastError_.clear();

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    auto jitExpected = LLJITBuilder().create();

    if (!jitExpected) {
        std::ostringstream oss;
        oss << "Failed to create JIT: " << toString(jitExpected.takeError());
        setError(oss.str());
        return false;
    }

    jit_ = std::move(*jitExpected);

    auto& registry = runtime::RuntimeRegistry::getInstance();
    auto& mainJD  = jit_->getMainJITDylib();
    registry.registerAllWithJIT(*jit_, mainJD);

    compiler_ = std::make_unique<HooCompiler>();

    return true;
}

bool HoocJIT::verifyAndAddModule(std::unique_ptr<Module> module,
                                 std::string& outIR) {
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

    ThreadSafeModule tsm(std::move(module), std::make_unique<LLVMContext>());
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

ExecutionResult HoocJIT::executeVoidFunction(const std::string& functionName) {
    auto symbolOrError = jit_->lookup(functionName);

    if (!symbolOrError) {
        std::ostringstream oss;
        oss << "Function '" << functionName << "' not found: "
            << toString(symbolOrError.takeError());
        return ExecutionResult::fail(oss.str());
    }

    auto addr = symbolOrError->getValue();
    using FuncPtr = void(*)();
    auto funcPtr = reinterpret_cast<FuncPtr>(
        static_cast<uintptr_t>(addr)
    );

    funcPtr();
    return ExecutionResult::ok();
}
