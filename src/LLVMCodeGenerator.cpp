#include "LLVMCodeGenerator.h"
#include "ast/AST.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <stdexcept>

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

// Avoid namespace conflicts with LLVM
namespace {
    using LLVMType = llvm::Type;
    using ASTType = hooc::ast::Type;
    using ASTBinaryOperator = hooc::ast::BinaryOperator;
    using ASTStringLiteral = hooc::ast::StringLiteral;
    using ASTArrayType = hooc::ast::ArrayType;
}

LLVMCodeGenerator::LLVMCodeGenerator(LLVMContext& context)
    : context_(context) {
    builder_ = std::make_unique<IRBuilder<>>(context_);
}

LLVMCodeGenerator::~LLVMCodeGenerator() {}

// Abstract interface implementation - returns wrapped types
std::unique_ptr<GeneratedModule> LLVMCodeGenerator::generateModule(const CompilationUnit& compilationUnit) {
    auto llvmModule = generateLLVMModule(compilationUnit);
    if (!llvmModule) {
        return nullptr;
    }
    return std::make_unique<LLVMGeneratedModule>(std::move(llvmModule));
}

GeneratedFunction* LLVMCodeGenerator::generateFunction(const ast::FunctionDeclaration& funcDecl) {
    auto* llvmFunc = generateLLVMFunction(funcDecl);
    if (!llvmFunc) {
        return nullptr;
    }
    return new LLVMGeneratedFunction(llvmFunc);
}

GeneratedValue* LLVMCodeGenerator::generateExpression(const ast::Expression& expr) {
    auto* llvmValue = generateLLVMExpression(expr);
    if (!llvmValue) {
        return nullptr;
    }
    return new LLVMGeneratedValue(llvmValue);
}

void LLVMCodeGenerator::generateStatement(const ast::Statement& stmt) {
    generateLLVMStatement(stmt);
}

GeneratedType* LLVMCodeGenerator::generateType(const ast::Type& type) {
    auto* llvmType = generateLLVMType(type);
    if (!llvmType) {
        return nullptr;
    }
    return new LLVMGeneratedType(llvmType);
}

// LLVM-specific implementation
std::unique_ptr<Module> LLVMCodeGenerator::generateLLVMModule(const CompilationUnit& compilationUnit) {
    // Create a new module for this compilation unit
    module_ = std::make_unique<Module>("hooc_module", context_);
    
    // Clear symbol tables for new module
    namedValues_.clear();
    functions_.clear();
    
    // Process all declarations in the compilation unit
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(decl.get())) {
            generateLLVMFunction(*funcDecl);
        } else if (auto varDecl = dynamic_cast<const VariableDeclaration*>(decl.get())) {
            generateVariableDeclaration(*varDecl);
        }
        // TODO: Add support for class declarations, interface declarations, etc.
    }
    
    // Verify the module
    std::string errorStr;
    raw_string_ostream errorStream(errorStr);
    if (verifyModule(*module_, &errorStream)) {
        std::cerr << "Module verification failed: " << errorStr << std::endl;
        return nullptr;
    }
    
    return std::move(module_);
}

Function* LLVMCodeGenerator::generateLLVMFunction(const FunctionDeclaration& funcDecl) {
    // Build parameter types
    std::vector<LLVMType*> paramTypes;
    for (const auto& param : funcDecl.getParameters()) {
        LLVMType* paramType = generateLLVMType(param->getType());
        paramTypes.push_back(paramType);
    }
    
    // Determine return type
    LLVMType* returnType = LLVMType::getVoidTy(context_); // Default to void
    if (funcDecl.getReturnType()) {
        returnType = generateLLVMType(*funcDecl.getReturnType());
    }
    
    // Create function type
    FunctionType* functionType = FunctionType::get(returnType, paramTypes, false);
    
    // Create the function
    Function* function = Function::Create(
        functionType, 
        Function::ExternalLinkage, 
        funcDecl.getName(), 
        module_.get()
    );
    
    // Create entry basic block
    BasicBlock* entryBlock = BasicBlock::Create(context_, "entry", function);
    builder_->SetInsertPoint(entryBlock);

    // Set parameter names, create allocas, and store parameter values
    auto paramIter = funcDecl.getParameters().begin();
    for (auto& arg : function->args()) {
        if (paramIter != funcDecl.getParameters().end()) {
            const std::string& paramName = (*paramIter)->getName();
            arg.setName(paramName);

            // Create alloca for the parameter
            AllocaInst* alloca = createEntryBlockAlloca(function, paramName, arg.getType());

            // Store the initial value
            builder_->CreateStore(&arg, alloca);

            // Add to symbol table
            namedValues_[paramName] = alloca;

            ++paramIter;
        }
    }
    
    // Generate function body
    generateBlock(funcDecl.getBody());

    // Ensure all basic blocks have terminators
    // Check the current block (not just entry block) in case we're in a merge block from an if-statement
    BasicBlock* currentBlock = builder_->GetInsertBlock();
    if (currentBlock && !currentBlock->getTerminator()) {
        if (returnType->isVoidTy()) {
            // For void functions, add return void
            builder_->CreateRetVoid();
        } else {
            // For non-void functions, if we reach here without a return statement,
            // add unreachable (this handles unreachable merge blocks)
            builder_->CreateUnreachable();
        }
    }
    
    // Verify function
    std::string errorStr;
    raw_string_ostream errorStream(errorStr);
    if (verifyFunction(*function, &errorStream)) {
        std::cerr << "Function verification failed: " << errorStr << std::endl;
        function->eraseFromParent();
        return nullptr;
    }
    
    // Add to function table
    functions_[funcDecl.getName()] = function;
    
    return function;
}

