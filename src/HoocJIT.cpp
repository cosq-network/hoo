#include "HoocJIT.h"
#include "HooCompiler.h"
#include "runtime/RuntimeRegistry.h"

#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include <sstream>

using namespace llvm;
using namespace llvm::orc;
using namespace hooc;

// ============================================================================
// Construction / Destruction
// ============================================================================

HoocJIT::HoocJIT() {
    if (!initializeJIT()) {
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
        jit_      = std::move(other.jit_);
        compiler_ = std::move(other.compiler_);
        lastError_ = std::move(other.lastError_);
    }
    return *this;
}

// ============================================================================
// JIT Initialization
// ============================================================================

bool HoocJIT::initializeJIT() {
    lastError_.clear();

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    auto jitExpected = LLJITBuilder().create();

    if (!jitExpected) {
        std::ostringstream oss;
        oss << "Failed to create JIT: " << toString(jitExpected.takeError());
        lastError_ = oss.str();
        return false;
    }

    jit_ = std::move(*jitExpected);

    auto& registry = runtime::RuntimeRegistry::getInstance();
    auto& mainJD  = jit_->getMainJITDylib();
    registry.registerAllWithJIT(*jit_, mainJD);

    compiler_ = std::make_unique<HooCompiler>();

    return true;
}

// ============================================================================
// Compilation API
// ============================================================================

HoocJIT::CompileResult HoocJIT::compile(
    const std::string& moduleName,
    const std::string& sourceCode) {

    lastError_.clear();

    auto module = compiler_->compile(moduleName, sourceCode);

    if (!module) {
        lastError_ = compiler_->getLastError();
        return {false, lastError_, {}};
    }

    if (verifyModule(*module, &errs())) {
        lastError_ = "LLVM module verification failed";
        return {false, lastError_, {}};
    }

    std::string ir;
    raw_string_ostream stream(ir);
    module->print(stream, nullptr);
    stream.flush();

    if (!addModule(std::move(module))) {
        return {false, lastError_, {}};
    }

    return {true, {}, ir};
}

bool HoocJIT::addModule(std::unique_ptr<Module> module) {
    if (!module) {
        lastError_ = "Cannot add null module to JIT";
        return false;
    }

    ThreadSafeModule tsm(std::move(module), std::make_unique<LLVMContext>());
    auto error = jit_->addIRModule(std::move(tsm));

    if (error) {
        std::ostringstream oss;
        oss << "Failed to add module to JIT: " << toString(std::move(error));
        lastError_ = oss.str();
        return false;
    }

    return true;
}

// ============================================================================
// Execution API
// ============================================================================

HoocJIT::ExecutionResult HoocJIT::executeRaw(const std::string& functionName) {
    auto symbolOrError = jit_->lookup(functionName);

    if (!symbolOrError) {
        std::ostringstream oss;
        oss << "Function '" << functionName << "' not found: "
            << toString(symbolOrError.takeError());
        return {false, oss.str()};
    }

    auto addr = symbolOrError->getValue();
    using FuncPtr = void(*)();
    auto funcPtr = reinterpret_cast<FuncPtr>(static_cast<uintptr_t>(addr));

    funcPtr();
    return {true, {}};
}

std::optional<HoocJIT::Symbol> HoocJIT::lookup(const std::string& symbolName) {
    auto symbolOrError = jit_->lookup(symbolName);

    if (!symbolOrError) {
        std::ostringstream oss;
        oss << "Symbol lookup failed for '" << symbolName << "': "
            << toString(symbolOrError.takeError());
        lastError_ = oss.str();
        return std::nullopt;
    }

    return Symbol(*symbolOrError, JITSymbolFlags::Exported);
}
