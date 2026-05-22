#include "HVMCodeGenerator.h"
#include "ast/AST.h"
#include "ast/Expression.h"
#include "ast/Statement.h"
#include "ast/Primary.h"
#include "ast/Type.h"
#include "ast/ClassDeclaration.h"
#include "ast/QualifiedIdentifier.h"
#include <stdexcept>
#include <algorithm>

using namespace hvm;

namespace hooc {

HVMCodeGenerator::HVMCodeGenerator() {
    for (int i = 0; i < 32; ++i) usedRegs_[i] = false;
    // Reserved registers
    usedRegs_[0] = true; // r0 is hardwired zero
    usedRegs_[29] = true; // lr
    usedRegs_[30] = true; // fp
    usedRegs_[31] = true; // sp
}

std::unique_ptr<GeneratedModule> HVMCodeGenerator::generateModule(const ast::CompilationUnit& compilationUnit) {
    module_ = std::make_unique<HoModule>("hvm_module");
    instructions_.clear();
    currentByteOffset_ = 0;
    errors_.clear();
    locals_.clear();
    currentStackOffset_ = 0;
    allLabels_.clear();

    // Process all top-level declarations
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(decl.get())) {
            visitFunction(*funcDecl);
        } else if (auto varDecl = dynamic_cast<const ast::VariableDeclaration*>(decl.get())) {
            // Allocate space for global variable
            uint32_t dataOffset = 0;
            Section* dataSec = module_->getSection(".data");
            if (!dataSec) {
                Section s;
                s.name = ".data";
                s.type = SectionType::SHT_DATA;
                s.flags = SectionFlags::ALLOC | SectionFlags::WRITE;
                module_->addSection(std::move(s));
                dataSec = module_->getSection(".data");
            }
            
            dataOffset = static_cast<uint32_t>(dataSec->data.size());
            // Reserve 8 bytes (all globals 64-bit for now)
            for (int i = 0; i < 8; ++i) dataSec->data.push_back(0);
            dataSec->virtual_size = dataSec->data.size();

            Symbol sym;
            sym.name = varDecl->getName();
            sym.value = dataOffset;
            sym.type = Symbol::STT_OBJECT;
            sym.binding = Symbol::STB_GLOBAL;
            sym.section_index = 0; // Will be fixed by HoModule during serialization
            module_->addSymbol(sym);
        }
    }

    if (hasErrors()) {
        return nullptr;
    }

    // Finalize instructions into .text section
    std::vector<uint8_t> textData = module_->encodeInstructions(instructions_);
    Section textSection;
    textSection.name = ".text";
    textSection.type = SectionType::SHT_TEXT;
    textSection.flags = SectionFlags::ALLOC | SectionFlags::EXECUTE;
    textSection.data = std::move(textData);
    textSection.virtual_size = textSection.data.size();
    module_->addSection(std::move(textSection));

    return std::make_unique<HVMGeneratedModule>(std::move(module_));
}

GeneratedFunction* HVMCodeGenerator::generateFunction(const ast::FunctionDeclaration& funcDecl) {
    // This is mainly used for incremental generation if needed
    visitFunction(funcDecl);
    return new HVMGeneratedFunction(0); // Placeholder
}

GeneratedValue* HVMCodeGenerator::generateExpression(const ast::Expression& expr) {
    uint8_t reg = visitExpression(expr);
    return new HVMGeneratedValue(HVMGeneratedValue::Kind::Register, reg);
}

void HVMCodeGenerator::generateStatement(const ast::Statement& stmt) {
    visitStatement(stmt);
}

GeneratedType* HVMCodeGenerator::generateType(const ast::Type& /*type*/) {
    return new HVMGeneratedType(0); // Placeholder
}

// ============================================================================
// Internal Visitors
// ============================================================================

