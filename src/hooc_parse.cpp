#include <iostream>
#include <string>
#include <fstream>
#include <memory>

#include "antlr4-runtime.h"
#include "HoocLexer.h"
#include "HoocParser.h"

/**
 * Standalone parser utility that runs ANTLR4 parsing in isolation.
 * This avoids global state corruption by running each parse in a fresh process.
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source_code>" << std::endl;
        return 1;
    }
    
    std::string source = argv[1];
    
    try {
        // Create ANTLR4 parser instances
        antlr4::ANTLRInputStream input(source);
        ::hooc::HoocLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        ::hooc::HoocParser parser(&tokens);
        
        // Remove error listeners
        lexer.removeErrorListeners();
        parser.removeErrorListeners();
        
        // Parse
        auto parseTree = parser.compilationUnit();
        if (parseTree) {
            std::cout << "SUCCESS" << std::endl;
            std::cout << parseTree->children.size() << std::endl;
            std::cout << parseTree->toStringTree(&parser) << std::endl;
        } else {
            std::cout << "FAILED" << std::endl;
            std::cout << "Parse tree creation failed" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "ERROR" << std::endl;
        std::cout << e.what() << std::endl;
    }
    
    return 0;
}