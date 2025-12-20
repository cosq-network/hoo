#pragma once

#include "ASTNode.h"
#include <vector>

namespace hooc {
namespace ast {

class ImportStatement;
class Declaration;

class CompilationUnit : public ASTNode {
public:
    CompilationUnit(std::vector<std::unique_ptr<ImportStatement>> imports,
                   std::vector<std::unique_ptr<Declaration>> declarations)
        : imports_(std::move(imports)), declarations_(std::move(declarations)) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<ImportStatement>>& getImports() const { return imports_; }
    const std::vector<std::unique_ptr<Declaration>>& getDeclarations() const { return declarations_; }

private:
    std::vector<std::unique_ptr<ImportStatement>> imports_;
    std::vector<std::unique_ptr<Declaration>> declarations_;
};

} // namespace ast
} // namespace hooc