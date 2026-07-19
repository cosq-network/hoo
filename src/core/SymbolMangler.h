#pragma once

#include <string>
#include <vector>
#include <memory>

namespace hooc {

struct DemangledSymbol {
    std::string originalName;
    std::vector<std::string> modulePath;
    std::string className;
    std::string functionName;
    std::string baseClassName;
    std::vector<std::string> classModifiers;
    std::vector<std::string> functionModifiers;
    std::string returnType;
    std::vector<std::string> parameterTypes;
    bool isConstructor = false;
    bool isDestructor = false;
    bool isStatic = false;
    bool isVirtual = false;
    bool isOverload = false;
    bool isAsync = false;      // true when the symbol is an async function
};

struct MangledFunctionParams {
    std::vector<std::string> modulePath;
    std::string className;
    std::string baseClassName;
    std::vector<std::string> classModifiers;
    std::string functionName;
    std::vector<std::string> functionModifiers;
    std::string returnType;
    std::vector<std::string> parameterTypes;
    bool isConstructor = false;
    bool isDestructor = false;
    bool isStatic = false;
    bool isVirtual = false;
    bool isOverload = false;
    bool isAsync = false;      // true when the function is declared async
};

const std::vector<std::pair<std::string, std::string>>& getTypeCodeMap();
const std::vector<std::pair<std::string, std::string>>& getModifierCodeMap();
const std::vector<std::pair<std::string, std::string>>& getFunctionModifierCodeMap();

std::string typeNameToCode(const std::string& typeName);
std::string codeToTypeName(const std::string& code);
std::string encodeComponent(const std::string& component);
std::string decodeComponent(const std::string& encoded);

class SymbolMangler {
public:
    static std::string mangleFunctionName(const MangledFunctionParams& params);

    static std::string mangleModuleSymbol(const std::vector<std::string>& modulePath,
                                          const std::string& symbolName,
                                          const std::string& kindTag = "");

    static DemangledSymbol demangleSymbol(const std::string& mangledName);

    static std::string demangle(const std::string& mangledName);

    static std::string typeKindToMangledString(const std::string& typeName);

    static std::string demangleType(const std::string& mangledType);

    static std::string mangleType(const std::string& typeName);
};

} // namespace hooc