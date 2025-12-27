#pragma once

#include "CodeGenerator.h"
#include "LLVMCodeGeneratorTypes.h"
#include "ast/AST.h"
#include "ast/ClassDeclaration.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "runtime/RuntimeClassRegistry.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace hooc {

/**
 * LLVMCodeGenerator translates hooc AST nodes into LLVM IR.
 * This is the concrete implementation of CodeGenerator that targets LLVM.
 */
class LLVMCodeGenerator : public CodeGenerator {
public:
    LLVMCodeGenerator(llvm::LLVMContext& context);
    ~LLVMCodeGenerator();

    /**
     * Get the backend type identifier
     */
    std::string getBackendType() const override { return "LLVM"; }

    /**
     * Generate a complete LLVM module from a compilation unit
     */
    std::unique_ptr<GeneratedModule> generateModule(const ast::CompilationUnit& compilationUnit) override;

    /**
     * Generate LLVM IR for individual AST components
     */
    GeneratedFunction* generateFunction(const ast::FunctionDeclaration& funcDecl) override;
    GeneratedValue* generateExpression(const ast::Expression& expr) override;
    void generateStatement(const ast::Statement& stmt) override;
    GeneratedType* generateType(const ast::Type& type) override;

    /**
     * LLVM-specific API for direct access to LLVM types
     * (for use when you know you're working with LLVM backend)
     */
    std::unique_ptr<llvm::Module> generateLLVMModule(const ast::CompilationUnit& compilationUnit);
    llvm::Function* generateLLVMFunction(const ast::FunctionDeclaration& funcDecl);
    llvm::Value* generateLLVMExpression(const ast::Expression& expr);
    void generateLLVMStatement(const ast::Statement& stmt);
    llvm::Type* generateLLVMType(const ast::Type& type);

private:
    llvm::LLVMContext& context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    
    // Symbol table for variables and functions
    std::unordered_map<std::string, llvm::Value*> namedValues_;
    std::unordered_map<std::string, llvm::Function*> functions_;
    std::unordered_map<std::string, std::string> variableTypes_; // Map var name -> class name (for objects)

    // Class type tracking
    std::unordered_map<std::string, llvm::StructType*> classTypes_;
    std::unordered_map<std::string, int64_t> classTypeIds_;
    std::unordered_map<std::string, const ast::ClassDeclaration*> classDeclarations_; // Track class declarations
    int64_t nextTypeId_ = 1;

    // Runtime function declarations (core allocation/refcount)
    llvm::Function* hoo_alloc_func_ = nullptr;
    llvm::Function* hoo_retain_func_ = nullptr;
    llvm::Function* hoo_release_func_ = nullptr;

    // ========================================================================
    // Runtime Class Function Pointers - String Class
    // ========================================================================

    llvm::Function* hoo_string_from_cstr_func_ = nullptr;
    llvm::Function* hoo_string_new_func_ = nullptr;
    llvm::Function* hoo_string_from_bytes_func_ = nullptr;
    llvm::Function* hoo_string_repeat_func_ = nullptr;
    llvm::Function* hoo_string_concat_func_ = nullptr;
    llvm::Function* hoo_string_substring_func_ = nullptr;
    llvm::Function* hoo_string_to_upper_func_ = nullptr;
    llvm::Function* hoo_string_to_lower_func_ = nullptr;
    llvm::Function* hoo_string_trim_func_ = nullptr;
    llvm::Function* hoo_string_replace_func_ = nullptr;
    llvm::Function* hoo_string_length_func_ = nullptr;
    llvm::Function* hoo_string_data_func_ = nullptr;
    llvm::Function* hoo_string_byte_at_func_ = nullptr;
    llvm::Function* hoo_string_is_empty_func_ = nullptr;
    llvm::Function* hoo_string_index_of_func_ = nullptr;
    llvm::Function* hoo_string_last_index_of_func_ = nullptr;
    llvm::Function* hoo_string_contains_func_ = nullptr;
    llvm::Function* hoo_string_starts_with_func_ = nullptr;
    llvm::Function* hoo_string_ends_with_func_ = nullptr;
    llvm::Function* hoo_string_compare_func_ = nullptr;
    llvm::Function* hoo_string_equals_func_ = nullptr;
    llvm::Function* hoo_string_equals_ignore_case_func_ = nullptr;
    llvm::Function* hoo_string_retain_func_ = nullptr;
    llvm::Function* hoo_string_release_func_ = nullptr;
    llvm::Function* hoo_string_refcount_func_ = nullptr;
    llvm::Function* hoo_string_from_int64_func_ = nullptr;
    llvm::Function* hoo_string_from_double_func_ = nullptr;
    llvm::Function* hoo_string_from_bool_func_ = nullptr;
    llvm::Function* hoo_string_to_int64_func_ = nullptr;
    llvm::Function* hoo_string_to_double_func_ = nullptr;
    llvm::Function* hoo_string_format_func_ = nullptr;
    llvm::Function* hoo_string_print_func_ = nullptr;
    llvm::Function* hoo_string_println_func_ = nullptr;
    llvm::Function* hoo_string_debug_func_ = nullptr;

