#pragma once

////////////////////////////////////////////////////////////////////////////////
/// @file LLVMCodeGenerator.h
/// @brief LLVM IR code generator for the Hooc compiler
///
/// @mainpage LLVMCodeGenerator
///
/// @section overview Overview
///
/// LLVMCodeGenerator translates hooc Abstract Syntax Tree (AST) nodes into
/// LLVM Intermediate Representation (IR). It provides both a high-level interface
/// (via CodeGenerator base class) and LLVM-specific APIs for direct access to
/// generated IR.
///
/// @section architecture Architecture
///
/// The code generator processes AST nodes in multiple passes:
/// 1. **First pass**: Class declarations are processed to create LLVM struct types
/// 2. **Second pass**: Functions and variable declarations are generated
///
/// The generator maintains symbol tables for:
/// - Named values (variables)
/// - Functions
/// - Class types
/// - Imported names (from module system)
///
/// @section runtime Runtime Integration
///
/// The generator integrates with Hooc's runtime system through:
/// - RuntimeRegistry: Central registration of runtime functions
/// - RuntimeFunctionStorage: Caches declared runtime functions
/// - Direct function pointers: Legacy support for string/array operations
///
/// @section memory Memory Management
///
/// Hooc uses Automatic Reference Counting (ARC):
/// - hoo_retain(): Increment reference count
/// - hoo_release(): Decrement reference count
/// - Objects are allocated via hoo_alloc()
///
/// @see CodeGenerator for the abstract base interface
/// @see HooCompiler for the compilation pipeline
///
////////////////////////////////////////////////////////////////////////////////

#include "CodeGenerator.h"
#include "LLVMCodeGeneratorTypes.h"
#include "../ast/AST.h"
#include "../ast/ClassDeclaration.h"
#include "../modules/ModuleSystem.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "runtime/llvm/RuntimeClassRegistry.h"
#include "runtime/llvm/RuntimeFunctionStorage.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace hooc {

/// @brief LLVM IR code generator for the Hooc compiler
/// @details Translates hooc AST into LLVM IR using the builder pattern.
///         Supports primitives, classes, arrays, strings, and control flow.
/// @note This class is not thread-safe - synchronize external access
class LLVMCodeGenerator : public CodeGenerator {
public:

    /// @brief Construct a new LLVM code generator
    /// @param context LLVM context for IR generation
    /// @details Creates an IRBuilder attached to the given context
    LLVMCodeGenerator(llvm::LLVMContext& context);

    /// @brief Destructor
    ~LLVMCodeGenerator();

    /// @brief Returns the backend identifier
    /// @return String literal "LLVM"
    std::string getBackendType() const override { return "LLVM"; }

    /// @brief Generate a complete module from compilation unit
    /// @param compilationUnit AST root node
    /// @return Generated module wrapper, or nullptr on failure
    /// @details Performs two-pass generation: classes first, then functions
    std::unique_ptr<GeneratedModule> generateModule(const ast::CompilationUnit& compilationUnit) override;

    /// @brief Generate a single function
    /// @param funcDecl Function declaration AST node
    /// @return Generated function wrapper, or nullptr on failure
    GeneratedFunction* generateFunction(const ast::FunctionDeclaration& funcDecl) override;

    /// @brief Generate an expression to LLVM value
    /// @param expr Expression AST node
    /// @return Generated value, or nullptr on failure
    GeneratedValue* generateExpression(const ast::Expression& expr) override;

    /// @brief Generate a statement
    /// @param stmt Statement AST node
    void generateStatement(const ast::Statement& stmt) override;

    /// @brief Generate an LLVM type from AST type
    /// @param type Type AST node
    /// @return Generated type wrapper, or nullptr on failure
    GeneratedType* generateType(const ast::Type& type) override;

    // ========================================================================
    // LLVM-Specific API
    // ========================================================================

    /// @brief Generate LLVM module directly (no wrapper)
    /// @param compilationUnit AST root node
    /// @return Raw LLVM module, or nullptr on failure
    std::unique_ptr<llvm::Module> generateLLVMModule(const ast::CompilationUnit& compilationUnit);

    /// @brief Generate LLVM function directly
    /// @param funcDecl Function declaration
    /// @return LLVM Function, or nullptr on failure
    llvm::Function* generateLLVMFunction(const ast::FunctionDeclaration& funcDecl);

