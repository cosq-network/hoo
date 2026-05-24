#include "HVMCodeGenerator.h"
#include "ast/AST.h"
#include "ast/Expression.h"
#include "ast/Statement.h"
#include "ast/Primary.h"
#include "ast/Type.h"
#include "ast/ClassDeclaration.h"
#include "ast/QualifiedIdentifier.h"
#include "ast/ImportStatement.h"
#include "core/SymbolMangler.h"
#include "parsing/HooParserWrapper.h"
#include "ast/SimpleASTBuilder.h"
#include <stdexcept>
#include <algorithm>
#include <typeinfo>
#include <atomic>
#include <sstream>

using namespace hvm;

namespace hooc {

HVMCodeGenerator::HVMCodeGenerator(ModuleRegistry& moduleRegistry)
    : moduleRegistry_(moduleRegistry) {
    for (int i = 0; i < 32; ++i) usedRegs_[i] = false;
    // Reserved registers
    usedRegs_[0] = true; // r0 is hardwired zero
    usedRegs_[29] = true; // lr
    usedRegs_[30] = true; // fp
    usedRegs_[31] = true; // sp

    // Pre-populate standard library class layouts
    ClassLayout excLayout;
    excLayout.name = "Exception";
    excLayout.fieldOffsets["typeId"] = 0;
    excLayout.fieldOffsets["typeName"] = 8;
    excLayout.fieldOffsets["message"] = 16;
    excLayout.fieldOffsets["refcount"] = 24;
    excLayout.fieldOffsets["cause"] = 32;
    excLayout.totalSize = 40;
    classes_["Exception"] = excLayout;
    classes_["hoo.Exception"] = excLayout;
}

void HVMCodeGenerator::setModuleContext(const std::string& moduleName) {
    pendingModuleName_ = moduleName;
}

std::unique_ptr<GeneratedModule> HVMCodeGenerator::generateModule(const ast::CompilationUnit& compilationUnit) {
    // 1. Determine Module Name/Path
    static std::atomic<uint64_t> sSyntheticModuleCounter{0};
    std::string moduleName = pendingModuleName_;
    if (moduleName.empty()) {
        moduleName = "hvm_module_" + std::to_string(++sSyntheticModuleCounter);
    }
    pendingModuleName_.clear();
    modulePath_.clear();
    {
        std::stringstream ss(moduleName);
        std::string part;
        while (std::getline(ss, part, '.')) {
            if (!part.empty()) modulePath_.push_back(part);
        }
    }
    
    // In a real scenario, this would come from the compiler's source tracking.
    // For now we look for a marker or use default.
    module_ = std::make_unique<hvm::HOModule>(moduleName);
    instructions_.clear();
    currentByteOffset_ = 0;
    errors_.clear();
    locals_.clear();
    currentStackOffset_ = 0;
    allLabels_.clear();
    symbolFixups_.clear();

    // 2. Process Imports (SHT_IMPORT)
    for (const auto& imp : compilationUnit.getImports()) {
        const ast::ModulePath* pathNode = nullptr;
        if (auto basic = dynamic_cast<const ast::BasicImport*>(imp.get())) {
            pathNode = basic->getModule();
        } else if (auto fromImp = dynamic_cast<const ast::FromImport*>(imp.get())) {
            pathNode = fromImp->getModule();
        }

        if (pathNode) {
            std::string fullName;
            for (const auto& part : pathNode->getComponents()) {
                if (!fullName.empty()) fullName += ".";
                fullName += part;
            }
            module_->addDependency(fullName, ModuleType::Compiled);
        }
    }

    // 3. Process all top-level declarations
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
            if (!modulePath_.empty()) {
                sym.name = SymbolMangler::mangleModuleSymbol(modulePath_, varDecl->getName());
            } else {
                sym.name = varDecl->getName();
            }
            sym.value = dataOffset;
            sym.type = Symbol::STT_OBJECT;
            sym.binding = Symbol::STB_GLOBAL;
            sym.section_index = 0; 
            module_->addSymbol(sym);
        } else if (auto classDecl = dynamic_cast<const ast::ClassDeclaration*>(decl.get())) {
            ClassLayout layout;
            layout.name = classDecl->getName();
            
            // Calculate field offsets
            int32_t currentOffset = 0;
            for (const auto& member : classDecl->getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto var = dynamic_cast<const ast::VariableDeclaration*>(declMember)) {
                        layout.fieldOffsets[var->getName()] = currentOffset;
                        currentOffset += 8;
                    }
                }
            }
            layout.totalSize = currentOffset;
            classes_[layout.name] = layout;

            // Process methods
            currentClass_ = &classes_[layout.name];
            for (const auto& member : classDecl->getBody().getMembers()) {
                if (auto declMember = member->getDeclaration()) {
                    if (auto fn = dynamic_cast<const ast::FunctionDeclaration*>(declMember)) {
                        visitMethod(*fn);
                    }
                } else if (auto ctor = member->getConstructor()) {
                    visitConstructor(*ctor);
                }
            }
            currentClass_ = nullptr;
        }
    }

    if (hasErrors()) {
        return nullptr;
    }

    // Resolve all symbol fixups
    for (const auto& fixup : symbolFixups_) {
        auto* sym = module_->getSymbol(fixup.symbolName);
        if (!sym) continue;

        auto& inst = instructions_[fixup.instructionIndex];
        int32_t wordOffset = (static_cast<int32_t>(sym->value) - static_cast<int32_t>(fixup.instructionByteOffset)) / 4;
        
        auto operands = inst.getOperands();
        if (std::holds_alternative<OperandsJ>(operands)) {
            auto& ops = std::get<OperandsJ>(operands);
            ops.offset = wordOffset;
            inst.setOperands(ops);
        }
    }

    // Finalize instructions
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
    visitFunction(funcDecl);
    return new HVMGeneratedFunction(0);
}

GeneratedValue* HVMCodeGenerator::generateExpression(const ast::Expression& expr) {
    uint8_t reg = visitExpression(expr);
    return new HVMGeneratedValue(HVMGeneratedValue::Kind::Register, reg);
}

void HVMCodeGenerator::generateStatement(const ast::Statement& stmt) {
    visitStatement(stmt);
}

GeneratedType* HVMCodeGenerator::generateType(const ast::Type& /*type*/) {
    return new HVMGeneratedType(0);
}