void LLVMCodeGenerator::generateBlock(const Block& block) {
    for (const auto& stmt : block.getStatements()) {
        generateLLVMStatement(*stmt);
    }
}

void LLVMCodeGenerator::generateLLVMStatement(const Statement& stmt) {
    if (auto retStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        generateReturnStatement(*retStmt);
    } else if (auto exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        generateExpressionStatement(*exprStmt);
    } else if (auto blockStmt = dynamic_cast<const Block*>(&stmt)) {
        generateBlock(*blockStmt);
    } else if (auto ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        generateIfStatement(*ifStmt);
    } else if (auto whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        generateWhileStatement(*whileStmt);
    } else if (auto forInStmt = dynamic_cast<const ForInStatement*>(&stmt)) {
        generateForInStatement(*forInStmt);
    } else if (auto forRangeStmt = dynamic_cast<const ForRangeStatement*>(&stmt)) {
        generateForRangeStatement(*forRangeStmt);
    } else if (auto varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(&stmt)) {
        generateVariableDeclarationStatement(*varDeclStmt);
    } else {
        std::cerr << "Unsupported statement type" << std::endl;
    }
}

void LLVMCodeGenerator::generateReturnStatement(const ReturnStatement& ret) {
    if (ret.hasExpression()) {
        Value* retValue = generateLLVMExpression(*ret.getExpression());
        builder_->CreateRet(retValue);
    } else {
        builder_->CreateRetVoid();
    }
}

void LLVMCodeGenerator::generateExpressionStatement(const ExpressionStatement& stmt) {
    generateLLVMExpression(stmt.getExpression());
}

Value* LLVMCodeGenerator::generateLLVMExpression(const Expression& expr) {
    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&expr)) {
        return generatePrimaryExpression(*primaryExpr);
    } else if (auto binaryExpr = dynamic_cast<const BinaryExpression*>(&expr)) {
        return generateBinaryExpression(*binaryExpr);
    } else if (auto funcCall = dynamic_cast<const FunctionCall*>(&expr)) {
        return generateFunctionCall(*funcCall);
    } else if (auto unaryMinus = dynamic_cast<const UnaryMinus*>(&expr)) {
        return generateUnaryExpression(*unaryMinus);
    } else if (auto logicalNot = dynamic_cast<const LogicalNot*>(&expr)) {
        return generateLogicalNot(*logicalNot);
    } else if (auto logicalAnd = dynamic_cast<const LogicalAnd*>(&expr)) {
        return generateLogicalAnd(*logicalAnd);
    } else if (auto logicalOr = dynamic_cast<const LogicalOr*>(&expr)) {
        return generateLogicalOr(*logicalOr);
    } else if (auto assignment = dynamic_cast<const AssignmentExpression*>(&expr)) {
        return generateAssignment(*assignment);
    } else if (auto memberAccess = dynamic_cast<const MemberAccess*>(&expr)) {
        return generateMemberAccess(*memberAccess);
    } else if (auto arrayAccess = dynamic_cast<const ArrayAccess*>(&expr)) {
        return generateArrayAccess(*arrayAccess);
    } else if (auto arrayLit = dynamic_cast<const ArrayLiteral*>(&expr)) {
        return generateArrayLiteral(*arrayLit);
    }

    std::cerr << "Unsupported expression type in generateExpression" << std::endl;
    return nullptr;
}

Value* LLVMCodeGenerator::generatePrimaryExpression(const PrimaryExpression& expr) {
    const ASTNode& primary = expr.getPrimary();
    
    if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
        // Look up variable
        auto it = namedValues_.find(identifier->getName());
        if (it != namedValues_.end()) {
            // Load the value from the alloca
            AllocaInst* alloca = llvm::cast<AllocaInst>(it->second);
            return builder_->CreateLoad(alloca->getAllocatedType(), alloca, identifier->getName());
        }

        std::cerr << "Unknown variable: " << identifier->getName() << std::endl;
        return nullptr;
        
    } else if (auto intLiteral = dynamic_cast<const IntegerLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt64Ty(context_), intLiteral->getValue());
        
    } else if (auto floatLiteral = dynamic_cast<const FloatingLiteral*>(&primary)) {
        return ConstantFP::get(LLVMType::getDoubleTy(context_), floatLiteral->getValue());
        
    } else if (auto boolLiteral = dynamic_cast<const BooleanLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt1Ty(context_), boolLiteral->getValue() ? 1 : 0);
        
    } else if (auto stringLiteral = dynamic_cast<const ASTStringLiteral*>(&primary)) {
        // Create global string constant
        return builder_->CreateGlobalString(stringLiteral->getValue());

    } else if (auto charLiteral = dynamic_cast<const CharacterLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt32Ty(context_), static_cast<uint32_t>(charLiteral->getValue()));
    }

    std::cerr << "Unsupported primary expression type" << std::endl;
    return nullptr;
}

