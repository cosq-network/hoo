#pragma once

#include "CodeGeneratorTypes.h"
#include "hvm/HoModule.h"
#include <memory>

namespace hooc {

/**
 * HVM implementation of GeneratedModule.
 * Holds the resulting HoModule containing bytecode.
 */
class HVMGeneratedModule : public GeneratedModule {
public:
    explicit HVMGeneratedModule(std::unique_ptr<hvm::HoModule> module)
        : module_(std::move(module)) {}

    void* getImplementation() override { return module_.get(); }
    const void* getImplementation() const override { return module_.get(); }

    std::unique_ptr<hvm::HoModule> takeModule() { return std::move(module_); }

private:
    std::unique_ptr<hvm::HoModule> module_;
};

/**
 * HVM implementation of GeneratedFunction.
 * For HVM, this could represent an entry point offset or symbol.
 */
class HVMGeneratedFunction : public GeneratedFunction {
public:
    explicit HVMGeneratedFunction(uint64_t entryRVA) : entryRVA_(entryRVA) {}

    void* getImplementation() override { return reinterpret_cast<void*>(entryRVA_); }
    const void* getImplementation() const override { return reinterpret_cast<const void*>(entryRVA_); }

private:
    uint64_t entryRVA_;
};

/**
 * HVM implementation of GeneratedValue.
 * For HVM, this represents a register index or stack offset.
 */
class HVMGeneratedValue : public GeneratedValue {
public:
    enum class Kind { Register, StackOffset };

    HVMGeneratedValue(Kind kind, int32_t value) : kind_(kind), value_(value) {}

    void* getImplementation() override { return reinterpret_cast<void*>(static_cast<uintptr_t>(value_)); }
    const void* getImplementation() const override { return reinterpret_cast<const void*>(static_cast<uintptr_t>(value_)); }

    Kind getKind() const { return kind_; }
    int32_t getValue() const { return value_; }

private:
    Kind kind_;
    int32_t value_;
};

/**
 * HVM implementation of GeneratedType.
 */
class HVMGeneratedType : public GeneratedType {
public:
    explicit HVMGeneratedType(uint32_t typeId) : typeId_(typeId) {}

    void* getImplementation() override { return reinterpret_cast<void*>(static_cast<uintptr_t>(typeId_)); }
    const void* getImplementation() const override { return reinterpret_cast<const void*>(static_cast<uintptr_t>(typeId_)); }

private:
    uint32_t typeId_;
};

} // namespace hooc
