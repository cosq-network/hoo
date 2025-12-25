#pragma once

#include <memory>
#include <string>
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/LLVMContext.h"

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
class LLVMCodeGenerator;

class HoocJIT {
private:
    std::unique_ptr<llvm::orc::LLJIT> JIT;
    llvm::LLVMContext Context;
    std::unique_ptr<ProcessIsolatedParser> parser_;
    std::unique_ptr<SimpleASTBuilder> astBuilder_;
    std::unique_ptr<LLVMCodeGenerator> codeGenerator_;
    
public:
    HoocJIT();
    ~HoocJIT();
    
    // Legacy demo functions
    void createSimpleFunction();
    void executeFunction();
    
    // New AST-based compilation
    bool compileHoocCode(const std::string& code);
    bool executeFunction(const std::string& functionName);
    
    // Utility methods
    void parseHoocCode(const std::string& code);
    std::unique_ptr<llvm::Module> generateModuleFromAST(const ast::CompilationUnit& ast);
};

} // namespace hooc // namespace hooc