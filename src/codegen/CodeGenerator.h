#pragma once

#include "../ast/AST.h"
#include "CodeGeneratorTypes.h"
#include <memory>
#include <string>

namespace hooc {

/**
 * Abstract base class for code generators.
 *
 * This interface defines the contract for translating hooc AST nodes
 * into executable code.
 *
 * The interface uses opaque wrapper types to hide implementation details.
 */
class CodeGenerator {
public:
    virtual ~CodeGenerator() = default;

    /**
     * Generate a complete module/compilation unit from a hooc AST.
     * This is the main entry point for code generation.
     *
     * @param compilationUnit The root AST node to generate code from
     * @return A generated module (implementation-specific), or nullptr on failure
     */
    virtual std::unique_ptr<GeneratedModule> generateModule(const ast::CompilationUnit& compilationUnit) = 0;

    /**
     * Generate code for a function declaration.
     *
     * @param funcDecl The function declaration AST node
     * @return A function representation (implementation-specific), or nullptr on failure
     */
    virtual GeneratedFunction* generateFunction(const ast::FunctionDeclaration& funcDecl) = 0;

    /**
     * Generate code for an expression.
     *
     * @param expr The expression AST node
     * @return A value representation (implementation-specific), or nullptr on failure
     */
    virtual GeneratedValue* generateExpression(const ast::Expression& expr) = 0;

    /**
     * Generate code for a statement.
     *
     * @param stmt The statement AST node
     */
    virtual void generateStatement(const ast::Statement& stmt) = 0;

    /**
     * Generate code for a type.
     *
     * @param type The type AST node
     * @return A type representation (implementation-specific), or nullptr on failure
     */
    virtual GeneratedType* generateType(const ast::Type& type) = 0;

protected:
    // Protected constructor - only derived classes can instantiate
    CodeGenerator() = default;

    // Prevent copying
    CodeGenerator(const CodeGenerator&) = delete;
    CodeGenerator& operator=(const CodeGenerator&) = delete;
};

} // namespace hooc
