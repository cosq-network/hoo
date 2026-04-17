#include "LLVMCodeGenerator.h"
#include "runtime/llvm/RuntimeRegistry.h"
#include "runtime/llvm/RuntimeMethodRegistry.h"
#include "ast/AST.h"
#include "ast/ClassDeclaration.h"
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
    using HoocModule = hooc::Module;
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
    // Create a new module for this compilation unit
    module_ = std::make_unique<llvm::Module>("hooc_module", context_);

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

    // Reset string function pointers
    // Note: Uses X-Macro pattern to generate nullptr initializers for all string functions
    // The registry file defines RUNTIME_FUNCTION macros, we override them to generate reset code
    #define DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate)
    #define BEGIN_RUNTIME_FUNCTIONS
    #define END_RUNTIME_FUNCTIONS
    #define RUNTIME_FUNCTION(FuncName, RetType, LLVMRetType, ...) \
        hoo_string_##FuncName##_func_ = nullptr;
    #define BEGIN_RUNTIME_OPERATORS
    #define END_RUNTIME_OPERATORS
    #define RUNTIME_OPERATOR(...)

    #include "runtime/llvm/RuntimeClassRegistry.h"

    #undef DEFINE_RUNTIME_CLASS
    #undef BEGIN_RUNTIME_FUNCTIONS
    #undef END_RUNTIME_FUNCTIONS
    #undef RUNTIME_FUNCTION
    #undef BEGIN_RUNTIME_OPERATORS
    #undef END_RUNTIME_OPERATORS
    #undef RUNTIME_OPERATOR

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

    // Second pass: process functions and variables
    for (const auto& decl : compilationUnit.getDeclarations()) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclaration*>(decl.get())) {
            generateLLVMFunction(*funcDecl);
        } else if (auto varDecl = dynamic_cast<const VariableDeclaration*>(decl.get())) {
            generateVariableDeclaration(*varDecl);
        }
        // Class declarations already handled in first pass
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
    } else if (auto newObjExpr = dynamic_cast<const NewObjectExpression*>(&expr)) {
        return generateNewObjectExpression(*newObjExpr);
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

    } else if (auto nullLiteral = dynamic_cast<const NullLiteral*>(&primary)) {
        // Return a null pointer for now - the actual nullable type handling
        // happens in variable declaration and function parameter contexts
        return ConstantPointerNull::get(llvm::PointerType::get(context_, 0));

    } else if (auto stringLiteral = dynamic_cast<const ASTStringLiteral*>(&primary)) {
        // Create global string constant
        Value* cstr = builder_->CreateGlobalString(stringLiteral->getValue(), "str");

        // Ensure string functions are declared
        declareRuntimeFunctions();

        // Call hoo_string_from_cstr(cstr) to create HooString object
        if (!hoo_string_from_cstr_func_) {
            std::cerr << "Error: hoo_string_from_cstr not declared" << std::endl;
            return nullptr;
        }

        Value* hooString = builder_->CreateCall(hoo_string_from_cstr_func_, {cstr}, "hoo_str");
        return hooString;

    } else if (auto charLiteral = dynamic_cast<const CharacterLiteral*>(&primary)) {
        return ConstantInt::get(LLVMType::getInt32Ty(context_), static_cast<uint32_t>(charLiteral->getValue()));
    } else if (auto arrayLiteral = dynamic_cast<const ArrayLiteral*>(&primary)) { // Handle ArrayLiteral
        return generateArrayLiteral(*arrayLiteral);
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
            // Check for string concatenation (both operands are pointers)
            if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
                // Ensure string functions are declared
                declareRuntimeFunctions();

                // Call hoo_string_concat(left, right)
                if (!hoo_string_concat_func_) {
                    std::cerr << "Error: hoo_string_concat not declared" << std::endl;
                    return nullptr;
                }

                Value* result = builder_->CreateCall(hoo_string_concat_func_, {left, right}, "concat");
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
                if (!hoo_string_compare_func_) {
                    std::cerr << "Error: hoo_string_compare not declared" << std::endl;
                    return nullptr;
                }

                Value* cmpResult = builder_->CreateCall(hoo_string_compare_func_, {left, right}, "strcmp");
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
                if (!hoo_string_compare_func_) {
                    std::cerr << "Error: hoo_string_compare not declared" << std::endl;
                    return nullptr;
                }

                Value* cmpResult = builder_->CreateCall(hoo_string_compare_func_, {left, right}, "strcmp");
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
                if (!hoo_string_compare_func_) {
                    std::cerr << "Error: hoo_string_compare not declared" << std::endl;
                    return nullptr;
                }

                Value* cmpResult = builder_->CreateCall(hoo_string_compare_func_, {left, right}, "strcmp");
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
                if (!hoo_string_compare_func_) {
                    std::cerr << "Error: hoo_string_compare not declared" << std::endl;
                    return nullptr;
                }

                Value* cmpResult = builder_->CreateCall(hoo_string_compare_func_, {left, right}, "strcmp");
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
                if (!hoo_string_equals_func_) {
                    std::cerr << "Error: hoo_string_equals not declared" << std::endl;
                    return nullptr;
                }

                Value* equalResult = builder_->CreateCall(hoo_string_equals_func_, {left, right}, "streq");
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
                if (!hoo_string_equals_func_) {
                    std::cerr << "Error: hoo_string_equals not declared" << std::endl;
                    return nullptr;
                }

                Value* equalResult = builder_->CreateCall(hoo_string_equals_func_, {left, right}, "strneq");
                // Convert i64 result to i1 (bool) - return true if NOT equal (equalResult == 0)
                return builder_->CreateICmpEQ(equalResult, ConstantInt::get(LLVMType::getInt64Ty(context_), 0), "neqtmp");
            } else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
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
    const Expression& funcExpr = call.getFunction();

    std::string functionName;
    Value* thisPtr = nullptr;
    bool isRuntimeMethod = false;
    std::string runtimeFuncName;

    if (auto memberAccess = dynamic_cast<const MemberAccess*>(&funcExpr)) {
        const std::string& methodName = memberAccess->getMember();

        Value* objectValue = generateLLVMExpression(memberAccess->getObject());
        if (!objectValue) {
            std::cerr << "Failed to generate object expression for method call" << std::endl;
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
            }
        }

        if (className.empty()) {
            std::cerr << "Cannot determine class type for method call" << std::endl;
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
        } else {
            std::cerr << "Function call must use identifier" << std::endl;
            return nullptr;
        }
    } else {
        std::cerr << "Complex function expressions not yet supported" << std::endl;
        return nullptr;
    }

    Function* calleeFunc = module_->getFunction(functionName);
    if (!calleeFunc && !isRuntimeMethod) {
        std::cerr << "Unknown function: " << functionName << std::endl;
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
            std::cerr << "Incorrect number of arguments for function " << functionName
                      << " (expected " << calleeFunc->arg_size() << ", got " << args.size() << ")" << std::endl;
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
            std::cerr << "Runtime function not declared: " << runtimeFuncName << std::endl;
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
    // Get the object value
    Value* objectValue = generateLLVMExpression(expr.getObject());
    if (!objectValue) {
        std::cerr << "Failed to generate object expression" << std::endl;
        return nullptr;
    }

    // Try to determine the class type from the object
    std::string className;

    // If the object is a primary identifier, look it up in variable types
    if (auto primaryExpr = dynamic_cast<const PrimaryExpression*>(&expr.getObject())) {
        const ASTNode& primary = primaryExpr->getPrimary();
        if (auto identifier = dynamic_cast<const Identifier*>(&primary)) {
            auto it = variableTypes_.find(identifier->getName());
            if (it != variableTypes_.end()) {
                className = it->second;
            }
        }
    }

    if (className.empty()) {
        std::cerr << "Cannot determine class type for member access" << std::endl;
        return nullptr;
    }

    // Get the class struct type
    auto classTypeIt = classTypes_.find(className);
    if (classTypeIt == classTypes_.end()) {
        std::cerr << "Unknown class type: " << className << std::endl;
        return nullptr;
    }
    llvm::StructType* classType = classTypeIt->second;

    // Get the class declaration to find member info
    auto declIt = classDeclarations_.find(className);
    if (declIt == classDeclarations_.end()) {
        std::cerr << "Missing class declaration for: " << className << std::endl;
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
        std::cerr << "Member not found: " << memberName << " in class " << className << std::endl;
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
        std::cerr << "Member type is null for: " << memberName << std::endl;
        return nullptr;
    }
    auto loadedValue = builder_->CreateLoad(memberType, fieldPtr, memberName);

    return loadedValue;
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
        std::cerr << "Failed to generate iterable expression for for-in loop" << std::endl;
        return;
    }

    // Check that we have the array runtime functions
    if (!hoo_array_length_func_ || !hoo_array_get_int64_func_) {
        std::cerr << "Error: Array runtime functions not available for for-in loop" << std::endl;
        return;
    }

    // Get the actual array length by calling hoo_array_length(arr)
    Value* arrayLength = builder_->CreateCall(hoo_array_length_func_, {iterableValue}, "array.length");

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
        hoo_array_get_int64_func_,
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
    Value* currentVal = builder_->CreateLoad(loopVar->getAllocatedType(), loopVar, stmt.getVariable());
    Value* condValue;
    if (loopVar->getAllocatedType()->isIntegerTy()) {
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
    Value* curVal = builder_->CreateLoad(loopVar->getAllocatedType(), loopVar, stmt.getVariable());
    Value* nextVal;
    if (loopVar->getAllocatedType()->isIntegerTy()) {
        nextVal = builder_->CreateAdd(curVal, ConstantInt::get(loopVar->getAllocatedType(), 1), "nextval");
    } else {
        nextVal = builder_->CreateFAdd(curVal, ConstantFP::get(loopVar->getAllocatedType(), 1.0), "nextval");
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
                builder_->CreateStore(initValue, alloca);
            }
        }
    }
}

