#include "LLVMCodeGenerator.h"
#include "core/SymbolMangler.h"
#include "runtime/llvm/RuntimeRegistry.h"
#include "runtime/llvm/RuntimeMethodRegistry.h"
#include "runtime/llvm/RuntimeNetMethods.h"
#include "../ast/AST.h"
#include "../ast/ClassDeclaration.h"
#include "../ast/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DerivedTypes.h"
#include <iostream>
#include <stdexcept>

// Force the runtime methods registration to be linked
extern void _hoo_runtime_methods_ensure_registration();

using namespace hooc;
using namespace hooc::ast;
using namespace hooc::runtime;
using namespace llvm;

// Avoid namespace conflicts with LLVM
namespace {
    using LLVMType = llvm::Type;
    using HoocModule = hooc::HooModule;
    using ASTType = hooc::ast::Type;
    using ASTBinaryOperator = hooc::ast::BinaryOperator;
    using ASTArrayType = hooc::ast::ArrayType;
    using ASTStringLiteral = hooc::ast::StringLiteral;
}

LLVMCodeGenerator::LLVMCodeGenerator(LLVMContext& context)
    : context_(context) {
    builder_ = std::make_unique<IRBuilder<>>(context_);
}

LLVMCodeGenerator::~LLVMCodeGenerator() {}

// ============================================================================
// Error Handling
// ============================================================================

void LLVMCodeGenerator::addError(const std::string& message) {
    errors_.push_back(message);
}

void LLVMCodeGenerator::addError(const std::string& message, int line, int column) {
    std::string fullMessage = message + " at line " + std::to_string(line) + ", column " + std::to_string(column);
    errors_.push_back(fullMessage);
}

// ============================================================================
// Runtime Function Accessors
// ============================================================================

Function* LLVMCodeGenerator::getStringFunc(const std::string& name) {
    std::string fullName = "hoo_string_" + name;
    return module_->getFunction(fullName);
}

Function* LLVMCodeGenerator::getArrayFunc(const std::string& name) {
    std::string fullName = "hoo_array_" + name;
    return module_->getFunction(fullName);
}

Function* LLVMCodeGenerator::getExceptionFunc(const std::string& name) {
    std::string fullName = "hoo_exception_" + name;
    return module_->getFunction(fullName);
}

Function* LLVMCodeGenerator::getMapFunc(const std::string& name) {
    std::string fullName = "hoo_map_" + name;
    return module_->getFunction(fullName);
}

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
std::unique_ptr<llvm::Module> LLVMCodeGenerator::generateLLVMModule(const CompilationUnit& compilationUnit) {
    // Create a new module for this compilation unit with local ownership
    auto ownedModule = std::make_unique<llvm::Module>("hooc_module", context_);
    module_ = ownedModule.get();

    // Clear symbol tables for new module
    namedValues_.clear();
    functions_.clear();
    classTypes_.clear();
    classTypeIds_.clear();
    nextTypeId_ = 1;
    importedNames_.clear();

    // Process imports to populate importedNames_ symbol table
    processImports(compilationUnit.getImports());

    // Reset runtime function pointers
    hoo_alloc_func_ = nullptr;
    hoo_retain_func_ = nullptr;
    hoo_release_func_ = nullptr;

    // Reset runtime function storage
    runtimeFunctionStorage_ = {};

    deferredInitializers_.clear();

    // Force runtime methods registration to be linked
    _hoo_runtime_methods_ensure_registration();

    // Declare string functions early so they're available
    declareRuntimeFunctions();

    // First pass: process class declarations to create types
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto classDecl = dynamic_cast<const ClassDeclaration*>(decl.get())) {
            generateClassDeclaration(*classDecl);
        }
    }

    if (hasErrors()) {
        module_ = nullptr;
        return nullptr;
    }

    // Second pass: process functions and variables
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(decl.get())) {
            generateLLVMFunction(*funcDecl);
        } else if (auto varDecl = dynamic_cast<const VariableDeclaration*>(decl.get())) {
            if (varDecl->isConstant()) {
                generateConstantDeclaration(*varDecl);
            } else if (varDecl->isGlobal()) {
                generateGlobalVariable(*varDecl);
            } else {
                generateVariableDeclaration(*varDecl);
            }
        }
    }
    
    if (hasErrors()) {
        module_ = nullptr;
        return nullptr;
    }
    
    generateModuleInitializer();

    // Verify the module
    std::string errorStr;
    raw_string_ostream errorStream(errorStr);
    if (verifyModule(*ownedModule, &errorStream)) {
        addError("Module verification failed: " + errorStr);
        module_ = nullptr;
        return nullptr;
    }
    
    module_ = nullptr;
    builder_->ClearInsertionPoint();
    return ownedModule;
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
        module_
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
    // This is critical even if generation had errors to keep the IR valid for verification
    for (auto& BB : *function) {
        if (!BB.getTerminator()) {
            builder_->SetInsertPoint(&BB);
            if (returnType->isVoidTy()) {
                builder_->CreateRetVoid();
            } else {
                builder_->CreateUnreachable();
            }
        }
    }
    
    // Verify function
    std::string errorStr;
    raw_string_ostream errorStream(errorStr);
    if (verifyFunction(*function, &errorStream)) {
        addError("Function '" + funcDecl.getName() + "' verification failed: " + errorStr);
        // Don't erase yet, let the module verify catch it or let the cleanup handle it
        return function;
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
    } else if (auto scopeStmt = dynamic_cast<const ScopeStatement*>(&stmt)) {
        generateScopeStatement(*scopeStmt);
    } else if (auto varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(&stmt)) {
        generateVariableDeclarationStatement(*varDeclStmt);
    } else if (auto breakStmt = dynamic_cast<const BreakStatement*>(&stmt)) {
        generateBreakStatement(*breakStmt);
    } else if (auto continueStmt = dynamic_cast<const ContinueStatement*>(&stmt)) {
        generateContinueStatement(*continueStmt);
    } else if (auto tryCatchStmt = dynamic_cast<const TryCatchStatement*>(&stmt)) {
        generateTryCatchStatement(*tryCatchStmt);
    } else if (auto throwStmt = dynamic_cast<const ThrowStatement*>(&stmt)) {
        generateThrowStatement(*throwStmt);
    } else {
        addError("Unsupported statement type");
    }
}

void LLVMCodeGenerator::generateReturnStatement(const ReturnStatement& ret) {
    if (ret.hasExpression()) {
        Value* retValue = generateLLVMExpression(*ret.getExpression());
        
        // Ensure return value matches function return type (handle nullable wrapping)
        Function* currentFunc = builder_->GetInsertBlock()->getParent();
        retValue = ensureTypeMatch(retValue, currentFunc->getReturnType());
        
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
    } else if (auto compound = dynamic_cast<const CompoundAssignmentExpression*>(&expr)) {
        return generateCompoundAssignment(*compound);
    } else if (auto incDec = dynamic_cast<const IncrementDecrementExpression*>(&expr)) {
        return generateIncrementDecrement(*incDec);
    } else if (auto memberAccess = dynamic_cast<const MemberAccess*>(&expr)) {
        return generateMemberAccess(*memberAccess);
    } else if (auto arrayAccess = dynamic_cast<const ArrayAccess*>(&expr)) {
        return generateArrayAccess(*arrayAccess);
    } else if (auto arrayLit = dynamic_cast<const ArrayLiteral*>(&expr)) {
        return generateArrayLiteral(*arrayLit);
    } else if (auto newObjExpr = dynamic_cast<const NewObjectExpression*>(&expr)) {
        return generateNewObjectExpression(*newObjExpr);
    }

    addError("Unsupported expression type in generateExpression");
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

        addError("Unknown variable: " + identifier->getName());
        return nullptr;
        
    } else if (auto intLiteral = dynamic_cast<const IntegerLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt64Ty(context_), intLiteral->getValue());
        
    } else if (auto floatLiteral = dynamic_cast<const FloatingLiteral*>(&primary)) {
        return ConstantFP::get(LLVMType::getDoubleTy(context_), floatLiteral->getValue());
        
    } else if (auto boolLiteral = dynamic_cast<const BooleanLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt1Ty(context_), boolLiteral->getValue() ? 1 : 0);

    } else if (auto nullLiteral = dynamic_cast<const NullLiteral*>(&primary)) {
        // Return a null pointer for now - the actual nullable type handling
        // happens in variable declaration and function parameter contexts
        return ConstantPointerNull::get(llvm::PointerType::get(context_, 0));

    } else if (auto thisLiteral = dynamic_cast<const ThisLiteral*>(&primary)) {
        return generateThisLiteral(*thisLiteral);

    } else if (auto stringLiteral = dynamic_cast<const ASTStringLiteral*>(&primary)) {
        // Create global string constant
        Value* cstr = builder_->CreateGlobalString(stringLiteral->getValue(), "str");

        // Ensure string functions are declared
        declareRuntimeFunctions();

        // Call hoo_string_from_cstr(cstr) to create HooString object
        auto* fromCstrFunc = getStringFunc("from_cstr");
        if (!fromCstrFunc) {
            addError("hoo_string_from_cstr not declared");
            return nullptr;
        }

        Value* hooString = builder_->CreateCall(fromCstrFunc, {cstr}, "hoo_str");
        return hooString;

    } else if (auto charLiteral = dynamic_cast<const CharacterLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt32Ty(context_), static_cast<uint32_t>(charLiteral->getValue()));
    } else if (auto arrayLiteral = dynamic_cast<const ArrayLiteral*>(&primary)) { // Handle ArrayLiteral
        return generateArrayLiteral(*arrayLiteral);
    } else if (auto interpolatedString = dynamic_cast<const InterpolatedString*>(&primary)) {
        // For now, treat interpolated strings as regular strings by stripping ${...} placeholders
        // TODO: Implement full runtime substitution for ${...} expressions
        std::string template_ = interpolatedString->getTemplate();
        size_t start = 0;
        std::string result;
        while (true) {
            size_t dollarPos = template_.find("${", start);
            if (dollarPos == std::string::npos) {
                result += template_.substr(start);
                break;
            }
            result += template_.substr(start, dollarPos - start);
            size_t endPos = template_.find("}", dollarPos);
            if (endPos == std::string::npos) {
                result += template_.substr(dollarPos);
                break;
            }
            result += "<placeholder>";
            start = endPos + 1;
        }

        Value* cstr = builder_->CreateGlobalString(result, "interp_str");
        declareRuntimeFunctions();
        auto* fromCstrFunc = getStringFunc("from_cstr");
        if (!fromCstrFunc) {
            addError("hoo_string_from_cstr not declared");
            return nullptr;
        }
        return builder_->CreateCall(fromCstrFunc, {cstr}, "hoo_interp_str");
    } else if (auto parenthesized = dynamic_cast<const ParenthesizedExpression*>(&primary)) {
        return generateLLVMExpression(parenthesized->getExpression());
    }

    addError("Unsupported primary expression type");
    return nullptr;
}