Value* LLVMCodeGenerator::generateBinaryExpression(const BinaryExpression& expr) {
    Value* left = generateLLVMExpression(expr.getLeft());
    Value* right = generateLLVMExpression(expr.getRight());
    
    if (!left || !right) {
        return nullptr;
    }
    
    switch (expr.getOperator()) {
        case ASTBinaryOperator::PLUS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateAdd(left, right, "addtmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFAdd(left, right, "addtmp");
            }
            break;
            
        case ASTBinaryOperator::MINUS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateSub(left, right, "subtmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFSub(left, right, "subtmp");
            }
            break;
            
        case ASTBinaryOperator::MULTIPLY:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateMul(left, right, "multmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFMul(left, right, "multmp");
            }
            break;
            
        case ASTBinaryOperator::DIVIDE:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateSDiv(left, right, "divtmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFDiv(left, right, "divtmp");
            }
            break;

        case ASTBinaryOperator::MODULO:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateSRem(left, right, "modtmp");
            }
            break;

        case ASTBinaryOperator::LESS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSLT(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOLT(left, right, "cmptmp");
            }
            break;
            
        case ASTBinaryOperator::LESS_EQUALS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSLE(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOLE(left, right, "cmptmp");
            }
            break;

        case ASTBinaryOperator::GREATER:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSGT(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOGT(left, right, "cmptmp");
            }
            break;

        case ASTBinaryOperator::GREATER_EQUALS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSGE(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOGE(left, right, "cmptmp");
            }
            break;

        case ASTBinaryOperator::EQUALS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpEQ(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOEQ(left, right, "cmptmp");
            }
            break;

        case ASTBinaryOperator::NOT_EQUALS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpNE(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpONE(left, right, "cmptmp");
            }
            break;

        default:
            std::cerr << "Unsupported binary operator" << std::endl;
            return nullptr;
    }
    
    std::cerr << "Type mismatch in binary expression" << std::endl;
    return nullptr;
}

Value* LLVMCodeGenerator::generateFunctionCall(const FunctionCall& call) {
    // Get the function being called
    const Expression& funcExpr = call.getFunction();

    // For now, only support direct function calls via identifier
    std::string functionName;
    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&funcExpr)) {
        const ASTNode& primary = primaryExpr->getPrimary();
        if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
            functionName = identifier->getName();
        } else {
            std::cerr << "Function call must use identifier" << std::endl;
            return nullptr;
        }
    } else {
        std::cerr << "Complex function expressions not yet supported" << std::endl;
        return nullptr;
    }

    // Look up the function
    Function* calleeFunc = module_->getFunction(functionName);
    if (!calleeFunc) {
        std::cerr << "Unknown function: " << functionName << std::endl;
        return nullptr;
    }

    // Generate argument values
    std::vector<Value*> args;
    if (call.getArguments()) {
        for (const auto& argExpr : call.getArguments()->getArguments()) {
            Value* argValue = generateLLVMExpression(*argExpr);
            if (!argValue) {
                return nullptr;
            }
            args.push_back(argValue);
        }
    }

    // Check argument count
    if (args.size() != calleeFunc->arg_size()) {
        std::cerr << "Incorrect number of arguments for function " << functionName << std::endl;
        return nullptr;
    }

    return builder_->CreateCall(calleeFunc, args, "calltmp");
}

Value* LLVMCodeGenerator::generateUnaryExpression(const UnaryMinus& expr) {
    Value* operand = generateLLVMExpression(expr.getOperand());
    if (!operand) return nullptr;

    if (operand->getType()->isIntegerTy()) {
        return builder_->CreateNeg(operand, "negtmp");
    } else if (operand->getType()->isFloatingPointTy()) {
        return builder_->CreateFNeg(operand, "negtmp");
    }

    std::cerr << "Unsupported type for unary minus" << std::endl;
    return nullptr;
}

Value* LLVMCodeGenerator::generateLogicalNot(const LogicalNot& expr) {
    Value* operand = generateLLVMExpression(expr.getOperand());
    if (!operand) return nullptr;

    // Convert to i1 if needed
    if (!operand->getType()->isIntegerTy(1)) {
        if (operand->getType()->isIntegerTy()) {
            operand = builder_->CreateICmpNE(operand, ConstantInt::get(operand->getType(), 0), "tobool");
        } else if (operand->getType()->isFloatingPointTy()) {
            operand = builder_->CreateFCmpONE(operand, ConstantFP::get(operand->getType(), 0.0), "tobool");
        }
    }

    return builder_->CreateNot(operand, "nottmp");
}

