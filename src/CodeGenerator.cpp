#include "CodeGenerator.h"
#include "ast/AST.h"
#include "llvm/IR/Verifier.h"
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

CodeGenerator::CodeGenerator(LLVMContext& context) 
    : context_(context) {
    builder_ = std::make_unique<IRBuilder<>>(context_);
}

CodeGenerator::~CodeGenerator() {}

std::unique_ptr<Module> CodeGenerator::generateModule(const CompilationUnit& compilationUnit) {
    // Create a new module for this compilation unit
    module_ = std::make_unique<Module>("hooc_module", context_);
    
    // Clear symbol tables for new module
    namedValues_.clear();
    functions_.clear();
    
    // Process all declarations in the compilation unit
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(decl.get())) {
            generateFunction(*funcDecl);
        }
        // TODO: Add support for class declarations, variable declarations, etc.
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

Function* CodeGenerator::generateFunction(const FunctionDeclaration& funcDecl) {
    // Build parameter types
    std::vector<LLVMType*> paramTypes;
    for (const auto& param : funcDecl.getParameters()) {
        LLVMType* paramType = generateType(param->getType());
        paramTypes.push_back(paramType);
    }
    
    // Determine return type
    LLVMType* returnType = LLVMType::getVoidTy(context_); // Default to void
    if (funcDecl.getReturnType()) {
        returnType = generateType(*funcDecl.getReturnType());
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
    
    // Set parameter names and add to symbol table
    auto paramIter = funcDecl.getParameters().begin();
    for (auto& arg : function->args()) {
        if (paramIter != funcDecl.getParameters().end()) {
            arg.setName((*paramIter)->getName());
            namedValues_[(*paramIter)->getName()] = &arg;
            ++paramIter;
        }
    }
    
    // Create entry basic block
    BasicBlock* entryBlock = BasicBlock::Create(context_, "entry", function);
    builder_->SetInsertPoint(entryBlock);
    
    // Generate function body
    generateBlock(funcDecl.getBody());
    
    // If no explicit return and void function, add return void
    if (returnType->isVoidTy() && !entryBlock->getTerminator()) {
        builder_->CreateRetVoid();
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

void CodeGenerator::generateBlock(const Block& block) {
    for (const auto& stmt : block.getStatements()) {
        generateStatement(*stmt);
    }
}

void CodeGenerator::generateStatement(const Statement& stmt) {
    if (auto retStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        generateReturnStatement(*retStmt);
    } else if (auto exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        generateExpressionStatement(*exprStmt);
    } else if (auto blockStmt = dynamic_cast<const Block*>(&stmt)) {
        generateBlock(*blockStmt);
    }
    // TODO: Add support for if, for, while statements
}

void CodeGenerator::generateReturnStatement(const ReturnStatement& ret) {
    if (ret.hasExpression()) {
        Value* retValue = generateExpression(*ret.getExpression());
        builder_->CreateRet(retValue);
    } else {
        builder_->CreateRetVoid();
    }
}

void CodeGenerator::generateExpressionStatement(const ExpressionStatement& stmt) {
    generateExpression(stmt.getExpression());
}

Value* CodeGenerator::generateExpression(const Expression& expr) {
    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&expr)) {
        return generatePrimaryExpression(*primaryExpr);
    } else if (auto binaryExpr = dynamic_cast<const BinaryExpression*>(&expr)) {
        return generateBinaryExpression(*binaryExpr);
    } else if (auto funcCall = dynamic_cast<const FunctionCall*>(&expr)) {
        return generateFunctionCall(*funcCall);
    }
    
    // TODO: Add support for other expression types
    std::cerr << "Unsupported expression type in generateExpression" << std::endl;
    return nullptr;
}

Value* CodeGenerator::generatePrimaryExpression(const PrimaryExpression& expr) {
    const ASTNode& primary = expr.getPrimary();
    
    if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
        // Look up variable or function
        auto it = namedValues_.find(identifier->getName());
        if (it != namedValues_.end()) {
            return it->second;
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
    }
    
    std::cerr << "Unsupported primary expression type" << std::endl;
    return nullptr;
}

Value* CodeGenerator::generateBinaryExpression(const BinaryExpression& expr) {
    Value* left = generateExpression(expr.getLeft());
    Value* right = generateExpression(expr.getRight());
    
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
            
        case ASTBinaryOperator::LESS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSLT(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOLT(left, right, "cmptmp");
            }
            break;
            
        case ASTBinaryOperator::EQUALS:
            if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpEQ(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOEQ(left, right, "cmptmp");
            }
            break;
            
        default:
            std::cerr << "Unsupported binary operator" << std::endl;
            return nullptr;
    }
    
    std::cerr << "Type mismatch in binary expression" << std::endl;
    return nullptr;
}

