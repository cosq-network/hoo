#include "RuntimeRegistry.h"
#include <iostream>

namespace hooc {
namespace runtime {

// ============================================================================
// RuntimeRegistry Implementation
// ============================================================================

RuntimeRegistry& RuntimeRegistry::getInstance() {
    static RuntimeRegistry instance;
    return instance;
}

void RuntimeRegistry::registerRuntime(const RuntimeRegistrationEntry& entry) {
    runtimes_.push_back(entry);
}

void RuntimeRegistry::registerAllWithJIT(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib)
{
    if (runtimes_.empty()) {
        std::cout << "[RuntimeRegistry] No runtimes registered with JIT\n";
        return;
    }

    std::cout << "[RuntimeRegistry] Registering " << runtimes_.size()
              << " runtime(s) with JIT...\n";

    for (const auto& entry : runtimes_) {
        if (entry.jitCallback) {
            std::cout << "  [" << entry.runtimeName << "] Registering JIT symbols...\n";
            entry.jitCallback(jit, mainDylib);
        }
    }
}

void RuntimeRegistry::declareAllFunctions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData)
{
    for (const auto& entry : runtimes_) {
        if (entry.llvmCallback) {
            entry.llvmCallback(module, context, userData);
        }
    }
}

const std::vector<RuntimeRegistrationEntry>&
RuntimeRegistry::getRegisteredRuntimes() const {
    return runtimes_;
}

} // namespace runtime
} // namespace hooc