Value* LLVMCodeGenerator::generateThisLiteral(const ThisLiteral& expr) {
    auto it = namedValues_.find("this");
    if (it != namedValues_.end()) {
        // 'this' is already a pointer (the implicit first parameter),
        // so we don't need to load it if it's stored directly in namedValues_.
        // In our implementation, 'this' is stored as the Value* of the argument.
        return it->second;
    }

    addError("'this' keyword used outside of class context");
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
            // Check for string concatenation (both operands are pointers)
            if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
                // Ensure string functions are declared
                declareRuntimeFunctions();

                // Call hoo_string_concat(left, right)
                auto* concatFunc = getStringFunc("concat");
                if (!concatFunc) {
                    addError("hoo_string_concat not declared");
                    return nullptr;
                }

                Value* result = builder_->CreateCall(concatFunc, {left, right}, "concat");
                return result;
            } else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
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
            // Check for string comparison (both operands are pointers)
            if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
                // Ensure string functions are declared
                declareRuntimeFunctions();

                // Call hoo_string_compare(left, right) - returns <0 if left < right
                auto* compareFunc = getStringFunc("compare");
                if (!compareFunc) {
                    addError("hoo_string_compare not declared");
                    return nullptr;
                }

                Value* cmpResult = builder_->CreateCall(compareFunc, {left, right}, "strcmp");
                return builder_->CreateICmpSLT(cmpResult, ConstantInt::get(LLVMType::getInt64Ty(context_), 0), "ltmp");
            } else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSLT(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOLT(left, right, "cmptmp");
            }
            break;
            
        case ASTBinaryOperator::LESS_EQUALS:
            // Check for string comparison (both operands are pointers)
            if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
                // Ensure string functions are declared
                declareRuntimeFunctions();

                // Call hoo_string_compare(left, right) - returns <=0 if left <= right
                auto* compareFunc = getStringFunc("compare");
                if (!compareFunc) {
                    addError("hoo_string_compare not declared");
                    return nullptr;
                }

                Value* cmpResult = builder_->CreateCall(compareFunc, {left, right}, "strcmp");
                return builder_->CreateICmpSLE(cmpResult, ConstantInt::get(LLVMType::getInt64Ty(context_), 0), "letmp");
            } else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSLE(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOLE(left, right, "cmptmp");
            }
            break;

        case ASTBinaryOperator::GREATER:
            // Check for string comparison (both operands are pointers)
            if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
                // Ensure string functions are declared
                declareRuntimeFunctions();

                // Call hoo_string_compare(left, right) - returns >0 if left > right
                auto* compareFunc = getStringFunc("compare");
                if (!compareFunc) {
                    addError("hoo_string_compare not declared");
                    return nullptr;
                }

                Value* cmpResult = builder_->CreateCall(compareFunc, {left, right}, "strcmp");
                return builder_->CreateICmpSGT(cmpResult, ConstantInt::get(LLVMType::getInt64Ty(context_), 0), "gtmp");
            } else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSGT(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOGT(left, right, "cmptmp");
            }
            break;

        case ASTBinaryOperator::GREATER_EQUALS:
            // Check for string comparison (both operands are pointers)
            if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
                // Ensure string functions are declared
                declareRuntimeFunctions();

                // Call hoo_string_compare(left, right) - returns >=0 if left >= right
                auto* compareFunc = getStringFunc("compare");
                if (!compareFunc) {
                    addError("hoo_string_compare not declared");
                    return nullptr;
                }

                Value* cmpResult = builder_->CreateCall(compareFunc, {left, right}, "strcmp");
                return builder_->CreateICmpSGE(cmpResult, ConstantInt::get(LLVMType::getInt64Ty(context_), 0), "getmp");
            } else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpSGE(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOGE(left, right, "cmptmp");
            }
            break;

        case ASTBinaryOperator::EQUALS:
            // Check for string comparison (both operands are pointers)
            if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
                // Ensure string functions are declared
                declareRuntimeFunctions();

                // Call hoo_string_equals(left, right) - returns 1 if equal, 0 if not
                auto* equalsFunc = getStringFunc("equals");
                if (!equalsFunc) {
                    addError("hoo_string_equals not declared");
                    return nullptr;
                }

                Value* equalResult = builder_->CreateCall(equalsFunc, {left, right}, "streq");
                // Convert i64 result to i1 (bool)
                return builder_->CreateICmpNE(equalResult, ConstantInt::get(LLVMType::getInt64Ty(context_), 0), "eqtmp");
            } else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpEQ(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpOEQ(left, right, "cmptmp");
            }
            break;

        case ASTBinaryOperator::NOT_EQUALS:
            // Check for string comparison (both operands are pointers)
            if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
                // Ensure string functions are declared
                declareRuntimeFunctions();

                // Call hoo_string_equals(left, right) - returns 1 if equal, 0 if not
                auto* equalsFunc = getStringFunc("equals");
                if (!equalsFunc) {
                    addError("hoo_string_equals not declared");
                    return nullptr;
                }

                Value* equalResult = builder_->CreateCall(equalsFunc, {left, right}, "strneq");
                // Convert i64 result to i1 (bool) - return true if NOT equal (equalResult == 0)
                return builder_->CreateICmpEQ(equalResult, ConstantInt::get(LLVMType::getInt64Ty(context_), 0), "neqtmp");
            } else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                return builder_->CreateICmpNE(left, right, "cmptmp");
            } else if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
                return builder_->CreateFCmpONE(left, right, "cmptmp");
            }
            break;

        default:
            addError("Unsupported binary operator");
            return nullptr;
    }
    
    addError("Type mismatch in binary expression");
    return nullptr;
}

Value* LLVMCodeGenerator::generateFunctionCall(const FunctionCall& call) {
    const Expression& funcExpr = call.getFunction();

    std::string functionName;
    Value* thisPtr = nullptr;
    bool isRuntimeMethod = false;
    std::string runtimeFuncName;

    if (auto memberAccess = dynamic_cast<const MemberAccess*>(&funcExpr)) {
        const std::string& methodName = memberAccess->getMember();

        Value* objectValue = generateLLVMExpression(memberAccess->getObject());
        if (!objectValue) {
            addError("Failed to generate object expression for method call");
            return nullptr;
        }

        std::string className;
        if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&memberAccess->getObject())) {
            const ASTNode& primary = primaryExpr->getPrimary();
            if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
                auto it = variableTypes_.find(identifier->getName());
                if (it != variableTypes_.end()) {
                    className = it->second;
                } else {
                    // For runtime classes, check if it matches a known class pattern like "hoo.net.HttpClient"
                    // Try lowercase version for runtime method lookup
                    std::string varName = identifier->getName();
                    if (RuntimeMethodRegistry::getInstance().isRuntimeClass(varName)) {
                        className = varName;
                    }
                }
            } else if (auto memberAccess2 = dynamic_cast<const MemberAccess*>(&primary)) {
                // Handle chained call like client.get("url").getStatusCode()
                // Recursively get the class name from the inner object
                auto innerObject = &memberAccess2->getObject();
                if (auto innerPrimary = dynamic_cast<const PrimaryExpression*>(innerObject)) {
                    const ASTNode& innerPrimaryNode = innerPrimary->getPrimary();
                    if (auto innerIdentifier = dynamic_cast<const Identifier*>(&innerPrimaryNode)) {
                        std::string innerVarName = innerIdentifier->getName();
                        auto it2 = variableTypes_.find(innerVarName);
                        if (it2 != variableTypes_.end()) {
                            className = it2->second;
                        } else if (RuntimeMethodRegistry::getInstance().isRuntimeClass(innerVarName)) {
                            className = innerVarName;
                        }
                    }
                }
            }
        }

        if (className.empty()) {
            // Last resort: check if the object expression is a NEW object, and extract class from that
            if (auto newObj = dynamic_cast<const NewObjectExpression*>(&memberAccess->getObject())) {
                if (auto qualifiedName = newObj->getQualifiedClassName()) {
                    className = qualifiedName->getFullName();
                }
            }
        }

        if (className.empty()) {
            addError("Cannot determine class type for method call");
            return nullptr;
        }

        const RuntimeMethodDescriptor* runtimeMethod =
            RuntimeMethodRegistry::getInstance().findMethod(className, methodName);

        if (runtimeMethod) {
            isRuntimeMethod = true;
            runtimeFuncName = runtimeMethod->runtimeFuncName;
            functionName = runtimeFuncName;
            thisPtr = objectValue;
        } else {
            functionName = className + "_" + methodName;
            thisPtr = objectValue;
        }

    } else if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&funcExpr)) {
        const ASTNode& primary = primaryExpr->getPrimary();
        if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
            functionName = identifier->getName();

            // Handle built-in I/O functions - redirect to hoo.* prefixed versions
            if (functionName == "print" || functionName == "println" ||
                functionName == "readline" || functionName == "readchar") {
                functionName = "hoo." + functionName;
            }
        } else {
            addError("Function call must use identifier");
            return nullptr;
        }
    } else {
        addError("Complex function expressions not yet supported");
        return nullptr;
    }

    Function* calleeFunc = module_->getFunction(functionName);
    if (!calleeFunc && !isRuntimeMethod) {
        addError("Unknown function: " + functionName);
        return nullptr;
    }

    // Generate argument values with type conversion
    std::vector<Value*> args;

    if (thisPtr) {
        args.push_back(thisPtr);
    }

    if (call.getArguments()) {
        const auto& argsList = call.getArguments()->getArguments();
        for (size_t i = 0; i < argsList.size(); ++i) {
            Value* argValue = generateLLVMExpression(*argsList[i]);
            if (!argValue) {
                return nullptr;
            }

            size_t paramIndex = thisPtr ? (i + 1) : i;
            if (calleeFunc && paramIndex < calleeFunc->arg_size()) {
                llvm::Type* expectedType = calleeFunc->getFunctionType()->getParamType(paramIndex);
                
                // Handle implicit conversions (numeric and nullable)
                argValue = ensureTypeMatch(argValue, expectedType);
                
                llvm::Type* actualType = argValue->getType();
                if (actualType != expectedType) {
                    if (actualType->isIntegerTy() && expectedType->isIntegerTy()) {
                        unsigned actualBits = actualType->getIntegerBitWidth();
                        unsigned expectedBits = expectedType->getIntegerBitWidth();

                        if (actualBits > expectedBits) {
                            argValue = builder_->CreateTrunc(argValue, expectedType);
                        } else if (actualBits < expectedBits) {
                            argValue = builder_->CreateSExt(argValue, expectedType);
                        }
                    } else if (actualType->isFloatingPointTy() && expectedType->isFloatingPointTy()) {
                        if (actualType->isDoubleTy() && expectedType->isFloatTy()) {
                            argValue = builder_->CreateFPTrunc(argValue, expectedType);
                        } else if (actualType->isFloatTy() && expectedType->isDoubleTy()) {
                            argValue = builder_->CreateFPExt(argValue, expectedType);
                        }
                    }
                }
            }

            args.push_back(argValue);
        }
    }

    if (calleeFunc) {
        if (args.size() != calleeFunc->arg_size()) {
            addError("Incorrect number of arguments for function " + functionName
                      + " (expected " + std::to_string(calleeFunc->arg_size()) + ", got " + std::to_string(args.size()) + ")");
            return nullptr;
        }

        if (calleeFunc->getReturnType()->isVoidTy()) {
            return builder_->CreateCall(calleeFunc, args);
        } else {
            return builder_->CreateCall(calleeFunc, args, "calltmp");
        }
    } else if (isRuntimeMethod) {
        Function* runtimeFunc = module_->getFunction(runtimeFuncName);
        if (!runtimeFunc) {
            addError("Runtime function not declared: " + runtimeFuncName);
            return nullptr;
        }
        return builder_->CreateCall(runtimeFunc, args, "runtime_call");
    }

    return nullptr;
}

Value* LLVMCodeGenerator::generateUnaryExpression(const UnaryMinus& expr) {
    Value* operand = generateLLVMExpression(expr.getOperand());
    if (!operand) return nullptr;

    if (operand->getType()->isIntegerTy()) {
        return builder_->CreateNeg(operand, "negtmp");
    } else if (operand->getType()->isFloatingPointTy()) {
        return builder_->CreateFNeg(operand, "negtmp");
    }

    addError("Unsupported type for unary minus");
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
    const Expression& lhs = expr.getLeft();

    // Handle member assignment: obj.member = value
    if (auto memberAccess = dynamic_cast<const MemberAccess*>(&lhs)) {
        Value* objectValue = generateLLVMExpression(memberAccess->getObject());
        if (!objectValue) {
            addError("Failed to generate object expression for assignment");
            return nullptr;
        }

        std::string className;
        if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&memberAccess->getObject())) {
            const ASTNode& primary = primaryExpr->getPrimary();
            if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
                auto it = variableTypes_.find(identifier->getName());
                if (it != variableTypes_.end()) {
                    className = it->second;
                }
            } else if (dynamic_cast<const ThisLiteral*>(&primary)) {
                auto it = variableTypes_.find("this");
                if (it != variableTypes_.end()) {
                    className = it->second;
                }
            }
        }

        if (className.empty()) {
            addError("Cannot determine class type for member assignment");
            return nullptr;
        }

        auto classTypeIt = classTypes_.find(className);
        if (classTypeIt == classTypes_.end()) {
            addError("Unknown class type: " + className);
            return nullptr;
        }
        llvm::StructType* classType = classTypeIt->second;

        auto declIt = classDeclarations_.find(className);
        if (declIt == classDeclarations_.end()) {
            addError("Missing class declaration for: " + className);
            return nullptr;
        }
        const ast::ClassDeclaration* classDecl = declIt->second;

        const std::string& memberName = memberAccess->getMember();
        int memberIndex = -1;
        int fieldIdx = 0;

        for (const auto& member : classDecl->getBody().getMembers()) {
            if (auto decl = member->getDeclaration()) {
                if (auto varMember = dynamic_cast<const ast::VariableDeclaration*>(decl)) {
                    if (varMember->getName() == memberName) {
                        memberIndex = fieldIdx;
                        break;
                    }
                    fieldIdx++;
                }
            }
        }

        if (memberIndex == -1) {
            addError("Member not found: " + memberName + " in class " + className);
            return nullptr;
        }

        auto structPtrType = llvm::PointerType::get(context_, 0);
        auto fieldPtr = builder_->CreateStructGEP(classType, objectValue, memberIndex, "field_ptr");

        Value* rvalue = generateLLVMExpression(expr.getRight());
        if (!rvalue) return nullptr;

        // Get the field type
        llvm::Type* fieldType = classType->getElementType(memberIndex);
        rvalue = ensureTypeMatch(rvalue, fieldType);

        builder_->CreateStore(rvalue, fieldPtr);
        return rvalue;
    }

    // Get the lvalue (must be an identifier or this for now)
    std::string varName;

    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&lhs)) {
        const ASTNode& primary = primaryExpr->getPrimary();
        if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
            varName = identifier->getName();
        } else if (dynamic_cast<const ThisLiteral*>(&primary)) {
            varName = "this";
        } else {
            addError("Assignment target must be an identifier or 'this'");
            return nullptr;
        }
    } else {
        addError("Complex assignment targets not yet supported");
        return nullptr;
    }

    // Generate the rvalue
    Value* rvalue = generateLLVMExpression(expr.getRight());
    if (!rvalue) return nullptr;

    // Look up the variable
    auto it = namedValues_.find(varName);
    if (it == namedValues_.end()) {
        addError("Unknown variable: " + varName);
        return nullptr;
    }

    // Ensure type match (handle nullable wrapping)
    if (auto alloca = llvm::dyn_cast<AllocaInst>(it->second)) {
        rvalue = ensureTypeMatch(rvalue, alloca->getAllocatedType());
    }

    // Store the value
    builder_->CreateStore(rvalue, it->second);
    return rvalue;
}