// ============================================================================
// Internal Visitors
// ============================================================================

HVMCodeGenerator::FunctionPrologueInfo HVMCodeGenerator::beginFunction(
    const ast::FunctionDeclaration* decl,
    const ast::ConstructorDeclaration* ctorDecl,
    bool isMethod, bool isConstructor)
{
    FunctionPrologueInfo info;
    info.funcStartOffset = currentByteOffset_;
    info.enterIdx = instructions_.size();
    emit(Opcode::ENTER, OperandsI{0, 0, 0});

    uint8_t firstArgReg = isMethod ? 2 : 1;
    uint8_t maxArgRegs = isMethod ? 7 : 8;

    auto mapParams = [&](const auto& params) {
        for (size_t i = 0; i < params.size() && i < maxArgRegs; ++i) {
            int32_t offset = reserveLocal(params[i]->getName(), getTypeId(&params[i]->getType(), nullptr));
            emit(Opcode::ST_D, OperandsI{static_cast<uint8_t>(i + firstArgReg), 30, static_cast<int16_t>(offset)});
        }
    };

    if (decl) {
        mapParams(decl->getParameters());
        visitStatement(decl->getBody());
    } else if (ctorDecl) {
        mapParams(ctorDecl->getParameters());
        visitStatement(ctorDecl->getBody());
    }

    if (instructions_.empty() || instructions_.back().getOpcode() != Opcode::RET) {
        emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
        emit(Opcode::RET, OperandsR{0, 0, 0, 0});
    }

    MangledFunctionParams mp;
    mp.modulePath = modulePath_;
    if (currentClass_) mp.className = currentClass_->name;
    mp.isConstructor = isConstructor;

    if (decl) {
        mp.functionName = decl->getName();
        if (decl->getReturnType()) {
            if (auto bt = dynamic_cast<const ast::BaseType*>(decl->getReturnType())) {
                if (bt->isPrimitive()) {
                    mp.returnType = primitiveTypeToString(bt->getPrimitiveType()->getKind());
                } else {
                    mp.returnType = bt->getIdentifier();
                }
            }
        } else {
            mp.returnType = "void";
        }
    }

    auto addParamTypes = [&](const auto& params) {
        for (const auto& param : params) {
            if (auto bt = dynamic_cast<const ast::BaseType*>(&param->getType())) {
                if (bt->isPrimitive()) {
                    mp.parameterTypes.push_back(primitiveTypeToString(bt->getPrimitiveType()->getKind()));
                } else {
                    mp.parameterTypes.push_back(bt->getIdentifier());
                }
            } else {
                mp.parameterTypes.push_back("object");
            }
        }
    };

    if (decl) {
        addParamTypes(decl->getParameters());
    } else if (ctorDecl) {
        addParamTypes(ctorDecl->getParameters());
    }

    bool shouldMangle = !modulePath_.empty() || currentClass_ != nullptr;
    if (isConstructor) {
        info.mangledName = shouldMangle ? SymbolMangler::mangleFunctionName(mp) : "constructor";
    } else if (decl) {
        info.mangledName = shouldMangle ? SymbolMangler::mangleFunctionName(mp) : decl->getName();
    }

    return info;
}

void HVMCodeGenerator::endFunction(const FunctionPrologueInfo& info) {
    int32_t frameSize = -currentStackOffset_;
    instructions_[info.enterIdx].setOperands(OperandsI{0, 0, static_cast<int16_t>(frameSize)});

    Symbol sym;
    sym.name = info.mangledName;
    sym.value = info.funcStartOffset;
    sym.type = Symbol::STT_FUNC;
    sym.binding = Symbol::STB_GLOBAL;
    module_->addSymbol(sym);

    locals_.clear();
    currentStackOffset_ = 0;
}

void HVMCodeGenerator::visitFunction(const ast::FunctionDeclaration& decl) {
    auto info = beginFunction(&decl, nullptr, false, false);
    endFunction(info);
}

void HVMCodeGenerator::visitConstructor(const ast::ConstructorDeclaration& decl) {
    auto info = beginFunction(nullptr, &decl, true, true);
    endFunction(info);
}

void HVMCodeGenerator::visitMethod(const ast::FunctionDeclaration& decl) {
    auto info = beginFunction(&decl, nullptr, true, false);
    endFunction(info);
}

