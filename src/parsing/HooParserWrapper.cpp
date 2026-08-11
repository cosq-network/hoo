#include "HooParserWrapper.h"
#include "HoocLexer.h"
#include "HoocParser.h"
#include "antlr4-runtime.h"
#include <iostream>
#include <sstream>
#include <array>
#include <memory>
#include <stdexcept>

namespace hooc {

HooParserWrapper::HooParserWrapper() 
    : lastParseSuccessful_(false), currentParseTree_(nullptr) {
}

HooParserWrapper::~HooParserWrapper() {
    // Clean up ANTLR4 objects
    currentParseTree_ = nullptr;
}

HoocParser::CompilationUnitContext* HooParserWrapper::parseForAST(const std::string& source) {
    const char* stage = "initializing";
    try {
        currentParseTree_ = nullptr;
        
        // Create local ANTLR4 components
        stage = "creating ANTLR input stream";
        input_ = std::make_unique<antlr4::ANTLRInputStream>(source);
        
        stage = "creating Hooc lexer";
        lexer_ = std::make_unique<HoocLexer>(input_.get());
        
        stage = "creating token stream";
        tokens_ = std::make_unique<antlr4::CommonTokenStream>(lexer_.get());
        
        stage = "creating Hooc parser";
        parser_ = std::make_unique<HoocParser>(tokens_.get());
        
        stage = "parsing compilation unit";
        currentParseTree_ = parser_->compilationUnit();

        // Check for parsing errors
        bool hasErrors = (parser_->getNumberOfSyntaxErrors() > 0);
        
        lastParseSuccessful_ = (currentParseTree_ != nullptr && !hasErrors);
        
        if (!lastParseSuccessful_) {
            if (hasErrors) {
                lastError_ = "Syntax errors detected during parsing";
            } else {
                lastError_ = "Failed to parse compilation unit";
            }
            return nullptr;
        }
        
        return currentParseTree_;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Parse error while ") + stage + ": " + e.what();
        lastParseSuccessful_ = false;
        return nullptr;
    }
}

HoocParser::ExpressionContext* HooParserWrapper::parseExpression(const std::string& source) {
    const char* stage = "initializing expression parse";
    try {
        input_ = std::make_unique<antlr4::ANTLRInputStream>(source);
        lexer_ = std::make_unique<HoocLexer>(input_.get());
        tokens_ = std::make_unique<antlr4::CommonTokenStream>(lexer_.get());
        parser_ = std::make_unique<HoocParser>(tokens_.get());
        
        auto* exprCtx = parser_->expression();
        bool hasErrors = (parser_->getNumberOfSyntaxErrors() > 0);
        lastParseSuccessful_ = (exprCtx != nullptr && !hasErrors);
        
        if (!lastParseSuccessful_) {
            lastError_ = "Syntax errors detected during expression parsing";
            return nullptr;
        }
        return exprCtx;
    } catch (const std::exception& e) {
        lastError_ = std::string("Expression parse error while ") + stage + ": " + e.what();
        lastParseSuccessful_ = false;
        return nullptr;
    }
}

} // namespace hooc
