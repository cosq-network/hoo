#pragma once

#include <string>
#include <vector>
#include <memory>
#include "HoocParser.h"
#include "HoocLexer.h"
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
     * Parse source code directly and return parse tree context
     * This is needed for AST building.
     */
    HoocParser::CompilationUnitContext* parseForAST(const std::string& source);
    
    /**
     * Get the result of the last parse
     */
    bool wasSuccessful() const { return lastParseSuccessful_; }
    
    /**
     * Get error message from last parse
     */
    const std::string& getLastError() const { return lastError_; }
    
private:
    bool lastParseSuccessful_;
    std::string lastError_;
    
    // Direct parsing components (for AST building)
    std::unique_ptr<antlr4::ANTLRInputStream> input_;
    std::unique_ptr<HoocLexer> lexer_;
    std::unique_ptr<antlr4::CommonTokenStream> tokens_;
    std::unique_ptr<HoocParser> parser_;
    HoocParser::CompilationUnitContext* currentParseTree_;
};

} // namespace hooc