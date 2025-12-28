#pragma once

#include "ASTNode.h"
#include <vector>
#include <string>

namespace hooc {
namespace ast {

// Represents a qualified identifier like std.String or std.io.File
// Used for module paths in type contexts (std.String) and constructor calls (new std.String())
class QualifiedIdentifier : public ASTNode {
public:
    // Constructor from vector of components
    explicit QualifiedIdentifier(std::vector<std::string> components)
        : components_(std::move(components)) {}

    std::string toString() const override;

    // Get all components (e.g., ["std", "String"])
    const std::vector<std::string>& getComponents() const { return components_; }

    // Get simple name (last component) - "String" from "std.String"
    const std::string& getName() const {
        return components_.empty() ? components_[0] : components_.back();
    }

    // Get module path (all but last component) - ["std"] from "std.String"
    std::vector<std::string> getModulePath() const {
        if (components_.size() <= 1) {
            return std::vector<std::string>();
        }
        return std::vector<std::string>(components_.begin(), components_.end() - 1);
    }

    // Check if this is a qualified name (has module prefix)
    // Returns true for "std.String", false for "String"
    bool isQualified() const { return components_.size() > 1; }

    // Get full qualified name as string - "std.String" or "std.io.File"
    std::string getFullName() const {
        std::string result;
        for (size_t i = 0; i < components_.size(); ++i) {
            if (i > 0) result += ".";
            result += components_[i];
        }
        return result;
    }

    // Get number of components
    size_t getComponentCount() const { return components_.size(); }

    // Get component at index
    const std::string& getComponent(size_t index) const {
        return components_[index];
    }

private:
    std::vector<std::string> components_;  // ["std", "String"] or ["std", "io", "File"]
};

} // namespace ast
} // namespace hooc
