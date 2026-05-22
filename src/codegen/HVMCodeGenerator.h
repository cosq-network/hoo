#pragma once

/**
 * @file HVMCodeGenerator.h
 * @brief HVM Bytecode generator that translates Hooc AST to HVM instructions.
 *
 * PURPOSE
 *   Implements the CodeGenerator interface for the HVM backend.
 *   Produces a binary HoModule containing bit-packed instructions.
 *
 * ARCHITECTURE
 *   - AST Traversal: Single-pass lowering of AST nodes to HInstructions.
 *   - Register Allocation: Simple management of r9-r15 for temporary values.
 *   - Stack Framing: Management of r30 (FP) offsets for local variables.
 *   - Label Fixups: Deferred offset calculation for forward branches/jumps.
 */

#include "CodeGenerator.h"
#include "HVMCodeGeneratorTypes.h"
#include "hvm/HInstruction.h"
#include "hvm/HoModule.h"
#include <vector>
#include <unordered_map>
#include <stack>
#include <string>

namespace hooc {

class HVMCodeGenerator : public CodeGenerator {
public:
    HVMCodeGenerator();
    virtual ~HVMCodeGenerator() = default;

    /**
     * @return "HVM" backend identifier.
     */
    std::string getBackendType() const override { return "HVM"; }

    /**
     * Main entry point: translates a full AST unit into a bytecode module.
     */
    std::unique_ptr<GeneratedModule> generateModule(const ast::CompilationUnit& compilationUnit) override;

    // CodeGenerator interface overrides (internal usage)
    GeneratedFunction* generateFunction(const ast::FunctionDeclaration& funcDecl) override;
    GeneratedValue* generateExpression(const ast::Expression& expr) override;
    void generateStatement(const ast::Statement& stmt) override;
    GeneratedType* generateType(const ast::Type& type) override;

    /**
     * Get any errors that occurred during generation.
     */
    const std::vector<std::string>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    // Core state
    std::unique_ptr<hvm::HoModule> module_;
    std::vector<hvm::HInstruction> instructions_;
    uint32_t currentByteOffset_ = 0;
    std::vector<std::string> errors_;

    // Register Management (r9-r15 available for temps)
    bool usedRegs_[32];
    uint8_t allocateRegister();
    void freeRegister(uint8_t reg);

    // Local Variable & Stack Management
    struct Local {
        int32_t offset; // Offset relative to FP (r30)
        uint32_t typeId;
    };
    std::unordered_map<std::string, Local> locals_;
    int32_t currentStackOffset_ = 0;
    
    /**
     * Reserve space on stack for a local variable.
     */
    int32_t reserveLocal(const std::string& name, uint32_t typeId);
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
    
    Label* createLabel();
    void bindLabel(Label* label);
    
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
    };
    std::stack<ControlFlowScope> controlFlowStack_;
    std::vector<std::unique_ptr<Label>> allLabels_;

    // AST Visiting logic
    void visitStatement(const ast::Statement& stmt);
    uint8_t visitExpression(const ast::Expression& expr); // Returns register index
    void visitFunction(const ast::FunctionDeclaration& decl);

    // Instruction Helpers
    void emit(hvm::Opcode op, const hvm::Operands& operands);
    uint8_t emitConstant(int64_t value);
    void addError(const std::string& message);
};

} // namespace hooc