void HVMCodeGenerator::visitStatement(const ast::Statement& stmt) {
    if (auto block = dynamic_cast<const ast::Block*>(&stmt)) {
        for (const auto& s : block->getStatements()) {
            visitStatement(*s);
        }
    } else if (auto ret = dynamic_cast<const ast::ReturnStatement*>(&stmt)) {
        if (ret->hasExpression()) {
            uint8_t reg = visitExpression(*ret->getExpression());
            emit(Opcode::MOV, OperandsR{1, reg, 0, 0});
            freeRegister(reg);
        }
        emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
        emit(Opcode::RET, OperandsR{0, 0, 0, 0});
    } else if (auto varDecl = dynamic_cast<const ast::VariableDeclarationStatement*>(&stmt)) {
        auto& decl = varDecl->getDeclaration();
        int32_t offset = reserveLocal(decl.getName(), getTypeId(decl.getType(), decl.getInitializer()));
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
    } else if (auto forRange = dynamic_cast<const ast::ForRangeStatement*>(&stmt)) {
        int32_t offset = reserveLocal(forRange->getVariable(), 1);
        uint8_t startReg = visitExpression(forRange->getStart());
        emit(Opcode::ST_D, OperandsI{startReg, 30, static_cast<int16_t>(offset)});
        freeRegister(startReg);
        Label* startLabel = createLabel();
        Label* endLabel = createLabel();
        Label* stepLabel = createLabel();
        bindLabel(startLabel);
        uint8_t iReg = allocateRegister();
        emit(Opcode::LD_D, OperandsI{iReg, 30, static_cast<int16_t>(offset)});
        uint8_t endReg = visitExpression(forRange->getEnd());
        uint8_t condReg = allocateRegister();
        emit(Opcode::CMP, OperandsR{condReg, iReg, endReg, 2});
        emitBranch(Opcode::BEQ, condReg, 0, endLabel);
        freeRegister(iReg);
        freeRegister(endReg);
        freeRegister(condReg);
        controlFlowStack_.push({endLabel, stepLabel});
        visitStatement(forRange->getBody());
        controlFlowStack_.pop();
        bindLabel(stepLabel);
        iReg = allocateRegister();
        emit(Opcode::LD_D, OperandsI{iReg, 30, static_cast<int16_t>(offset)});
        uint8_t stepReg = forRange->getStep() ? visitExpression(*forRange->getStep()) : emitConstant(1);
        uint8_t nextIReg = allocateRegister();
        emit(Opcode::ARITH, OperandsR{nextIReg, iReg, stepReg, 0});
        emit(Opcode::ST_D, OperandsI{nextIReg, 30, static_cast<int16_t>(offset)});
        freeRegister(iReg);
        freeRegister(stepReg);
        freeRegister(nextIReg);
        emitJump(Opcode::JMP, 0, startLabel);
        bindLabel(endLabel);
    } else if (auto forIn = dynamic_cast<const ast::ForInStatement*>(&stmt)) {
        uint8_t iterReg = visitExpression(forIn->getIterable());
        
        // Lowered: Get length from header (offset 0)
        uint8_t lenReg = allocateRegister();
        emit(Opcode::LD_D, OperandsI{lenReg, iterReg, 0});
        
        uint8_t iReg = emitConstant(0);
        Label* startLabel = createLabel();
        Label* endLabel = createLabel();
        Label* stepLabel = createLabel();
        bindLabel(startLabel);
        uint8_t condReg = allocateRegister();
        emit(Opcode::CMP, OperandsR{condReg, iReg, lenReg, 2});
        emitBranch(Opcode::BEQ, condReg, 0, endLabel);
        freeRegister(condReg);
        
        // Lowered: item = iter[i] -> addr = iter + 8 + i * 8
        uint8_t shiftReg = emitConstant(3);
        uint8_t scaledIdx = allocateRegister();
        emit(Opcode::SHIFT, OperandsR{scaledIdx, iReg, shiftReg, 0}); // SHL
        freeRegister(shiftReg);

        uint8_t eightReg = emitConstant(8);
        uint8_t offsetReg = allocateRegister();
        emit(Opcode::ARITH, OperandsR{offsetReg, scaledIdx, eightReg, 0}); // ADD
        freeRegister(eightReg);
        freeRegister(scaledIdx);

        uint8_t finalAddr = allocateRegister();
        emit(Opcode::ARITH, OperandsR{finalAddr, iterReg, offsetReg, 0});
        freeRegister(offsetReg);
        
        uint8_t itemReg = allocateRegister();
        emit(Opcode::LD_D, OperandsI{itemReg, finalAddr, 0});
        freeRegister(finalAddr);
        
        int32_t itemOffset = reserveLocal(forIn->getVariable(), 100);
        emit(Opcode::ST_D, OperandsI{itemReg, 30, static_cast<int16_t>(itemOffset)});
        freeRegister(itemReg);
        controlFlowStack_.push({endLabel, stepLabel});
        visitStatement(forIn->getBody());
        controlFlowStack_.pop();
        bindLabel(stepLabel);
        uint8_t oneReg = emitConstant(1);
        uint8_t nextIReg = allocateRegister();
        emit(Opcode::ARITH, OperandsR{nextIReg, iReg, oneReg, 0});
        emit(Opcode::MOV, OperandsR{iReg, nextIReg, 0, 0});
        freeRegister(oneReg);
        freeRegister(nextIReg);
        emitJump(Opcode::JMP, 0, startLabel);
        bindLabel(endLabel);
        freeRegister(iterReg);
        freeRegister(lenReg);
        freeRegister(iReg);
    } else if (auto tryCatch = dynamic_cast<const ast::TryCatchStatement*>(&stmt)) {
        Label* catchStartLabel = createLabel();
        Label* endLabel = createLabel();
        
        // 1. Register handler: CALL hoo_push_handler(catchStartLabel)
        uint8_t handlerAddrReg = allocateRegister();
        emit(Opcode::LDA, OperandsI{handlerAddrReg, 0, 0}); 
        size_t ldaIdx = instructions_.size() - 1;
        uint32_t ldaOff = currentByteOffset_ - instructions_.back().getSize();
        
        emit(Opcode::MOV, OperandsR{1, handlerAddrReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_push_handler_v_p"); // Correct mangled name
        freeRegister(handlerAddrReg);

        visitStatement(tryCatch->getTryBlock());
        
        // 2. Unregister handler
        emitCall(Opcode::CALL, "_F_hoo_pop_handler_v");
        emitJump(Opcode::JMP, 0, endLabel);

        bindLabel(catchStartLabel);
        // Fixup LDA
        int32_t catchOffset = catchStartLabel->targetByteOffset - static_cast<int32_t>(ldaOff);
        auto ldaOps = instructions_[ldaIdx].getOperands();
        auto& ldaOpsI = std::get<OperandsI>(ldaOps);
        ldaOpsI.imm15 = static_cast<int16_t>(catchOffset);
        instructions_[ldaIdx].setOperands(ldaOpsI);

        for (const auto& clause : tryCatch->getCatchClauses()) {
            uint8_t excReg = allocateRegister();
            emit(Opcode::MOV, OperandsR{excReg, 1, 0, 0});
            int32_t itemOffset = reserveLocal(clause.variable, getTypeId(clause.type.get(), nullptr));
            emit(Opcode::ST_D, OperandsI{excReg, 30, static_cast<int16_t>(itemOffset)});
            freeRegister(excReg);
            visitStatement(*clause.block);
            emitJump(Opcode::JMP, 0, endLabel);
        }
        if (tryCatch->getFinallyBlock()) {
            visitStatement(*tryCatch->getFinallyBlock());
        }
        bindLabel(endLabel);
    } else if (auto throwStmt = dynamic_cast<const ast::ThrowStatement*>(&stmt)) {
        if (throwStmt->isRethrow()) {
            emitCall(Opcode::CALL, "_F_hoo_rethrow_v");
        } else {
            uint8_t excReg = visitExpression(*throwStmt->getExpression());
            emit(Opcode::MOV, OperandsR{1, excReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_throw_v_p");
            freeRegister(excReg);
        }
    }
}

uint8_t HVMCodeGenerator::visitExpression(const ast::Expression& expr) {
    if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(&expr)) {
        const auto& primary = primaryExpr->getPrimary();
        if (auto intLit = dynamic_cast<const ast::IntegerLiteral*>(&primary)) {
            return emitConstant(intLit->getValue());
        }
        if (auto id = dynamic_cast<const ast::Identifier*>(&primary)) {
            int32_t offset = getLocalOffset(id->getName());
            uint8_t reg = allocateRegister();
            emit(Opcode::LD_D, OperandsI{reg, 30, static_cast<int16_t>(offset)});
            return reg;
        }
        if (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(&primary)) {
            return visitExpression(paren->getExpression());
        }
        if (auto floatLit = dynamic_cast<const ast::FloatingLiteral*>(&primary)) {
            double val = floatLit->getValue();
            Section* rodata = module_->getSection(".rodata");
            if (!rodata) {
                Section s;
                s.name = ".rodata";
                s.type = SectionType::SHT_RODATA;
                s.flags = SectionFlags::ALLOC;
                module_->addSection(std::move(s));
                rodata = module_->getSection(".rodata");
            }
            // Align to 8 bytes
            while (rodata->data.size() % 8 != 0) rodata->data.push_back(0);
            
            uint32_t offset = static_cast<uint32_t>(rodata->data.size());
            uint64_t bits;
            std::memcpy(&bits, &val, sizeof(bits));
            for (int i = 0; i < 8; ++i) {
                rodata->data.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xFF));
            }
            rodata->virtual_size = rodata->data.size();
            
            uint8_t addrReg = emitRoDataAddress(offset);
            uint8_t dest = allocateRegister();
            emit(Opcode::LD_D, OperandsI{dest, addrReg, 0});
            freeRegister(addrReg);
            return dest;
        }
        if (auto charLit = dynamic_cast<const ast::CharacterLiteral*>(&primary)) {
            uint8_t cpReg = emitConstant(static_cast<int64_t>(charLit->getValue()));
            emit(Opcode::MOV, OperandsR{1, cpReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_Character_from_codepoint_p_i8");
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            freeRegister(cpReg);
            return dest;
        }
        if (auto boolLit = dynamic_cast<const ast::BooleanLiteral*>(&primary)) {
            return emitConstant(boolLit->getValue() ? 1 : 0);
        }
        if (auto nullLit = dynamic_cast<const ast::NullLiteral*>(&primary)) {
            return emitConstant(0);
        }
        if (auto thisLit = dynamic_cast<const ast::ThisLiteral*>(&primary)) {
            uint8_t reg = allocateRegister();
            emit(Opcode::MOV, OperandsR{reg, 1, 0, 0});
            return reg;
        }
        if (auto arrayLit = dynamic_cast<const ast::ArrayLiteral*>(&primary)) {
            auto& elements = arrayLit->getElements()->getExpressions();
            std::vector<uint8_t> elementRegs;
            for (const auto& elem : elements) {
                elementRegs.push_back(visitExpression(*elem));
            }
            
            // 1. Allocate: CALL hoo_alloc(size, typeId)
            uint64_t totalSize = 8 + (elements.size() * 8);
            uint8_t sizeReg = emitConstant(static_cast<int64_t>(totalSize));
            uint8_t typeReg = emitConstant(102); // Array Type ID
            emit(Opcode::MOV, OperandsR{1, sizeReg, 0, 0});
            emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_alloc_p_i8_i8");
            
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
            freeRegister(sizeReg);
            freeRegister(typeReg);

            // 2. Store length in header (offset 0)
            uint8_t lenReg = emitConstant(static_cast<int64_t>(elements.size()));
            emit(Opcode::ST_D, OperandsI{lenReg, dest, 0});
            freeRegister(lenReg);

            // 3. Store elements (offset 8+)
            for (size_t i = 0; i < elementRegs.size(); ++i) {
                emit(Opcode::ST_D, OperandsI{elementRegs[i], dest, static_cast<int16_t>(8 + (i * 8))});
                freeRegister(elementRegs[i]);
            }
            return dest;
        }
        if (auto strLit = dynamic_cast<const ast::StringLiteral*>(&primary)) {
            std::string val = strLit->getValue();
            Section* rodata = module_->getSection(".rodata");
            if (!rodata) {
                Section s;
                s.name = ".rodata";
                s.type = SectionType::SHT_RODATA;
                s.flags = SectionFlags::ALLOC;
                module_->addSection(std::move(s));
                rodata = module_->getSection(".rodata");
            }
            uint32_t offset = static_cast<uint32_t>(rodata->data.size());
            for (char c : val) rodata->data.push_back(c);
            rodata->data.push_back('\0');
            rodata->virtual_size = rodata->data.size();
            uint8_t addrReg = emitRoDataAddress(offset);

            // 3. Call runtime allocator
            emit(Opcode::MOV, OperandsR{1, addrReg, 0, 0});
            emitCall(Opcode::CALL, "_F_hoo_String_from_cstr_p_p");
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});

            freeRegister(addrReg);
            return dest;
        }
        if (auto interpStr = dynamic_cast<const ast::InterpolatedString*>(&primary)) {
            uint8_t resReg = 0;

            auto appendToRes = [&](uint8_t partReg) {
                if (resReg == 0) {
                    resReg = allocateRegister();
                    emit(Opcode::MOV, OperandsR{resReg, partReg, 0, 0});
                } else {
                    uint8_t nextRes = allocateRegister();
                    emit(Opcode::MOV, OperandsR{1, resReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, partReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_String_concat_p_p_p");
                    emit(Opcode::MOV, OperandsR{nextRes, 1, 0, 0});
                    
                    // Release old res and part
                    emit(Opcode::MOV, OperandsR{1, resReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                    emit(Opcode::MOV, OperandsR{1, partReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_release_v_p");
                    
                    freeRegister(resReg);
                    freeRegister(partReg);
                    resReg = nextRes;
                }
            };

            auto emitStringPart = [&](const std::string& text) {
                Section* rodata = module_->getSection(".rodata");
                if (!rodata) {
                    Section s;
                    s.name = ".rodata";
                    s.type = SectionType::SHT_RODATA;
                    s.flags = SectionFlags::ALLOC;
                    module_->addSection(std::move(s));
                    rodata = module_->getSection(".rodata");
                }
                uint32_t offset = static_cast<uint32_t>(rodata->data.size());
                for (char c : text) rodata->data.push_back(c);
                rodata->data.push_back('\0');
                rodata->virtual_size = rodata->data.size();
                uint8_t addrReg = emitRoDataAddress(offset);
                
                emit(Opcode::MOV, OperandsR{1, addrReg, 0, 0});
                emitCall(Opcode::CALL, "_F_hoo_String_from_cstr_p_p");
                uint8_t strReg = allocateRegister();
                emit(Opcode::MOV, OperandsR{strReg, 1, 0, 0});
                freeRegister(addrReg);
                return strReg;
            };

            for (const auto& part : interpStr->getParts()) {
                uint8_t partReg = 0;
                if (part.isExpression) {
                    uint8_t valReg = visitExpression(*part.expression);
                    
                    int64_t typeId = 100; // Default: Object
                    const ast::Expression* actualExpr = part.expression.get();
                    
                    // Unfold PrimaryExpression to find literals
                    const ast::ASTNode* targetNode = actualExpr;
                    if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(actualExpr)) {
                        targetNode = &pe->getPrimary();
                    }

                    if (dynamic_cast<const ast::IntegerLiteral*>(targetNode)) typeId = 1;
                    else if (dynamic_cast<const ast::FloatingLiteral*>(targetNode)) typeId = 2;
                    else if (dynamic_cast<const ast::BooleanLiteral*>(targetNode)) typeId = 3;
                    else if (dynamic_cast<const ast::StringLiteral*>(targetNode)) typeId = 101;
                    else if (dynamic_cast<const ast::CharacterLiteral*>(targetNode)) typeId = 109;
                    else if (auto id = dynamic_cast<const ast::Identifier*>(targetNode)) {
                        auto it = locals_.find(id->getName());
                        if (it != locals_.end()) {
                            typeId = it->second.typeId != 0 ? it->second.typeId : 100;
                        }
                    }

                    uint8_t typeIdReg = emitConstant(typeId);
                    emit(Opcode::MOV, OperandsR{1, valReg, 0, 0});
                    emit(Opcode::MOV, OperandsR{2, typeIdReg, 0, 0});
                    emitCall(Opcode::CALL, "_F_hoo_String_from_any_p_i8_i8");
                    
                    partReg = allocateRegister();
                    emit(Opcode::MOV, OperandsR{partReg, 1, 0, 0});
                    
                    freeRegister(valReg);
                    freeRegister(typeIdReg);
                } else {
                    partReg = emitStringPart(part.literal);
                }
                appendToRes(partReg);
            }

            if (resReg == 0) {
                emitCall(Opcode::CALL, "_F_hoo_String_new_p");
                resReg = allocateRegister();
                emit(Opcode::MOV, OperandsR{resReg, 1, 0, 0});
            }

            return resReg;
        }
    }

    if (auto newExpr = dynamic_cast<const ast::NewObjectExpression*>(&expr)) {
        std::string className = newExpr->getClassName();
        auto it = classes_.find(className);
        if (it == classes_.end()) {
            addError("Unknown class: " + className);
            return 0;
        }
        
        // 1. Allocate: CALL hoo_alloc(size, typeId)
        uint8_t sizeReg = emitConstant(static_cast<int64_t>(it->second.totalSize));
        uint8_t typeReg = emitConstant(100); // Generic Object typeId
        emit(Opcode::MOV, OperandsR{1, sizeReg, 0, 0});
        emit(Opcode::MOV, OperandsR{2, typeReg, 0, 0});
        emitCall(Opcode::CALL, "_F_hoo_alloc_p_i8_i8");
        
        uint8_t dest = allocateRegister();
        emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
        freeRegister(sizeReg);
        freeRegister(typeReg);
        
        // 2. Call constructor
        MangledFunctionParams mp;
        mp.className = className;
        mp.isConstructor = true;

        for (const auto& param : newExpr->getArguments()->getArguments()) {
            // Parameter types logic for constructor call needed here
            mp.parameterTypes.push_back("object"); // Fallback
        }
        std::string ctorName = SymbolMangler::mangleFunctionName(mp);

        // Set 'this' in r1
        emit(Opcode::MOV, OperandsR{1, dest, 0, 0});
        
        if (newExpr->getArguments()) {
            auto& args = newExpr->getArguments()->getArguments();
            for (size_t i = 0; i < args.size() && i < 7; ++i) {
                uint8_t argReg = visitExpression(*args[i]);
                emit(Opcode::MOV, OperandsR{static_cast<uint8_t>(i + 2), argReg, 0, 0});
                freeRegister(argReg);
            }
        }

        emitCall(Opcode::CALL, ctorName); 

        return dest;
    }

    if (auto memberAccess = dynamic_cast<const ast::MemberAccess*>(&expr)) {
        uint8_t objReg = visitExpression(memberAccess->getObject());
        int32_t offset = 0; 
        bool found = false;
        for (const auto& [className, layout] : classes_) {
            auto it = layout.fieldOffsets.find(memberAccess->getMember());
            if (it != layout.fieldOffsets.end()) {
                offset = it->second;
                found = true;
                break;
            }
        }
        if (!found) {
            addError("Undefined member: " + memberAccess->getMember());
            return objReg;
        }
        uint8_t dest = allocateRegister();
        emit(Opcode::LD_D, OperandsI{dest, objReg, static_cast<int16_t>(offset)});
        freeRegister(objReg);
        return dest;
    }

    if (auto funcCall = dynamic_cast<const ast::FunctionCall*>(&expr)) {
        const ast::Expression* targetPtr = &funcCall->getFunction();
        
        // Unwrap PrimaryExpression/ParenthesizedExpression if needed
        while (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(targetPtr)) {
            if (auto paren = dynamic_cast<const ast::ParenthesizedExpression*>(&primaryExpr->getPrimary())) {
                targetPtr = &paren->getExpression();
            } else {
                break;
            }
        }

        if (auto memberAccess = dynamic_cast<const ast::MemberAccess*>(targetPtr)) {
            uint8_t objReg = visitExpression(memberAccess->getObject());
            emit(Opcode::MOV, OperandsR{1, objReg, 0, 0});
            freeRegister(objReg);

            if (funcCall->getArguments()) {
                auto& args = funcCall->getArguments()->getArguments();
                for (size_t i = 0; i < args.size() && i < 7; ++i) {
                    uint8_t argReg = visitExpression(*args[i]);
                    emit(Opcode::MOV, OperandsR{static_cast<uint8_t>(i + 2), argReg, 0, 0});
                    freeRegister(argReg);
                }
            }

            // Logic for method mangling resolution needed here
            emitCall(Opcode::CALL, memberAccess->getMember()); 
            
            uint8_t dest = allocateRegister();
            emit(Opcode::MOV, OperandsR{dest, 1, 0, 0}); 
            return dest;
        } else {
            const ast::Identifier* id = nullptr;
            if (auto idTarget = dynamic_cast<const ast::Identifier*>(targetPtr)) {
                id = idTarget;
            } else if (auto primaryExpr = dynamic_cast<const ast::PrimaryExpression*>(targetPtr)) {
                id = dynamic_cast<const ast::Identifier*>(&primaryExpr->getPrimary());
            }

            if (id) {
                std::string functionName = id->getName();
                
                // Plain function mangling logic
                MangledFunctionParams mp;
                mp.functionName = functionName;

                // Redirect built-ins to hoo namespace
                if (functionName == "print" || functionName == "println" ||
                    functionName == "readline" || functionName == "readchar") {
                    mp.modulePath = {"hoo"};
                }

                if (funcCall->getArguments()) {
                    auto& args = funcCall->getArguments()->getArguments();
                    for (size_t i = 0; i < args.size() && i < 8; ++i) {
                        uint8_t argReg = visitExpression(*args[i]);
                        emit(Opcode::MOV, OperandsR{static_cast<uint8_t>(i + 1), argReg, 0, 0});
                        freeRegister(argReg);
                    }
                }
                
                // Try to infer return and parameter types for better mangling
                mp.returnType = "void"; 
                if (funcCall->getArguments()) {
                    for (const auto& arg : funcCall->getArguments()->getArguments()) {
                         mp.parameterTypes.push_back("ptr"); // Use ptr (p) to match runtime void*
                    }
                }
                
                std::string mangledName = SymbolMangler::mangleFunctionName(mp);
                emitCall(Opcode::CALL, mangledName); 
                
                uint8_t dest = allocateRegister();
                emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
                return dest;
            }
        }
        addError("Unsupported function call target: " + std::string(typeid(*targetPtr).name()));
        return 0;
    }

    if (auto binary = dynamic_cast<const ast::BinaryExpression*>(&expr)) {
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
            case ast::BinaryOperator::MODULO: func = 7; break;
            case ast::BinaryOperator::EQUALS: op = Opcode::CMP; func = 0; break;
            case ast::BinaryOperator::NOT_EQUALS: op = Opcode::CMP; func = 1; break;
            case ast::BinaryOperator::LESS: op = Opcode::CMP; func = 2; break;
            case ast::BinaryOperator::LESS_EQUALS: op = Opcode::CMP; func = 3; break;
            case ast::BinaryOperator::GREATER: {
                op = Opcode::CMP; func = 2;
                std::swap(left, right);
                break;
            }
            case ast::BinaryOperator::GREATER_EQUALS: {
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
    }

    if (auto logicAnd = dynamic_cast<const ast::LogicalAnd*>(&expr)) {
        uint8_t left = visitExpression(logicAnd->getLeft());
        uint8_t right = visitExpression(logicAnd->getRight());
        uint8_t dest = allocateRegister();
        emit(Opcode::LOGIC, OperandsR{dest, left, right, 0});
        freeRegister(left);
        freeRegister(right);
        return dest;
    }
    if (auto logicOr = dynamic_cast<const ast::LogicalOr*>(&expr)) {
        uint8_t left = visitExpression(logicOr->getLeft());
        uint8_t right = visitExpression(logicOr->getRight());
        uint8_t dest = allocateRegister();
        emit(Opcode::LOGIC, OperandsR{dest, left, right, 1});
        freeRegister(left);
        freeRegister(right);
        return dest;
    }

    if (auto logicalNot = dynamic_cast<const ast::LogicalNot*>(&expr)) {
        uint8_t src = visitExpression(logicalNot->getOperand());
        uint8_t dest = allocateRegister();
        emit(Opcode::CMP, OperandsR{dest, src, 0, 0});
        freeRegister(src);
        return dest;
    }

    if (auto unaryMinus = dynamic_cast<const ast::UnaryMinus*>(&expr)) {
        uint8_t src = visitExpression(unaryMinus->getOperand());
        uint8_t dest = allocateRegister();
        emit(Opcode::ARITH, OperandsR{dest, 0, src, 1});
        freeRegister(src);
        return dest;
    }

    if (auto compoundAssign = dynamic_cast<const ast::CompoundAssignmentExpression*>(&expr)) {
        uint8_t rhsReg = visitExpression(compoundAssign->getRight());
        uint8_t lhsReg = 0;
        int32_t offset = 0;
        uint8_t objReg = 0;
        bool isMember = false;

        if (auto leftPrimary = dynamic_cast<const ast::PrimaryExpression*>(&compoundAssign->getLeft())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&leftPrimary->getPrimary())) {
                offset = getLocalOffset(id->getName());
                lhsReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{lhsReg, 30, static_cast<int16_t>(offset)});
            }
        } else if (auto leftMember = dynamic_cast<const ast::MemberAccess*>(&compoundAssign->getLeft())) {
            objReg = visitExpression(leftMember->getObject());
            isMember = true;
            bool found = false;
            for (const auto& [className, layout] : classes_) {
                auto it = layout.fieldOffsets.find(leftMember->getMember());
                if (it != layout.fieldOffsets.end()) {
                    offset = it->second;
                    found = true;
                    break;
                }
            }
            if (found) {
                lhsReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{lhsReg, objReg, static_cast<int16_t>(offset)});
            } else {
                addError("Undefined member: " + leftMember->getMember());
            }
        }

        if (lhsReg != 0) {
            uint8_t resultReg = allocateRegister();
            uint16_t func = 0;
            Opcode op = Opcode::ARITH;
            switch (compoundAssign->getOperator()) {
                case ast::CompoundAssignmentOperator::PLUS_ASSIGN: func = 0; break;
                case ast::CompoundAssignmentOperator::MINUS_ASSIGN: func = 1; break;
                case ast::CompoundAssignmentOperator::MULTIPLY_ASSIGN: func = 2; break;
                case ast::CompoundAssignmentOperator::DIVIDE_ASSIGN: func = 5; break;
                case ast::CompoundAssignmentOperator::MODULO_ASSIGN: func = 7; break;
                case ast::CompoundAssignmentOperator::LEFT_SHIFT_ASSIGN: op = Opcode::SHIFT; func = 0; break;
                case ast::CompoundAssignmentOperator::RIGHT_SHIFT_ASSIGN: op = Opcode::SHIFT; func = 1; break;
                default: addError("Unsupported compound assignment");
            }
            emit(op, OperandsR{resultReg, lhsReg, rhsReg, func});
            
            if (isMember) {
                emit(Opcode::ST_D, OperandsI{resultReg, objReg, static_cast<int16_t>(offset)});
                freeRegister(objReg);
            } else {
                emit(Opcode::ST_D, OperandsI{resultReg, 30, static_cast<int16_t>(offset)});
            }
            
            freeRegister(lhsReg);
            freeRegister(rhsReg);
            return resultReg;
        }
        freeRegister(rhsReg);
        return 0;
    }

    if (auto incDec = dynamic_cast<const ast::IncrementDecrementExpression*>(&expr)) {
        uint8_t lhsReg = 0;
        int32_t offset = 0;
        uint8_t objReg = 0;
        bool isMember = false;

        if (auto leftPrimary = dynamic_cast<const ast::PrimaryExpression*>(&incDec->getOperand())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&leftPrimary->getPrimary())) {
                offset = getLocalOffset(id->getName());
                lhsReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{lhsReg, 30, static_cast<int16_t>(offset)});
            }
        } else if (auto leftMember = dynamic_cast<const ast::MemberAccess*>(&incDec->getOperand())) {
            objReg = visitExpression(leftMember->getObject());
            isMember = true;
            bool found = false;
            for (const auto& [className, layout] : classes_) {
                auto it = layout.fieldOffsets.find(leftMember->getMember());
                if (it != layout.fieldOffsets.end()) {
                    offset = it->second;
                    found = true;
                    break;
                }
            }
            if (found) {
                lhsReg = allocateRegister();
                emit(Opcode::LD_D, OperandsI{lhsReg, objReg, static_cast<int16_t>(offset)});
            } else {
                addError("Undefined member: " + leftMember->getMember());
            }
        }

        if (lhsReg != 0) {
            uint8_t oneReg = emitConstant(1);
            uint8_t resultReg = allocateRegister();
            uint16_t func = (incDec->getOperator() == ast::IncrementDecrementOperator::INCREMENT) ? 0 : 1;
            emit(Opcode::ARITH, OperandsR{resultReg, lhsReg, oneReg, func});
            
            if (isMember) {
                emit(Opcode::ST_D, OperandsI{resultReg, objReg, static_cast<int16_t>(offset)});
                freeRegister(objReg);
            } else {
                emit(Opcode::ST_D, OperandsI{resultReg, 30, static_cast<int16_t>(offset)});
            }
            
            freeRegister(oneReg);
            if (incDec->isPrefix()) {
                freeRegister(lhsReg);
                return resultReg;
            } else {
                freeRegister(resultReg);
                return lhsReg;
            }
        }
        return 0;
    }

    if (auto assign = dynamic_cast<const ast::AssignmentExpression*>(&expr)) {
        uint8_t valueReg = visitExpression(assign->getRight());
        if (auto leftPrimary = dynamic_cast<const ast::PrimaryExpression*>(&assign->getLeft())) {
            if (auto id = dynamic_cast<const ast::Identifier*>(&leftPrimary->getPrimary())) {
                int32_t offset = getLocalOffset(id->getName());
                emit(Opcode::ST_D, OperandsI{valueReg, 30, static_cast<int16_t>(offset)});
                return valueReg;
            }
        } else if (auto leftMember = dynamic_cast<const ast::MemberAccess*>(&assign->getLeft())) {
            uint8_t objReg = visitExpression(leftMember->getObject());
            int32_t offset = 0;
            bool found = false;
            for (const auto& [className, layout] : classes_) {
                auto it = layout.fieldOffsets.find(leftMember->getMember());
                if (it != layout.fieldOffsets.end()) {
                    offset = it->second;
                    found = true;
                    break;
                }
            }
            if (found) {
                emit(Opcode::ST_D, OperandsI{valueReg, objReg, static_cast<int16_t>(offset)});
            } else {
                addError("Undefined member: " + leftMember->getMember());
            }
            freeRegister(objReg);
            return valueReg;
        }
        addError("Unsupported assignment target");
        return valueReg;
    }

    if (auto arrayAccess = dynamic_cast<const ast::ArrayAccess*>(&expr)) {
        uint8_t arrReg = visitExpression(arrayAccess->getArray());
        uint8_t idxReg = visitExpression(arrayAccess->getIndex());
        
        uint8_t shiftReg = emitConstant(3); 
        uint8_t scaledIdx = allocateRegister();
        emit(Opcode::SHIFT, OperandsR{scaledIdx, idxReg, shiftReg, 0}); 
        freeRegister(shiftReg);
        freeRegister(idxReg);

        uint8_t eightReg = emitConstant(8);
        uint8_t offsetReg = allocateRegister();
        emit(Opcode::ARITH, OperandsR{offsetReg, scaledIdx, eightReg, 0}); 
        freeRegister(eightReg);
        freeRegister(scaledIdx);
        
        uint8_t finalAddr = allocateRegister();
        emit(Opcode::ARITH, OperandsR{finalAddr, arrReg, offsetReg, 0});
        freeRegister(arrReg);
        freeRegister(offsetReg);

        uint8_t dest = allocateRegister();
        emit(Opcode::LD_D, OperandsI{dest, finalAddr, 0});
        freeRegister(finalAddr);
        
        return dest;
    }

    addError("Unsupported expression type: " + std::string(typeid(expr).name()));
    return 0;
}