Value* LLVMCodeGenerator::generateLogicalAnd(const LogicalAnd& expr) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Create blocks for short-circuit evaluation
    BasicBlock* rhsBlock = BasicBlock::Create(context_, "and.rhs", currentFunc);
    BasicBlock* mergeBlock = BasicBlock::Create(context_, "and.merge", currentFunc);

    // Evaluate left operand
    Value* leftVal = generateLLVMExpression(expr.getLeft());
    if (!leftVal) return nullptr;

    // Convert to boolean
    if (!leftVal->getType()->isIntegerTy(1)) {
        if (leftVal->getType()->isIntegerTy()) {
            leftVal = builder_->CreateICmpNE(leftVal, ConstantInt::get(leftVal->getType(), 0), "tobool");
        } else if (leftVal->getType()->isFloatingPointTy()) {
            leftVal = builder_->CreateFCmpONE(leftVal, ConstantFP::get(leftVal->getType(), 0.0), "tobool");
        }
    }

    BasicBlock* lhsBlock = builder_->GetInsertBlock();
    builder_->CreateCondBr(leftVal, rhsBlock, mergeBlock);

    // Evaluate right operand
    builder_->SetInsertPoint(rhsBlock);
    Value* rightVal = generateLLVMExpression(expr.getRight());
    if (!rightVal) return nullptr;

    // Convert to boolean
    if (!rightVal->getType()->isIntegerTy(1)) {
        if (rightVal->getType()->isIntegerTy()) {
            rightVal = builder_->CreateICmpNE(rightVal, ConstantInt::get(rightVal->getType(), 0), "tobool");
        } else if (rightVal->getType()->isFloatingPointTy()) {
            rightVal = builder_->CreateFCmpONE(rightVal, ConstantFP::get(rightVal->getType(), 0.0), "tobool");
        }
    }

    rhsBlock = builder_->GetInsertBlock();
    builder_->CreateBr(mergeBlock);

    // Merge block with PHI node
    builder_->SetInsertPoint(mergeBlock);
    PHINode* phi = builder_->CreatePHI(LLVMType::getInt1Ty(context_), 2, "andtmp");
    phi->addIncoming(ConstantInt::get(LLVMType::getInt1Ty(context_), 0), lhsBlock);
    phi->addIncoming(rightVal, rhsBlock);

    return phi;
}

Value* LLVMCodeGenerator::generateLogicalOr(const LogicalOr& expr) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Create blocks for short-circuit evaluation
    BasicBlock* rhsBlock = BasicBlock::Create(context_, "or.rhs", currentFunc);
    BasicBlock* mergeBlock = BasicBlock::Create(context_, "or.merge", currentFunc);

    // Evaluate left operand
    Value* leftVal = generateLLVMExpression(expr.getLeft());
    if (!leftVal) return nullptr;

    // Convert to boolean
    if (!leftVal->getType()->isIntegerTy(1)) {
        if (leftVal->getType()->isIntegerTy()) {
            leftVal = builder_->CreateICmpNE(leftVal, ConstantInt::get(leftVal->getType(), 0), "tobool");
        } else if (leftVal->getType()->isFloatingPointTy()) {
            leftVal = builder_->CreateFCmpONE(leftVal, ConstantFP::get(leftVal->getType(), 0.0), "tobool");
        }
    }

    BasicBlock* lhsBlock = builder_->GetInsertBlock();
    builder_->CreateCondBr(leftVal, mergeBlock, rhsBlock);

    // Evaluate right operand
    builder_->SetInsertPoint(rhsBlock);
    Value* rightVal = generateLLVMExpression(expr.getRight());
    if (!rightVal) return nullptr;

    // Convert to boolean
    if (!rightVal->getType()->isIntegerTy(1)) {
        if (rightVal->getType()->isIntegerTy()) {
            rightVal = builder_->CreateICmpNE(rightVal, ConstantInt::get(rightVal->getType(), 0), "tobool");
        } else if (rightVal->getType()->isFloatingPointTy()) {
            rightVal = builder_->CreateFCmpONE(rightVal, ConstantFP::get(rightVal->getType(), 0.0), "tobool");
        }
    }

    rhsBlock = builder_->GetInsertBlock();
    builder_->CreateBr(mergeBlock);

    // Merge block with PHI node
    builder_->SetInsertPoint(mergeBlock);
    PHINode* phi = builder_->CreatePHI(LLVMType::getInt1Ty(context_), 2, "ortmp");
    phi->addIncoming(ConstantInt::get(LLVMType::getInt1Ty(context_), 1), lhsBlock);
    phi->addIncoming(rightVal, rhsBlock);

    return phi;
}