Value* LLVMCodeGenerator::generateCompoundAssignment(const CompoundAssignmentExpression& expr) {
    const Expression& lhs = expr.getLeft();

    std::string varName;
    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&lhs)) {
        const ASTNode& primary = primaryExpr->getPrimary();
        if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
            varName = identifier->getName();
        } else {
            addError("Compound assignment target must be an identifier");
            return nullptr;
        }
    } else {
        addError("Compound assignment target must be an identifier");
        return nullptr;
    }

    auto it = namedValues_.find(varName);
    if (it == namedValues_.end()) {
        addError("Unknown variable: " + varName);
        return nullptr;
    }

    AllocaInst* alloca = llvm::cast<AllocaInst>(it->second);
    Value* currentValue = builder_->CreateLoad(alloca->getAllocatedType(), alloca, varName);
    Value* rhsValue = generateLLVMExpression(expr.getRight());
    if (!rhsValue) return nullptr;

    Value* result = nullptr;
    switch (expr.getOperator()) {
        case CompoundAssignmentOperator::PLUS_ASSIGN:
            result = builder_->CreateAdd(currentValue, rhsValue);
            break;
        case CompoundAssignmentOperator::MINUS_ASSIGN:
            result = builder_->CreateSub(currentValue, rhsValue);
            break;
        case CompoundAssignmentOperator::MULTIPLY_ASSIGN:
            result = builder_->CreateMul(currentValue, rhsValue);
            break;
        case CompoundAssignmentOperator::DIVIDE_ASSIGN:
            result = builder_->CreateSDiv(currentValue, rhsValue);
            break;
        case CompoundAssignmentOperator::MODULO_ASSIGN:
            result = builder_->CreateSRem(currentValue, rhsValue);
            break;
        case CompoundAssignmentOperator::LEFT_SHIFT_ASSIGN:
            result = builder_->CreateShl(currentValue, rhsValue);
            break;
        case CompoundAssignmentOperator::RIGHT_SHIFT_ASSIGN:
            result = builder_->CreateAShr(currentValue, rhsValue);
            break;
        default:
            addError("Unknown compound assignment operator");
            return nullptr;
    }

    builder_->CreateStore(result, it->second);
    return result;
}

Value* LLVMCodeGenerator::generateIncrementDecrement(const IncrementDecrementExpression& expr) {
    const Expression& operand = expr.getOperand();

    std::string varName;
    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&operand)) {
        const ASTNode& primary = primaryExpr->getPrimary();
        if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
            varName = identifier->getName();
        } else {
            addError("Increment/decrement target must be an identifier");
            return nullptr;
        }
    } else {
        addError("Increment/decrement target must be an identifier");
        return nullptr;
    }

    auto it = namedValues_.find(varName);
    if (it == namedValues_.end()) {
        addError("Unknown variable: " + varName);
        return nullptr;
    }

    AllocaInst* alloca = llvm::cast<AllocaInst>(it->second);
    Value* currentValue = builder_->CreateLoad(alloca->getAllocatedType(), alloca, varName);
    Value* one = ConstantInt::get(currentValue->getType(), 1);

    Value* result = nullptr;
    if (expr.getOperator() == IncrementDecrementOperator::INCREMENT) {
        result = builder_->CreateAdd(currentValue, one);
    } else {
        result = builder_->CreateSub(currentValue, one);
    }

    builder_->CreateStore(result, it->second);
    return result;
}

std::string LLVMCodeGenerator::getExpressionClassName(const ast::Expression& expr) {
    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&expr)) {
        const ASTNode& primary = primaryExpr->getPrimary();
        if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
            auto it = variableTypes_.find(identifier->getName());
            if (it != variableTypes_.end()) {
                return it->second;
            }
        } else if (dynamic_cast<const ThisLiteral*>(&primary)) {
            auto it = variableTypes_.find("this");
            if (it != variableTypes_.end()) {
                return it->second;
            }
        }
    } else if (auto memberAccess = dynamic_cast<const MemberAccess*>(&expr)) {
        std::string parentClass = getExpressionClassName(memberAccess->getObject());
        if (parentClass.empty()) return "";
        
        auto declIt = classDeclarations_.find(parentClass);
        if (declIt == classDeclarations_.end()) return "";
        
        const ast::ClassDeclaration* classDecl = declIt->second;
        for (const auto& member : classDecl->getBody().getMembers()) {
            if (auto decl = member->getDeclaration()) {
                if (auto varMember = dynamic_cast<const ast::VariableDeclaration*>(decl)) {
                    if (varMember->getName() == memberAccess->getMember()) {
                        if (auto baseType = dynamic_cast<const ast::BaseType*>(varMember->getType())) {
                            return baseType->getIdentifier();
                        }
                    }
                }
            }
        }
    } else if (auto newExpr = dynamic_cast<const NewObjectExpression*>(&expr)) {
        if (newExpr->getQualifiedClassName()) {
            return newExpr->getQualifiedClassName()->getFullName();
        }
        return newExpr->getClassName();
    }
    return "";
}

Value* LLVMCodeGenerator::generateMemberAccess(const MemberAccess& expr) {
    // Get the object value
    Value* objectValue = generateLLVMExpression(expr.getObject());
    if (!objectValue) {
        addError("Failed to generate object expression");
        return nullptr;
    }

    // Try to determine the class type from the object using type inference
    std::string className = getExpressionClassName(expr.getObject());

    if (className.empty()) {
        addError("Cannot determine class type for member access");
        return nullptr;
    }

    // Get the class struct type
    auto classTypeIt = classTypes_.find(className);
    if (classTypeIt == classTypes_.end()) {
        addError("Unknown class type: " + className);
        return nullptr;
    }
    llvm::StructType* classType = classTypeIt->second;

    // Get the class declaration to find member info
    auto declIt = classDeclarations_.find(className);
    if (declIt == classDeclarations_.end()) {
        addError("Missing class declaration for: " + className);
        return nullptr;
    }
    const ast::ClassDeclaration* classDecl = declIt->second;

    // Find the member in the class
    const std::string& memberName = expr.getMember();
    int memberIndex = -1;
    llvm::Type* memberType = nullptr;
    int fieldIdx = 0;

    for (const auto& member : classDecl->getBody().getMembers()) {
        // Variables in class members are stored as declarations
        if (auto decl = member->getDeclaration()) {
            if (auto varMember = dynamic_cast<const ast::VariableDeclaration*>(decl)) {
                if (varMember->getName() == memberName) {
                    memberIndex = fieldIdx;
                    if (varMember->getType()) {
                        memberType = generateLLVMType(*varMember->getType());
                    }
                    break;
                }
                fieldIdx++;
            }
        }
    }

    if (memberIndex == -1) {
        addError("Member not found: " + memberName + " in class " + className);
        return nullptr;
    }

    // objectValue is a void* pointer to the object data (after the header)
    // We need to cast it to the struct type and use GEP to access the member

    // Cast void* to struct pointer using PointerType::get with address space 0
    auto structPtrType = llvm::PointerType::get(context_, 0);
    auto castPtr = builder_->CreateBitCast(objectValue, structPtrType, "struct_ptr_cast");

    // Use GEP to access the field
    auto fieldPtr = builder_->CreateStructGEP(classType, castPtr, memberIndex, "field_ptr");

    // Load the field value
    if (!memberType) {
        addError("Member type is null for: " + memberName);
        return nullptr;
    }
    auto loadedValue = builder_->CreateLoad(memberType, fieldPtr, memberName);

    return loadedValue;
}

Value* LLVMCodeGenerator::generateArrayAccess(const ArrayAccess& expr) {
    Value* arrayValue = generateLLVMExpression(expr.getArray());
    Value* indexValue = generateLLVMExpression(expr.getIndex());
    
    if (!arrayValue || !indexValue) {
        addError("Failed to generate array or index expression");
        return nullptr;
    }
    
    // Convert index to i64 if needed
    if (!indexValue->getType()->isIntegerTy(64)) {
        if (indexValue->getType()->isIntegerTy()) {
            indexValue = builder_->CreateSExt(indexValue, LLVMType::getInt64Ty(context_), "index_ext");
        } else {
            addError("Array index must be integer type");
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
        addError("Array access on non-pointer type not supported");
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

    // Push loop context for break/continue
    loopStack_.push_back({afterBlock, condBlock});

    // Jump to condition block
    builder_->CreateBr(condBlock);

    // Condition block
    builder_->SetInsertPoint(condBlock);
    Value* condValue = generateLLVMExpression(stmt.getCondition());
    if (!condValue) {
        loopStack_.pop_back();
        return;
    }

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

    // Pop loop context
    loopStack_.pop_back();

    // Continue after loop
    builder_->SetInsertPoint(afterBlock);
}

void LLVMCodeGenerator::generateForInStatement(const ForInStatement& stmt) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Evaluate the iterable expression
    Value* iterableValue = generateLLVMExpression(stmt.getIterable());
    if (!iterableValue) {
        addError("Failed to generate iterable expression for for-in loop");
        return;
    }

    // Check that we have the array runtime functions
    auto* lengthFunc = getArrayFunc("length");
    auto* getInt64Func = runtimeFunctionStorage_.arrays.hoo_array_get_int64_func;
    if (!lengthFunc || !getInt64Func) {
        addError("Array runtime functions not available for for-in loop");
        return;
    }

    // Get the actual array length by calling hoo_array_length(arr)
    Value* arrayLength = builder_->CreateCall(lengthFunc, {iterableValue}, "array.length");

    // Create loop variable - assume int64 elements for now
    // TODO: In future, query array element type and use appropriate get function
    LLVMType* elementType = LLVMType::getInt64Ty(context_);
    AllocaInst* loopVar = createEntryBlockAlloca(currentFunc, stmt.getVariable(), elementType);

    // Create index variable
    AllocaInst* indexVar = createEntryBlockAlloca(currentFunc, "for.index", LLVMType::getInt64Ty(context_));
    builder_->CreateStore(ConstantInt::get(LLVMType::getInt64Ty(context_), 0), indexVar);

    // Create blocks for loop structure
    BasicBlock* condBlock = BasicBlock::Create(context_, "for.cond", currentFunc);
    BasicBlock* bodyBlock = BasicBlock::Create(context_, "for.body", currentFunc);
    BasicBlock* incrBlock = BasicBlock::Create(context_, "for.incr", currentFunc);
    BasicBlock* endBlock = BasicBlock::Create(context_, "for.end", currentFunc);

    // Push loop context for break/continue
    loopStack_.push_back({endBlock, incrBlock});

    // Branch to condition check
    builder_->CreateBr(condBlock);

    // Condition block: check if currentIndex < arrayLength
    builder_->SetInsertPoint(condBlock);
    Value* currentIndex = builder_->CreateLoad(LLVMType::getInt64Ty(context_), indexVar, "index");
    Value* cond = builder_->CreateICmpSLT(currentIndex, arrayLength, "loop.cond");
    builder_->CreateCondBr(cond, bodyBlock, endBlock);

    // Body block: get element and execute loop body
    builder_->SetInsertPoint(bodyBlock);

    // Allocate space for the element value to be retrieved
    AllocaInst* elemDest = createEntryBlockAlloca(currentFunc, "elem.dest", elementType);

    // Call hoo_array_get_int64(arr, index, &dest)
    // Returns 1 if successful, 0 if out of bounds
    Value* getResult = builder_->CreateCall(
        getInt64Func,
        {iterableValue, currentIndex, elemDest},
        "get.result"
    );

    // Load the retrieved element value
    Value* element = builder_->CreateLoad(elementType, elemDest, "elem");

    // Store element in loop variable and add to scope
    builder_->CreateStore(element, loopVar);
    namedValues_[stmt.getVariable()] = loopVar;

    // Generate the loop body statements
    generateBlock(stmt.getBody());

    // Branch to increment
    builder_->CreateBr(incrBlock);

    // Increment block: increment index and loop back to condition
    builder_->SetInsertPoint(incrBlock);
    Value* nextIndex = builder_->CreateAdd(
        currentIndex,
        ConstantInt::get(LLVMType::getInt64Ty(context_), 1),
        "next.index"
    );
    builder_->CreateStore(nextIndex, indexVar);
    builder_->CreateBr(condBlock);

    // Pop loop context
    loopStack_.pop_back();

    // End block: clean up and continue
    builder_->SetInsertPoint(endBlock);

    // Remove loop variable from scope
    namedValues_.erase(stmt.getVariable());
}

void LLVMCodeGenerator::generateForRangeStatement(const ForRangeStatement& stmt) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Evaluate range bounds
    Value* startValue = generateLLVMExpression(stmt.getStart());
    Value* endValue = generateLLVMExpression(stmt.getEnd());
    if (!startValue || !endValue) return;

    // Evaluate optional step, default to 1
    Value* stepValue = nullptr;
    if (stmt.hasStep()) {
        stepValue = generateLLVMExpression(*stmt.getStep());
    } else {
        if (startValue->getType()->isIntegerTy()) {
            stepValue = ConstantInt::get(startValue->getType(), 1);
        } else {
            stepValue = ConstantFP::get(startValue->getType(), 1.0);
        }
    }
    if (!stepValue) return;

    // Allocate loop variable
    AllocaInst* loopVar = createEntryBlockAlloca(currentFunc, stmt.getVariable(), startValue->getType());
    builder_->CreateStore(startValue, loopVar);
    namedValues_[stmt.getVariable()] = loopVar;

    // Create blocks
    BasicBlock* condBlock = BasicBlock::Create(context_, "for.cond", currentFunc);
    BasicBlock* bodyBlock = BasicBlock::Create(context_, "for.body", currentFunc);
    BasicBlock* incBlock = BasicBlock::Create(context_, "for.inc", currentFunc);
    BasicBlock* afterBlock = BasicBlock::Create(context_, "for.end", currentFunc);

    // Push loop context for break/continue
    loopStack_.push_back({afterBlock, incBlock});

    // Jump to condition
    builder_->CreateBr(condBlock);

    // Condition block: check if loopVar reaches end
    builder_->SetInsertPoint(condBlock);
    Value* currentVal = builder_->CreateLoad(loopVar->getAllocatedType(), loopVar, stmt.getVariable());
    
    // Logic: if step > 0, condition is current < end
    //        if step < 0, condition is current > end
    Value* isPositiveStep;
    if (stepValue->getType()->isIntegerTy()) {
        isPositiveStep = builder_->CreateICmpSGT(stepValue, ConstantInt::get(stepValue->getType(), 0), "is_pos_step");
    } else {
        isPositiveStep = builder_->CreateFCmpOGT(stepValue, ConstantFP::get(stepValue->getType(), 0.0), "is_pos_step");
    }

    Value* condValue;
    if (loopVar->getAllocatedType()->isIntegerTy()) {
        Value* posCond = builder_->CreateICmpSLT(currentVal, endValue, "pos_cond");
        Value* negCond = builder_->CreateICmpSGT(currentVal, endValue, "neg_cond");
        condValue = builder_->CreateSelect(isPositiveStep, posCond, negCond, "forcond");
    } else {
        Value* posCond = builder_->CreateFCmpOLT(currentVal, endValue, "pos_cond");
        Value* negCond = builder_->CreateFCmpOGT(currentVal, endValue, "neg_cond");
        condValue = builder_->CreateSelect(isPositiveStep, posCond, negCond, "forcond");
    }

    builder_->CreateCondBr(condValue, bodyBlock, afterBlock);

    // Body block
    builder_->SetInsertPoint(bodyBlock);
    generateBlock(stmt.getBody());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(incBlock);
    }

    // Increment block: currentVal + stepValue
    builder_->SetInsertPoint(incBlock);
    Value* curVal = builder_->CreateLoad(loopVar->getAllocatedType(), loopVar, stmt.getVariable());
    Value* nextVal;
    if (loopVar->getAllocatedType()->isIntegerTy()) {
        nextVal = builder_->CreateAdd(curVal, stepValue, "nextval");
    } else {
        nextVal = builder_->CreateFAdd(curVal, stepValue, "nextval");
    }
    builder_->CreateStore(nextVal, loopVar);
    builder_->CreateBr(condBlock);

    // Pop loop context
    loopStack_.pop_back();

    // Continue after loop
    builder_->SetInsertPoint(afterBlock);
    namedValues_.erase(stmt.getVariable());
}