Value* CodeGenerator::generateFunctionCall(const FunctionCall& call) {
    // TODO: Implement function call generation
    std::cerr << "Function calls not yet implemented" << std::endl;
    return nullptr;
}

LLVMType* CodeGenerator::generateType(const ASTType& type) {
    if (auto unionType = dynamic_cast<const UnionType*>(&type)) {
        // For now, just use the first type in the union
        const auto& types = unionType->getTypes();
        if (!types.empty()) {
            return generateType(*types[0]);
        }
    }
    
    if (auto optionalType = dynamic_cast<const OptionalType*>(&type)) {
        // For now, treat optional types as their underlying type
        return generateType(optionalType->getArrayType());
    }
    
    if (auto arrayType = dynamic_cast<const ASTArrayType*>(&type)) {
        return convertArrayType(*arrayType);
    }
    
    if (auto baseType = dynamic_cast<const BaseType*>(&type)) {
        if (baseType->isPrimitive()) {
            return convertPrimitiveType(baseType->getPrimitiveType()->getKind());
        } else {
            // Custom type - for now, treat as opaque pointer
            return llvm::PointerType::get(LLVMType::getInt8Ty(context_), 0);
        }
    }
    
    // Default to void
    return LLVMType::getVoidTy(context_);
}

LLVMType* CodeGenerator::convertPrimitiveType(PrimitiveTypeKind kind) {
    switch (kind) {
        case PrimitiveTypeKind::BYTE:
        case PrimitiveTypeKind::UINT8:
            return LLVMType::getInt8Ty(context_);
        case PrimitiveTypeKind::INT64:
            return LLVMType::getInt64Ty(context_);
        case PrimitiveTypeKind::DOUBLE:
        case PrimitiveTypeKind::F64:
            return LLVMType::getDoubleTy(context_);
        case PrimitiveTypeKind::BOOL:
            return LLVMType::getInt1Ty(context_);
        case PrimitiveTypeKind::CHAR:
            return LLVMType::getInt32Ty(context_); // Unicode scalar
        case PrimitiveTypeKind::STRING:
            return llvm::PointerType::get(LLVMType::getInt8Ty(context_), 0); // String as char*
        default:
            return LLVMType::getVoidTy(context_);
    }
}

LLVMType* CodeGenerator::convertArrayType(const ASTArrayType& arrayType) {
    LLVMType* elementType = generateType(arrayType.getBaseType());
    
    // For now, treat all arrays as pointers to the element type
    // TODO: Implement proper array handling with size tracking
    return llvm::PointerType::get(elementType, 0);
}

Constant* CodeGenerator::createConstant(const Primary& primary) {
    if (auto intLiteral = dynamic_cast<const IntegerLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt64Ty(context_), intLiteral->getValue());
    } else if (auto floatLiteral = dynamic_cast<const FloatingLiteral*>(&primary)) {
        return ConstantFP::get(LLVMType::getDoubleTy(context_), floatLiteral->getValue());
    } else if (auto boolLiteral = dynamic_cast<const BooleanLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt1Ty(context_), boolLiteral->getValue() ? 1 : 0);
    }
    
    return nullptr;
}

std::string CodeGenerator::mangleFunctionName(const std::string& name, const std::vector<LLVMType*>& paramTypes) {
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