#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../antlr4/generated/HoocParser.h"
#include "../antlr4/generated/HoocLexer.h"
#include "antlr4-runtime.h"

namespace hooc {

/**
 * Parser that can work in both isolated and direct modes.
 * Isolated mode uses process isolation to avoid ANTLR4 global state issues.
 * Direct mode provides access to parse trees for AST building.
 */
class ProcessIsolatedParser {
public:
    ProcessIsolatedParser();
    ~ProcessIsolatedParser();
    
    /**
     * Parse source code using isolated process (validation only)
     */
    bool parse(const std::string& source);
    
    /**
     * Parse source code directly and return parse tree context
     * This is needed for AST building but may have ANTLR4 state issues
     */
    HoocParser::CompilationUnitContext* parseForAST(const std::string& source);
    
    /**
     * Get the result of the last parse
     */
    bool wasSuccessful() const { return lastParseSuccessful_; }
    
    /**
     * Get the parse tree string from last successful parse
     */
    const std::string& getParseTreeString() const { return parseTreeString_; }
    
    /**
     * Get the number of children in the parse tree
     */
    size_t getParseTreeChildCount() const { return parseTreeChildCount_; }
    
    /**
     * Get error message from last parse
     */
    const std::string& getLastError() const { return lastError_; }
    
private:
    bool lastParseSuccessful_;
    std::string parseTreeString_;
    size_t parseTreeChildCount_;
    std::string lastError_;
    std::string parserExecutablePath_;
    
    // Direct parsing components (for AST building)
    std::unique_ptr<antlr4::ANTLRInputStream> input_;
    std::unique_ptr<HoocLexer> lexer_;
    std::unique_ptr<antlr4::CommonTokenStream> tokens_;
    std::unique_ptr<HoocParser> parser_;
    HoocParser::CompilationUnitContext* currentParseTree_;
    
    // Execute parser process and capture output
    std::vector<std::string> executeParser(const std::string& source);
};

} // namespace hooc