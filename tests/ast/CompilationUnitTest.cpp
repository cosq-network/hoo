#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>

// Include ASTNode.h first - this is the base class
#include "ast/ASTNode.h"

namespace hooc {
namespace ast {

// Forward declare the classes we need (matching what CompilationUnit.h does)
class ImportStatement : public ASTNode {
public:
    virtual ~ImportStatement() = default;
};

class Declaration : public ASTNode {
public:
    virtual ~Declaration() = default;
};

// Mock ImportStatement for testing
class MockImportStatement : public ImportStatement {
public:
    explicit MockImportStatement(const std::string& name) : name_(name) {}
    std::string toString() const override { return "MockImport(" + name_ + ")"; }
    const std::string& getName() const { return name_; }
private:
    std::string name_;
};

// Mock Declaration for testing
class MockDeclaration : public Declaration {
public:
    explicit MockDeclaration(const std::string& name) : name_(name) {}
    std::string toString() const override { return "MockDeclaration(" + name_ + ")"; }
    const std::string& getName() const { return name_; }
private:
    std::string name_;
};

} // namespace ast
} // namespace hooc

// Now include the header we're testing
#include "ast/CompilationUnit.h"

// Re-open namespace to add any specializations if needed
// (none needed for this test)

using namespace hooc::ast;

class CompilationUnitTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ============================================================================
// Construction and Basic Access Tests
// ============================================================================

TEST_F(CompilationUnitTest, EmptyConstruction) {
    CompilationUnit unit({}, {});
    
    EXPECT_FALSE(unit.hasImports());
    EXPECT_FALSE(unit.hasDeclarations());
    EXPECT_EQ(unit.getImportCount(), 0);
    EXPECT_EQ(unit.getDeclarationCount(), 0);
    EXPECT_TRUE(unit.getImports().empty());
    EXPECT_TRUE(unit.getDeclarations().empty());
}

TEST_F(CompilationUnitTest, ConstructionWithImportsOnly) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    imports.push_back(std::make_unique<MockImportStatement>("import1"));
    imports.push_back(std::make_unique<MockImportStatement>("import2"));
    
    CompilationUnit unit(std::move(imports), {});
    
    EXPECT_TRUE(unit.hasImports());
    EXPECT_FALSE(unit.hasDeclarations());
    EXPECT_EQ(unit.getImportCount(), 2);
    EXPECT_EQ(unit.getDeclarationCount(), 0);
}

TEST_F(CompilationUnitTest, ConstructionWithDeclarationsOnly) {
    std::vector<std::unique_ptr<Declaration>> declarations;
    declarations.push_back(std::make_unique<MockDeclaration>("decl1"));
    declarations.push_back(std::make_unique<MockDeclaration>("decl2"));
    declarations.push_back(std::make_unique<MockDeclaration>("decl3"));
    
    CompilationUnit unit({}, std::move(declarations));
    
    EXPECT_FALSE(unit.hasImports());
    EXPECT_TRUE(unit.hasDeclarations());
    EXPECT_EQ(unit.getImportCount(), 0);
    EXPECT_EQ(unit.getDeclarationCount(), 3);
}

TEST_F(CompilationUnitTest, ConstructionWithBothImportsAndDeclarations) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    imports.push_back(std::make_unique<MockImportStatement>("import1"));
    
    std::vector<std::unique_ptr<Declaration>> declarations;
    declarations.push_back(std::make_unique<MockDeclaration>("decl1"));
    declarations.push_back(std::make_unique<MockDeclaration>("decl2"));
    
    CompilationUnit unit(std::move(imports), std::move(declarations));
    
    EXPECT_TRUE(unit.hasImports());
    EXPECT_TRUE(unit.hasDeclarations());
    EXPECT_EQ(unit.getImportCount(), 1);
    EXPECT_EQ(unit.getDeclarationCount(), 2);
}

// ============================================================================
// toString Tests
// ============================================================================

TEST_F(CompilationUnitTest, ToStringEmpty) {
    CompilationUnit unit({}, {});
    std::string result = unit.toString();
    
    EXPECT_NE(result.find("CompilationUnit"), std::string::npos);
    EXPECT_NE(result.find("imports=0"), std::string::npos);
    EXPECT_NE(result.find("declarations=0"), std::string::npos);
}