void HVMCodeGenerator::visitFunction(const ast::FunctionDeclaration& decl) {
    uint32_t funcStartOffset = currentByteOffset_;

    // 1. Setup frame: ENTER size
    // For now, we don't know the size yet. We'll emit NOP and fix it later, 
    // or just use a conservative estimate.
    size_t enterIdx = instructions_.size();
    emit(Opcode::ENTER, OperandsI{0, 0, 0}); 

    // 2. Map parameters to stack/registers
    // Argument registers are r1..r8
    auto& params = decl.getParameters();
    for (size_t i = 0; i < params.size() && i < 8; ++i) {
        int32_t offset = reserveLocal(params[i]->getName(), 0); // TODO: typeId
        emit(Opcode::ST_D, OperandsI{static_cast<uint8_t>(i + 1), 30, static_cast<int16_t>(offset)});
    }

    // 3. Generate body
    visitStatement(decl.getBody());

    // 4. Ensure return
    if (instructions_.empty() || instructions_.back().getOpcode() != Opcode::RET) {
        emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
        emit(Opcode::RET, OperandsR{0, 0, 0, 0});
    }

    // Fixup ENTER size
    int32_t frameSize = -currentStackOffset_;
    instructions_[enterIdx].setOperands(OperandsI{0, 0, static_cast<int16_t>(frameSize)});

    // Add symbol to module
    Symbol sym;
    sym.name = decl.getName();
    sym.value = funcStartOffset; // Correct byte offset
    sym.type = Symbol::STT_FUNC;
    sym.binding = Symbol::STB_GLOBAL;
    module_->addSymbol(sym);
    
    // Reset function-specific state
    locals_.clear();
    currentStackOffset_ = 0;
}

void HVMCodeGenerator::visitStatement(const ast::Statement& stmt) {
    if (auto block = dynamic_cast<const ast::Block*>(&stmt)) {
        for (const auto& s : block->getStatements()) {
            visitStatement(*s);
        }
    } else if (auto ret = dynamic_cast<const ast::ReturnStatement*>(&stmt)) {
        if (ret->hasExpression()) {
            uint8_t reg = visitExpression(*ret->getExpression());
            emit(Opcode::MOV, OperandsR{1, reg, 0, 0}); // r1 is return register
            freeRegister(reg);
        }
        emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
        emit(Opcode::RET, OperandsR{0, 0, 0, 0});
    } else if (auto varDecl = dynamic_cast<const ast::VariableDeclarationStatement*>(&stmt)) {
        auto& decl = varDecl->getDeclaration();
        int32_t offset = reserveLocal(decl.getName(), 0);
        if (decl.getInitializer()) {
            uint8_t reg = visitExpression(*decl.getInitializer());
            emit(Opcode::ST_D, OperandsI{reg, 30, static_cast<int16_t>(offset)});
            freeRegister(reg);
        }
    } else if (auto exprStmt = dynamic_cast<const ast::ExpressionStatement*>(&stmt)) {
        uint8_t reg = visitExpression(exprStmt->getExpression());
        freeRegister(reg);
    } else if (auto ifStmt = dynamic_cast<const ast::IfStatement*>(&stmt)) {
        Label* elseLabel = createLabel();
        Label* endLabel = createLabel();

        uint8_t condReg = visitExpression(ifStmt->getCondition());
        // Branch to else if cond is zero (false)
        emitBranch(Opcode::BEQ, condReg, 0, elseLabel);
        freeRegister(condReg);

        visitStatement(ifStmt->getThenBlock());
        emitJump(Opcode::JMP, 0, endLabel);

        bindLabel(elseLabel);
        if (ifStmt->hasElse()) {
            visitStatement(*ifStmt->getElseBlock());
        }
        bindLabel(endLabel);
    } else if (auto whileStmt = dynamic_cast<const ast::WhileStatement*>(&stmt)) {
        Label* startLabel = createLabel();
        Label* endLabel = createLabel();

        bindLabel(startLabel);
        uint8_t condReg = visitExpression(whileStmt->getCondition());
        emitBranch(Opcode::BEQ, condReg, 0, endLabel);
        freeRegister(condReg);

        controlFlowStack_.push({endLabel, startLabel});
        visitStatement(whileStmt->getBody());
        controlFlowStack_.pop();

        emitJump(Opcode::JMP, 0, startLabel);
        bindLabel(endLabel);
    } else if (auto breakStmt = dynamic_cast<const ast::BreakStatement*>(&stmt)) {
        if (controlFlowStack_.empty()) {
            addError("break statement outside of loop");
        } else {
            emitJump(Opcode::JMP, 0, controlFlowStack_.top().breakLabel);
        }
    } else if (auto continueStmt = dynamic_cast<const ast::ContinueStatement*>(&stmt)) {
        if (controlFlowStack_.empty()) {
            addError("continue statement outside of loop");
        } else {
            emitJump(Opcode::JMP, 0, controlFlowStack_.top().continueLabel);
        }
    }
}

