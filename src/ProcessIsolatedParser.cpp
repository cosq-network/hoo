#include "ProcessIsolatedParser.h"
#include "../antlr4/generated/HoocLexer.h"
#include "../antlr4/generated/HoocParser.h"
#include "antlr4-runtime.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <array>
#include <memory>
#include <stdexcept>

namespace hooc {

ProcessIsolatedParser::ProcessIsolatedParser() 
    : lastParseSuccessful_(false), parseTreeChildCount_(0), currentParseTree_(nullptr) {
    // Assume the parser executable is in the same directory as the main executable
    parserExecutablePath_ = "./build/hooc_parse";
}

ProcessIsolatedParser::~ProcessIsolatedParser() {
    // Clean up ANTLR4 objects
    currentParseTree_ = nullptr;
}

bool ProcessIsolatedParser::parse(const std::string& source) {
    lastParseSuccessful_ = false;
    parseTreeString_.clear();
    parseTreeChildCount_ = 0;
    lastError_.clear();
    
    try {
        auto output = executeParser(source);
        
        if (output.empty()) {
            lastError_ = "No output from parser process";
            return false;
        }
        
        if (output[0] == "SUCCESS") {
            lastParseSuccessful_ = true;
            if (output.size() >= 2) {
                parseTreeChildCount_ = std::stoul(output[1]);
            }
            if (output.size() >= 3) {
                parseTreeString_ = output[2];
            }
            return true;
        } else if (output[0] == "ERROR" || output[0] == "FAILED") {
            if (output.size() >= 2) {
                lastError_ = output[1];
            } else {
                lastError_ = "Parse failed";
            }
            return false;
        } else {
            lastError_ = "Unknown parser output format";
            return false;
        }
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Process execution error: ") + e.what();
        return false;
    }
}

std::vector<std::string> ProcessIsolatedParser::executeParser(const std::string& source) {
    // Escape the source code for shell command
    std::string escapedSource = "'" + source + "'";
    
    // Build command
    std::string command = parserExecutablePath_ + " " + escapedSource;
    
    // Execute command and capture output
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    // Split result into lines
    std::vector<std::string> lines;
    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

HoocParser::CompilationUnitContext* ProcessIsolatedParser::parseForAST(const std::string& source) {
    try {
        // Clean up previous parse state
        currentParseTree_ = nullptr;
        
        // Create ANTLR4 input stream from source
        input_ = std::make_unique<antlr4::ANTLRInputStream>(source);
        
        // Create lexer
        lexer_ = std::make_unique<HoocLexer>(input_.get());
        
        // Create token stream
        tokens_ = std::make_unique<antlr4::CommonTokenStream>(lexer_.get());
        
        // Create parser
        parser_ = std::make_unique<HoocParser>(tokens_.get());
        
        // Parse the compilation unit
        currentParseTree_ = parser_->compilationUnit();
        
        lastParseSuccessful_ = (currentParseTree_ != nullptr);
        
        if (!lastParseSuccessful_) {
            lastError_ = "Failed to parse compilation unit";
            return nullptr;
        }
        
        return currentParseTree_;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Parse error: ") + e.what();
        lastParseSuccessful_ = false;
        return nullptr;
    }
}

} // namespace hooc