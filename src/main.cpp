#include <iostream>
#include <fstream>
#include <string>
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "HoocJIT.h"
#include "ProcessIsolatedParser.h"
#include "SimpleASTBuilder.h"
#include "CodeGenerator.h"
#include "../antlr4/generated/HoocParser.h"

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
        // Step 2: Parse the source code  
        std::cout << "Step 1: Parsing...\n";
        ProcessIsolatedParser parser;
        
        // Parse and get parse tree for AST building
        auto* parseTree = parser.parseForAST(sourceCode);
        if (!parseTree) {
            std::cerr << "Parse error: " << parser.getLastError() << "\n";
            return 1;
        }
        std::cout << "✅ Parse tree generated\n\n";
        
        // Step 3: Build AST from parse tree
        std::cout << "Step 2: Building AST...\n";
        SimpleASTBuilder astBuilder;
        auto ast = astBuilder.buildAST(parseTree);
        if (!ast) {
            std::cerr << "AST building failed\n";
            return 1;
        }
        std::cout << "✅ AST built successfully\n";
        std::cout << "AST: " << ast->toString() << "\n\n";
        
        // Step 4: Generate LLVM IR from AST
        std::cout << "Step 3: Code generation...\n";
        llvm::LLVMContext context;
        CodeGenerator codeGen(context);
        auto module = codeGen.generateModule(*ast);
        if (!module) {
            std::cerr << "Code generation failed\n";
            return 1;
        }
        std::cout << "✅ LLVM IR generated\n";
        
        // Print the generated IR
        std::string irString;
        llvm::raw_string_ostream rso(irString);
        module->print(rso, nullptr);
        std::cout << "Generated LLVM IR:\n" << irString << "\n";
        
        // Step 5: JIT compile and execute
        std::cout << "Step 4: JIT execution...\n";
        HoocJIT jit;
        // TODO: Add method to JIT compile and execute the module
        std::cout << "🚧 JIT execution integration pending\n\n";
        
        std::cout << "🎉 Compilation pipeline completed successfully!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
