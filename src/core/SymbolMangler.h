#pragma once

#include <string>
#include <vector>
#include <llvm/IR/Type.h>

namespace hooc {

/**
 * @class SymbolMangler
 * @brief Provides centralized name mangling for the Hooc compiler, JIT, and linker.
 * 
 * This class ensures consistent symbol naming across different compilation and execution
 * stages, supporting function overloading and module isolation.
 */
class SymbolMangler {
public:
    /**
     * @brief Mangle a function name based on its parameter types.
     * 
     * @param name The base function name.
     * @param paramTypes The LLVM types of the function parameters.
     * @return A unique mangled symbol string.
     */
    static std::string mangleFunctionName(const std::string& name, 
                                         const std::vector<llvm::Type*>& paramTypes);

    /**
     * @brief Mangle a symbol name with its module path.
     * 
     * Follows the scheme: _H_<module_path>_<symbol_name>
     * 
     * @param modulePath Components of the module path (e.g., ["hoo", "io"]).
     * @param symbolName The name of the symbol (function, class, variable).
     * @return A unique mangled symbol string for cross-module linking.
     */
    static std::string mangleModuleSymbol(const std::vector<std::string>& modulePath,
                                         const std::string& symbolName);
};

} // namespace hooc
