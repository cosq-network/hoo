#pragma once

#include <memory>
#include <vector>
#include <string>

namespace hooc {
namespace ast {

// Base class for all AST nodes
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

// Smart pointer type for AST nodes
using ASTNodePtr = std::unique_ptr<ASTNode>;

// Helper template for creating AST nodes
template<typename T, typename... Args>
std::unique_ptr<T> makeAST(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace ast
} // namespace hooc