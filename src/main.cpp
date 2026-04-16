#include <iostream>
#include <fstream>
#include <string>
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
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
    
    std::string sourceCode = readFile(filename);
    if (sourceCode.empty()) {
        return 1;
    }
    
    std::cout << "Source code:\n";
    std::cout << "```hoo\n" << sourceCode << "```\n\n";
    
    try {
        std::cout << "Initializing HoocJIT...\n";
        HoocJIT jit;
        
        std::string moduleName = filename.substr(filename.find_last_of("/\\") + 1);
        if (moduleName.length() > 4) {
            moduleName = moduleName.substr(0, moduleName.length() - 4);
        }
        
        std::cout << "\nStep 1: Compiling...\n";
        auto result = jit.compile(moduleName, sourceCode);
        
        if (!result.success) {
            std::cerr << "Compilation failed: " << result.error << "\n";
            return 1;
        }
        std::cout << "Compilation successful!\n\n";
        
        if (!result.ir.empty()) {
            std::cout << "Step 2: Generated LLVM IR:\n";
            std::cout << result.ir << "\n";
        }
        
        std::cout << "Step 3: JIT execution...\n";
        auto execResult = jit.execute("main");
        if (execResult) {
            std::cout << "Function executed successfully!\n";
        } else {
            std::cout << "Note: " << execResult->error << "\n";
            std::cout << "(main function may not be defined or has different signature)\n";
        }
        
        std::cout << "\nCompilation pipeline completed successfully!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