    // ========================================================================
    // Runtime Class Function Pointers - Int64Array Class
    // ========================================================================

    llvm::Function* hoo_int64_array_new_func_ = nullptr;
    llvm::Function* hoo_int64_array_from_buffer_func_ = nullptr;
    llvm::Function* hoo_int64_array_repeat_func_ = nullptr;
    llvm::Function* hoo_int64_array_length_func_ = nullptr;
    llvm::Function* hoo_int64_array_get_func_ = nullptr;
    llvm::Function* hoo_int64_array_set_func_ = nullptr;
    llvm::Function* hoo_int64_array_push_func_ = nullptr;
    llvm::Function* hoo_int64_array_pop_func_ = nullptr;
    llvm::Function* hoo_int64_array_clear_func_ = nullptr;
    llvm::Function* hoo_int64_array_contains_func_ = nullptr;
    llvm::Function* hoo_int64_array_index_of_func_ = nullptr;
    llvm::Function* hoo_int64_array_concat_func_ = nullptr;
    llvm::Function* hoo_int64_array_slice_func_ = nullptr;
    llvm::Function* hoo_int64_array_clone_func_ = nullptr;
    llvm::Function* hoo_int64_array_retain_func_ = nullptr;
    llvm::Function* hoo_int64_array_release_func_ = nullptr;
    llvm::Function* hoo_int64_array_refcount_func_ = nullptr;

    // ========================================================================
    // Runtime Class Function Pointers - DoubleArray Class
    // ========================================================================

    llvm::Function* hoo_double_array_new_func_ = nullptr;
    llvm::Function* hoo_double_array_from_buffer_func_ = nullptr;
    llvm::Function* hoo_double_array_repeat_func_ = nullptr;
    llvm::Function* hoo_double_array_length_func_ = nullptr;
    llvm::Function* hoo_double_array_get_func_ = nullptr;
    llvm::Function* hoo_double_array_set_func_ = nullptr;
    llvm::Function* hoo_double_array_push_func_ = nullptr;
    llvm::Function* hoo_double_array_pop_func_ = nullptr;
    llvm::Function* hoo_double_array_clear_func_ = nullptr;
    llvm::Function* hoo_double_array_concat_func_ = nullptr;
    llvm::Function* hoo_double_array_slice_func_ = nullptr;
    llvm::Function* hoo_double_array_clone_func_ = nullptr;
    llvm::Function* hoo_double_array_retain_func_ = nullptr;
    llvm::Function* hoo_double_array_release_func_ = nullptr;
    llvm::Function* hoo_double_array_refcount_func_ = nullptr;

    // ========================================================================
    // Auto-Generated Runtime Function Declaration Methods
    // ========================================================================
    // Each runtime class gets a declare<Class>Functions() method

    void declareRuntimeFunctions();

    #define DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate) \
        void declare##ClassName##Functions();
    #define BEGIN_RUNTIME_FUNCTIONS
    #define END_RUNTIME_FUNCTIONS
    #define RUNTIME_FUNCTION(...)
    #define BEGIN_RUNTIME_OPERATORS
    #define END_RUNTIME_OPERATORS
    #define RUNTIME_OPERATOR(...)

    RUNTIME_CLASSES

    #undef DEFINE_RUNTIME_CLASS
    #undef BEGIN_RUNTIME_FUNCTIONS
    #undef END_RUNTIME_FUNCTIONS
    #undef RUNTIME_FUNCTION
    #undef BEGIN_RUNTIME_OPERATORS
    #undef END_RUNTIME_OPERATORS
    #undef RUNTIME_OPERATOR