void HVMCodeGenerator::addError(const std::string& message) {
    errors_.push_back(message);
}

uint8_t HVMCodeGenerator::allocateRegister() {
    for (uint8_t i = 9; i <= 20; ++i) {
        if (!usedRegs_[i]) {
            usedRegs_[i] = true;
            return i;
        }
    }
    addError("Register pressure: out of temporary registers");
    return 0;
}

void HVMCodeGenerator::freeRegister(uint8_t reg) {
    if (reg >= 9 && reg <= 20) usedRegs_[reg] = false;
}


int32_t HVMCodeGenerator::reserveLocal(const std::string& name, uint32_t typeId) {
    currentStackOffset_ -= 8;
    locals_[name] = {currentStackOffset_, typeId};
    return currentStackOffset_;
}

int32_t HVMCodeGenerator::getLocalOffset(const std::string& name) {
    auto it = locals_.find(name);
    if (it != locals_.end()) return it->second.offset;
    addError("Undefined variable: " + name);
    return 0;
}

void HVMCodeGenerator::emit(Opcode op, const Operands& operands) {
    HVMInstruction inst(op, operands);
    instructions_.push_back(inst);
    currentByteOffset_ += inst.getSize();
}

uint8_t HVMCodeGenerator::emitConstant(int64_t value) {
    uint8_t reg = allocateRegister();
    
    if (value >= 0 && value <= 32767) {
        emit(Opcode::MOVZ, OperandsI{reg, 0, static_cast<int16_t>(value)});
    } else if (value >= -16384 && value <= 16383) {
        emit(Opcode::ADDI, OperandsI{reg, 0, static_cast<int16_t>(value)});
    } else {
        Section* rodata = module_->getSection(".rodata");
        if (!rodata) {
            Section s;
            s.name = ".rodata";
            s.type = SectionType::SHT_RODATA;
            s.flags = SectionFlags::ALLOC;
            module_->addSection(std::move(s));
            rodata = module_->getSection(".rodata");
        }
        
        uint32_t offset = static_cast<uint32_t>(rodata->data.size());
        for (int i = 0; i < 8; ++i) {
            rodata->data.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
        }
        rodata->virtual_size = rodata->data.size();
        
        uint8_t addrReg = emitRoDataAddress(offset);
        emit(Opcode::LD_D, OperandsI{reg, addrReg, 0});
        freeRegister(addrReg);
    }
    return reg;
}

