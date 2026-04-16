#pragma once

#include "CodeGenerator.h"
#include "LLVMCodeGeneratorTypes.h"
#include "ast/AST.h"
#include "ast/ClassDeclaration.h"
#include "ModuleSystem.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "runtime/RuntimeClassRegistry.h"
#include "runtime/RuntimeFunctionStorage.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

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

    // ========================================================================
    // Runtime Function Registration
    // ========================================================================

    /**
     * Declare all runtime functions in the current module.
     *
     * Invokes the central RuntimeRegistry to call all registered runtime
     * callbacks, which declare LLVM function prototypes and populate
     * the runtimeFunctionStorage_ with function pointers.
     *
     * This should be called once during module creation, before generating
     * code that uses runtime functions.
     */
    void declareRuntimeFunctions();

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
    // Runtime Function Storage (populated by registry callbacks)
    // ========================================================================
    // This storage is populated by runtime libraries during module generation.
    // Each runtime library registers a callback that populates its section
    // of this storage with function pointers.
    //
    // Usage: See declareRuntimeFunctions() method

    runtime::RuntimeFunctionStorage runtimeFunctionStorage_;

    // ========================================================================
    // Runtime Class Function Pointers - String Class
    // ========================================================================
    // DEPRECATED: For backward compatibility, these members point into
    // runtimeFunctionStorage_.strings. They will be removed in a future refactor.
    // New code should access functions via runtimeFunctionStorage_.strings.

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
    // Runtime Array Function Pointers (Generic Array via type-specific wrappers)
    // ========================================================================

    llvm::Function* hoo_int64_array_from_buffer_func_ = nullptr;
    llvm::Function* hoo_double_array_from_buffer_func_ = nullptr;
    llvm::Function* hoo_array_new_func_ = nullptr;      // For dynamic array construction

    // Phase 7: Type-specific push function pointers
    llvm::Function* hoo_array_push_int64_func_ = nullptr;
    llvm::Function* hoo_array_push_double_func_ = nullptr;
    llvm::Function* hoo_array_push_float_func_ = nullptr;
    llvm::Function* hoo_array_push_bool_func_ = nullptr;
    llvm::Function* hoo_array_push_char_func_ = nullptr;
    llvm::Function* hoo_array_push_object_func_ = nullptr;

    // Type-specific array get functions (only int64 currently used in for-in loops)
    llvm::Function* hoo_array_get_int64_func_ = nullptr;

    // Callback-based Array function pointers (synced from registry)
    llvm::Function* hoo_array_from_buffer_func_ = nullptr;
    llvm::Function* hoo_array_repeat_func_ = nullptr;
    llvm::Function* hoo_array_length_func_ = nullptr;
    llvm::Function* hoo_array_get_func_ = nullptr;
    llvm::Function* hoo_array_set_func_ = nullptr;
    llvm::Function* hoo_array_pop_func_ = nullptr;
    llvm::Function* hoo_array_retain_func_ = nullptr;
    llvm::Function* hoo_array_release_func_ = nullptr;
    llvm::Function* hoo_array_refcount_func_ = nullptr;
    llvm::Function* hoo_array_empty_func_ = nullptr;
    llvm::Function* hoo_array_clear_func_ = nullptr;

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

    // ========================================================================
    // Standard Library Class Constructor Generation
    // ========================================================================

    /**
     * Generate code for standard library class constructors
     * Maps std.String and std.Array to runtime functions
     */
    llvm::Value* generateStdClassConstructor(const ModuleExport& moduleExport,
                                            const ast::NewObjectExpression& newExpr);

    /**
     * Generate constructor for std.String
     */
    llvm::Value* generateStringConstructor(const ast::NewObjectExpression& newExpr);

    /**
     * Generate constructor for std.Array
     */
    llvm::Value* generateArrayConstructor(const ast::NewObjectExpression& newExpr);

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

    // Array runtime function helpers for Phase 4 (generic array integration)
    llvm::Function* getArrayFromBufferFunc(llvm::Type* elementType);
    llvm::Value* generateArrayLiteralWithRuntime(const std::vector<llvm::Constant*>& elements, llvm::Type* elementType);

    // Array support for pointer types (classes) - Phase 6
    llvm::Value* generateDynamicArrayLiteral(const std::vector<llvm::Value*>& elements, llvm::Type* elementType);
    llvm::Function* getArrayNewFunc(size_t elementSize);

    // Phase 7: Type-specific array push functions for generic array redesign
    llvm::Function* getArrayPushInt64Func();
    llvm::Function* getArrayPushDoubleFunc();
    llvm::Function* getArrayPushFloatFunc();
    llvm::Function* getArrayPushBoolFunc();
    llvm::Function* getArrayPushCharFunc();
    llvm::Function* getArrayPushObjectFunc();

    // Helper to get the appropriate type-specific push function
    llvm::Function* getArrayPushFuncForType(llvm::Type* elementType);

    // ========================================================================
    // Module System - Import Resolution and Module Registry
    // ========================================================================

    ModuleRegistry moduleRegistry_;
    std::unordered_map<std::string, const ModuleExport*> importedNames_;

    /**
     * Process import statements from the compilation unit
     * Populates importedNames_ map with resolved imports
     */
    void processImports(const std::vector<std::unique_ptr<ast::ImportStatement>>& imports);

    /**
     * Process a basic import statement (import module.path as alias)
     */
    void processBasicImport(const ast::BasicImport& import);

    /**
     * Process a from import statement (from module import item1, item2)
     */
    void processFromImport(const ast::FromImport& import);
};

} // namespace hooc