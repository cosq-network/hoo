#pragma once

#include "CodeGeneratorTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include <memory>

namespace hooc {

/**
 * LLVM-specific implementations of the opaque code generator types.
 * These wrap LLVM IR types and provide safe casting methods.
 */

/**
 * Wrapper for LLVM Module
 */
class LLVMGeneratedModule : public GeneratedModule {
public:
    explicit LLVMGeneratedModule(std::unique_ptr<llvm::Module> module)
        : module_(std::move(module)) {}

    void* getImplementation() override {
        return module_.get();
    }

    const void* getImplementation() const override {
        return module_.get();
    }

    // LLVM-specific accessors
    llvm::Module* getLLVMModule() { return module_.get(); }
    const llvm::Module* getLLVMModule() const { return module_.get(); }
    std::unique_ptr<llvm::Module> releaseLLVMModule() { return std::move(module_); }

private:
    std::unique_ptr<llvm::Module> module_;
};

/**
 * Wrapper for LLVM Function
 */
class LLVMGeneratedFunction : public GeneratedFunction {
public:
    explicit LLVMGeneratedFunction(llvm::Function* function)
        : function_(function) {}

    void* getImplementation() override {
        return function_;
    }

    const void* getImplementation() const override {
        return function_;
    }

    // LLVM-specific accessors
    llvm::Function* getLLVMFunction() { return function_; }
    const llvm::Function* getLLVMFunction() const { return function_; }

private:
    llvm::Function* function_;
};

/**
 * Wrapper for LLVM Value
 */
class LLVMGeneratedValue : public GeneratedValue {
public:
    explicit LLVMGeneratedValue(llvm::Value* value)
        : value_(value) {}

    void* getImplementation() override {
        return value_;
    }

    const void* getImplementation() const override {
        return value_;
    }

    // LLVM-specific accessors
    llvm::Value* getLLVMValue() { return value_; }
    const llvm::Value* getLLVMValue() const { return value_; }

private:
    llvm::Value* value_;
};

/**
 * Wrapper for LLVM Type
 */
class LLVMGeneratedType : public GeneratedType {
public:
    explicit LLVMGeneratedType(llvm::Type* type)
        : type_(type) {}

    void* getImplementation() override {
        return type_;
    }

    const void* getImplementation() const override {
        return type_;
    }

    // LLVM-specific accessors
    llvm::Type* getLLVMType() { return type_; }
    const llvm::Type* getLLVMType() const { return type_; }

private:
    llvm::Type* type_;
};

} // namespace hooc