Value* LLVMCodeGenerator::generateAssignment(const AssignmentExpression& expr) {
    // Get the lvalue (must be an identifier for now)
    const Expression& lhs = expr.getLeft();
    std::string varName;

    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&lhs)) {
        const ASTNode& primary = primaryExpr->getPrimary();
        if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
            varName = identifier->getName();
        } else {
            std::cerr << "Assignment target must be an identifier" << std::endl;
            return nullptr;
        }
    } else {
        std::cerr << "Complex assignment targets not yet supported" << std::endl;
        return nullptr;
    }

    // Generate the rvalue
    Value* rvalue = generateLLVMExpression(expr.getRight());
    if (!rvalue) return nullptr;

    // Look up the variable
    auto it = namedValues_.find(varName);
    if (it == namedValues_.end()) {
        std::cerr << "Unknown variable: " << varName << std::endl;
        return nullptr;
    }

    // Store the value
    builder_->CreateStore(rvalue, it->second);
    return rvalue;
}

Value* LLVMCodeGenerator::generateMemberAccess(const MemberAccess& expr) {
    // TODO: Implement struct/class member access
    std::cerr << "Member access not yet implemented" << std::endl;
    return nullptr;
}

Value* LLVMCodeGenerator::generateArrayAccess(const ArrayAccess& expr) {
    Value* arrayValue = generateLLVMExpression(expr.getArray());
    Value* indexValue = generateLLVMExpression(expr.getIndex());
    
    if (!arrayValue || !indexValue) {
        std::cerr << "Failed to generate array or index expression" << std::endl;
        return nullptr;
    }
    
    // Convert index to i64 if needed
    if (!indexValue->getType()->isIntegerTy(64)) {
        if (indexValue->getType()->isIntegerTy()) {
            indexValue = builder_->CreateSExt(indexValue, LLVMType::getInt64Ty(context_), "index_ext");
        } else {
            std::cerr << "Array index must be integer type" << std::endl;
            return nullptr;
        }
    }
    
    // For pointer types (dynamic arrays), use GEP with single index
    if (arrayValue->getType()->isPointerTy()) {
        // For newer LLVM versions, we need to explicitly specify the element type
        LLVMType* elementType = LLVMType::getInt64Ty(context_); // default assumption
        
        Value* elementPtr = builder_->CreateGEP(
            elementType, 
            arrayValue, indexValue, "arrayidx");
        
        // Load the value from the computed address
        return builder_->CreateLoad(
            elementType, 
            elementPtr, "arrayval");
    } else {
        std::cerr << "Array access on non-pointer type not supported" << std::endl;
        return nullptr;
    }
}

void LLVMCodeGenerator::generateIfStatement(const IfStatement& stmt) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Evaluate condition
    Value* condValue = generateLLVMExpression(stmt.getCondition());
    if (!condValue) return;

    // Convert condition to boolean if needed
    if (!condValue->getType()->isIntegerTy(1)) {
        if (condValue->getType()->isIntegerTy()) {
            condValue = builder_->CreateICmpNE(condValue, ConstantInt::get(condValue->getType(), 0), "ifcond");
        } else if (condValue->getType()->isFloatingPointTy()) {
            condValue = builder_->CreateFCmpONE(condValue, ConstantFP::get(condValue->getType(), 0.0), "ifcond");
        }
    }

    // Create blocks
    BasicBlock* thenBlock = BasicBlock::Create(context_, "then", currentFunc);
    BasicBlock* elseBlock = stmt.getElseBlock() ? BasicBlock::Create(context_, "else", currentFunc) : nullptr;
    BasicBlock* mergeBlock = BasicBlock::Create(context_, "ifcont", currentFunc);

    // Branch based on condition
    if (elseBlock) {
        builder_->CreateCondBr(condValue, thenBlock, elseBlock);
    } else {
        builder_->CreateCondBr(condValue, thenBlock, mergeBlock);
    }

    // Generate then block
    builder_->SetInsertPoint(thenBlock);
    generateBlock(stmt.getThenBlock());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(mergeBlock);
    }

    // Generate else block if present
    if (elseBlock) {
        builder_->SetInsertPoint(elseBlock);
        generateBlock(*stmt.getElseBlock());
        if (!builder_->GetInsertBlock()->getTerminator()) {
            builder_->CreateBr(mergeBlock);
        }
    }

    // Continue with merge block
    // Note: We don't add a terminator here - subsequent code will add instructions
    // to the merge block (like return statements, more code, etc.)
    builder_->SetInsertPoint(mergeBlock);
}