uint8_t HVMCodeGenerator::visitExpression(const ast::Expression& expr) {
    if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(&expr)) {
        auto& primary = primaryExpr->getPrimary();
        if (auto intLit = dynamic_cast<const ast::IntegerLiteral*>(&primary)) {
            return emitConstant(intLit->getValue());
        } else if (auto id = dynamic_cast<const ast::Identifier*>(&primary)) {
            int32_t offset = getLocalOffset(id->getName());
            uint8_t reg = allocateRegister();
            emit(Opcode::LD_D, OperandsI{reg, 30, static_cast<int16_t>(offset)});
            return reg;
        } else if (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(&primary)) {
            return visitExpression(paren->getExpression());
        } else if (auto boolLit = dynamic_cast<const ast::BooleanLiteral*>(&primary)) {
            return emitConstant(boolLit->getValue() ? 1 : 0);
        } else if (auto nullLit = dynamic_cast<const ast::NullLiteral*>(&primary)) {
            return emitConstant(0);
        }
    } else if (auto binary = dynamic_cast<const ast::BinaryExpression*>(&expr)) {
        uint8_t left = visitExpression(binary->getLeft());
        uint8_t right = visitExpression(binary->getRight());
        uint8_t dest = allocateRegister();
        
        Opcode op = Opcode::ARITH;
        uint16_t func = 0;
        
        switch (binary->getOperator()) {
            case ast::BinaryOperator::PLUS:  func = 0; break;
            case ast::BinaryOperator::MINUS: func = 1; break;
            case ast::BinaryOperator::MULTIPLY: func = 2; break;
            case ast::BinaryOperator::DIVIDE: func = 5; break;
            case ast::BinaryOperator::EQUALS: op = Opcode::CMP; func = 0; break;
            case ast::BinaryOperator::NOT_EQUALS: op = Opcode::CMP; func = 1; break;
            case ast::BinaryOperator::LESS: op = Opcode::CMP; func = 2; break;
            case ast::BinaryOperator::LESS_EQUALS: op = Opcode::CMP; func = 3; break;
            case ast::BinaryOperator::GREATER: {
                // a > b  =>  b < a
                op = Opcode::CMP; func = 2;
                std::swap(left, right);
                break;
            }
            case ast::BinaryOperator::GREATER_EQUALS: {
                // a >= b =>  b <= a
                op = Opcode::CMP; func = 3;
                std::swap(left, right);
                break;
            }
            case ast::BinaryOperator::AND: op = Opcode::LOGIC; func = 0; break;
            case ast::BinaryOperator::OR:  op = Opcode::LOGIC; func = 1; break;
            default: addError("Unsupported binary operator");
        }
        
        emit(op, OperandsR{dest, left, right, func});
        freeRegister(left);
        freeRegister(right);
        return dest;
    } else if (auto logicAnd = dynamic_cast<const ast::LogicalAnd*>(&expr)) {
        uint8_t left = visitExpression(logicAnd->getLeft());
        uint8_t right = visitExpression(logicAnd->getRight());
        uint8_t dest = allocateRegister();
        emit(Opcode::LOGIC, OperandsR{dest, left, right, 0}); // func 0 = AND
        freeRegister(left);
        freeRegister(right);
        return dest;
    } else if (auto logicOr = dynamic_cast<const ast::LogicalOr*>(&expr)) {
        uint8_t left = visitExpression(logicOr->getLeft());
        uint8_t right = visitExpression(logicOr->getRight());
        uint8_t dest = allocateRegister();
        emit(Opcode::LOGIC, OperandsR{dest, left, right, 1}); // func 1 = OR
        freeRegister(left);
        freeRegister(right);
        return dest;
    } else if (auto unaryMinus = dynamic_cast<const ast::UnaryMinus*>(&expr)) {
        uint8_t src = visitExpression(unaryMinus->getOperand());
        uint8_t dest = allocateRegister();
        // NEG rd, rs  => SUB rd, r0, rs
        emit(Opcode::ARITH, OperandsR{dest, 0, src, 1}); // func 1 = SUB, r0 = 0
        freeRegister(src);
        return dest;
    } else if (auto assign = dynamic_cast<const ast::AssignmentExpression*>(&expr)) {
        uint8_t valueReg = visitExpression(assign->getRight());
        
        // Handle LHS: only simple identifiers for now
        if (auto leftPrimary = dynamic_cast<const ast::PrimaryExpression*>(&assign->getLeft())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&leftPrimary->getPrimary())) {
                int32_t offset = getLocalOffset(id->getName());
                emit(Opcode::ST_D, OperandsI{valueReg, 30, static_cast<int16_t>(offset)});
                return valueReg; // Assignment returns the value
            }
        }
        addError("Unsupported assignment target");
        return valueReg;
    }

    addError("Unsupported expression type");
    return 0;
}