TEST_F(CompilationUnitTest, ToStringWithContents) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    imports.push_back(std::make_unique<MockImportStatement>("import1"));
    
    std::vector<std::unique_ptr<Declaration>> declarations;
    declarations.push_back(std::make_unique<MockDeclaration>("decl1"));
    
    CompilationUnit unit(std::move(imports), std::move(declarations));
    std::string result = unit.toString();
    
    EXPECT_NE(result.find("CompilationUnit"), std::string::npos);
    EXPECT_NE(result.find("imports=1"), std::string::npos);
    EXPECT_NE(result.find("declarations=1"), std::string::npos);
}

// ============================================================================
// Iterator Access Tests
// ============================================================================

TEST_F(CompilationUnitTest, IteratorAccessImports) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    imports.push_back(std::make_unique<MockImportStatement>("import1"));
    imports.push_back(std::make_unique<MockImportStatement>("import2"));
    imports.push_back(std::make_unique<MockImportStatement>("import3"));
    
    CompilationUnit unit(std::move(imports), {});
    
    int count = 0;
    for (auto it = unit.begin_imports(); it != unit.end_imports(); ++it) {
        EXPECT_NE(*it, nullptr);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(CompilationUnitTest, IteratorAccessDeclarations) {
    std::vector<std::unique_ptr<Declaration>> declarations;
    declarations.push_back(std::make_unique<MockDeclaration>("decl1"));
    declarations.push_back(std::make_unique<MockDeclaration>("decl2"));
    
    CompilationUnit unit({}, std::move(declarations));
    
    int count = 0;
    for (auto it = unit.begin_declarations(); it != unit.end_declarations(); ++it) {
        EXPECT_NE(*it, nullptr);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(CompilationUnitTest, RangeBasedForLoopImports) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    imports.push_back(std::make_unique<MockImportStatement>("import1"));
    imports.push_back(std::make_unique<MockImportStatement>("import2"));
    
    CompilationUnit unit(std::move(imports), {});
    
    int count = 0;
    for (const auto& import : unit.getImports()) {
        EXPECT_NE(import, nullptr);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(CompilationUnitTest, RangeBasedForLoopDeclarations) {
    std::vector<std::unique_ptr<Declaration>> declarations;
    declarations.push_back(std::make_unique<MockDeclaration>("decl1"));
    declarations.push_back(std::make_unique<MockDeclaration>("decl2"));
    declarations.push_back(std::make_unique<MockDeclaration>("decl3"));
    
    CompilationUnit unit({}, std::move(declarations));
    
    int count = 0;
    for (const auto& decl : unit.getDeclarations()) {
        EXPECT_NE(decl, nullptr);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

// ============================================================================
// Move Operations Tests
// ============================================================================

TEST_F(CompilationUnitTest, MoveConstructor) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    imports.push_back(std::make_unique<MockImportStatement>("import1"));
    
    std::vector<std::unique_ptr<Declaration>> declarations;
    declarations.push_back(std::make_unique<MockDeclaration>("decl1"));
    
    CompilationUnit original(std::move(imports), std::move(declarations));
    EXPECT_EQ(original.getImportCount(), 1);
    EXPECT_EQ(original.getDeclarationCount(), 1);
    
    // Move from original
    CompilationUnit moved = std::move(original);
    
    EXPECT_EQ(moved.getImportCount(), 1);
    EXPECT_EQ(moved.getDeclarationCount(), 1);
}

TEST_F(CompilationUnitTest, MoveAssignment) {
    std::vector<std::unique_ptr<ImportStatement>> imports1;
    imports1.push_back(std::make_unique<MockImportStatement>("import1"));
    
    std::vector<std::unique_ptr<Declaration>> declarations1;
    declarations1.push_back(std::make_unique<MockDeclaration>("decl1"));
    CompilationUnit unit1(std::move(imports1), std::move(declarations1));
    
    std::vector<std::unique_ptr<ImportStatement>> imports2;
    imports2.push_back(std::make_unique<MockImportStatement>("import2"));
    imports2.push_back(std::make_unique<MockImportStatement>("import3"));
    
    std::vector<std::unique_ptr<Declaration>> declarations2;
    declarations2.push_back(std::make_unique<MockDeclaration>("decl2"));
    CompilationUnit unit2(std::move(imports2), std::move(declarations2));
    
    unit1 = std::move(unit2);
    
    EXPECT_EQ(unit1.getImportCount(), 2);
    EXPECT_EQ(unit1.getDeclarationCount(), 1);
}

// ============================================================================
// Copy Operations Tests (should be deleted)
// ============================================================================

TEST_F(CompilationUnitTest, CopyConstructorIsDeleted) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    std::vector<std::unique_ptr<Declaration>> declarations;
    CompilationUnit unit(std::move(imports), std::move(declarations));
    
    static_assert(!std::is_copy_constructible<CompilationUnit>::value, 
                  "CompilationUnit should not be copy constructible");
}

TEST_F(CompilationUnitTest, CopyAssignmentIsDeleted) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    std::vector<std::unique_ptr<Declaration>> declarations;
    CompilationUnit unit1(std::move(imports), std::move(declarations));
    CompilationUnit unit2({}, {});
    
    static_assert(!std::is_copy_assignable<CompilationUnit>::value,
                  "CompilationUnit should not be copy assignable");
}

// ============================================================================
// ASTNode Interface Tests
// ============================================================================

TEST_F(CompilationUnitTest, InheritsFromASTNode) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    std::vector<std::unique_ptr<Declaration>> declarations;
    CompilationUnit unit(std::move(imports), std::move(declarations));
    
    // Should be able to upcast to ASTNode
    ASTNode* node = &unit;
    EXPECT_NE(node, nullptr);
    
    // Should be able to call toString through base pointer
    std::string result = node->toString();
    EXPECT_NE(result.find("CompilationUnit"), std::string::npos);
}

TEST_F(CompilationUnitTest, VirtualDestructor) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    std::vector<std::unique_ptr<Declaration>> declarations;
    
    // Should be able to delete through base pointer
    ASTNode* node = new CompilationUnit(std::move(imports), std::move(declarations));
    delete node;  // Should not leak memory
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

TEST_F(CompilationUnitTest, LargeNumberOfImports) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    for (int i = 0; i < 1000; ++i) {
        imports.push_back(std::make_unique<MockImportStatement>("import" + std::to_string(i)));
    }
    
    CompilationUnit unit(std::move(imports), {});
    
    EXPECT_EQ(unit.getImportCount(), 1000);
    EXPECT_TRUE(unit.hasImports());
}

TEST_F(CompilationUnitTest, LargeNumberOfDeclarations) {
    std::vector<std::unique_ptr<Declaration>> declarations;
    for (int i = 0; i < 1000; ++i) {
        declarations.push_back(std::make_unique<MockDeclaration>("decl" + std::to_string(i)));
    }
    
    CompilationUnit unit({}, std::move(declarations));
    
    EXPECT_EQ(unit.getDeclarationCount(), 1000);
    EXPECT_TRUE(unit.hasDeclarations());
}

TEST_F(CompilationUnitTest, MixedLargeContents) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    for (int i = 0; i < 500; ++i) {
        imports.push_back(std::make_unique<MockImportStatement>("import" + std::to_string(i)));
    }
    
    std::vector<std::unique_ptr<Declaration>> declarations;
    for (int i = 0; i < 500; ++i) {
        declarations.push_back(std::make_unique<MockDeclaration>("decl" + std::to_string(i)));
    }
    
    CompilationUnit unit(std::move(imports), std::move(declarations));
    
    EXPECT_EQ(unit.getImportCount(), 500);
    EXPECT_EQ(unit.getDeclarationCount(), 500);
    EXPECT_TRUE(unit.hasImports());
    EXPECT_TRUE(unit.hasDeclarations());
}

TEST_F(CompilationUnitTest, GettersReturnConstReferences) {
    std::vector<std::unique_ptr<ImportStatement>> imports;
    imports.push_back(std::make_unique<MockImportStatement>("import1"));
    
    std::vector<std::unique_ptr<Declaration>> declarations;
    declarations.push_back(std::make_unique<MockDeclaration>("decl1"));
    
    CompilationUnit unit(std::move(imports), std::move(declarations));
    
    // Verify that getters return const references
    const auto& unitRef = unit;
    const auto& importsRef = unitRef.getImports();
    const auto& declarationsRef = unitRef.getDeclarations();
    
    EXPECT_EQ(importsRef.size(), 1);
    EXPECT_EQ(declarationsRef.size(), 1);
}
