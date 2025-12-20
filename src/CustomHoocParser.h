#pragma once

#include <string>
#include <memory>

namespace hooc {

// Forward declaration
class ASTNode;

/**
 * Single-shot parser that creates isolated ANTLR4 instances
 * to avoid global state corruption issues.
 */
class HoocParser {
public:
    HoocParser();
    ~HoocParser();
    
    /**
     * Parse hooc source code and return AST root node.
     * Each call creates a completely fresh parser instance.
     */
    std::unique_ptr<ASTNode> parse(const std::string& source);
    
    /**
     * Check if the last parse was successful
     */
    bool wasSuccessful() const { return lastParseSuccessful_; }
    
    /**
     * Get the last error message
     */
    const std::string& getLastError() const { return lastError_; }

private:
    bool lastParseSuccessful_;
    std::string lastError_;
    
    // Internal method to handle parsing with proper cleanup
    std::unique_ptr<ASTNode> parseInternal(const std::string& source);
};

} // namespace hooc