void LLVMCodeGenerator::generateWhileStatement(const WhileStatement& stmt) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Create blocks
    BasicBlock* condBlock = BasicBlock::Create(context_, "while.cond", currentFunc);
    BasicBlock* bodyBlock = BasicBlock::Create(context_, "while.body", currentFunc);
    BasicBlock* afterBlock = BasicBlock::Create(context_, "while.end", currentFunc);

    // Jump to condition block
    builder_->CreateBr(condBlock);

    // Condition block
    builder_->SetInsertPoint(condBlock);
    Value* condValue = generateLLVMExpression(stmt.getCondition());
    if (!condValue) return;

    // Convert condition to boolean if needed
    if (!condValue->getType()->isIntegerTy(1)) {
        if (condValue->getType()->isIntegerTy()) {
            condValue = builder_->CreateICmpNE(condValue, ConstantInt::get(condValue->getType(), 0), "whilecond");
        } else if (condValue->getType()->isFloatingPointTy()) {
            condValue = builder_->CreateFCmpONE(condValue, ConstantFP::get(condValue->getType(), 0.0), "whilecond");
        }
    }

    builder_->CreateCondBr(condValue, bodyBlock, afterBlock);

    // Body block
    builder_->SetInsertPoint(bodyBlock);
    generateBlock(stmt.getBody());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(condBlock);
    }

    // Continue after loop
    builder_->SetInsertPoint(afterBlock);
}

void LLVMCodeGenerator::generateForInStatement(const ForInStatement& stmt) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();
    
    // Evaluate the iterable expression
    Value* iterableValue = generateLLVMExpression(stmt.getIterable());
    if (!iterableValue) {
        std::cerr << "Failed to generate iterable expression" << std::endl;
        return;
    }
    
    // For now, assume the iterable is an array (pointer)
    // In a full implementation, we'd check the type and handle different iterables
    
    // Create loop variable - assume int64 elements for now
    LLVMType* elementType = LLVMType::getInt64Ty(context_);
    // In newer LLVM, we can't easily get pointer element type, so use default
    
    AllocaInst* loopVar = createEntryBlockAlloca(currentFunc, stmt.getVariable(), elementType);
    
    // Create index variable
    AllocaInst* indexVar = createEntryBlockAlloca(currentFunc, "for.index", LLVMType::getInt64Ty(context_));
    builder_->CreateStore(ConstantInt::get(LLVMType::getInt64Ty(context_), 0), indexVar);
    
    // Create blocks
    BasicBlock* condBlock = BasicBlock::Create(context_, "for.cond", currentFunc);
    BasicBlock* bodyBlock = BasicBlock::Create(context_, "for.body", currentFunc);
    BasicBlock* incrBlock = BasicBlock::Create(context_, "for.incr", currentFunc);
    BasicBlock* endBlock = BasicBlock::Create(context_, "for.end", currentFunc);
    
    // Branch to condition
    builder_->CreateBr(condBlock);
    
    // Condition block - for simplicity, assume we iterate 10 times (should be array length)
    builder_->SetInsertPoint(condBlock);
    Value* currentIndex = builder_->CreateLoad(LLVMType::getInt64Ty(context_), indexVar, "index");
    Value* arrayLength = ConstantInt::get(LLVMType::getInt64Ty(context_), 10); // hardcoded for now
    Value* cond = builder_->CreateICmpSLT(currentIndex, arrayLength, "loopcond");
    builder_->CreateCondBr(cond, bodyBlock, endBlock);
    
    // Body block
    builder_->SetInsertPoint(bodyBlock);
    
    // Load current element (array[index])
    if (iterableValue->getType()->isPointerTy()) {
        Value* elementPtr = builder_->CreateGEP(elementType, iterableValue, currentIndex, "elem.ptr");
        Value* element = builder_->CreateLoad(elementType, elementPtr, "elem");
        
        // Store current element in loop variable
        builder_->CreateStore(element, loopVar);
        namedValues_[stmt.getVariable()] = loopVar;
    }
    
    // Generate loop body
    generateBlock(stmt.getBody());
    
    // Branch to increment
    builder_->CreateBr(incrBlock);
    
    // Increment block
    builder_->SetInsertPoint(incrBlock);
    Value* nextIndex = builder_->CreateAdd(currentIndex, ConstantInt::get(LLVMType::getInt64Ty(context_), 1), "nextindex");
    builder_->CreateStore(nextIndex, indexVar);
    builder_->CreateBr(condBlock);
    
    // End block
    builder_->SetInsertPoint(endBlock);
    
    // Clean up loop variable from scope
    namedValues_.erase(stmt.getVariable());
}

