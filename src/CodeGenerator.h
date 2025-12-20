#pragma once

#include "ast/AST.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace hooc {

/**
 * CodeGenerator translates hooc AST nodes into LLVM IR.
 * This is the bridge between the parsed AST and executable code.
 */
class CodeGenerator {
public:
    CodeGenerator(llvm::LLVMContext& context);
    ~CodeGenerator();

    /**
     * Generate a complete LLVM module from a compilation unit
     */
    std::unique_ptr<llvm::Module> generateModule(const ast::CompilationUnit& compilationUnit);

    /**
     * Generate LLVM IR for individual AST components
     */
    llvm::Function* generateFunction(const ast::FunctionDeclaration& funcDecl);
    llvm::Value* generateExpression(const ast::Expression& expr);
    void generateStatement(const ast::Statement& stmt);
    llvm::Type* generateType(const ast::Type& type);

private:
    llvm::LLVMContext& context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    
    // Symbol table for variables and functions
    std::unordered_map<std::string, llvm::Value*> namedValues_;
    std::unordered_map<std::string, llvm::Function*> functions_;

    // Helper methods for specific AST node types
    llvm::Value* generatePrimaryExpression(const ast::PrimaryExpression& expr);
    llvm::Value* generateBinaryExpression(const ast::BinaryExpression& expr);
    llvm::Value* generateFunctionCall(const ast::FunctionCall& call);
    
    void generateBlock(const ast::Block& block);
    void generateReturnStatement(const ast::ReturnStatement& ret);
    void generateExpressionStatement(const ast::ExpressionStatement& stmt);
    
    // Type conversion helpers
    llvm::Type* convertPrimitiveType(ast::PrimitiveTypeKind kind);
    llvm::Type* convertArrayType(const ast::ArrayType& arrayType);
    
    // Utility methods
    llvm::Constant* createConstant(const ast::Primary& primary);
    std::string mangleFunctionName(const std::string& name, const std::vector<llvm::Type*>& paramTypes);
};

} // namespace hooc