// ============================================================================
// Helpers
// ============================================================================

uint8_t HVMCodeGenerator::allocateRegister() {
    for (uint8_t i = 9; i <= 15; ++i) { // Use temporary registers
        if (!usedRegs_[i]) {
            usedRegs_[i] = true;
            return i;
        }
    }
    addError("Register pressure: out of temporary registers (spilling not implemented)");
    return 0;
}

void HVMCodeGenerator::freeRegister(uint8_t reg) {
    if (reg >= 9 && reg <= 15) {
        usedRegs_[reg] = false;
    }
}

int32_t HVMCodeGenerator::reserveLocal(const std::string& name, uint32_t typeId) {
    currentStackOffset_ -= 8; // All variables 8 bytes for now
    locals_[name] = {currentStackOffset_, typeId};
    return currentStackOffset_;
}

int32_t HVMCodeGenerator::getLocalOffset(const std::string& name) {
    auto it = locals_.find(name);
    if (it != locals_.end()) {
        return it->second.offset;
    }
    addError("Undefined variable: " + name);
    return 0;
}

void HVMCodeGenerator::emit(Opcode op, const Operands& operands) {
    HInstruction inst(op, operands);
    instructions_.push_back(inst);
    currentByteOffset_ += inst.getSize();
}

uint8_t HVMCodeGenerator::emitConstant(int64_t value) {
    uint8_t reg = allocateRegister();
    if (value >= 0 && value <= 32767) {
        // MOVZ is zero-extended 15-bit
        emit(Opcode::MOVZ, OperandsI{reg, 0, static_cast<int16_t>(value)});
    } else if (value >= -16384 && value <= 16383) {
        // ADDI is sign-extended 15-bit
        emit(Opcode::ADDI, OperandsI{reg, 0, static_cast<int16_t>(value)});
    } else {
        // TODO: Handle large constants with LUI + ADDI
        addError("Large constants not yet implemented in HVM backend");
    }
    return reg;
}

void HVMCodeGenerator::addError(const std::string& message) {
    errors_.push_back(message);
}

HVMCodeGenerator::Label* HVMCodeGenerator::createLabel() {
    allLabels_.push_back(std::make_unique<Label>());
    return allLabels_.back().get();
}

void HVMCodeGenerator::bindLabel(Label* label) {
    label->targetByteOffset = static_cast<int32_t>(currentByteOffset_);
    
    // Resolve forward jumps
    for (const auto& fixup : label->fixups) {
        auto& inst = instructions_[fixup.instructionIndex];
        // Relative offset in 4-byte words
        int32_t byteOffset = label->targetByteOffset - static_cast<int32_t>(fixup.instructionByteOffset);
        int32_t wordOffset = byteOffset / 4;
        
        auto operands = inst.getOperands();
        if (std::holds_alternative<OperandsB>(operands)) {
            auto& ops = std::get<OperandsB>(operands);
            ops.imm15 = static_cast<int16_t>(wordOffset);
            inst.setOperands(ops);
        } else if (std::holds_alternative<OperandsJ>(operands)) {
            auto& ops = std::get<OperandsJ>(operands);
            ops.offset = wordOffset;
            inst.setOperands(ops);
        }
    }
    label->fixups.clear();
}

void HVMCodeGenerator::emitJump(Opcode op, uint8_t rd, Label* target) {
    size_t instrIdx = instructions_.size();
    uint32_t instrOff = currentByteOffset_;

    if (target->targetByteOffset != -1) {
        int32_t wordOffset = (target->targetByteOffset - static_cast<int32_t>(instrOff)) / 4;
        emit(op, OperandsJ{rd, wordOffset});
    } else {
        target->fixups.push_back({instrIdx, instrOff});
        emit(op, OperandsJ{rd, 0});
    }
}

void HVMCodeGenerator::emitBranch(Opcode op, uint8_t rs1, uint8_t rs2, Label* target) {
    size_t instrIdx = instructions_.size();
    uint32_t instrOff = currentByteOffset_;

    if (target->targetByteOffset != -1) {
        int32_t wordOffset = (target->targetByteOffset - static_cast<int32_t>(instrOff)) / 4;
        emit(op, OperandsB{rs1, rs2, static_cast<int16_t>(wordOffset)});
    } else {
        target->fixups.push_back({instrIdx, instrOff});
        emit(op, OperandsB{rs1, rs2, 0});
    }
}

} // namespace hooc
