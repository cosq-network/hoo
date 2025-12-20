#pragma once

#include <string>
#include <vector>

namespace hooc {

/**
 * Parser that uses process isolation to avoid ANTLR4 global state issues.
 * Each parse call spawns a fresh hooc_parse process.
 */
class ProcessIsolatedParser {
public:
    ProcessIsolatedParser();
    ~ProcessIsolatedParser();
    
    /**
     * Parse source code using isolated process
     */
    bool parse(const std::string& source);
    
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
    
    // Execute parser process and capture output
    std::vector<std::string> executeParser(const std::string& source);
};

} // namespace hooc