void LLVMCodeGenerator::generateForRangeStatement(const ForRangeStatement& stmt) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Evaluate range bounds
    Value* startValue = generateLLVMExpression(stmt.getStart());
    Value* endValue = generateLLVMExpression(stmt.getEnd());
    if (!startValue || !endValue) return;

    // Allocate loop variable
    AllocaInst* loopVar = createEntryBlockAlloca(currentFunc, stmt.getVariable(), startValue->getType());
    builder_->CreateStore(startValue, loopVar);
    namedValues_[stmt.getVariable()] = loopVar;

    // Create blocks
    BasicBlock* condBlock = BasicBlock::Create(context_, "for.cond", currentFunc);
    BasicBlock* bodyBlock = BasicBlock::Create(context_, "for.body", currentFunc);
    BasicBlock* incBlock = BasicBlock::Create(context_, "for.inc", currentFunc);
    BasicBlock* afterBlock = BasicBlock::Create(context_, "for.end", currentFunc);

    // Jump to condition
    builder_->CreateBr(condBlock);

    // Condition block: check if loopVar < end
    builder_->SetInsertPoint(condBlock);
    Value* currentVal = builder_->CreateLoad(startValue->getType(), loopVar, stmt.getVariable());
    Value* condValue;
    if (startValue->getType()->isIntegerTy()) {
        condValue = builder_->CreateICmpSLT(currentVal, endValue, "forcond");
    } else {
        condValue = builder_->CreateFCmpOLT(currentVal, endValue, "forcond");
    }
    builder_->CreateCondBr(condValue, bodyBlock, afterBlock);

    // Body block
    builder_->SetInsertPoint(bodyBlock);
    generateBlock(stmt.getBody());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(incBlock);
    }

    // Increment block
    builder_->SetInsertPoint(incBlock);
    Value* curVal = builder_->CreateLoad(startValue->getType(), loopVar, stmt.getVariable());
    Value* nextVal;
    if (startValue->getType()->isIntegerTy()) {
        nextVal = builder_->CreateAdd(curVal, ConstantInt::get(startValue->getType(), 1), "nextval");
    } else {
        nextVal = builder_->CreateFAdd(curVal, ConstantFP::get(startValue->getType(), 1.0), "nextval");
    }
    builder_->CreateStore(nextVal, loopVar);
    builder_->CreateBr(condBlock);

    // Continue after loop
    builder_->SetInsertPoint(afterBlock);
    namedValues_.erase(stmt.getVariable());
}

void LLVMCodeGenerator::generateVariableDeclaration(const VariableDeclaration& decl) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Determine type
    LLVMType* varType;
    if (decl.hasTypeInference()) {
        // Infer type from initializer
        if (!decl.getInitializer()) {
            std::cerr << "Type inference requires initializer" << std::endl;
            return;
        }
        Value* initValue = generateLLVMExpression(*decl.getInitializer());
        if (!initValue) return;
        varType = initValue->getType();

        // Create alloca and store
        AllocaInst* alloca = createEntryBlockAlloca(currentFunc, decl.getName(), varType);
        builder_->CreateStore(initValue, alloca);
        namedValues_[decl.getName()] = alloca;
    } else {
        // Explicit type
        varType = generateLLVMType(*decl.getType());
        AllocaInst* alloca = createEntryBlockAlloca(currentFunc, decl.getName(), varType);
        namedValues_[decl.getName()] = alloca;

        // Initialize if initializer present
        if (decl.getInitializer()) {
            Value* initValue = generateLLVMExpression(*decl.getInitializer());
            if (initValue) {
                builder_->CreateStore(initValue, alloca);
            }
        }
    }
}

LLVMType* LLVMCodeGenerator::generateLLVMType(const ASTType& type) {
    if (auto unionType = dynamic_cast<const UnionType*>(&type)) {
        // For now, just use the first type in the union
        const auto& types = unionType->getTypes();
        if (!types.empty()) {
            return generateLLVMType(*types[0]);
        }
    }
    
    if (auto optionalType = dynamic_cast<const OptionalType*>(&type)) {
        // For now, treat optional types as their underlying type
        return generateLLVMType(optionalType->getArrayType());
    }
    
    if (auto arrayType = dynamic_cast<const ASTArrayType*>(&type)) {
        return convertArrayType(*arrayType);
    }
    
    if (auto baseType = dynamic_cast<const BaseType*>(&type)) {
        if (baseType->isPrimitive()) {
            return convertPrimitiveType(baseType->getPrimitiveType()->getKind());
        } else {
            // Custom type - for now, treat as opaque pointer
            return llvm::PointerType::get(context_, 0);
        }
    }
    
    // Default to void
    return LLVMType::getVoidTy(context_);
}

LLVMType* LLVMCodeGenerator::convertPrimitiveType(PrimitiveTypeKind kind) {
    switch (kind) {
        case PrimitiveTypeKind::BYTE:
        case PrimitiveTypeKind::UINT8:
            return LLVMType::getInt8Ty(context_);
        case PrimitiveTypeKind::INT64:
            return LLVMType::getInt64Ty(context_);
        case PrimitiveTypeKind::FLOAT:
            return LLVMType::getFloatTy(context_);
        case PrimitiveTypeKind::DOUBLE:
        case PrimitiveTypeKind::F64:
            return LLVMType::getDoubleTy(context_);
        case PrimitiveTypeKind::BOOL:
            return LLVMType::getInt1Ty(context_);
        case PrimitiveTypeKind::CHAR:
            return LLVMType::getInt32Ty(context_); // Unicode scalar
        case PrimitiveTypeKind::STRING:
            return llvm::PointerType::get(context_, 0); // String as char*
        case PrimitiveTypeKind::VOID:
            return LLVMType::getVoidTy(context_);
        default:
            return LLVMType::getVoidTy(context_);
    }
}

