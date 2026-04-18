#include "core/HooCLI.h"
#include "core/DefaultIOProvider.h"

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"

using namespace hooc;
using namespace llvm;

int main(int argc, char* argv[]) {
    InitLLVM llvm(argc, argv);

    // Initialize LLVM JIT targets
    // Required for JIT execution functionality
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    HooCLI cli(std::make_unique<DefaultIOProvider>());

    return cli.run(argc, argv);
}