LLVMType* LLVMCodeGenerator::generateLLVMType(const ASTType& type) {
    if (auto unionType = dynamic_cast<const UnionType*>(&type)) {
        // For union types, check if any type is nullable
        const auto& types = unionType->getTypes();
        if (!types.empty()) {
            // If the union contains optional types, create a tagged union
            bool hasOptional = false;
            for (const auto& t : types) {
                if (t->isOptional()) {
                    hasOptional = true;
                    break;
                }
            }

            if (hasOptional && types.size() == 1) {
                // Single optional type in union - wrap it in nullable type
                auto valueType = generateLLVMType(*types[0]);
                return createNullableType(valueType);
            } else if (hasOptional) {
                // Multiple types with optional - use first type for now
                return generateLLVMType(*types[0]);
            } else {
                // Non-optional union - just use first type
                return generateLLVMType(*types[0]);
            }
        }
    }

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
            std::cerr << "Failed to generate array element expression" << std::endl;
            return nullptr;
        }

        // Infer element type from first element
        if (elementType == nullptr) {
            elementType = elemValue->getType();
        } else if (elemValue->getType() != elementType) {
            std::cerr << "Array literal elements must have uniform type" << std::endl;
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
            std::cerr << "Array literal elements must be compile-time constants (unless they are class instances)" << std::endl;
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
        // Declare hoo_int64_array_from_buffer if not already done
        if (!hoo_int64_array_from_buffer_func_) {
            std::vector<LLVMType*> params = {
                llvm::PointerType::get(context_, 0),  // data pointer
                LLVMType::getInt64Ty(context_)         // length
            };
            FunctionType* funcType = FunctionType::get(llvm::PointerType::get(context_, 0), params, false);
            hoo_int64_array_from_buffer_func_ = Function::Create(funcType, Function::ExternalLinkage, "hoo_int64_array_from_buffer", module_.get());
        }
        return hoo_int64_array_from_buffer_func_;
    } else if (elementType->isDoubleTy()) {
        // Declare hoo_double_array_from_buffer if not already done
        if (!hoo_double_array_from_buffer_func_) {
            std::vector<LLVMType*> params = {
                llvm::PointerType::get(context_, 0),  // data pointer
                LLVMType::getInt64Ty(context_)         // length
            };
            FunctionType* funcType = FunctionType::get(llvm::PointerType::get(context_, 0), params, false);
            hoo_double_array_from_buffer_func_ = Function::Create(funcType, Function::ExternalLinkage, "hoo_double_array_from_buffer", module_.get());
        }
        return hoo_double_array_from_buffer_func_;
    }

    std::cerr << "Error: No array from_buffer function for element type" << std::endl;
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
        std::cerr << "Error: No array creation function for element type" << std::endl;
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
        std::cerr << "Error: Failed to declare hoo_array_new function" << std::endl;
        return nullptr;
    }

    // Create empty array with no parameters
    std::vector<llvm::Value*> newArgs;  // No arguments
    llvm::Value* arrayHandle = builder_->CreateCall(arrayNewFunc, newArgs, "hoo_arr_new");

    // Get the type-specific push function for this element type
    auto arrayPushFunc = getArrayPushFuncForType(elementType);
    if (!arrayPushFunc) {
        std::cerr << "Error: Failed to get type-specific array push function" << std::endl;
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
    if (!hoo_array_new_func_) {
        // Phase 7: New API - hoo_array_new(void) with no parameters
        // elementSize parameter is now ignored - the array uses std::any internally
        std::vector<LLVMType*> params;  // No parameters
        FunctionType* funcType = FunctionType::get(
            llvm::PointerType::get(context_, 0),  // return HooArray (void*)
            params,
            false
        );
        hoo_array_new_func_ = Function::Create(
            funcType,
            Function::ExternalLinkage,
            "hoo_array_new",
            module_.get()
        );
    }
    return hoo_array_new_func_;
}