    /// @brief Generate LLVM value from expression
    /// @param expr Expression node
    /// @return LLVM Value, or nullptr on failure
    llvm::Value* generateLLVMExpression(const ast::Expression& expr);

    /// @brief Generate LLVM instructions from statement
    /// @param stmt Statement node
    void generateLLVMStatement(const ast::Statement& stmt);

    /// @brief Generate LLVM type from AST type
    /// @param type Type node
    /// @return LLVM Type, or nullptr on failure
    llvm::Type* generateLLVMType(const ast::Type& type);

    // ========================================================================
    // Runtime Function Registration
    // ========================================================================

    /// @brief Declare all runtime functions in current module
    /// @details Calls RuntimeRegistry callbacks to declare LLVM prototypes
    ///         and populate runtimeFunctionStorage_ with function pointers.
    ///         Also syncs legacy function pointers for backward compatibility.
    /// @note Must be called before generating code that uses runtime functions
    void declareRuntimeFunctions();

// ========================================================================
    // Runtime Function Access
    // ========================================================================

    /// @brief Get a string runtime function by name
    /// @param name Function name suffix (e.g., "from_cstr" for hoo_string_from_cstr)
    /// @return Function pointer, or nullptr if not found
    llvm::Function* getStringFunc(const std::string& name);

    /// @brief Get an array runtime function by name
    /// @param name Function name suffix (e.g., "length" for hoo_array_length)
    /// @return Function pointer, or nullptr if not found
    llvm::Function* getArrayFunc(const std::string& name);

    /// @brief Get core allocation function
    llvm::Function* getAllocFunc() const { return hoo_alloc_func_; }

    /// @brief Get core retain function
    llvm::Function* getRetainFunc() const { return hoo_retain_func_; }

    /// @brief Get core release function
    llvm::Function* getReleaseFunc() const { return hoo_release_func_; }

    // ========================================================================
    // Error Handling
    // ========================================================================

    /// @brief Add an error message
    void addError(const std::string& message);

    /// @brief Add an error with location info
    void addError(const std::string& message, int line, int column);

    /// @brief Get all accumulated errors
    const std::vector<std::string>& getErrors() const { return errors_; }

    /// @brief Check if any errors occurred
    bool hasErrors() const { return !errors_.empty(); }

    /// @brief Clear all errors
    void clearErrors() { errors_.clear(); }

    /// @brief Get last error message
    std::string getLastError() const {
        return errors_.empty() ? "" : errors_.back();
    }

private:

    // ========================================================================
    // Error State
    // ========================================================================

    /// @brief Accumulated error messages
    std::vector<std::string> errors_;

    // ========================================================================
    // Core State
    // ========================================================================

    /// @brief LLVM context for type and constant creation
    llvm::LLVMContext& context_;

    /// @brief Current module being generated
    std::unique_ptr<llvm::Module> module_;

    /// @brief IR builder for instruction emission
    std::unique_ptr<llvm::IRBuilder<>> builder_;

    // ========================================================================
    // Loop Context for break/continue
    // ========================================================================

    struct LoopContext {
        llvm::BasicBlock* breakBlock;
        llvm::BasicBlock* continueBlock;
    };

    std::vector<LoopContext> loopStack_;

    // ========================================================================
    // Symbol Tables
    // ========================================================================

    /// @brief Maps variable names to their alloca instructions
    std::unordered_map<std::string, llvm::Value*> namedValues_;

    /// @brief Maps function names to their LLVM Function objects
    std::unordered_map<std::string, llvm::Function*> functions_;

    /// @brief Maps variable names to their class type names (for objects)
    std::unordered_map<std::string, std::string> variableTypes_;

    // ========================================================================
    // Class Type Management
    // ========================================================================

    /// @brief Maps class names to their LLVM struct types
    std::unordered_map<std::string, llvm::StructType*> classTypes_;

    /// @brief Maps class names to unique type IDs (for allocation)
    std::unordered_map<std::string, int64_t> classTypeIds_;

    /// @brief Maps class names to their AST declarations
    std::unordered_map<std::string, const ast::ClassDeclaration*> classDeclarations_;

    /// @brief Next available type ID for class allocation
    int64_t nextTypeId_ = 1;

    // ========================================================================
    // Core Runtime Functions
    // ========================================================================

