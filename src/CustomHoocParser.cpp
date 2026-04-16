#include "HoocParser.h"
#include "ast/AST.h"
#include <iostream>
#include <memory>

// Include ANTLR4 generated headers
#include "HoocLexer.h"
#include "HoocParser.h"
#include "antlr4-runtime.h"

namespace hooc {

HoocParser::HoocParser() : lastParseSuccessful_(false) {}

HoocParser::~HoocParser() {}

std::unique_ptr<ASTNode> HoocParser::parse(const std::string& source) {
    lastError_.clear();
    lastParseSuccessful_ = false;
    
    try {
        auto result = parseInternal(source);
        if (result) {
            lastParseSuccessful_ = true;
        }
        return result;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return nullptr;
    } catch (...) {
        lastError_ = "Unknown parsing error occurred";
        return nullptr;
    }
}

std::unique_ptr<ASTNode> HoocParser::parseInternal(const std::string& source) {
    // Create completely isolated ANTLR4 instances
    auto input = std::make_unique<antlr4::ANTLRInputStream>(source);
    auto lexer = std::make_unique<::hooc::HoocLexer>(input.get());
    auto tokens = std::make_unique<antlr4::CommonTokenStream>(lexer.get());
    auto parser = std::make_unique<::hooc::HoocParser>(tokens.get());
    
    // Remove error listeners to avoid state issues
    lexer->removeErrorListeners();
    parser->removeErrorListeners();
    
    // Parse the compilation unit
    auto parseTree = parser->compilationUnit();
    if (!parseTree) {
        lastError_ = "Failed to create parse tree";
        return nullptr;
    }
    
    // For now, create a simple CompilationUnit AST node
    // TODO: Integrate with ASTBuilder when std::any issues are fixed
    auto compilationUnit = std::make_unique<ast::CompilationUnit>(
        std::vector<std::unique_ptr<ast::ImportStatement>>(),
        std::vector<std::unique_ptr<ast::Declaration>>()
    );
    
    return std::move(compilationUnit);
}

} // namespace hooc