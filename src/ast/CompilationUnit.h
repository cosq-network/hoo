#pragma once

#include "ASTNode.h"
#include <memory>
#include <vector>

namespace hooc {
namespace ast {

class ImportStatement;
class Declaration;

/**
 * \class CompilationUnit
 * \brief Root AST node representing a complete source file.
 *
 * Contains all import statements and declarations in a compilation unit.
 * This is the top-level node of the Abstract Syntax Tree.
 */
class CompilationUnit : public ASTNode {
public:
    CompilationUnit(std::vector<std::unique_ptr<ImportStatement>> imports,
                   std::vector<std::unique_ptr<Declaration>> declarations)
        : imports_(std::move(imports)), declarations_(std::move(declarations)) {}

    // Copy operations are deleted because of unique_ptr members
    CompilationUnit(const CompilationUnit&) = delete;
    CompilationUnit& operator=(const CompilationUnit&) = delete;

    // Move operations are defaulted for efficient transfers
    CompilationUnit(CompilationUnit&&) = default;
    CompilationUnit& operator=(CompilationUnit&&) = default;

    std::string toString() const override;

    const std::vector<std::unique_ptr<ImportStatement>>& getImports() const { return imports_; }
    const std::vector<std::unique_ptr<Declaration>>& getDeclarations() const { return declarations_; }

    // Convenience methods for checking contents
    bool hasImports() const { return !imports_.empty(); }
    bool hasDeclarations() const { return !declarations_.empty(); }
    size_t getImportCount() const { return imports_.size(); }
    size_t getDeclarationCount() const { return declarations_.size(); }

    // Iterator access for range-based for loops
    auto begin_imports() const { return imports_.begin(); }
    auto end_imports() const { return imports_.end(); }
    auto begin_declarations() const { return declarations_.begin(); }
    auto end_declarations() const { return declarations_.end(); }

private:
    std::vector<std::unique_ptr<ImportStatement>> imports_;
    std::vector<std::unique_ptr<Declaration>> declarations_;
};

} // namespace ast
} // namespace hooc