    /// @brief hoo_alloc(size, typeId) - Object allocation
    llvm::Function* hoo_alloc_func_ = nullptr;

    /// @brief hoo_retain(obj) - Increment reference count
    llvm::Function* hoo_retain_func_ = nullptr;

    /// @brief hoo_release(obj) - Decrement reference count
    llvm::Function* hoo_release_func_ = nullptr;

    // ========================================================================
    // Runtime Function Storage
    // ========================================================================

    /// @brief Central storage for runtime function pointers
    /// @details Populated by RuntimeRegistry callbacks during declareRuntimeFunctions()
    runtime::RuntimeFunctionStorage runtimeFunctionStorage_;

    // ========================================================================
    // Operator Dispatch (Auto-generated via X-Macro)
    // ========================================================================

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

    // ========================================================================
    // Class Generation
    // ========================================================================

    /// @brief Get or create LLVM struct type for a class
    /// @param className Name of the class
    /// @return LLVM struct type for the class
    llvm::StructType* getOrCreateClassType(const std::string& className);

    /// @brief Get unique type ID for class allocation
    /// @param className Name of the class
    /// @return Unique type identifier
    int64_t getClassTypeId(const std::string& className);

    /// @brief Generate LLVM struct type and methods for a class
    /// @param classDecl Class declaration AST node
    void generateClassDeclaration(const ast::ClassDeclaration& classDecl);

    /// @brief Generate constructor function for a class
    /// @param classDecl Class declaration
    /// @param constructor Constructor declaration
    void generateConstructor(const ast::ClassDeclaration& classDecl,
                             const ast::ConstructorDeclaration& constructor);

    // ========================================================================
    // Object Creation
    // ========================================================================

    /// @brief Generate allocation and constructor call for new expression
    /// @param newExpr NewObjectExpression AST node
    /// @return Pointer to newly allocated object
    llvm::Value* generateNewObjectExpression(const ast::NewObjectExpression& newExpr);

    /// @brief Generate standard library class constructor
    /// @param moduleExport Resolved module export for the class
    /// @param newExpr NewObjectExpression node
    /// @return Constructed object value
    llvm::Value* generateStdClassConstructor(const ModuleExport& moduleExport,
                                            const ast::NewObjectExpression& newExpr);

    /// @brief Generate hoo.String constructor
    /// @param newExpr NewObjectExpression for String
    /// @return Constructed string value
    llvm::Value* generateStringConstructor(const ast::NewObjectExpression& newExpr);

    /// @brief Generate hoo.Array constructor
    /// @param newExpr NewObjectExpression for Array
    /// @return Constructed array value
    llvm::Value* generateArrayConstructor(const ast::NewObjectExpression& newExpr);

    // ========================================================================
    // Expression Generation
    // ========================================================================

    /// @brief Generate primary expression (literals, identifiers)
    /// @param expr Primary expression node
    /// @return Generated LLVM value
    llvm::Value* generatePrimaryExpression(const ast::PrimaryExpression& expr);

    /// @brief Generate this literal expression
    /// @param expr ThisLiteral expression node
    /// @return Pointer to current object instance (this)
    llvm::Value* generateThisLiteral(const ast::ThisLiteral& expr);

    /// @brief Generate binary expression (arithmetic, comparison)
    /// @param expr Binary expression node
    /// @return Generated LLVM value
    llvm::Value* generateBinaryExpression(const ast::BinaryExpression& expr);

    /// @brief Generate function call
    /// @param call FunctionCall expression node
    /// @return Generated LLVM value (function return)
    llvm::Value* generateFunctionCall(const ast::FunctionCall& call);

    /// @brief Generate unary minus expression
    /// @param expr UnaryMinus expression node
    /// @return Generated LLVM value
    llvm::Value* generateUnaryExpression(const ast::UnaryMinus& expr);

    /// @brief Generate logical NOT expression
    /// @param expr LogicalNot expression node
    /// @return Generated LLVM value (i1)
    llvm::Value* generateLogicalNot(const ast::LogicalNot& expr);

    /// @brief Generate logical AND expression (short-circuit)
    /// @param expr LogicalAnd expression node
    /// @return Generated LLVM value (i1)
    llvm::Value* generateLogicalAnd(const ast::LogicalAnd& expr);

    /// @brief Generate logical OR expression (short-circuit)
    /// @param expr LogicalOr expression node
    /// @return Generated LLVM value (i1)
    llvm::Value* generateLogicalOr(const ast::LogicalOr& expr);

