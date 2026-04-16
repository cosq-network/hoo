#pragma once

#include <memory>
#include <string>
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/LLVMContext.h"
#include "runtime/RuntimeClassRegistry.h"

namespace llvm {
namespace orc {
class LLJIT;
}
class LLVMContext;
class Module;
}

namespace hooc {
namespace ast {
class CompilationUnit;
}

class ProcessIsolatedParser;
class SimpleASTBuilder;
class CodeGenerator;

class HoocJIT {
private:
    std::unique_ptr<llvm::orc::LLJIT> JIT;
    llvm::LLVMContext Context;
    std::unique_ptr<ProcessIsolatedParser> parser_;
    std::unique_ptr<SimpleASTBuilder> astBuilder_;
    std::unique_ptr<CodeGenerator> codeGenerator_;

public:
    HoocJIT();
    ~HoocJIT();

    // AST-based compilation
    bool compileHoocCode(const std::string& code);
    bool executeFunction(const std::string& functionName);
};

} // namespace hooc // namespace hooc