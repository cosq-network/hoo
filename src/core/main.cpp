#include "core/HooCLI.h"
#include "core/DefaultIOProvider.h"

#include <cstdio>

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"

using namespace hooc;
using namespace llvm;

int main(int argc, char* argv[]) {
    InitLLVM llvm(argc, argv);

    // Initialize LLVM JIT targets
    // Required for JIT execution functionality
    if (llvm::InitializeNativeTarget()) {
        fprintf(stderr, "FATAL: Failed to initialize LLVM native target\n");
        return 1;
    }
    if (llvm::InitializeNativeTargetAsmPrinter()) {
        fprintf(stderr, "FATAL: Failed to initialize LLVM native ASM printer\n");
        return 1;
    }
    if (llvm::InitializeNativeTargetAsmParser()) {
        fprintf(stderr, "FATAL: Failed to initialize LLVM native ASM parser\n");
        return 1;
    }

    HooCLI cli(std::make_unique<DefaultIOProvider>());

    return cli.run(argc, argv);
}