    /// @brief Generate assignment expression
    /// @param expr Assignment expression node
    /// @return Generated LLVM value (assigned value)
    llvm::Value* generateAssignment(const ast::AssignmentExpression& expr);

    /// @brief Generate member access expression
    /// @param expr MemberAccess expression node
    /// @return Generated LLVM value (member value)
    llvm::Value* generateMemberAccess(const ast::MemberAccess& expr);

    /// @brief Generate array access expression
    /// @param expr ArrayAccess expression node
    /// @return Generated LLVM value (element value)
    llvm::Value* generateArrayAccess(const ast::ArrayAccess& expr);

    /// @brief Generate array literal expression
    /// @param literal ArrayLiteral expression node
    /// @return Generated LLVM value (array pointer)
    llvm::Value* generateArrayLiteral(const ast::ArrayLiteral& literal);

    // ========================================================================
    // Statement Generation
    // ========================================================================

    /// @brief Generate block of statements
    /// @param block Block statement node
    void generateBlock(const ast::Block& block);

    /// @brief Generate return statement
    /// @param ret Return statement node
    void generateReturnStatement(const ast::ReturnStatement& ret);

    /// @brief Generate expression statement (expression as statement)
    /// @param stmt Expression statement node
    void generateExpressionStatement(const ast::ExpressionStatement& stmt);

    /// @brief Generate if/else statement
    /// @param stmt If statement node
    void generateIfStatement(const ast::IfStatement& stmt);

    /// @brief Generate while loop statement
    /// @param stmt While statement node
    void generateWhileStatement(const ast::WhileStatement& stmt);

    /// @brief Generate for-in loop (iterate array elements)
    /// @param stmt ForIn statement node
    void generateForInStatement(const ast::ForInStatement& stmt);

    /// @brief Generate for-range loop (iterate numeric range)
    /// @param stmt ForRange statement node
    void generateForRangeStatement(const ast::ForRangeStatement& stmt);

    /// @brief Generate scope statement (explicit block scope)
    /// @param stmt Scope statement node
    void generateScopeStatement(const ast::ScopeStatement& stmt);

    /// @brief Generate break statement (exit enclosing loop)
    /// @param stmt Break statement node
    void generateBreakStatement(const ast::BreakStatement& stmt);

    /// @brief Generate continue statement (skip to next iteration)
    /// @param stmt Continue statement node
    void generateContinueStatement(const ast::ContinueStatement& stmt);

    /// @brief Generate variable declaration
    /// @param decl Variable declaration node
    void generateVariableDeclaration(const ast::VariableDeclaration& decl);

    /// @brief Generate global variable declaration
    void generateGlobalVariable(const ast::VariableDeclaration& decl);


    /// @brief Generate variable declaration statement
    /// @param stmt Variable declaration statement node
    void generateVariableDeclarationStatement(const ast::VariableDeclarationStatement& stmt);

    // ========================================================================
    // Type Conversion
    // ========================================================================

    /// @brief Convert primitive type kind to LLVM type
    /// @param kind Primitive type enumeration
    /// @return Corresponding LLVM type
    llvm::Type* convertPrimitiveType(ast::PrimitiveTypeKind kind);

    /// @brief Convert array type to LLVM pointer type
    /// @param arrayType AST array type node
    /// @return LLVM pointer type (arrays are represented as pointers)
    llvm::Type* convertArrayType(const ast::ArrayType& arrayType);

    // ========================================================================
    // Nullable Type Support
    // ========================================================================

    /// @brief Create nullable struct type { i1 isNull, T value }
    /// @param valueType The underlying value type
    /// @return Struct type for nullable values
    llvm::StructType* createNullableType(llvm::Type* valueType);

    /// @brief Create null constant for nullable type
    /// @param valueType The underlying value type
    /// @return Constant representing null { i1 true, undef }
    llvm::Value* createNullValue(llvm::Type* valueType);

    /// @brief Wrap value in nullable type
    /// @param value The value to wrap
    /// @param nullableType Nullable struct type
    /// @return Value wrapped as { i1 false, value }
    llvm::Value* wrapValueInNullable(llvm::Value* value, llvm::Type* nullableType);

