#include "SymbolMangler.h"
#include <llvm/IR/DerivedTypes.h>
#include <sstream>

namespace hooc {

std::string SymbolMangler::mangleFunctionName(const std::string& name, 
                                             const std::vector<llvm::Type*>& paramTypes) {
    // Simple name mangling for overloading
    std::string mangledName = name;
    for (llvm::Type* type : paramTypes) {
        mangledName += "_";
        if (type->isIntegerTy()) {
            mangledName += "i" + std::to_string(type->getIntegerBitWidth());
        } else if (type->isFloatingPointTy()) {
            mangledName += "f";
        } else if (type->isPointerTy()) {
            mangledName += "ptr";
        } else {
            mangledName += "v"; // void or unknown
        }
    }
    return mangledName;
}

std::string SymbolMangler::mangleModuleSymbol(const std::vector<std::string>& modulePath,
                                             const std::string& symbolName) {
    std::stringstream ss;
    ss << "_H_";
    
    for (const auto& part : modulePath) {
        ss << part << "_";
    }
    
    ss << symbolName;
    return ss.str();
}

} // namespace hooc