void LLVMCodeGenerator::generateScopeStatement(const ScopeStatement& stmt) {
    generateBlock(stmt.getBody());
}

void LLVMCodeGenerator::generateBreakStatement(const BreakStatement& stmt) {
    if (loopStack_.empty()) {
        addError("break statement outside of loop");
        return;
    }
    builder_->CreateBr(loopStack_.back().breakBlock);
}

void LLVMCodeGenerator::generateContinueStatement(const ContinueStatement& stmt) {
    if (loopStack_.empty()) {
        addError("continue statement outside of loop");
        return;
    }
    builder_->CreateBr(loopStack_.back().continueBlock);
}

void LLVMCodeGenerator::generateTryCatchStatement(const TryCatchStatement& stmt) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();
    LLVMType* i8PtrTy = llvm::PointerType::get(context_, 0);

    BasicBlock* tryEntryBlock = BasicBlock::Create(context_, "try.entry", currentFunc);
    BasicBlock* catchEntryBlock = BasicBlock::Create(context_, "catch.entry", currentFunc);
    BasicBlock* finallyBlock = nullptr;
    BasicBlock* mergeBlock = BasicBlock::Create(context_, "try.end", currentFunc);

    builder_->CreateBr(tryEntryBlock);
    builder_->SetInsertPoint(tryEntryBlock);

    Value* exnPtr = builder_->CreateAlloca(i8PtrTy, nullptr, "exn.save");
    builder_->CreateStore(Constant::getNullValue(i8PtrTy), exnPtr);

    generateBlock(stmt.getTryBlock());

    if (!builder_->GetInsertBlock()->getTerminator()) {
        if (stmt.hasFinally()) {
            finallyBlock = BasicBlock::Create(context_, "finally.entry", currentFunc);
            builder_->CreateBr(finallyBlock);
            builder_->SetInsertPoint(finallyBlock);
            generateBlock(*stmt.getFinallyBlock());
            if (!builder_->GetInsertBlock()->getTerminator()) {
                builder_->CreateBr(mergeBlock);
            }
        } else {
            builder_->CreateBr(mergeBlock);
        }
    }

    if (stmt.hasCatch()) {
        builder_->SetInsertPoint(catchEntryBlock);

        Value* caughtExn = builder_->CreateLoad(i8PtrTy, exnPtr, "caught.exn");

        auto* getTypeIdFunc = getExceptionFunc("get_type_id");
        if (!getTypeIdFunc) {
            addError("hoo_exception_get_type_id not available");
            return;
        }

        BasicBlock* nextCatchBlock = mergeBlock;
        for (size_t i = stmt.getCatchClauses().size(); i > 0; i--) {
            const auto& catchClause = stmt.getCatchClauses()[i - 1];
            BasicBlock* currentCatchBlock = BasicBlock::Create(
                context_, "catch." + catchClause.variable, currentFunc);
            BasicBlock* typeMatchBlock = BasicBlock::Create(
                context_, "catch.type.match", currentFunc);

            Value* loadExn = builder_->CreateLoad(i8PtrTy, exnPtr, "exn");

            if (i == stmt.getCatchClauses().size()) {
                builder_->SetInsertPoint(catchEntryBlock);
            } else {
                builder_->SetInsertPoint(nextCatchBlock);
            }

            builder_->CreateBr(currentCatchBlock);
            builder_->SetInsertPoint(currentCatchBlock);

            int64_t typeId = 99;
            if (auto* primType = dynamic_cast<const PrimitiveType*>(catchClause.type.get())) {
                switch (primType->getKind()) {
                    case hooc::ast::PrimitiveTypeKind::STRING: typeId = 0; break;
                    case hooc::ast::PrimitiveTypeKind::INT64: typeId = 0; break;
                    default: typeId = 99;
                }
            }

            Value* typeIdValue = builder_->CreateCall(
                getTypeIdFunc,
                {loadExn},
                "type.id"
            );

            Value* cmp = builder_->CreateICmpEQ(
                typeIdValue,
                ConstantInt::get(LLVMType::getInt64Ty(context_), typeId),
                "cmp"
            );

            builder_->CreateCondBr(cmp, typeMatchBlock, nextCatchBlock);

            builder_->SetInsertPoint(typeMatchBlock);
            AllocaInst* exnVar = createEntryBlockAlloca(
                currentFunc, catchClause.variable, i8PtrTy);
            builder_->CreateStore(loadExn, exnVar);
            namedValues_[catchClause.variable] = exnVar;

            generateBlock(*catchClause.block);

            if (!builder_->GetInsertBlock()->getTerminator()) {
                if (stmt.hasFinally()) {
                    builder_->CreateBr(finallyBlock);
                } else {
                    builder_->CreateBr(mergeBlock);
                }
            }
            namedValues_.erase(catchClause.variable);

            nextCatchBlock = currentCatchBlock;
        }

        builder_->SetInsertPoint(nextCatchBlock);
        builder_->CreateBr(mergeBlock);
    } else {
        builder_->SetInsertPoint(catchEntryBlock);
        builder_->CreateBr(mergeBlock);
    }

    builder_->SetInsertPoint(mergeBlock);
}

void LLVMCodeGenerator::generateThrowStatement(const ThrowStatement& stmt) {
    if (stmt.isRethrow()) {
        builder_->CreateResume(nullptr);
        return;
    }

    Value* exception = generateLLVMExpression(*stmt.getExpression());
    if (!exception) {
        addError("Failed to generate throw expression");
        return;
    }

    auto* fromCstrFunc = getStringFunc("from_cstr");
    auto* createFunc = getExceptionFunc("create");
    auto* throwFunc = getExceptionFunc("throw");

    if (!fromCstrFunc || !createFunc || !throwFunc) {
        addError("Exception runtime functions not available for throw");
        return;
    }

    llvm::Value* messageValue = nullptr;
    llvm::Type* i8PtrTy = llvm::PointerType::get(context_, 0);

    if (exception->getType() == i8PtrTy) {
        messageValue = exception;
    } else if (auto* castExpr = llvm::cast< llvm::Value>(exception)) {
        messageValue = builder_->CreateBitCast(castExpr, i8PtrTy, "msg");
    } else {
        messageValue = builder_->CreateCall(
            fromCstrFunc,
            {exception},
            "msg"
        );
    }

    llvm::Value* exc = builder_->CreateCall(
        createFunc,
        {ConstantInt::get(LLVMType::getInt64Ty(context_), 0), messageValue},
        "exc"
    );

    builder_->CreateCall(throwFunc, {exc});
    builder_->CreateUnreachable();
}

void LLVMCodeGenerator::generateVariableDeclaration(const VariableDeclaration& decl) {
    Function* currentFunc = builder_->GetInsertBlock()->getParent();

    // Determine type
    LLVMType* varType;
    if (decl.hasTypeInference()) {
        // Infer type from initializer
        if (!decl.getInitializer()) {
            addError("Type inference requires initializer");
            return;
        }
        Value* initValue = generateLLVMExpression(*decl.getInitializer());
        if (!initValue) return;
        varType = initValue->getType();

        // Create alloca and store
        AllocaInst* alloca = createEntryBlockAlloca(currentFunc, decl.getName(), varType);
        builder_->CreateStore(initValue, alloca);
        namedValues_[decl.getName()] = alloca;

        // Track type for new expressions (class type inference)
        if (decl.getInitializer()) {
            if (auto newExpr = dynamic_cast<const NewObjectExpression*>(decl.getInitializer())) {
                std::string className = newExpr->getClassName();
                if (newExpr->getQualifiedClassName() && newExpr->getQualifiedClassName()->isQualified()) {
                    className = newExpr->getQualifiedClassName()->toString();
                }
                variableTypes_[decl.getName()] = className;
            }
        }
    } else {
        // Explicit type
        varType = generateLLVMType(*decl.getType());
        AllocaInst* alloca = createEntryBlockAlloca(currentFunc, decl.getName(), varType);
        namedValues_[decl.getName()] = alloca;

        // Track variable type for all named types (classes and built-in types)
        if (auto baseType = dynamic_cast<const ast::BaseType*>(decl.getType())) {
            std::string typeId;
            if (baseType->isPrimitive()) {
                typeId = primitiveTypeToString(baseType->getPrimitiveType()->getKind());
            } else {
                typeId = baseType->getIdentifier();
            }
            variableTypes_[decl.getName()] = typeId;
        } else if (dynamic_cast<const hooc::ast::ArrayType*>(decl.getType())) {
            variableTypes_[decl.getName()] = "array";
        }

        // Initialize if initializer present
        if (decl.getInitializer()) {
            Value* initValue = generateLLVMExpression(*decl.getInitializer());
            if (initValue) {
                initValue = ensureTypeMatch(initValue, varType);
                builder_->CreateStore(initValue, alloca);
            }
        }
    }
}

