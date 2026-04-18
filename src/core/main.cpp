#include "core/HooCLI.h"
#include "core/DefaultIOProvider.h"

#include "llvm/Support/InitLLVM.h"

using namespace hooc;
using namespace llvm;

int main(int argc, char* argv[]) {
    InitLLVM llvm(argc, argv);

    HooCLI cli(std::make_unique<DefaultIOProvider>());

    return cli.run(argc, argv);
}