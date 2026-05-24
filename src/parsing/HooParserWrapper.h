#pragma once

#include <string>
#include <vector>
#include <memory>
#include "HoocParser.h"
#include "HoocLexer.h"
#include "antlr4-runtime.h"

namespace hooc {

/**
 * Wrapper for the ANTLR4-generated Hooc parser.
 * This class encapsulates the ANTLR4 runtime components and provides
 * a simplified interface for producing parse trees for AST building.
 */
class HooParserWrapper {
public:
    HooParserWrapper();
    ~HooParserWrapper();
    
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