LLVMType* LLVMCodeGenerator::generateLLVMType(const ASTType& type) {
    if (auto optionalType = dynamic_cast<const OptionalType*>(&type)) {
        // Handle optional types with tagged union pattern
        if (optionalType->isOptional()) {
            auto valueType = generateLLVMType(optionalType->getArrayType());
            return createNullableType(valueType);
        } else {
            // Not actually optional, just return the array type
            return generateLLVMType(optionalType->getArrayType());
        }
    }

    if (auto arrayType = dynamic_cast<const ASTArrayType*>(&type)) {
        return convertArrayType(*arrayType);
    }

    if (auto mapType = dynamic_cast<const MapType*>(&type)) {
        (void)mapType;  // Map is represented as pointer to HooMap
        return llvm::PointerType::get(context_, 0);
    }

    if (auto baseType = dynamic_cast<const BaseType*>(&type)) {
        if (baseType->isPrimitive()) {
            return convertPrimitiveType(baseType->getPrimitiveType()->getKind());
        } else {
            // Custom type - treat as opaque pointer
            return llvm::PointerType::get(context_, 0);
        }
    }

    // Default to void
    return LLVMType::getVoidTy(context_);
}

LLVMType* LLVMCodeGenerator::convertPrimitiveType(PrimitiveTypeKind kind) {
    switch (kind) {
        case PrimitiveTypeKind::INT8:
        case PrimitiveTypeKind::BYTE:
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
            addError("Fixed-size array syntax no longer supported. Use array literals.");
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
        // Empty array - return null pointer
        // Note: In Phase 5, this should call hoo_*_array_new(0) with proper type inference
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(context_, 0));
    }

    const auto& expressions = elementsList->getExpressions();
    std::vector<Constant*> constantElements;
    std::vector<Value*> dynamicElements;
    LLVMType* elementType = nullptr;
    bool allConstant = true;

    // Evaluate all elements and infer type from first element
    for (const auto& expr : expressions) {
        Value* elemValue = generateLLVMExpression(*expr);
        if (!elemValue) {
            addError("Failed to generate array element expression");
            return nullptr;
        }

        // Infer element type from first element
        if (elementType == nullptr) {
            elementType = elemValue->getType();
        } else if (elemValue->getType() != elementType) {
            addError("Array literal elements must have uniform type");
            return nullptr;
        }

        // Check if element is a compile-time constant
        Constant* constElem = llvm::dyn_cast<Constant>(elemValue);
        if (!constElem) {
            allConstant = false;
            dynamicElements.push_back(elemValue);
        } else {
            constantElements.push_back(constElem);
            dynamicElements.push_back(elemValue);
        }
    }

    // Phase 6: Handle pointer-type arrays (classes) with dynamic construction
    if (!allConstant) {
        // For non-constant elements (which are typically object pointers)
        if (elementType->isPointerTy()) {
            // Use dynamic array construction for class instances
            return generateDynamicArrayLiteral(dynamicElements, elementType);
        } else {
            // Non-constant elements that aren't pointers are not supported
            addError("Array literal elements must be compile-time constants (unless they are class instances)");
            return nullptr;
        }
    }

    // Phase 4: Use generic array runtime functions instead of LLVM constant arrays
    // This creates a global data buffer and calls hoo_*_array_from_buffer()
    return generateArrayLiteralWithRuntime(constantElements, elementType);
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

llvm::Function* LLVMCodeGenerator::getArrayFromBufferFunc(llvm::Type* elementType) {
    if (elementType->isIntegerTy(64)) {
        // Check if already declared
        if (auto* f = module_->getFunction("hoo_int64_array_from_buffer")) {
            return f;
        }
        // Declare hoo_int64_array_from_buffer
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),  // data pointer
            LLVMType::getInt64Ty(context_)         // length
        };
        FunctionType* funcType = FunctionType::get(llvm::PointerType::get(context_, 0), params, false);
        return Function::Create(funcType, Function::ExternalLinkage, "hoo_int64_array_from_buffer", module_);
    } else if (elementType->isDoubleTy()) {
        // Check if already declared
        if (auto* f = module_->getFunction("hoo_double_array_from_buffer")) {
            return f;
        }
        // Declare hoo_double_array_from_buffer
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),  // data pointer
            LLVMType::getInt64Ty(context_)         // length
        };
        FunctionType* funcType = FunctionType::get(llvm::PointerType::get(context_, 0), params, false);
        return Function::Create(funcType, Function::ExternalLinkage, "hoo_double_array_from_buffer", module_);
    } else if (elementType->isIntegerTy(1)) { // bool
        if (auto* f = module_->getFunction("hoo_bool_array_from_buffer")) {
            return f;
        }
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),
            LLVMType::getInt64Ty(context_)
        };
        FunctionType* funcType = FunctionType::get(llvm::PointerType::get(context_, 0), params, false);
        return Function::Create(funcType, Function::ExternalLinkage, "hoo_bool_array_from_buffer", module_);
    } else if (elementType->isIntegerTy(32)) { // char (Unicode)
        if (auto* f = module_->getFunction("hoo_char_array_from_buffer")) {
            return f;
        }
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),
            LLVMType::getInt64Ty(context_)
        };
        FunctionType* funcType = FunctionType::get(llvm::PointerType::get(context_, 0), params, false);
        return Function::Create(funcType, Function::ExternalLinkage, "hoo_char_array_from_buffer", module_);
    }

    addError("No array from_buffer function for element type");
    return nullptr;
}

llvm::Value* LLVMCodeGenerator::generateArrayLiteralWithRuntime(
    const std::vector<llvm::Constant*>& elements,
    llvm::Type* elementType) {

    if (elements.empty()) {
        // Return null for empty arrays - should ideally call hoo_*_array_new(0) instead
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(context_, 0));
    }

    // Create array type [N x elementType]
    auto arrayType = llvm::ArrayType::get(elementType, elements.size());

    // Create constant array initializer
    auto arrayInit = llvm::ConstantArray::get(arrayType, elements);

    // Create global variable for the array data (buffer)
    auto globalData = new llvm::GlobalVariable(
        *module_,
        arrayType,
        true,  // isConstant
        llvm::GlobalValue::PrivateLinkage,
        arrayInit,
        ".array_data"
    );

    // Create GEP to get pointer to first element
    std::vector<llvm::Constant*> indices = {
        llvm::ConstantInt::get(LLVMType::getInt64Ty(context_), 0),
        llvm::ConstantInt::get(LLVMType::getInt64Ty(context_), 0)
    };

    auto dataPtr = llvm::ConstantExpr::getGetElementPtr(
        arrayType,
        globalData,
        indices
    );

    // Get the array creation function for this element type
    auto arrayFunc = getArrayFromBufferFunc(elementType);
    if (!arrayFunc) {
        addError("No array creation function for element type");
        return nullptr;
    }

    // Create call: hoo_*_array_from_buffer(dataPtr, length)
    auto lengthConst = llvm::ConstantInt::get(
        LLVMType::getInt64Ty(context_),
        elements.size()
    );

    std::vector<llvm::Value*> args = {dataPtr, lengthConst};

    return builder_->CreateCall(arrayFunc, args, "hoo_arr");
}

// ============================================================================
// Phase 6: Dynamic Array Construction for Pointer Types (Class Instances)
// ============================================================================

llvm::Value* LLVMCodeGenerator::generateDynamicArrayLiteral(
    const std::vector<llvm::Value*>& elements,
    llvm::Type* elementType) {

    if (elements.empty()) {
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(context_, 0));
    }

    // Phase 7: New API - hoo_array_new() takes no parameters
    auto arrayNewFunc = getArrayNewFunc(0);  // elementSize parameter ignored
    if (!arrayNewFunc) {
        addError("Failed to declare hoo_array_new function");
        return nullptr;
    }

    // Create empty array with no parameters
    std::vector<llvm::Value*> newArgs;  // No arguments
    llvm::Value* arrayHandle = builder_->CreateCall(arrayNewFunc, newArgs, "hoo_arr_new");

    // Get the type-specific push function for this element type
    auto arrayPushFunc = getArrayPushFuncForType(elementType);
    if (!arrayPushFunc) {
        addError("Failed to get type-specific array push function");
        return nullptr;
    }

    // Push each element into the array using the type-specific function
    for (const auto& elem : elements) {
        std::vector<llvm::Value*> pushArgs = {arrayHandle, elem};
        builder_->CreateCall(arrayPushFunc, pushArgs);
    }

    return arrayHandle;
}

llvm::Function* LLVMCodeGenerator::getArrayNewFunc(size_t elementSize) {
    // Check if already declared
    if (auto* f = module_->getFunction("hoo_array_new")) {
        return f;
    }
    // Phase 7: New API - hoo_array_new(void) with no parameters
    // elementSize parameter is now ignored - the array uses std::any internally
    std::vector<LLVMType*> params;  // No parameters
    FunctionType* funcType = FunctionType::get(
        llvm::PointerType::get(context_, 0),  // return HooArray (void*)
        params,
        false
    );
    return Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_array_new",
        module_
    );
}

// ============================================================================
// Phase 7: Type-Specific Array Push Function Getters
// ============================================================================

llvm::Function* LLVMCodeGenerator::getArrayPushInt64Func() {
    // Check if already declared
    if (auto* f = module_->getFunction("hoo_array_push_int64")) {
        return f;
    }
    std::vector<LLVMType*> params = {
        llvm::PointerType::get(context_, 0),  // HooArray
        LLVMType::getInt64Ty(context_)        // int64_t value
    };
    FunctionType* funcType = FunctionType::get(
        LLVMType::getInt64Ty(context_),       // return int64_t (new length)
        params,
        false
    );
    return Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_array_push_int64",
        module_
    );
}

llvm::Function* LLVMCodeGenerator::getArrayPushDoubleFunc() {
    // Check if already declared
    if (auto* f = module_->getFunction("hoo_array_push_double")) {
        return f;
    }
    std::vector<LLVMType*> params = {
        llvm::PointerType::get(context_, 0),  // HooArray
        LLVMType::getDoubleTy(context_)       // double value
    };
    FunctionType* funcType = FunctionType::get(
        LLVMType::getInt64Ty(context_),       // return int64_t (new length)
        params,
        false
    );
    return Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_array_push_double",
        module_
    );
}

llvm::Function* LLVMCodeGenerator::getArrayPushFloatFunc() {
    // Check if already declared
    if (auto* f = module_->getFunction("hoo_array_push_float")) {
        return f;
    }
    std::vector<LLVMType*> params = {
        llvm::PointerType::get(context_, 0),  // HooArray
        LLVMType::getFloatTy(context_)        // float value
    };
    FunctionType* funcType = FunctionType::get(
        LLVMType::getInt64Ty(context_),       // return int64_t (new length)
        params,
        false
    );
    return Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_array_push_float",
        module_
    );
}

llvm::Function* LLVMCodeGenerator::getArrayPushBoolFunc() {
    // Check if already declared
    if (auto* f = module_->getFunction("hoo_array_push_bool")) {
        return f;
    }
    std::vector<LLVMType*> params = {
        llvm::PointerType::get(context_, 0),  // HooArray
        LLVMType::getInt64Ty(context_)        // int64_t bool (0 or 1)
    };
    FunctionType* funcType = FunctionType::get(
        LLVMType::getInt64Ty(context_),       // return int64_t (new length)
        params,
        false
    );
    return Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_array_push_bool",
        module_
    );
}

llvm::Function* LLVMCodeGenerator::getArrayPushCharFunc() {
    // Check if already declared
    if (auto* f = module_->getFunction("hoo_array_push_char")) {
        return f;
    }
    std::vector<LLVMType*> params = {
        llvm::PointerType::get(context_, 0),  // HooArray
        LLVMType::getInt8Ty(context_)         // char value (i8)
    };
    FunctionType* funcType = FunctionType::get(
        LLVMType::getInt64Ty(context_),       // return int64_t (new length)
        params,
        false
    );
    return Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_array_push_char",
        module_
    );
}

llvm::Function* LLVMCodeGenerator::getArrayPushObjectFunc() {
    // Check if already declared
    if (auto* f = module_->getFunction("hoo_array_push_object")) {
        return f;
    }
    std::vector<LLVMType*> params = {
        llvm::PointerType::get(context_, 0),  // HooArray
        llvm::PointerType::get(context_, 0)   // void* (object pointer)
    };
    FunctionType* funcType = FunctionType::get(
        LLVMType::getInt64Ty(context_),       // return int64_t (new length)
        params,
        false
    );
    return Function::Create(
        funcType,
        Function::ExternalLinkage,
        "hoo_array_push_object",
        module_
    );
}

llvm::Function* LLVMCodeGenerator::getArrayPushFuncForType(llvm::Type* elementType) {
    if (!elementType) {
        return getArrayPushObjectFunc();  // Default to object pointer
    }

    if (elementType == LLVMType::getInt64Ty(context_)) {
        return getArrayPushInt64Func();
    } else if (elementType == LLVMType::getDoubleTy(context_)) {
        return getArrayPushDoubleFunc();
    } else if (elementType == LLVMType::getFloatTy(context_)) {
        return getArrayPushFloatFunc();
    } else if (elementType == LLVMType::getInt1Ty(context_)) {
        return getArrayPushBoolFunc();
    } else if (elementType == LLVMType::getInt8Ty(context_)) {
        return getArrayPushCharFunc();
    } else if (elementType->isPointerTy()) {
        // For now, treat all pointers as object pointers
        // Could refine this to distinguish between strings and objects
        return getArrayPushObjectFunc();
    } else {
        // Unknown type - default to object
        return getArrayPushObjectFunc();
    }
}

