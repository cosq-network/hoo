#include <iostream>
#include "llvm/Support/InitLLVM.h"
#include "HoocJIT.h"

using namespace llvm;

int main(int argc, char* argv[]) {
    InitLLVM X(argc, argv);
    
    std::cout << "=== Hooc JIT Compiler Demo ===\n";
    
    // Create JIT instance
    hooc::HoocJIT jit;
    
    // Create and execute a simple function
    jit.createSimpleFunction();
    jit.executeFunction();
    
    // Test parsing some simple Hooc code
    std::cout << "\n=== Testing ANTLR4 Parsing ===\n";
    jit.parseHoocCode("func test() { }");           // Empty function
    
    hooc::HoocJIT jit2;
    jit2.parseHoocCode("func calc() { return; }");  // Function with return
    
    hooc::HoocJIT jit3;
    jit3.parseHoocCode("func math() { 42; }");       // Function with expression
    
    // Test the new AST-based compilation (currently shows integration status)
    std::cout << "\n=== Testing AST-Based Compilation ===\n";
    jit.compileHoocCode("func example() { return; }");
    
    std::cout << "\nDemo completed successfully!\n";
    return 0;
}