    // ========================================================================
    // Auto-Generated Operator Dispatch Methods
    // ========================================================================
    // Each runtime class can handle specific binary operators

    #define DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate) \
        llvm::Value* try##ClassName##Operator( \
            ast::BinaryOperator op, llvm::Value* left, llvm::Value* right);
    #define BEGIN_RUNTIME_FUNCTIONS
    #define END_RUNTIME_FUNCTIONS
    #define RUNTIME_FUNCTION(...)
    #define BEGIN_RUNTIME_OPERATORS
    #define END_RUNTIME_OPERATORS
    #define RUNTIME_OPERATOR(...)

    RUNTIME_CLASSES

    #undef DEFINE_RUNTIME_CLASS
    #undef BEGIN_RUNTIME_FUNCTIONS
    #undef END_RUNTIME_FUNCTIONS
    #undef RUNTIME_FUNCTION
    #undef BEGIN_RUNTIME_OPERATORS
    #undef END_RUNTIME_OPERATORS
    #undef RUNTIME_OPERATOR

    // Class type management
    llvm::StructType* getOrCreateClassType(const std::string& className);
    int64_t getClassTypeId(const std::string& className);
    void generateClassDeclaration(const ast::ClassDeclaration& classDecl);
    void generateConstructor(const ast::ClassDeclaration& classDecl, const ast::ConstructorDeclaration& constructor);

    // Object creation
    llvm::Value* generateNewObjectExpression(const ast::NewObjectExpression& newExpr);

    // Helper methods for specific AST node types
    llvm::Value* generatePrimaryExpression(const ast::PrimaryExpression& expr);
    llvm::Value* generateBinaryExpression(const ast::BinaryExpression& expr);
    llvm::Value* generateFunctionCall(const ast::FunctionCall& call);
    llvm::Value* generateUnaryExpression(const ast::UnaryMinus& expr);
    llvm::Value* generateLogicalNot(const ast::LogicalNot& expr);
    llvm::Value* generateLogicalAnd(const ast::LogicalAnd& expr);
    llvm::Value* generateLogicalOr(const ast::LogicalOr& expr);
    llvm::Value* generateAssignment(const ast::AssignmentExpression& expr);
    llvm::Value* generateMemberAccess(const ast::MemberAccess& expr);
    llvm::Value* generateArrayAccess(const ast::ArrayAccess& expr);
    llvm::Value* generateArrayLiteral(const ast::ArrayLiteral& literal);

    void generateBlock(const ast::Block& block);
    void generateReturnStatement(const ast::ReturnStatement& ret);
    void generateExpressionStatement(const ast::ExpressionStatement& stmt);
    void generateIfStatement(const ast::IfStatement& stmt);
    void generateWhileStatement(const ast::WhileStatement& stmt);
    void generateForInStatement(const ast::ForInStatement& stmt);
    void generateForRangeStatement(const ast::ForRangeStatement& stmt);
    void generateVariableDeclaration(const ast::VariableDeclaration& decl);
    void generateVariableDeclarationStatement(const ast::VariableDeclarationStatement& stmt);
    
    // Type conversion helpers
    llvm::Type* convertPrimitiveType(ast::PrimitiveTypeKind kind);
    llvm::Type* convertArrayType(const ast::ArrayType& arrayType);

    // Nullable type helpers (tagged union pattern: { i1 flag, T value })
    llvm::StructType* createNullableType(llvm::Type* valueType);
    llvm::Value* createNullValue(llvm::Type* valueType);
    llvm::Value* wrapValueInNullable(llvm::Value* value, llvm::Type* nullableType);
    llvm::Value* extractValueFromNullable(llvm::Value* nullableValue);
    llvm::Value* extractNullFlagFromNullable(llvm::Value* nullableValue);
    bool isTypeNullable(const ast::Type& type);

    // Utility methods
    llvm::Constant* createConstant(const ast::Primary& primary);
    llvm::Constant* createGlobalArrayConstant(const std::vector<llvm::Constant*>& elements, llvm::Type* elementType);
    std::string mangleFunctionName(const std::string& name, const std::vector<llvm::Type*>& paramTypes);
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* function, const std::string& varName, llvm::Type* type);
};

} // namespace hooc