LLVMType* LLVMCodeGenerator::convertArrayType(const ASTArrayType& arrayType) {
    // Get the base element type
    LLVMType* elementType = generateLLVMType(arrayType.getBaseType());
    if (!elementType) {
        elementType = LLVMType::getInt32Ty(context_); // fallback
    }

    const auto& dimensions = arrayType.getDimensions();
    LLVMType* currentType = elementType;

    // Build array type from innermost to outermost dimension
    for (int i = dimensions.size() - 1; i >= 0; i--) {
        // Verify dimension is nullptr (should be unsized after grammar change)
        if (dimensions[i] != nullptr) {
            std::cerr << "Error: Fixed-size array syntax no longer supported. Use array literals." << std::endl;
            return nullptr;
        }
        // Treat all arrays as slices (pointers)
        currentType = llvm::PointerType::get(context_, 0);
    }

    return currentType;
}

Constant* LLVMCodeGenerator::createConstant(const Primary& primary) {
    if (auto intLiteral = dynamic_cast<const IntegerLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt64Ty(context_), intLiteral->getValue());
    } else if (auto floatLiteral = dynamic_cast<const FloatingLiteral*>(&primary)) {
        return ConstantFP::get(LLVMType::getDoubleTy(context_), floatLiteral->getValue());
    } else if (auto boolLiteral = dynamic_cast<const BooleanLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt1Ty(context_), boolLiteral->getValue() ? 1 : 0);
    }

    return nullptr;
}

Value* LLVMCodeGenerator::generateArrayLiteral(const ArrayLiteral& literal) {
    const ExpressionList* elementsList = literal.getElements();

    if (!elementsList || elementsList->getExpressions().empty()) {
        // Empty array - requires type context
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(context_, 0));
    }

    const auto& expressions = elementsList->getExpressions();
    std::vector<Constant*> constantElements;
    LLVMType* elementType = nullptr;

    // Evaluate all elements and infer type from first element
    for (const auto& expr : expressions) {
        Value* elemValue = generateLLVMExpression(*expr);
        if (!elemValue) {
            std::cerr << "Failed to generate array element expression" << std::endl;
            return nullptr;
        }

        // Array literals must contain compile-time constants
        Constant* constElem = llvm::dyn_cast<Constant>(elemValue);
        if (!constElem) {
            std::cerr << "Array literal elements must be compile-time constants" << std::endl;
            return nullptr;
        }

        // Infer element type from first element
        if (elementType == nullptr) {
            elementType = elemValue->getType();
        } else if (elemValue->getType() != elementType) {
            std::cerr << "Array literal elements must have uniform type" << std::endl;
            return nullptr;
        }

        constantElements.push_back(constElem);
    }

    // Create global constant array
    return createGlobalArrayConstant(constantElements, elementType);
}

Constant* LLVMCodeGenerator::createGlobalArrayConstant(
    const std::vector<Constant*>& elements,
    LLVMType* elementType) {

    // Create array type [N x elementType]
    auto arrayType = llvm::ArrayType::get(elementType, elements.size());

    // Create constant array initializer
    auto arrayInit = llvm::ConstantArray::get(arrayType, elements);

    // Create global variable for the array constant
    auto globalArray = new llvm::GlobalVariable(
        *module_,
        arrayType,
        true,  // isConstant
        llvm::GlobalValue::PrivateLinkage,
        arrayInit,
        ".array_literal"
    );

    // Return pointer to the array (decay to pointer)
    // Create GEP to get pointer to first element
    std::vector<llvm::Constant*> indices = {
        llvm::ConstantInt::get(LLVMType::getInt64Ty(context_), 0),
        llvm::ConstantInt::get(LLVMType::getInt64Ty(context_), 0)
    };

    return llvm::ConstantExpr::getGetElementPtr(
        arrayType,
        globalArray,
        indices
    );
}

std::string LLVMCodeGenerator::mangleFunctionName(const std::string& name, const std::vector<LLVMType*>& paramTypes) {
    // Simple name mangling - for production, use a more sophisticated scheme
    std::string mangledName = name;
    for (LLVMType* type : paramTypes) {
        mangledName += "_";
        if (type->isIntegerTy()) {
            mangledName += "i" + std::to_string(type->getIntegerBitWidth());
        } else if (type->isFloatingPointTy()) {
            mangledName += "f";
        } else if (type->isPointerTy()) {
            mangledName += "ptr";
        }
    }
    return mangledName;
}

void LLVMCodeGenerator::generateVariableDeclarationStatement(const VariableDeclarationStatement& stmt) {
    generateVariableDeclaration(stmt.getDeclaration());
}

AllocaInst* LLVMCodeGenerator::createEntryBlockAlloca(Function* function, const std::string& varName, LLVMType* type) {
    IRBuilder<> tmpBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, varName);
}