#pragma once

/**
 * @file HVMCodeGenerator.h
 * @brief HVM Bytecode generator that translates Hooc AST to HVM instructions.
 */

#include "CodeGenerator.h"
#include "HVMCodeGeneratorTypes.h"
#include "hvm/HVMInstruction.h"
#include "hvm/HOModule.h"
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <string>

namespace hooc {

class HVMCodeGenerator : public CodeGenerator {
public:
    HVMCodeGenerator();
    virtual ~HVMCodeGenerator() = default;
    void setModuleContext(const std::string& moduleName);
    void setExternalFunctionImports(const std::unordered_map<std::string, std::pair<std::string, std::string>>& functions);

    /**
     * Main entry point: translates a full AST unit into a bytecode module.
     */
    std::unique_ptr<GeneratedModule> generateModule(const ast::CompilationUnit& compilationUnit) override;

    // CodeGenerator interface overrides (internal usage)
    std::unique_ptr<GeneratedFunction> generateFunction(const ast::FunctionDeclaration& funcDecl) override;
    std::unique_ptr<GeneratedValue> generateExpression(const ast::Expression& expr) override;
    void generateStatement(const ast::Statement& stmt) override;
    std::unique_ptr<GeneratedType> generateType(const ast::Type& type) override;

    /**
     * Get any errors that occurred during generation.
     */
    const std::vector<std::string>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    // Core state
    std::vector<std::string> modulePath_;
    std::string pendingModuleName_;
    std::unique_ptr<hvm::HOModule> module_;
    std::vector<hvm::HVMInstruction> instructions_;
    uint32_t currentByteOffset_ = 0;
    std::vector<uint8_t> compressedInstructions_;


    std::vector<std::string> errors_;
    std::unordered_set<std::string> importedModules_;
    std::unordered_map<std::string, std::string> importedSymbols_;
    std::unordered_map<std::string, std::pair<std::string, std::string>> externalFunctionImports_;

    bool isModuleImported(const std::string& moduleName) const;
    bool isSymbolImported(const std::string& name, const std::string& requiredModule) const;
    std::string getRequiredModule(const std::string& name) const;

    // Register Management (r9-r20 available for temps)
    bool usedRegs_[32];
    uint8_t allocateRegister();
    void freeRegister(uint8_t reg);
    void emitCompressed(uint8_t opcode4, uint8_t rd, uint8_t rs1, uint8_t imm4);



    // Local Variable & Stack Management
    struct Local {
        int32_t offset; // Offset relative to FP (r30)
        uint32_t typeId;
        std::string className; // Class name for user-defined types (empty for primitives)
        uint32_t elementTypeId = 0; // Element type for Array variables (0 = unknown/Object)
        uint32_t keyTypeId = 0; // Key type for HashMap variables
    };
    std::vector<std::unordered_map<std::string, Local>> scopeStack_;
    int32_t currentStackOffset_ = 0;
    
    // Object & Class Management
    enum class FieldAccess { PUBLIC, PRIVATE, DEFAULT_VAR };
    struct ClassLayout {
        std::string name;
        std::string baseClass; // empty if no base class
        std::unordered_map<std::string, int32_t> fieldOffsets;
        std::unordered_map<std::string, bool> privateMethods; // methodName -> isPrivate
        std::unordered_map<std::string, FieldAccess> fieldAccess; // fieldName -> access level
        std::unordered_map<std::string, uint32_t> methodReturnTypes; // methodName -> typeId
        int32_t totalSize = 0;
        bool isSingleton = false;
        bool isFinal = false;
        bool isImmutable = false;
        bool isService = false;
        bool isSerializable = false;
        uint32_t singletonDataOffset = 0; // .data offset for singleton pointer
    };
    std::unordered_map<std::string, ClassLayout> classes_;
    std::unordered_map<std::string, std::string> methodNameToClass_;
    std::unordered_map<std::string, bool> isOverloadedFunction_;
    std::unordered_map<std::string, std::unordered_map<std::string, bool>> isOverloadedMethod_; // methodName -> className
    std::unordered_map<std::string, uint32_t> functionReturnTypes_; // functionName -> typeId
    std::unordered_map<std::string, std::string> functionReturnClass_; // functionName -> className (for user-defined types)
    ClassLayout* currentClass_ = nullptr;
    bool inConstructor_ = false;
    bool currentFunctionHasReturn_ = false;
    std::vector<std::pair<std::string, uint32_t>> pendingSingletons_; // className, .data offset
    
    // Serializable class adjacency for cycle detection: className -> ser. dependency class names
    std::unordered_map<std::string, std::vector<std::string>> serializableAdjacency_;
    
    /**
     * Emit a module_init function that runs once at module load time.
     */
    void emitModuleInit();
    
    /**
     * Reserve space on stack for a local variable.
     */
    int32_t reserveLocal(const std::string& name, uint32_t typeId, const std::string& className = "", uint32_t elementTypeId = 0, uint32_t keyTypeId = 0);
    int32_t getLocalOffset(const std::string& name);

    // Label & Control Flow
    struct Label {
        int32_t targetByteOffset = -1; // Absolute byte offset in text section
        struct Fixup {
            size_t instructionIndex;
            uint32_t instructionByteOffset;
        };
        std::vector<Fixup> fixups; // Indices and offsets to update when bound
    };

    struct SymbolFixup {
        std::string symbolName;
        size_t instructionIndex;
        uint32_t instructionByteOffset;
    };
    std::vector<SymbolFixup> symbolFixups_;
    