// ============================================================================
// Phase 7: Type-Specific Array Push Function Getters
// ============================================================================

llvm::Function* LLVMCodeGenerator::getArrayPushInt64Func() {
    if (!hoo_array_push_int64_func_) {
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),  // HooArray
            LLVMType::getInt64Ty(context_)        // int64_t value
        };
        FunctionType* funcType = FunctionType::get(
            LLVMType::getInt64Ty(context_),       // return int64_t (new length)
            params,
            false
        );
        hoo_array_push_int64_func_ = Function::Create(
            funcType,
            Function::ExternalLinkage,
            "hoo_array_push_int64",
            module_.get()
        );
    }
    return hoo_array_push_int64_func_;
}

llvm::Function* LLVMCodeGenerator::getArrayPushDoubleFunc() {
    if (!hoo_array_push_double_func_) {
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),  // HooArray
            LLVMType::getDoubleTy(context_)       // double value
        };
        FunctionType* funcType = FunctionType::get(
            LLVMType::getInt64Ty(context_),       // return int64_t (new length)
            params,
            false
        );
        hoo_array_push_double_func_ = Function::Create(
            funcType,
            Function::ExternalLinkage,
            "hoo_array_push_double",
            module_.get()
        );
    }
    return hoo_array_push_double_func_;
}

