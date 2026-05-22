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

// Module path for dotted module names (e.g., os.path)
class ModulePath : public ASTNode {
public:
    ModulePath(std::vector<std::string> components)
        : components_(std::move(components)) {}

    std::string toString() const override;

    const std::vector<std::string>& getComponents() const { return components_; }

private:
    std::vector<std::string> components_;
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

// Python-style: import module [as alias]
class BasicImport : public ImportStatement {
public:
    BasicImport(std::unique_ptr<ModulePath> module, const std::string& alias = "")
        : module_(std::move(module)), alias_(alias) {}

    std::string toString() const override;

    const ModulePath* getModule() const { return module_.get(); }
    const std::string& getAlias() const { return alias_; }
    bool hasAlias() const { return !alias_.empty(); }

private:
    std::unique_ptr<ModulePath> module_;
    std::string alias_;
};

// Python-style: from module import name1, name2
class FromImport : public ImportStatement {
public:
    FromImport(std::unique_ptr<ModulePath> module, std::vector<std::unique_ptr<ImportItem>> items)
        : module_(std::move(module)), items_(std::move(items)) {}

    std::string toString() const override;

    const ModulePath* getModule() const { return module_.get(); }
    const std::vector<std::unique_ptr<ImportItem>>& getItems() const { return items_; }

private:
    std::unique_ptr<ModulePath> module_;
    std::vector<std::unique_ptr<ImportItem>> items_;
};

} // namespace ast
} // namespace hooc