void LLVMCodeGenerator::generateGlobalVariable(const VariableDeclaration& decl) {
    // Determine type
    LLVMType* varType;
    if (decl.hasTypeInference()) {
        if (!decl.getInitializer()) {
            addError("Global type inference requires initializer");
            return;
        }

        // Try to evaluate as constant first
        Value* initValue = nullptr;
        
        // Simple heuristic: if it's a literal, try to generate it
        if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(decl.getInitializer())) {
            const auto& primary = primaryExpr->getPrimary();
            if (dynamic_cast<const IntegerLiteral*>(&primary) ||
                dynamic_cast<const FloatingLiteral*>(&primary) ||
                dynamic_cast<const BooleanLiteral*>(&primary) ||
                dynamic_cast<const CharacterLiteral*>(&primary)) {
                initValue = generateLLVMExpression(*decl.getInitializer());
            }
        }

        if (initValue && llvm::isa<Constant>(initValue)) {
            varType = initValue->getType();
            auto* globalVar = new GlobalVariable(
                *module_,
                varType,
                false, // isConstant
                GlobalValue::ExternalLinkage,
                llvm::cast<Constant>(initValue),
                decl.getName()
            );
            namedValues_[decl.getName()] = globalVar;
        } else {
            // Must defer. But we need a type! 
            // For now, if we can't infer type statically, we might need a placeholder or just fail.
            // But usually we can infer from literals.
            // If it's an array literal or string literal, we know the type.
            if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(decl.getInitializer())) {
                const auto& primary = primaryExpr->getPrimary();
                if (dynamic_cast<const ast::StringLiteral*>(&primary)) {
                    varType = llvm::PointerType::get(context_, 0);
                } else if (dynamic_cast<const ArrayLiteral*>(&primary)) {
                    varType = llvm::PointerType::get(context_, 0);
                } else {
                    addError("Global variable '" + decl.getName() + "' has complex initializer that prevents type inference");
                    return;
                }
            } else {
                addError("Global variable '" + decl.getName() + "' has complex initializer that prevents type inference");
                return;
            }

            auto* globalVar = new GlobalVariable(
                *module_,
                varType,
                false, // isConstant
                GlobalValue::ExternalLinkage,
                Constant::getNullValue(varType),
                decl.getName()
            );
            namedValues_[decl.getName()] = globalVar;
            deferredInitializers_.push_back({globalVar, decl.getInitializer()});
        }
    } else {
        varType = generateLLVMType(*decl.getType());
        
        Constant* constInit = nullptr;
        if (decl.getInitializer()) {
            // Try to evaluate as constant first
            Value* initValue = nullptr;
            if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(decl.getInitializer())) {
                const auto& primary = primaryExpr->getPrimary();
                if (dynamic_cast<const IntegerLiteral*>(&primary) ||
                    dynamic_cast<const FloatingLiteral*>(&primary) ||
                    dynamic_cast<const BooleanLiteral*>(&primary) ||
                    dynamic_cast<const CharacterLiteral*>(&primary)) {
                    initValue = generateLLVMExpression(*decl.getInitializer());
                }
            }

            if (initValue && llvm::isa<Constant>(initValue)) {
                constInit = llvm::cast<Constant>(ensureTypeMatch(initValue, varType));
            }
        }

        if (!constInit && !decl.getInitializer()) {
            constInit = Constant::getNullValue(varType);
        }

        auto* globalVar = new GlobalVariable(
            *module_,
            varType,
            false, // isConstant
            GlobalValue::ExternalLinkage,
            constInit ? constInit : Constant::getNullValue(varType),
            decl.getName()
        );
        namedValues_[decl.getName()] = globalVar;

        if (!constInit && decl.getInitializer()) {
            deferredInitializers_.push_back({globalVar, decl.getInitializer()});
        }

        // Track type info
        if (auto baseType = dynamic_cast<const ast::BaseType*>(decl.getType())) {
            std::string typeId;
            if (baseType->isPrimitive()) {
                typeId = primitiveTypeToString(baseType->getPrimitiveType()->getKind());
            } else {
                typeId = baseType->getIdentifier();
            }
            variableTypes_[decl.getName()] = typeId;
        }
    }
}

void LLVMCodeGenerator::generateConstantDeclaration(const VariableDeclaration& decl) {
    // Determine type
    LLVMType* varType;
    if (decl.hasTypeInference()) {
        if (!decl.getInitializer()) {
            addError("Constant type inference requires initializer");
            return;
        }

        // Try to evaluate as constant first
        Value* initValue = nullptr;
        if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(decl.getInitializer())) {
            const auto& primary = primaryExpr->getPrimary();
            if (dynamic_cast<const IntegerLiteral*>(&primary) ||
                dynamic_cast<const FloatingLiteral*>(&primary) ||
                dynamic_cast<const BooleanLiteral*>(&primary) ||
                dynamic_cast<const CharacterLiteral*>(&primary)) {
                initValue = generateLLVMExpression(*decl.getInitializer());
            }
        }

        if (initValue && llvm::isa<Constant>(initValue)) {
            varType = initValue->getType();
            auto* globalVar = new GlobalVariable(
                *module_,
                varType,
                true,  // isConstant
                GlobalValue::ExternalLinkage,
                llvm::cast<Constant>(initValue),
                decl.getName()
            );
            namedValues_[decl.getName()] = globalVar;
        } else {
            // Must defer.
            if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(decl.getInitializer())) {
                const auto& primary = primaryExpr->getPrimary();
                if (dynamic_cast<const ast::StringLiteral*>(&primary)) {
                    varType = llvm::PointerType::get(context_, 0);
                } else if (dynamic_cast<const ArrayLiteral*>(&primary)) {
                    varType = llvm::PointerType::get(context_, 0);
                } else {
                    addError("Constant '" + decl.getName() + "' has complex initializer that prevents type inference");
                    return;
                }
            } else {
                addError("Constant '" + decl.getName() + "' has complex initializer that prevents type inference");
                return;
            }

            auto* globalVar = new GlobalVariable(
                *module_,
                varType,
                true,  // isConstant
                GlobalValue::ExternalLinkage,
                Constant::getNullValue(varType),
                decl.getName()
            );
            namedValues_[decl.getName()] = globalVar;
            deferredInitializers_.push_back({globalVar, decl.getInitializer()});
        }
    } else {
        varType = generateLLVMType(*decl.getType());
        
        if (!decl.getInitializer()) {
            addError("Constant '" + decl.getName() + "' must have an initializer");
            return;
        }

        // Try to evaluate as constant first
        Value* initValue = nullptr;
        if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(decl.getInitializer())) {
            const auto& primary = primaryExpr->getPrimary();
            if (dynamic_cast<const IntegerLiteral*>(&primary) ||
                dynamic_cast<const FloatingLiteral*>(&primary) ||
                dynamic_cast<const BooleanLiteral*>(&primary) ||
                dynamic_cast<const CharacterLiteral*>(&primary)) {
                initValue = generateLLVMExpression(*decl.getInitializer());
            }
        }

        Constant* constInit = nullptr;
        if (initValue && llvm::isa<Constant>(initValue)) {
            constInit = llvm::cast<Constant>(ensureTypeMatch(initValue, varType));
        }

        auto* globalVar = new GlobalVariable(
            *module_,
            varType,
            true,  // isConstant
            GlobalValue::ExternalLinkage,
            constInit ? constInit : Constant::getNullValue(varType),
            decl.getName()
        );
        namedValues_[decl.getName()] = globalVar;

        if (!constInit) {
            deferredInitializers_.push_back({globalVar, decl.getInitializer()});
        }

        // Track type info for type inference
        if (auto baseType = dynamic_cast<const ast::BaseType*>(decl.getType())) {
            std::string typeId;
            if (baseType->isPrimitive()) {
                typeId = primitiveTypeToString(baseType->getPrimitiveType()->getKind());
            } else {
                typeId = baseType->getIdentifier();
            }
            variableTypes_[decl.getName()] = typeId;
        }
    }
}

void LLVMCodeGenerator::generateModuleInitializer() {
    if (deferredInitializers_.empty()) return;

    // Create __hoo_init function
    FunctionType* initFuncType = FunctionType::get(LLVMType::getVoidTy(context_), false);
    Function* initFunc = Function::Create(
        initFuncType,
        Function::InternalLinkage,
        "__hoo_init",
        module_
    );

    BasicBlock* entry = BasicBlock::Create(context_, "entry", initFunc);
    
    // Save current builder state
    auto* oldBlock = builder_->GetInsertBlock();
    auto oldIP = builder_->GetInsertPoint();
    
    builder_->SetInsertPoint(entry);

    for (const auto& deferred : deferredInitializers_) {
        Value* initVal = generateLLVMExpression(*deferred.initializer);
        if (initVal) {
            initVal = ensureTypeMatch(initVal, deferred.target->getValueType());
            builder_->CreateStore(initVal, deferred.target);
        }
    }

    builder_->CreateRetVoid();

    // Restore builder state
    if (oldBlock) {
        builder_->SetInsertPoint(oldBlock, oldIP);
    } else {
        builder_->ClearInsertionPoint();
    }

    // Add to llvm.global_ctors
    // struct { i32, void()*, i8* }
    llvm::StructType* ctorStructTy = llvm::StructType::get(
        context_,
        {LLVMType::getInt32Ty(context_), 
         llvm::PointerType::get(context_, 0),
         llvm::PointerType::get(context_, 0)}
    );

    Constant* ctorEntry = ConstantStruct::get(
        ctorStructTy,
        {ConstantInt::get(LLVMType::getInt32Ty(context_), 65535),
         initFunc,
         ConstantPointerNull::get(llvm::PointerType::get(context_, 0))}
    );

    std::vector<Constant*> ctors = {ctorEntry};
    auto* ctorsArrayTy = llvm::ArrayType::get(ctorStructTy, ctors.size());
    
    new GlobalVariable(
        *module_,
        ctorsArrayTy,
        false,
        GlobalValue::AppendingLinkage,
        ConstantArray::get(ctorsArrayTy, ctors),
        "llvm.global_ctors"
    );
}

void LLVMCodeGenerator::generateVariableDeclarationStatement(const VariableDeclarationStatement& stmt) {
    generateVariableDeclaration(stmt.getDeclaration());
}

AllocaInst* LLVMCodeGenerator::createEntryBlockAlloca(Function* function, const std::string& varName, LLVMType* type) {
    IRBuilder<> tmpBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, varName);
}

// Nullable type helpers (tagged union pattern: { i1 flag, T value })
llvm::StructType* LLVMCodeGenerator::createNullableType(llvm::Type* valueType) {
    // Create a struct type: { i1 isNull, T value }
    std::vector<llvm::Type*> elements = {
        llvm::Type::getInt1Ty(context_),  // isNull flag
        valueType                          // value
    };

    // Generate a name for the nullable type
    std::string typeName = "nullable";
    if (valueType->isIntegerTy()) {
        typeName += "_i" + std::to_string(valueType->getIntegerBitWidth());
    } else if (valueType->isDoubleTy()) {
        typeName += "_f64";
    } else if (valueType->isFloatTy()) {
        typeName += "_f32";
    } else if (valueType->isPointerTy()) {
        typeName += "_ptr";
    } else if (auto structTy = llvm::dyn_cast<llvm::StructType>(valueType)) {
        if (!structTy->getName().empty()) {
            typeName += "_" + std::string(structTy->getName());
        } else {
            typeName += "_struct";
        }
    } else {
        typeName += "_unknown";
    }

    // Reuse existing type if available in current context
    if (auto* existingType = llvm::StructType::getTypeByName(context_, typeName)) {
        return existingType;
    }

    return llvm::StructType::create(context_, elements, typeName);
}

llvm::Value* LLVMCodeGenerator::createNullValue(llvm::Type* valueType) {
    // Create a null value: { i1 true, undef }
    auto nullableType = createNullableType(valueType);
    auto nullStruct = llvm::ConstantAggregateZero::get(nullableType);

    // Set the isNull flag to true
    std::vector<unsigned> indices = {0};
    return llvm::ConstantStruct::get(nullableType, {
        llvm::ConstantInt::getTrue(context_),
        llvm::UndefValue::get(valueType)
    });
}

llvm::Value* LLVMCodeGenerator::wrapValueInNullable(llvm::Value* value, llvm::Type* nullableType) {
    // Wrap a value in the nullable type: { i1 false, value }
    auto structType = llvm::dyn_cast<llvm::StructType>(nullableType);
    if (!structType) return nullptr;

    // Create the struct with isNull=false and the provided value
    llvm::Value* nullableValue = llvm::UndefValue::get(nullableType);
    nullableValue = builder_->CreateInsertValue(nullableValue,
                                                llvm::ConstantInt::getFalse(context_), 0);
    nullableValue = builder_->CreateInsertValue(nullableValue, value, 1);
    return nullableValue;
}