llvm::Function* LLVMCodeGenerator::getArrayPushFloatFunc() {
    if (!hoo_array_push_float_func_) {
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),  // HooArray
            LLVMType::getFloatTy(context_)        // float value
        };
        FunctionType* funcType = FunctionType::get(
            LLVMType::getInt64Ty(context_),       // return int64_t (new length)
            params,
            false
        );
        hoo_array_push_float_func_ = Function::Create(
            funcType,
            Function::ExternalLinkage,
            "hoo_array_push_float",
            module_.get()
        );
    }
    return hoo_array_push_float_func_;
}

llvm::Function* LLVMCodeGenerator::getArrayPushBoolFunc() {
    if (!hoo_array_push_bool_func_) {
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),  // HooArray
            LLVMType::getInt64Ty(context_)        // int64_t bool (0 or 1)
        };
        FunctionType* funcType = FunctionType::get(
            LLVMType::getInt64Ty(context_),       // return int64_t (new length)
            params,
            false
        );
        hoo_array_push_bool_func_ = Function::Create(
            funcType,
            Function::ExternalLinkage,
            "hoo_array_push_bool",
            module_.get()
        );
    }
    return hoo_array_push_bool_func_;
}

llvm::Function* LLVMCodeGenerator::getArrayPushCharFunc() {
    if (!hoo_array_push_char_func_) {
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),  // HooArray
            LLVMType::getInt8Ty(context_)         // char value (i8)
        };
        FunctionType* funcType = FunctionType::get(
            LLVMType::getInt64Ty(context_),       // return int64_t (new length)
            params,
            false
        );
        hoo_array_push_char_func_ = Function::Create(
            funcType,
            Function::ExternalLinkage,
            "hoo_array_push_char",
            module_.get()
        );
    }
    return hoo_array_push_char_func_;
}