uint8_t HVMCodeGenerator::emitRoDataAddress(uint32_t offset) {
    // LDA with rs=r0 is interpreted as .rodata-base addressing by the HVM runtime.
    // For large offsets, build the full address with bounded ADDI steps.
    uint8_t addrReg = allocateRegister();
    const int64_t kLdaMax = 16383;
    const int64_t kAddiMax = 16383;

    int64_t remaining = static_cast<int64_t>(offset);
    int16_t baseImm = static_cast<int16_t>(std::min<int64_t>(remaining, kLdaMax));
    emit(Opcode::LDA, OperandsI{addrReg, 0, baseImm});
    remaining -= baseImm;

    while (remaining > 0) {
        int16_t step = static_cast<int16_t>(std::min<int64_t>(remaining, kAddiMax));
        emit(Opcode::ADDI, OperandsI{addrReg, addrReg, step});
        remaining -= step;
    }

    return addrReg;
}

HVMCodeGenerator::Label* HVMCodeGenerator::createLabel() {
    allLabels_.push_back(std::make_unique<Label>());
    return allLabels_.back().get();
}

void HVMCodeGenerator::bindLabel(Label* label) {
    label->targetByteOffset = static_cast<int32_t>(currentByteOffset_);
    for (const auto& fixup : label->fixups) {
        auto& inst = instructions_[fixup.instructionIndex];
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

void HVMCodeGenerator::emitCall(Opcode op, const std::string& symbol) {
    size_t instrIdx = instructions_.size();
    uint32_t instrOff = currentByteOffset_;
    
    auto* sym = module_->getSymbol(symbol);
    
    // If it's an external symbol (not found or already marked as undefined)
    // we add a new symbol record for this specific call site so the JIT can resolve it.
    if (!sym || sym->section_index == -1) {
        Symbol undefSym;
        undefSym.name = symbol;
        undefSym.type = Symbol::STT_FUNC;
        undefSym.binding = Symbol::STB_GLOBAL;
        undefSym.section_index = -1; // Undefined
        undefSym.value = instrOff;   // Target this specific instruction for JIT resolution
        module_->addSymbol(undefSym);
        sym = &module_->getSymbols().back();
    }

    int32_t wordOffset = 0;
    if (sym && sym->section_index != -1) {
        wordOffset = (static_cast<int32_t>(sym->value) - static_cast<int32_t>(instrOff)) / 4;
    } else {
        symbolFixups_.push_back({symbol, instrIdx, instrOff});
    }

    emit(op, OperandsJ{29, wordOffset}); 
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


uint32_t HVMCodeGenerator::getTypeId(const ast::Type* type, const ast::Expression* initializer) {
    if (!type) {
        if (initializer) {
            // Basic inference from literal
            if (auto pe = dynamic_cast<const ast::PrimaryExpression*>(initializer)) {
                const ast::ASTNode& node = pe->getPrimary();
                if (dynamic_cast<const ast::IntegerLiteral*>(&node)) return 1;
                if (dynamic_cast<const ast::FloatingLiteral*>(&node)) return 2;
                if (dynamic_cast<const ast::BooleanLiteral*>(&node)) return 3;
                if (dynamic_cast<const ast::StringLiteral*>(&node)) return 101;
                if (dynamic_cast<const ast::CharacterLiteral*>(&node)) return 109;
            }
        }
        return 100; // Default to Object
    }
    
    if (auto bt = dynamic_cast<const ast::BaseType*>(type)) {
        if (bt->isPrimitive()) {
            switch (bt->getPrimitiveType()->getKind()) {
                case ast::PrimitiveTypeKind::INT64: return 1;
                case ast::PrimitiveTypeKind::FLOAT:
                case ast::PrimitiveTypeKind::DOUBLE:
                case ast::PrimitiveTypeKind::F64:   return 2;
                case ast::PrimitiveTypeKind::BOOL:    return 3;
                case ast::PrimitiveTypeKind::VOID:    return 4;
                case ast::PrimitiveTypeKind::INT8:    return 5;
                case ast::PrimitiveTypeKind::BYTE:    return 6;
                case ast::PrimitiveTypeKind::CHAR:    return 7;
                case ast::PrimitiveTypeKind::STRING:  return 101;
                default: return 1;
            }
        } else {
            std::string name = bt->getIdentifier();
            if (name == "String") return 101;
            if (name == "Character") return 109;
            return 100;
        }
    }
    if (dynamic_cast<const ast::ArrayType*>(type)) return 102;
    if (dynamic_cast<const ast::MapType*>(type)) return 103;
    if (dynamic_cast<const ast::OptionalType*>(type)) return 100;
    
    return 100;
}
} // namespace hooc
