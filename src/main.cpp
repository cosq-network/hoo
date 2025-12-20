#include <iostream>
#include <fstream>
#include <string>
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "HooCompiler.h"
#include "HoocJIT.h"

using namespace llvm;
using namespace hooc;

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file '" << filename << "'\n";
        return "";
    }
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    
    return content;
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <source_file.hoo>\n";
    std::cout << "       " << programName << " --help\n\n";
    std::cout << "hooc - The hooc Programming Language Compiler\n";
    std::cout << "Compiles and executes hoo source files using JIT compilation.\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " hello.hoo\n";
    std::cout << "  " << programName << " examples/fibonacci.hoo\n";
}

int main(int argc, char* argv[]) {
    InitLLVM X(argc, argv);
    
    // Check command line arguments
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string arg = argv[1];
    if (arg == "--help" || arg == "-h") {
        printUsage(argv[0]);
        return 0;
    }
    
    std::string filename = arg;
    std::cout << "=== hooc Compiler ===\n";
    std::cout << "Source file: " << filename << "\n\n";
    
    // Step 1: Read source file
    std::string sourceCode = readFile(filename);
    if (sourceCode.empty()) {
        return 1;
    }
    
    std::cout << "Source code:\n";
    std::cout << "```hoo\n" << sourceCode << "```\n\n";
    
    try {
        // Step 2: Compile source to LLVM IR
        std::cout << "Step 1: Compiling...\n";
        HooCompiler compiler;
        
        std::string moduleName = filename.substr(filename.find_last_of("/\\") + 1);
        if (moduleName.length() > 4) {
            moduleName = moduleName.substr(0, moduleName.length() - 4); // Remove .hoo extension
        }
        
        auto module = compiler.compile(moduleName, sourceCode);
        if (!module) {
            std::cerr << "Compilation failed: " << compiler.getLastError() << "\n";
            return 1;
        }
        
        std::cout << "✅ Compilation successful\n\n";
        
        // Step 3: Display generated LLVM IR
        std::cout << "Step 2: Generated LLVM IR:\n";
        std::string moduleStr;
        llvm::raw_string_ostream stream(moduleStr);
        module->print(stream, nullptr);
        std::cout << moduleStr << "\n";
        
        // Step 4: JIT execution preparation
        std::cout << "Step 3: JIT execution...\n";
        std::cout << "HoocJIT available for module execution\n";
        std::cout << "🚧 JIT execution integration pending\n";
        
        std::cout << "\n🎉 Compilation pipeline completed successfully!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
