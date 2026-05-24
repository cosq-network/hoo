#ifndef HOO_COMPILER_H
#define HOO_COMPILER_H

////////////////////////////////////////////////////////////////////////////////
/// @file HooCompiler.h
/// @brief Main compiler interface for the Hooc language
///
/// @mainpage HooCompiler
///
/// @section overview Overview
///
/// HooCompiler is the main entry point for compiling Hooc source code. It
/// orchestrates the compilation pipeline:
///
/// 1. **Parsing**: Source code is parsed using ANTLR4 to produce a parse tree
/// 2. **AST Building**: Parse tree is converted to an Abstract Syntax Tree
/// 3. **Code Generation**: AST is translated to HVM bytecode
///
/// @section usage Usage
///
/// @code
/// HooCompiler compiler;
/// auto module = compiler.compile("myModule", sourceCode);
/// if (!module) {
///     std::cerr << "Error: " << compiler.getLastError() << std::endl;
/// }
/// @endcode
///
/// @section error_handling Error Handling
///
/// Compilation errors can be retrieved via getLastError() after a failed
/// compilation. The wasLastCompilationSuccessful() method indicates whether
/// the previous compile() call succeeded.
///
/// @see HVMCodeGenerator for the code generation backend
/// @see SimpleASTBuilder for AST construction
/// @see HooParserWrapper for parsing
///
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <memory>

#include "modules/ModuleSystem.h"

namespace hvm {
class HOModule;
}

namespace hooc {

// ============================================================================
// Forward Declarations
// ============================================================================

class HooParserWrapper;
class SimpleASTBuilder;
class CodeGenerator;

// ============================================================================
// HooCompiler - Main Compilation Interface
// ============================================================================

/// @brief Main compiler class for the Hooc programming language
/// @details Coordinates parsing, AST building, and HVM bytecode generation
/// @note Thread safety: This class is not thread-safe; synchronize access
class HooCompiler {

public:

    // ========================================================================
    // Construction / Destruction
    // ========================================================================

    /// @brief Construct a new Hooc compiler
    HooCompiler();

    /// @brief Destructor
    ~HooCompiler();

    // ========================================================================
    // Compilation API
    // ========================================================================

    /**
     * @brief Compile Hooc source code to an HVM HOModule
     * @param moduleName Name for the compiled module
     * @param sourceCode Hooc source code to compile
     * @return HVM HOModule on success, nullptr on failure
     */
    std::unique_ptr<hvm::HOModule> compile(const std::string& moduleName,
                                          const std::string& sourceCode);

    /// @brief Get error message from last failed compilation
    /// @return Error message string
    /// @note Only valid after compile() returns nullptr
    const std::string& getLastError() const {
        return lastError_;
    }

    /// @brief Check if last compilation succeeded
    /// @return true if last compile() call succeeded
    bool wasLastCompilationSuccessful() const {
        return lastCompilationSuccessful_;
    }

private:

    // ========================================================================
    // Compilation Pipeline Components
    // ========================================================================

    /// @brief Parser for source code
    std::unique_ptr<HooParserWrapper>      parser_;

    /// @brief AST builder from parse tree
    std::unique_ptr<SimpleASTBuilder>      astBuilder_;

    /// @brief HVM code generator
    std::unique_ptr<CodeGenerator>         codeGenerator_;

    // ========================================================================
    // Error State
    // ========================================================================

    /// @brief Error message from last failed compilation
    std::string lastError_;

    /// @brief Success flag from last compilation
    bool        lastCompilationSuccessful_;

    /// @brief Module registry for symbol resolution
    ModuleRegistry moduleRegistry_;

    // ========================================================================
    // Non-Copyable
    // ========================================================================

    HooCompiler(const HooCompiler&)            = delete;
    HooCompiler& operator=(const HooCompiler&) = delete;
};

}  // namespace hooc

#endif  // HOO_COMPILER_H
