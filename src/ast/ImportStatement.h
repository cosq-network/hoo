#pragma once

#include "ASTNode.h"
#include <vector>
#include <string>

namespace hooc {
namespace ast {

// Base class for import statements
class ImportStatement : public ASTNode {
public:
    virtual ~ImportStatement() = default;
};

// Import item for named imports
class ImportItem : public ASTNode {
public:
    ImportItem(const std::string& name, const std::string& alias = "")
        : name_(name), alias_(alias) {}

    std::string toString() const override;

    const std::string& getName() const { return name_; }
    const std::string& getAlias() const { return alias_; }
    bool hasAlias() const { return !alias_.empty(); }

private:
    std::string name_;
    std::string alias_;
};

// Named imports: import { User, Role } from "module"
class NamedImports : public ImportStatement {
public:
    NamedImports(std::vector<std::unique_ptr<ImportItem>> items, const std::string& module)
        : items_(std::move(items)), module_(module) {}

    std::string toString() const override;

    const std::vector<std::unique_ptr<ImportItem>>& getItems() const { return items_; }
    const std::string& getModule() const { return module_; }

private:
    std::vector<std::unique_ptr<ImportItem>> items_;
    std::string module_;
};

// Namespace import: import * as math from "module"
class NamespaceImport : public ImportStatement {
public:
    NamespaceImport(const std::string& alias, const std::string& module)
        : alias_(alias), module_(module) {}

    std::string toString() const override;

    const std::string& getAlias() const { return alias_; }
    const std::string& getModule() const { return module_; }

private:
    std::string alias_;
    std::string module_;
};

// Side-effect import: import "module"
class SideEffectImport : public ImportStatement {
public:
    SideEffectImport(const std::string& module) : module_(module) {}

    std::string toString() const override;

    const std::string& getModule() const { return module_; }

private:
    std::string module_;
};

} // namespace ast
} // namespace hooc