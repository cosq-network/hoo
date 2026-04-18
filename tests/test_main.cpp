#include <gtest/gtest.h>
#include <llvm/Support/TargetSelect.h>
#include <iostream>

// Main function for Google Test
int main(int argc, char **argv) {
    // Initialize LLVM targets once globally before running tests
    // This is required for JIT functionality and must be done before any
    // HoocJIT instances are created

    // Initialize native target with error checking
    if (llvm::InitializeNativeTarget()) {
        std::cerr << "ERROR: Failed to initialize native target" << std::endl;
        std::cerr << "JIT tests will likely fail" << std::endl;
        // Continue anyway to see what happens
    } else {
        std::cout << "Native target initialized successfully" << std::endl;
    }

    if (llvm::InitializeNativeTargetAsmPrinter()) {
        std::cerr << "ERROR: Failed to initialize native target ASM printer" << std::endl;
    } else {
        std::cout << "Native ASM printer initialized successfully" << std::endl;
    }

    if (llvm::InitializeNativeTargetAsmParser()) {
        std::cerr << "ERROR: Failed to initialize native target ASM parser" << std::endl;
    } else {
        std::cout << "Native ASM parser initialized successfully" << std::endl;
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}