    /// @brief Extract value from nullable (without checking null flag)
    /// @param nullableValue Nullable value
    /// @return The wrapped value field
    llvm::Value* extractValueFromNullable(llvm::Value* nullableValue);

    /// @brief Extract null flag from nullable value
    /// @param nullableValue Nullable value
    /// @return The isNull flag (i1)
    llvm::Value* extractNullFlagFromNullable(llvm::Value* nullableValue);

    /// @brief Check if AST type is nullable
    /// @param type AST type node
    /// @return true if type is optional or union containing optional
    bool isTypeNullable(const ast::Type& type);

    // ========================================================================
    // Utility Methods
    // ========================================================================

    /// @brief Create constant value from primary literal
    /// @param primary Primary AST node (integer, float, bool)
    /// @return LLVM Constant, or nullptr
    llvm::Constant* createConstant(const ast::Primary& primary);

    /// @brief Create global constant array
    /// @param elements Array of constant values
    /// @param elementType LLVM type of array elements
    /// @return Pointer to global array constant
    llvm::Constant* createGlobalArrayConstant(const std::vector<llvm::Constant*>& elements,
                                           llvm::Type* elementType);

    /// @brief Mangle function name for overloading
    /// @param name Base function name
    /// @param paramTypes Parameter types
    /// @return Mangled name string
    std::string mangleFunctionName(const std::string& name,
                                  const std::vector<llvm::Type*>& paramTypes);

    /// @brief Create alloca in entry block (proper placement for allocas)
    /// @param function Target function
    /// @param varName Variable name for debugging
    /// @param type Alloca type
    /// @return Created alloca instruction
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* function,
                                              const std::string& varName,
                                              llvm::Type* type);

    /// @brief Ensure value matches target type, applying implicit conversions
    /// @param value The value to check/convert
    /// @param targetType The required LLVM type
    /// @return The converted value, or original if types already match
    llvm::Value* ensureTypeMatch(llvm::Value* value, llvm::Type* targetType);

    // ========================================================================
    // Array Runtime Helpers
    // ========================================================================

    /// @brief Get or declare hoo_*_array_from_buffer function
    /// @param elementType Element LLVM type
    /// @return Array creation function, or nullptr
    llvm::Function* getArrayFromBufferFunc(llvm::Type* elementType);

    /// @brief Generate array literal using runtime functions
    /// @param elements Constant element values
    /// @param elementType LLVM element type
    /// @return Array pointer value
    llvm::Value* generateArrayLiteralWithRuntime(const std::vector<llvm::Constant*>& elements,
                                               llvm::Type* elementType);

    /// @brief Generate dynamic array with runtime element insertion
    /// @param elements Runtime-computed element values (for class instances)
    /// @param elementType LLVM element type
    /// @return Array pointer value
    llvm::Value* generateDynamicArrayLiteral(const std::vector<llvm::Value*>& elements,
                                           llvm::Type* elementType);

    /// @brief Get or declare hoo_array_new() function
    /// @param elementSize Element size (ignored in Phase 7)
    /// @return Array creation function
    llvm::Function* getArrayNewFunc(size_t elementSize);

    // Type-specific array push functions
    llvm::Function* getArrayPushInt64Func();
    llvm::Function* getArrayPushDoubleFunc();
    llvm::Function* getArrayPushFloatFunc();
    llvm::Function* getArrayPushBoolFunc();
    llvm::Function* getArrayPushCharFunc();
    llvm::Function* getArrayPushObjectFunc();

    /// @brief Select appropriate push function for element type
    /// @param elementType LLVM element type
    /// @return Matching push function, or object push as fallback
    llvm::Function* getArrayPushFuncForType(llvm::Type* elementType);

    // ========================================================================
    // Module System
    // ========================================================================

    /// @brief Module registry for import resolution
    ModuleRegistry moduleRegistry_;

    /// @brief Maps imported names to their resolved exports
    std::unordered_map<std::string, const ModuleExport*> importedNames_;

    /// @brief Process all import statements in compilation unit
    /// @param imports Vector of import statements
    void processImports(const std::vector<std::unique_ptr<ast::ImportStatement>>& imports);

    /// @brief Process basic import (import module.path as alias)
    /// @param import Basic import AST node
    void processBasicImport(const ast::BasicImport& import);

    /// @brief Process from import (from module import name)
    /// @param import From import AST node
    void processFromImport(const ast::FromImport& import);
};

} // namespace hooc