    Label* createLabel();
    void bindLabel(Label* label);
    
    /**
     * Map an AST type to its runtime Type ID.
     * If outClassName is provided, it will be set to the class name for user-defined types.
     */
    uint32_t getTypeId(const ast::Type* type, const ast::Expression* initializer = nullptr, std::string* outClassName = nullptr);

    /**
     * Look up the typeId of a local variable from scope.
     */
    uint32_t getLocalTypeId(const std::string& name) const;

    /**
     * Look up the className of a local variable from scope.
     */
    std::string getLocalClassName(const std::string& name) const;

    /**
     * Look up the elementTypeId of a local variable (for Array types) from scope.
     */
    uint32_t getLocalElementTypeId(const std::string& name) const;
    uint32_t getLocalKeyTypeId(const std::string& name) const;

    /**
     * Emit hoo_release for managed locals in scopes [to, from).
     */
    void emitScopeCleanup(size_t from, size_t to);

    /**
     * Check whether an expression evaluates to a freshly allocated managed
     * temporary (string literal, interpolated string, new object/map, array/tensor literal).
     * These temporaries are not tracked in locals and must be released at the point of discard.
     */
    bool isManagedTemporary(const ast::Expression& expr);

    /**
     * Convert a declared AST type to a runtime typeId.
     */
    uint32_t typeIdFromDeclaredType(const ast::Type* type, std::string* outClassName = nullptr) const;

    /**
     * Infer the runtime typeId of an expression in the current scope.
     */
    uint32_t inferExpressionTypeId(const ast::Expression& expr);

    /**
     * Convert a runtime typeId to the return-type string used by the mangler.
     */
    std::string typeIdToMangleType(uint32_t typeId) const;

    uint32_t tensorElementTypeIdFromType(const ast::TensorType& type) const;
    uint32_t tensorElementTypeIdFromLiteral(const ast::TensorLiteral& literal);
    std::vector<int64_t> tensorShapeFromLiteral(const ast::TensorLiteral& literal);
    void emitFlattenTensorLiteralElements(const ast::Expression& expr, uint8_t tensorReg);
    uint8_t emitTensorLiteral(const ast::TensorLiteral& literal);
    uint8_t emitTensorBinaryCall(const ast::BinaryExpression& binary, const std::string& symbolName);
    uint8_t emitTensorVectorArith(const ast::BinaryExpression& binary, hvm::Opcode vecOp, uint16_t func);
    uint8_t emitDecimalBinaryOp(const ast::BinaryExpression& binary);
    /**
     * Check if a name matches a known built-in class for static dispatch.
     */
    bool isBuiltinClassName(const std::string& name) const;
    
    /**
     * Emit a call instruction with a deferred symbol target.
     */
    void emitCall(hvm::Opcode op, const std::string& symbol);
    
    /**
     * Emit a jump instruction with a deferred target.
     */
    void emitJump(hvm::Opcode op, uint8_t rd, Label* target);
    
    /**
     * Emit a branch instruction with a deferred target.
     */
    void emitBranch(hvm::Opcode op, uint8_t rs1, uint8_t rs2, Label* target);

    struct ControlFlowScope {
        Label* breakLabel;
        Label* continueLabel;
        size_t scopeDepth;
    };
    std::stack<ControlFlowScope> controlFlowStack_;
    std::vector<std::unique_ptr<Label>> allLabels_;

    // AST Visiting logic
    void visitStatement(const ast::Statement& stmt);
    bool isDerivedFrom(const std::string& className, const std::string& potentialBase) const;
    bool canWriteField(const std::string& fieldName, const std::string& owningClass) const;
    bool canReadField(const std::string& fieldName, const std::string& owningClass) const;
    uint8_t visitExpression(const ast::Expression& expr); // Returns register index
    void visitFunction(const ast::FunctionDeclaration& decl);
    void visitConstructor(const ast::ConstructorDeclaration& decl);
    void visitMethod(const ast::FunctionDeclaration& decl);

    // Shared function prologue/epilogue helpers
    struct FunctionPrologueInfo {
        size_t enterIdx;
        uint32_t funcStartOffset;
        std::string mangledName;
    };
    FunctionPrologueInfo beginFunction(const ast::FunctionDeclaration* decl,
                                       const ast::ConstructorDeclaration* ctorDecl,
                                       bool isMethod, bool isConstructor);
    void endFunction(const FunctionPrologueInfo& info);

    // Serializable class validation
    void validateSerializableClass(const ast::ClassDeclaration& classDecl,
                                   const ClassLayout& layout,
                                   const std::string& name);
    bool isValidSerializableType(const ast::Type& type,
                                  const std::string& className,
                                  const std::string& fieldName);
    void detectSerializableCycles();

    // Serializable class code generation
    void emitSerializeMethod(const ClassLayout& layout, const ast::ClassDeclaration& classDecl);
    void emitDeserializeMethod(const ClassLayout& layout, const ast::ClassDeclaration& classDecl);
    uint8_t emitStringLiteral(const std::string& str);
    uint32_t serializeFieldTypeId(const ast::Type& type) const;

    // Instruction Helpers
    void emit(hvm::Opcode op, const hvm::Operands& operands);
    uint8_t emitConstant(int64_t value);
    uint8_t emitRoDataAddress(uint32_t offset);
    void addError(const std::string& message);
    // Compressed 16‑bit instruction support

};

} // namespace hooc
