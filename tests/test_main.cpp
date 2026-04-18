#include <gtest/gtest.h>
#include <llvm/Support/TargetSelect.h>
#include <iostream>

// Main function for Google Test
int main(int argc, char **argv) {
    // Initialize LLVM targets once globally before running tests
    // This is required for JIT functionality and must be done before any
    // HoocJIT instances are created

    // Initialize all targets (not just native) for maximum compatibility
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // Also initialize native target specifically
    if (llvm::InitializeNativeTarget()) {
        std::cerr << "Failed to initialize native target" << std::endl;
        return 1;
    }
    if (llvm::InitializeNativeTargetAsmPrinter()) {
        std::cerr << "Failed to initialize native target ASM printer" << std::endl;
        return 1;
    }
    if (llvm::InitializeNativeTargetAsmParser()) {
        std::cerr << "Failed to initialize native target ASM parser" << std::endl;
        return 1;
    }

    std::cout << "LLVM targets initialized successfully" << std::endl;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}