llvm::Value* LLVMCodeGenerator::extractValueFromNullable(llvm::Value* nullableValue) {
    // Extract the value (second field) from nullable type
    return builder_->CreateExtractValue(nullableValue, 1);
}

llvm::Value* LLVMCodeGenerator::extractNullFlagFromNullable(llvm::Value* nullableValue) {
    // Extract the isNull flag (first field) from nullable type
    return builder_->CreateExtractValue(nullableValue, 0);
}

bool LLVMCodeGenerator::isTypeNullable(const ast::Type& type) {
    if (auto optionalType = dynamic_cast<const ast::OptionalType*>(&type)) {
        return optionalType->isOptional();
    }
    return false;
}

// ============================================================================
// Runtime function declarations for reference counting
// ============================================================================

void LLVMCodeGenerator::declareRuntimeFunctions() {
    // Declare hoo_alloc: void* hoo_alloc(size_t size, int64_t type_id)
    if (!hoo_alloc_func_) {
        std::vector<LLVMType*> allocParams = {
            LLVMType::getInt64Ty(context_),  // size_t size
            LLVMType::getInt64Ty(context_)   // int64_t type_id
        };
        FunctionType* allocType = FunctionType::get(
            llvm::PointerType::get(context_, 0),  // returns void*
            allocParams,
            false
        );
        hoo_alloc_func_ = Function::Create(
            allocType,
            Function::ExternalLinkage,
            "hoo_alloc",
            module_
        );
    }

    // Declare hoo_retain: void* hoo_retain(void* obj)
    if (!hoo_retain_func_) {
        std::vector<LLVMType*> retainParams = {
            llvm::PointerType::get(context_, 0)  // void* obj
        };
        FunctionType* retainType = FunctionType::get(
            llvm::PointerType::get(context_, 0),  // returns void*
            retainParams,
            false
        );
        hoo_retain_func_ = Function::Create(
            retainType,
            Function::ExternalLinkage,
            "hoo_retain",
            module_
        );
    }

    // Declare hoo_release: void hoo_release(void* obj)
    if (!hoo_release_func_) {
        std::vector<LLVMType*> releaseParams = {
            llvm::PointerType::get(context_, 0)  // void* obj
        };
        FunctionType* releaseType = FunctionType::get(
            LLVMType::getVoidTy(context_),  // returns void
            releaseParams,
            false
        );
        hoo_release_func_ = Function::Create(
            releaseType,
            Function::ExternalLinkage,
            "hoo_release",
            module_
        );
    }

    // ========================================================================
    // Invoke Registry Callbacks to Declare Runtime Functions
    // ========================================================================
    // Call all registered runtime libraries to declare their LLVM functions
    // and populate the runtimeFunctionStorage_.
    // This replaces the manual declareRuntimeFunctions() approach.

    // Force String, Array, IO, Math, and Net runtimes to be linked and registered (works around linker optimization)
    extern void _hoo_string_ensure_registration();
    extern void _hoo_array_ensure_registration();
    extern void _hoo_io_ensure_registration();
    extern void _hoo_math_ensure_registration();
    extern void _hoo_net_ensure_registration();
    _hoo_string_ensure_registration();
    _hoo_array_ensure_registration();
    _hoo_io_ensure_registration();
    _hoo_math_ensure_registration();
    _hoo_net_ensure_registration();

    auto& registry = runtime::RuntimeRegistry::getInstance();
    registry.declareAllFunctions(*module_, context_, &runtimeFunctionStorage_);
}

// ============================================================================
// Auto-Generated Operator Dispatch Methods
// ============================================================================
// These are generated from RUNTIME_CLASSES registry. Each runtime class gets
// a tryMxxxOperator() method that handles binary operators for that class.
// TODO: Implement auto-generation when __VA_ARGS__ parameter handling is fixed