llvm::Function* LLVMCodeGenerator::getArrayPushObjectFunc() {
    if (!hoo_array_push_object_func_) {
        std::vector<LLVMType*> params = {
            llvm::PointerType::get(context_, 0),  // HooArray
            llvm::PointerType::get(context_, 0)   // void* (object pointer)
        };
        FunctionType* funcType = FunctionType::get(
            LLVMType::getInt64Ty(context_),       // return int64_t (new length)
            params,
            false
        );
        hoo_array_push_object_func_ = Function::Create(
            funcType,
            Function::ExternalLinkage,
            "hoo_array_push_object",
            module_.get()
        );
    }
    return hoo_array_push_object_func_;
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
    // Check if the type is an optional type
    if (auto optionalType = dynamic_cast<const ast::OptionalType*>(&type)) {
        return optionalType->isOptional();
    }
    if (auto unionType = dynamic_cast<const ast::UnionType*>(&type)) {
        // Union types containing at least one optional type are nullable
        for (const auto& t : unionType->getTypes()) {
            if (t->isOptional()) return true;
        }
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
            module_.get()
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
            module_.get()
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
            module_.get()
        );
    }

    // ========================================================================
    // Invoke Registry Callbacks to Declare Runtime Functions
    // ========================================================================
    // Call all registered runtime libraries to declare their LLVM functions
    // and populate the runtimeFunctionStorage_.
    // This replaces the manual declareRuntimeFunctions() approach.

    // Force String and Array runtimes to be linked and registered (works around linker optimization)
    extern void _hoo_string_ensure_registration();
    extern void _hoo_array_ensure_registration();
    _hoo_string_ensure_registration();
    _hoo_array_ensure_registration();

    auto& registry = runtime::RuntimeRegistry::getInstance();
    registry.declareAllFunctions(*module_, context_, &runtimeFunctionStorage_);

    // ========================================================================
    // Sync Legacy Function Pointers (for backward compatibility)
    // ========================================================================
    // The new architecture uses runtimeFunctionStorage_.strings, but the rest
    // of the code still uses individual function pointers. Sync them here.
    // This will be removed once all code is refactored to use storage.

    auto& stringStorage = runtimeFunctionStorage_.strings;
    hoo_string_from_cstr_func_ = stringStorage.hoo_string_from_cstr_func;
    hoo_string_new_func_ = stringStorage.hoo_string_new_func;
    hoo_string_from_bytes_func_ = stringStorage.hoo_string_from_bytes_func;
    hoo_string_repeat_func_ = stringStorage.hoo_string_repeat_func;
    hoo_string_concat_func_ = stringStorage.hoo_string_concat_func;
    hoo_string_substring_func_ = stringStorage.hoo_string_substring_func;
    hoo_string_to_upper_func_ = stringStorage.hoo_string_to_upper_func;
    hoo_string_to_lower_func_ = stringStorage.hoo_string_to_lower_func;
    hoo_string_trim_func_ = stringStorage.hoo_string_trim_func;
    hoo_string_replace_func_ = stringStorage.hoo_string_replace_func;
    hoo_string_split_func_ = stringStorage.hoo_string_split_func;
    hoo_string_length_func_ = stringStorage.hoo_string_length_func;
    hoo_string_data_func_ = stringStorage.hoo_string_data_func;
    hoo_string_byte_at_func_ = stringStorage.hoo_string_byte_at_func;
    hoo_string_is_empty_func_ = stringStorage.hoo_string_is_empty_func;
    hoo_string_index_of_func_ = stringStorage.hoo_string_index_of_func;
    hoo_string_last_index_of_func_ = stringStorage.hoo_string_last_index_of_func;
    hoo_string_contains_func_ = stringStorage.hoo_string_contains_func;
    hoo_string_starts_with_func_ = stringStorage.hoo_string_starts_with_func;
    hoo_string_ends_with_func_ = stringStorage.hoo_string_ends_with_func;
    hoo_string_compare_func_ = stringStorage.hoo_string_compare_func;
    hoo_string_equals_func_ = stringStorage.hoo_string_equals_func;
    hoo_string_equals_ignore_case_func_ = stringStorage.hoo_string_equals_ignore_case_func;
    hoo_string_retain_func_ = stringStorage.hoo_string_retain_func;
    hoo_string_release_func_ = stringStorage.hoo_string_release_func;
    hoo_string_refcount_func_ = stringStorage.hoo_string_refcount_func;
    hoo_string_from_int64_func_ = stringStorage.hoo_string_from_int64_func;
    hoo_string_from_double_func_ = stringStorage.hoo_string_from_double_func;
    hoo_string_from_bool_func_ = stringStorage.hoo_string_from_bool_func;
    hoo_string_to_int64_func_ = stringStorage.hoo_string_to_int64_func;
    hoo_string_to_double_func_ = stringStorage.hoo_string_to_double_func;
    hoo_string_format_func_ = stringStorage.hoo_string_format_func;
    hoo_string_print_func_ = stringStorage.hoo_string_print_func;
    hoo_string_println_func_ = stringStorage.hoo_string_println_func;
    hoo_string_debug_func_ = stringStorage.hoo_string_debug_func;

    // Sync Array function pointers from storage
    auto& arrayStorage = runtimeFunctionStorage_.arrays;
    hoo_array_new_func_ = arrayStorage.hoo_array_new_func;
    hoo_array_from_buffer_func_ = arrayStorage.hoo_array_from_buffer_func;
    hoo_array_repeat_func_ = arrayStorage.hoo_array_repeat_func;
    hoo_array_length_func_ = arrayStorage.hoo_array_length_func;
    hoo_array_get_func_ = arrayStorage.hoo_array_get_func;
    hoo_array_set_func_ = arrayStorage.hoo_array_set_func;
    hoo_array_pop_func_ = arrayStorage.hoo_array_pop_func;
    hoo_array_push_int64_func_ = arrayStorage.hoo_array_push_int64_func;
    hoo_array_push_double_func_ = arrayStorage.hoo_array_push_double_func;
    hoo_array_push_float_func_ = arrayStorage.hoo_array_push_float_func;
    hoo_array_push_bool_func_ = arrayStorage.hoo_array_push_bool_func;
    hoo_array_push_char_func_ = arrayStorage.hoo_array_push_char_func;
    hoo_array_get_int64_func_ = arrayStorage.hoo_array_get_int64_func;
    hoo_array_retain_func_ = arrayStorage.hoo_array_retain_func;
    hoo_array_release_func_ = arrayStorage.hoo_array_release_func;
    hoo_array_refcount_func_ = arrayStorage.hoo_array_refcount_func;
    hoo_array_empty_func_ = arrayStorage.hoo_array_empty_func;
    hoo_array_clear_func_ = arrayStorage.hoo_array_clear_func;
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
            if (!hoo_string_concat_func_) return nullptr;
            return builder_->CreateCall(hoo_string_concat_func_, {left, right}, "concat_result");

        case ASTBinaryOperator::EQUALS:
            if (!hoo_string_equals_func_) return nullptr;
            {
                Value* result = builder_->CreateCall(hoo_string_equals_func_, {left, right}, "equals_result");
                return builder_->CreateICmpNE(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }

        case ASTBinaryOperator::NOT_EQUALS:
            if (!hoo_string_equals_func_) return nullptr;
            {
                Value* result = builder_->CreateCall(hoo_string_equals_func_, {left, right}, "neq_result");
                return builder_->CreateICmpEQ(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }

        case ASTBinaryOperator::LESS:
            if (!hoo_string_compare_func_) return nullptr;
            {
                Value* result = builder_->CreateCall(hoo_string_compare_func_, {left, right}, "cmp_result");
                return builder_->CreateICmpSLT(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }

        case ASTBinaryOperator::LESS_EQUALS:
            if (!hoo_string_compare_func_) return nullptr;
            {
                Value* result = builder_->CreateCall(hoo_string_compare_func_, {left, right}, "cmp_result");
                return builder_->CreateICmpSLE(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }

        case ASTBinaryOperator::GREATER:
            if (!hoo_string_compare_func_) return nullptr;
            {
                Value* result = builder_->CreateCall(hoo_string_compare_func_, {left, right}, "cmp_result");
                return builder_->CreateICmpSGT(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }

        case ASTBinaryOperator::GREATER_EQUALS:
            if (!hoo_string_compare_func_) return nullptr;
            {
                Value* result = builder_->CreateCall(hoo_string_compare_func_, {left, right}, "cmp_result");
                return builder_->CreateICmpSGE(result, ConstantInt::get(LLVMType::getInt64Ty(context_), 0));
            }

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
                    llvm::Function::ExternalLinkage, mangledName, module_.get());

                // Generate function body
                auto saveBB = builder_->GetInsertBlock();
                auto saveFn = saveBB ? saveBB->getParent() : nullptr;

                llvm::BasicBlock* BB = llvm::BasicBlock::Create(context_, "entry", methodFunc);
                builder_->SetInsertPoint(BB);

                // Set up local scope with parameters
                namedValues_.clear();

                // Add 'this' as a local variable (skip it, it's used directly)
                // Add explicit parameters to symbol table
                auto argIt = methodFunc->arg_begin();
                ++argIt; // Skip 'this' pointer

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
        module_.get()
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
        std::cerr << "Constructor verification failed: " << errorStr << std::endl;
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
        // This is a qualified name like std.String
        const ModuleExport* moduleExport = moduleRegistry_.resolveQualifiedName(*qualifiedName);
        if (moduleExport && moduleExport->kind == ModuleExport::Kind::CLASS) {
            // This is a standard library class - use runtime constructor
            return generateStdClassConstructor(*moduleExport, newExpr);
        }
    }

    // Try to resolve as an imported name (e.g., "String" after "import std.String")
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
                    std::cerr << "Failed to generate constructor argument" << std::endl;
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
    }

    // Unknown standard library class - should not reach here
    std::cerr << "Unknown standard library class: " << moduleExport.runtimeClassName << std::endl;
    return nullptr;
}

llvm::Value* LLVMCodeGenerator::generateStringConstructor(const ast::NewObjectExpression& newExpr) {
    // std.String() -> hoo_string_new()
    // std.String("hello") -> hoo_string_from_cstr("hello")
    // std.String(other_string) -> hoo_string_from_cstr(hoo_string_data(other_string))

    const ast::ArgumentList* args = newExpr.getArguments();

    // Ensure string functions are declared
    declareRuntimeFunctions();

    if (!args || args->getArguments().empty()) {
        // new std.String() - create empty string
        if (!hoo_string_new_func_) {
            std::cerr << "hoo_string_new function not declared" << std::endl;
            return nullptr;
        }
        return builder_->CreateCall(hoo_string_new_func_, {}, "new_string");
    }

    // Get the first argument
    const auto& firstArg = args->getArguments()[0];
    llvm::Value* argValue = generateLLVMExpression(*firstArg);
    if (!argValue) {
        std::cerr << "Failed to generate String constructor argument" << std::endl;
        return nullptr;
    }

    // Check argument type
    llvm::Type* argType = argValue->getType();

    // If it's a string literal or pointer to i8 (C string), use hoo_string_from_cstr
    if (argType->isPointerTy()) {
        if (!hoo_string_from_cstr_func_) {
            std::cerr << "hoo_string_from_cstr function not declared" << std::endl;
            return nullptr;
        }
        return builder_->CreateCall(hoo_string_from_cstr_func_, {argValue}, "new_string");
    }

    // If it's a HooString pointer, return as-is (already a string)
    // This handles: new std.String(existingString)
    if (argType->isPointerTy()) {
        return argValue;
    }

    std::cerr << "Invalid argument type for std.String constructor" << std::endl;
    return nullptr;
}

llvm::Value* LLVMCodeGenerator::generateArrayConstructor(const ast::NewObjectExpression& newExpr) {
    // std.Array() -> hoo_array_new()
    // std.Array<T>() -> hoo_array_new()
    // std.Array can also be initialized with array literal in future

    // Get the array_new function (declares it if needed)
    llvm::Function* arrayNewFunc = getArrayNewFunc(0);  // elementSize parameter ignored in Phase 7
    if (!arrayNewFunc) {
        std::cerr << "hoo_array_new function could not be declared" << std::endl;
        return nullptr;
    }

    return builder_->CreateCall(arrayNewFunc, {}, "new_array");
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
    // For basic imports like "import std.String as String" or "import std.io.File"
    // Extract the module path from the ModulePath
    const ast::ModulePath* modulePath = import.getModule();
    if (!modulePath) {
        return;
    }
    const auto& components = modulePath->getComponents();

    // Resolve the module path in the registry
    hooc::Module* module = moduleRegistry_.resolveModulePath(components);
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
    // For from imports like "from std import String, Array" or "from std.io import File"
    // Extract the module path
    const ast::ModulePath* modulePath = import.getModule();
    if (!modulePath) {
        return;
    }
    const auto& components = modulePath->getComponents();

    // Resolve the module path in the registry
    hooc::Module* module = moduleRegistry_.resolveModulePath(components);
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