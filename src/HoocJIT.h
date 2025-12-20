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
}

namespace hooc {

class ProcessIsolatedParser;

class HoocJIT {
private:
    std::unique_ptr<llvm::orc::LLJIT> JIT;
    llvm::LLVMContext Context;
    std::unique_ptr<ProcessIsolatedParser> parser_;
    
public:
    HoocJIT();
    ~HoocJIT();
    
    void createSimpleFunction();
    void executeFunction();
    void parseHoocCode(const std::string& code);
};

} // namespace hooc // namespace hooc