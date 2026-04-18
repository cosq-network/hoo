#include <gtest/gtest.h>
#include <llvm/Support/TargetSelect.h>

// Main function for Google Test
int main(int argc, char **argv) {
    // Initialize LLVM targets once globally before running tests
    // This is required for JIT functionality and must be done before any
    // HoocJIT instances are created
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}