Value* LLVMCodeGenerator::tryStringOperator(
    ASTBinaryOperator op, Value* left, Value* right) {
    // Check if both operands are string pointers
    if (!left->getType()->isPointerTy() || !right->getType()->isPointerTy()) {
        return nullptr;
    }

    // Ensure string functions are declared
    declareRuntimeFunctions();

    // Dispatch to appropriate operator
    switch (op) {
        case ASTBinaryOperator::PLUS:
            if (auto* f = getStringFunc("concat")) {
                return builder_->CreateCall(f, {left, right}, "concat_result");
            }
            return nullptr;

        case ASTBinaryOperator::EQUALS:
            if (auto* f = getStringFunc("equals")) {
                Value* result = builder_->CreateCall(f, {left, right}, "equals_result");
                return builder_->CreateICmpNE(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }
            return nullptr;

        case ASTBinaryOperator::NOT_EQUALS:
            if (auto* f = getStringFunc("equals")) {
                Value* result = builder_->CreateCall(f, {left, right}, "neq_result");
                return builder_->CreateICmpEQ(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }
            return nullptr;

        case ASTBinaryOperator::LESS:
            if (auto* f = getStringFunc("compare")) {
                Value* result = builder_->CreateCall(f, {left, right}, "cmp_result");
                return builder_->CreateICmpSLT(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }
            return nullptr;

        case ASTBinaryOperator::LESS_EQUALS:
            if (auto* f = getStringFunc("compare")) {
                Value* result = builder_->CreateCall(f, {left, right}, "cmp_result");
                return builder_->CreateICmpSLE(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }
            return nullptr;

        case ASTBinaryOperator::GREATER:
            if (auto* f = getStringFunc("compare")) {
                Value* result = builder_->CreateCall(f, {left, right}, "cmp_result");
                return builder_->CreateICmpSGT(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }
            return nullptr;

        case ASTBinaryOperator::GREATER_EQUALS:
            if (auto* f = getStringFunc("compare")) {
                Value* result = builder_->CreateCall(f, {left, right}, "cmp_result");
                return builder_->CreateICmpSGE(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }
            return nullptr;

        default:
            return nullptr;
    }
}

// ============================================================================
// Class type management
// ============================================================================

llvm::StructType* LLVMCodeGenerator::getOrCreateClassType(const std::string& className) {
    // Check if already created
    auto it = classTypes_.find(className);
    if (it != classTypes_.end()) {
        return it->second;
    }

    // Create a named struct type for the class (use className directly, no prefix)
    // For non-generic classes and instantiated generics, use the class/mangled name as-is
    llvm::StructType* classType = llvm::StructType::create(context_, className);
    classTypes_[className] = classType;

    // Assign a unique type ID
    classTypeIds_[className] = nextTypeId_++;

    return classType;
}

int64_t LLVMCodeGenerator::getClassTypeId(const std::string& className) {
    auto it = classTypeIds_.find(className);
    if (it != classTypeIds_.end()) {
        return it->second;
    }

    // Create type ID if doesn't exist
    getOrCreateClassType(className);
    return classTypeIds_[className];
}

void LLVMCodeGenerator::generateClassDeclaration(const ClassDeclaration& classDecl) {
    const std::string& className = classDecl.getName();

    // Store the class declaration for later use (e.g., member access)
    classDeclarations_[className] = &classDecl;

    // Get or create the class struct type
    llvm::StructType* classType = getOrCreateClassType(className);

    // Collect field types from class body
    // For now, we only look at variable declarations in the class
    std::vector<LLVMType*> fieldTypes;

    const ClassBody& body = classDecl.getBody();
    for (const auto& member : body.getMembers()) {
        if (auto decl = member->getDeclaration()) {
            if (auto varDecl = dynamic_cast<const VariableDeclaration*>(decl)) {
                if (varDecl->getType()) {
                    LLVMType* fieldType = generateLLVMType(*varDecl->getType());
                    fieldTypes.push_back(fieldType);
                }
            }
        }
    }

    // If no fields, add a dummy byte to avoid zero-sized struct
    if (fieldTypes.empty()) {
        fieldTypes.push_back(LLVMType::getInt8Ty(context_));
    }

    // Set the body of the struct type
    classType->setBody(fieldTypes);

    // Generate constructor(s)
    for (const auto& member : body.getMembers()) {
        if (member->isConstructor()) {
            generateConstructor(classDecl, *member->getConstructor());
        }
    }

    // Generate methods
    for (const auto& member : body.getMembers()) {
        if (auto decl = member->getDeclaration()) {
            if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(decl)) {
                // Generate method with implicit 'this' parameter (object pointer)
                const std::string& methodName = funcDecl->getName();
                std::string mangledName = className + "_" + methodName;

                // Create function with signature: method(void* this, ...params) -> returnType
                std::vector<llvm::Type*> paramTypes;
                paramTypes.push_back(llvm::PointerType::get(context_, 0)); // void* this

                // Add explicit parameters
                for (const auto& param : funcDecl->getParameters()) {
                    paramTypes.push_back(generateLLVMType(param->getType()));
                }

                // Get return type
                llvm::Type* returnType = funcDecl->getReturnType()
                    ? generateLLVMType(*funcDecl->getReturnType())
                    : llvm::Type::getVoidTy(context_);

                // Create function type and function
                auto funcType = llvm::FunctionType::get(returnType, paramTypes, false);
                Function* methodFunc = llvm::Function::Create(funcType,
                    llvm::Function::ExternalLinkage, mangledName, module_);

                // Generate function body
                auto saveBB = builder_->GetInsertBlock();
                auto saveFn = saveBB ? saveBB->getParent() : nullptr;

                llvm::BasicBlock* BB = llvm::BasicBlock::Create(context_, "entry", methodFunc);
                builder_->SetInsertPoint(BB);

                // Set up local scope with parameters
                namedValues_.clear();
                variableTypes_.clear();

                // Add 'this' to symbol table
                auto argIt = methodFunc->arg_begin();
                Value* thisPtr = &*argIt;
                thisPtr->setName("this");
                namedValues_["this"] = thisPtr;
                variableTypes_["this"] = className;
                ++argIt;

                for (const auto& param : funcDecl->getParameters()) {
                    auto& arg = *argIt++;
                    arg.setName(param->getName());
                    AllocaInst* alloca = createEntryBlockAlloca(methodFunc, param->getName(), arg.getType());
                    builder_->CreateStore(&arg, alloca);
                    namedValues_[param->getName()] = alloca;
                }

                // Generate method body
                if (funcDecl->getBody().getStatements().empty()) {
                    // Empty body
                    if (returnType->isVoidTy()) {
                        builder_->CreateRetVoid();
                    }
                } else {
                    generateBlock(funcDecl->getBody());

                    // Ensure method has a proper return terminator
                    if (!builder_->GetInsertBlock()->getTerminator()) {
                        if (returnType->isVoidTy()) {
                            builder_->CreateRetVoid();
                        } else {
                            // Non-void return without explicit return statement is an error
                            // For now, return a default value
                            builder_->CreateRet(Constant::getNullValue(returnType));
                        }
                    }
                }

                // Restore insertion point
                if (saveBB) {
                    builder_->SetInsertPoint(saveBB);
                }
                namedValues_.clear();
            }
        }
    }
}

void LLVMCodeGenerator::generateConstructor(const ClassDeclaration& classDecl,
                                            const ConstructorDeclaration& constructor) {
    const std::string& className = classDecl.getName();
    llvm::StructType* classType = getOrCreateClassType(className);
    int64_t typeId = getClassTypeId(className);

    // Constructor function name: ClassName_init
    std::string ctorName = className + "_init";

    // Build parameter types: first parameter is 'this' pointer, then explicit params
    std::vector<LLVMType*> paramTypes;
    paramTypes.push_back(llvm::PointerType::get(context_, 0));  // this pointer

    for (const auto& param : constructor.getParameters()) {
        LLVMType* paramType = generateLLVMType(param->getType());
        paramTypes.push_back(paramType);
    }

    // Constructor returns void (initializes in-place)
    FunctionType* ctorType = FunctionType::get(
        LLVMType::getVoidTy(context_),
        paramTypes,
        false
    );

    Function* ctorFunc = Function::Create(
        ctorType,
        Function::ExternalLinkage,
        ctorName,
        module_
    );

    // Create entry block
    BasicBlock* entryBlock = BasicBlock::Create(context_, "entry", ctorFunc);
    builder_->SetInsertPoint(entryBlock);

    // Set up parameters
    auto argIt = ctorFunc->arg_begin();
    Value* thisPtr = &*argIt;
    thisPtr->setName("this");
    ++argIt;

    // Store 'this' in namedValues for access in constructor body
    namedValues_["this"] = thisPtr;
    variableTypes_["this"] = className;

    // Add remaining parameters to symbol table
    auto paramIt = constructor.getParameters().begin();
    while (argIt != ctorFunc->arg_end() && paramIt != constructor.getParameters().end()) {
        const std::string& paramName = (*paramIt)->getName();
        argIt->setName(paramName);

        // Create alloca for parameter
        AllocaInst* alloca = createEntryBlockAlloca(ctorFunc, paramName, argIt->getType());
        builder_->CreateStore(&*argIt, alloca);
        namedValues_[paramName] = alloca;

        ++argIt;
        ++paramIt;
    }

    // Generate constructor body
    generateBlock(constructor.getBody());

    // Add return void if not already terminated
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateRetVoid();
    }

    // Clean up 'this' from symbol table
    namedValues_.erase("this");

    // Verify the function
    std::string errorStr;
    raw_string_ostream errorStream(errorStr);
    if (verifyFunction(*ctorFunc, &errorStream)) {
        addError("Constructor verification failed: " + errorStr);
        ctorFunc->eraseFromParent();
    } else {
        functions_[ctorName] = ctorFunc;
    }
}

// ============================================================================
// Object creation
// ============================================================================

Value* LLVMCodeGenerator::generateNewObjectExpression(const NewObjectExpression& newExpr) {
    const ast::QualifiedIdentifier* qualifiedName = newExpr.getQualifiedClassName();
    std::string className = newExpr.getClassName();

    // Ensure runtime functions are declared
    declareRuntimeFunctions();

    // ========================================================================
    // Try to resolve as a standard library class (qualified name or import)
    // ========================================================================

    if (qualifiedName && qualifiedName->isQualified()) {
        // This is a qualified name like hoo.String
        const ModuleExport* moduleExport = moduleRegistry_.resolveQualifiedName(qualifiedName->getComponents());
        if (moduleExport && moduleExport->kind == ModuleExport::Kind::CLASS) {
            // This is a standard library class - use runtime constructor
            return generateStdClassConstructor(*moduleExport, newExpr);
        }
    }

    // Try to resolve as an imported name (e.g., "String" after "import hoo.String")
    auto importedIt = importedNames_.find(className);
    if (importedIt != importedNames_.end()) {
        const ModuleExport* moduleExport = importedIt->second;
        if (moduleExport && moduleExport->kind == ModuleExport::Kind::CLASS) {
            // This is an imported standard library class
            return generateStdClassConstructor(*moduleExport, newExpr);
        }
    }

    // ========================================================================
    // Fall back to user-defined class
    // ========================================================================

    // Get or create class type
    llvm::StructType* classType = getOrCreateClassType(className);
    int64_t typeId = getClassTypeId(className);

    // Calculate object size using DataLayout
    // For now, use a simple size calculation
    auto& dataLayout = module_->getDataLayout();
    uint64_t objectSize = dataLayout.getTypeAllocSize(classType);

    // Call hoo_alloc(size, type_id)
    Value* sizeArg = ConstantInt::get(LLVMType::getInt64Ty(context_), objectSize);
    Value* typeIdArg = ConstantInt::get(LLVMType::getInt64Ty(context_), typeId);

    Value* rawPtr = builder_->CreateCall(hoo_alloc_func_, {sizeArg, typeIdArg}, "newobj");

    // Look for constructor and call it if exists
    std::string ctorName = className + "_init";
    Function* ctorFunc = module_->getFunction(ctorName);

    if (ctorFunc) {
        // Build constructor arguments: this + explicit args
        std::vector<Value*> ctorArgs;
        ctorArgs.push_back(rawPtr);

        // Add user-provided arguments with type conversion if needed
        if (newExpr.getArguments()) {
            const auto& argsList = newExpr.getArguments()->getArguments();
            for (size_t i = 0; i < argsList.size(); ++i) {
                Value* argValue = generateLLVMExpression(*argsList[i]);
                if (!argValue) {
                    addError("Failed to generate constructor argument");
                    return nullptr;
                }

                // Get expected parameter type (param index = arg index + 1 due to 'this' pointer)
                size_t paramIndex = i + 1;
                if (paramIndex < ctorFunc->arg_size()) {
                    llvm::Type* expectedType = ctorFunc->getFunctionType()->getParamType(paramIndex);
                    llvm::Type* actualType = argValue->getType();

                    // Convert if types don't match
                    if (actualType != expectedType) {
                        if (actualType->isIntegerTy() && expectedType->isIntegerTy()) {
                            // Integer to integer conversion
                            unsigned actualBits = actualType->getIntegerBitWidth();
                            unsigned expectedBits = expectedType->getIntegerBitWidth();

                            if (actualBits > expectedBits) {
                                // Truncate larger integer to smaller
                                argValue = builder_->CreateTrunc(argValue, expectedType);
                            } else if (actualBits < expectedBits) {
                                // Sign-extend smaller integer to larger
                                argValue = builder_->CreateSExt(argValue, expectedType);
                            }
                        } else if (actualType->isFloatingPointTy() && expectedType->isFloatingPointTy()) {
                            // Floating point to floating point conversion
                            if (actualType->isDoubleTy() && expectedType->isFloatTy()) {
                                // Convert double to float
                                argValue = builder_->CreateFPTrunc(argValue, expectedType);
                            } else if (actualType->isFloatTy() && expectedType->isDoubleTy()) {
                                // Convert float to double
                                argValue = builder_->CreateFPExt(argValue, expectedType);
                            }
                        }
                    }
                }

                ctorArgs.push_back(argValue);
            }
        }

        // Call constructor
        builder_->CreateCall(ctorFunc, ctorArgs);
    }

    return rawPtr;
}

// ============================================================================
// Standard Library Class Constructor Generation
// ============================================================================

llvm::Value* LLVMCodeGenerator::generateStdClassConstructor(const ModuleExport& moduleExport,
                                                            const ast::NewObjectExpression& newExpr) {
    // Dispatch to specific constructor based on runtime class name
    if (moduleExport.runtimeClassName == "HooString") {
        return generateStringConstructor(newExpr);
    } else if (moduleExport.runtimeClassName == "HooArray") {
        return generateArrayConstructor(newExpr);
    } else if (moduleExport.runtimeClassName == "HooMap") {
        return generateMapConstructor(newExpr);
    } else if (moduleExport.runtimeClassName == "HooException") {
        return generateExceptionConstructor(newExpr);
    }

    // Unknown standard library class - should not reach here
    addError("Unknown standard library class: " + moduleExport.runtimeClassName);
    return nullptr;
}

llvm::Value* LLVMCodeGenerator::generateExceptionConstructor(const ast::NewObjectExpression& newExpr) {
    // hoo.Exception(message) -> hoo_exception_runtime(message)
    
    // Get the exception_runtime function
    auto* runtimeFunc = getExceptionFunc("runtime");
    if (!runtimeFunc) {
        addError("hoo_exception_runtime function could not be declared");
        return nullptr;
    }

    // Default message if not provided
    llvm::Value* messageArg = nullptr;
    if (newExpr.getArguments() && !newExpr.getArguments()->getArguments().empty()) {
        messageArg = generateLLVMExpression(*newExpr.getArguments()->getArguments()[0]);
    } else {
        messageArg = builder_->CreateGlobalString("Hoo Runtime Exception", "exc_msg");
    }

    return builder_->CreateCall(runtimeFunc, {messageArg}, "new_exception");
}

llvm::Value* LLVMCodeGenerator::generateStringConstructor(const ast::NewObjectExpression& newExpr) {
    // hoo.String() -> hoo_string_new()
    // hoo.String("hello") -> hoo_string_from_cstr("hello")
    // hoo.String(other_string) -> hoo_string_from_cstr(hoo_string_data(other_string))

    const ast::ArgumentList* args = newExpr.getArguments();

    // Ensure string functions are declared
    declareRuntimeFunctions();

    if (!args || args->getArguments().empty()) {
        // new hoo.String() - create empty string
        auto* newStringFunc = getStringFunc("new");
        if (!newStringFunc) {
            addError("hoo_string_new function not declared");
            return nullptr;
        }
        return builder_->CreateCall(newStringFunc, {}, "new_string");
    }

    // Get the first argument
    const auto& firstArg = args->getArguments()[0];
    llvm::Value* argValue = generateLLVMExpression(*firstArg);
    if (!argValue) {
        addError("Failed to generate String constructor argument");
        return nullptr;
    }

    // Check argument type
    llvm::Type* argType = argValue->getType();

    // If it's a string literal or pointer to i8 (C string), use hoo_string_from_cstr
    if (argType->isPointerTy()) {
        auto* fromCstrFunc = getStringFunc("from_cstr");
        if (!fromCstrFunc) {
            addError("hoo_string_from_cstr function not declared");
            return nullptr;
        }
        return builder_->CreateCall(fromCstrFunc, {argValue}, "new_string");
    }

    // If it's a HooString pointer, return as-is (already a string)
    // This handles: new hoo.String(existingString)
    if (argType->isPointerTy()) {
        return argValue;
    }

    addError("Invalid argument type for hoo.String constructor");
    return nullptr;
}

llvm::Value* LLVMCodeGenerator::generateArrayConstructor(const ast::NewObjectExpression& newExpr) {
    // hoo.Array() -> hoo_array_new()
    // hoo.Array<T>() -> hoo_array_new()
    // hoo.Array can also be initialized with array literal in future

    // Get the array_new function (declares it if needed)
    llvm::Function* arrayNewFunc = getArrayNewFunc(0);  // elementSize parameter ignored in Phase 7
    if (!arrayNewFunc) {
        addError("hoo_array_new function could not be declared");
        return nullptr;
    }

    return builder_->CreateCall(arrayNewFunc, {}, "new_array");
}

llvm::Value* LLVMCodeGenerator::generateMapConstructor(const ast::NewObjectExpression& newExpr) {
    // hoo.Map() -> hoo_map_new(keyType)
    // hoo.Map<K, V>() -> hoo_map_new(keyType)
    // Key type is determined from the generic parameters

    // Get the map_new function
    auto* mapNewFunc = getMapFunc("new");
    if (!mapNewFunc) {
        addError("hoo_map_new function could not be declared");
        return nullptr;
    }

    // For now, default to string key type (keyType = 4)
    // In a full implementation, this would be determined from the generic parameters
    // Map key types: BYTE=0, INT8=1, INT64=2, CHAR=3, STRING=4
    llvm::Value* keyTypeArg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), 4);

    return builder_->CreateCall(mapNewFunc, {keyTypeArg}, "new_map");
}

// ============================================================================
// Import Resolution and Module System Integration
// ============================================================================

void LLVMCodeGenerator::processImports(const std::vector<std::unique_ptr<ast::ImportStatement>>& imports) {
    for (const auto& import : imports) {
        if (auto basicImport = dynamic_cast<const ast::BasicImport*>(import.get())) {
            processBasicImport(*basicImport);
        } else if (auto fromImport = dynamic_cast<const ast::FromImport*>(import.get())) {
            processFromImport(*fromImport);
        }
        // Add other import types here as needed
    }
}

void LLVMCodeGenerator::processBasicImport(const ast::BasicImport& import) {
    // For basic imports like "import hoo.String as String" or "import hoo.io.File"
    // Extract the module path from the ModulePath
    const ast::ModulePath* modulePath = import.getModule();
    if (!modulePath) {
        return;
    }
    const auto& components = modulePath->getComponents();

    // Resolve the module path in the registry
    HooModule* module = moduleRegistry_.resolveModulePath(components);
    if (!module) {
        // Module not found - for now, silently skip
        // In a full implementation, this would be a compile error
        return;
    }

    // For basic imports, we add all exports from the module/namespace
    // The alias (if provided) would apply to the module as a whole
    // For now, we just add the module exports with their original names
    const auto& exports = module->getExports();
    for (const auto& pair : exports) {
        // Store the export under its name (or alias if provided)
        std::string importName = pair.first;
        if (!import.getAlias().empty()) {
            // If there's an alias, prepend it to the name
            importName = import.getAlias() + "." + pair.first;
        }
        importedNames_[importName] = &pair.second;
    }
}

void LLVMCodeGenerator::processFromImport(const ast::FromImport& import) {
    // For from imports like "from hoo import String, Array" or "from hoo.io import File"
    // Extract the module path
    const ast::ModulePath* modulePath = import.getModule();
    if (!modulePath) {
        return;
    }
    const auto& components = modulePath->getComponents();

    // Resolve the module path in the registry
    HooModule* module = moduleRegistry_.resolveModulePath(components);
    if (!module) {
        // Module not found - for now, silently skip
        return;
    }

    // Get the items to import
    const auto& items = import.getItems();
    for (const auto& item : items) {
        // item is an ImportItem with name and optional alias
        const std::string& name = item->getName();
        const std::string& alias = item->getAlias();

        // Look up the export in the module
        const ModuleExport* export_ = module->getExport(name);
        if (!export_) {
            // Export not found in module
            continue;
        }

        // Add to importedNames using the alias (if provided) or the original name
        std::string importName = alias.empty() ? name : alias;
        importedNames_[importName] = export_;
    }
}
Value* LLVMCodeGenerator::ensureTypeMatch(Value* value, llvm::Type* targetType) {
    if (!value || !targetType) return value;
    if (value->getType() == targetType) return value;

    // Handle implicit conversion to nullable (tagged union pattern: { i1 flag, T value })
    if (targetType->isStructTy() && targetType->getStructName().starts_with("nullable")) {
        auto structTy = llvm::cast<llvm::StructType>(targetType);
        if (structTy->getNumElements() == 2 && structTy->getElementType(0)->isIntegerTy(1)) {
            auto innerType = structTy->getElementType(1);
            if (value->getType() == innerType) {
                return wrapValueInNullable(value, targetType);
            }
        }
    }

    return value;
}
