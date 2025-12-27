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

    // ========================================================================
    // Auto-generated runtime function registration declarations
    // ========================================================================
    // These are generated from the RUNTIME_CLASSES registry using X-Macro
    // pattern. Each runtime class gets a registerXxxFunctions() method.

    #define DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate) \
        void register##ClassName##Functions();
    #define BEGIN_RUNTIME_FUNCTIONS
    #define END_RUNTIME_FUNCTIONS
    #define BEGIN_RUNTIME_OPERATORS
    #define END_RUNTIME_OPERATORS
    #define RUNTIME_FUNCTION(...)
    #define RUNTIME_OPERATOR(...)

    RUNTIME_CLASSES

    #undef DEFINE_RUNTIME_CLASS
    #undef BEGIN_RUNTIME_FUNCTIONS
    #undef END_RUNTIME_FUNCTIONS
    #undef BEGIN_RUNTIME_OPERATORS
    #undef END_RUNTIME_OPERATORS
    #undef RUNTIME_FUNCTION
    #undef RUNTIME_OPERATOR

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