#include "SymbolMangler.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace hooc {

const std::vector<std::pair<std::string, std::string>>& getTypeCodeMap() {
    static std::vector<std::pair<std::string, std::string>> map = {
        {"int8", "i1"},
        {"byte", "i1"},
        {"int64", "i8"},
        {"int", "i8"},
        {"float", "f"},
        {"double", "d"},
        {"f64", "d"},
        {"bool", "b"},
        {"char", "c"},
        {"string", "s"},
        {"void", "v"},
        {"ptr", "p"},
        {"array", "a"}
    };
    return map;
}

const std::vector<std::pair<std::string, std::string>>& getModifierCodeMap() {
    static std::vector<std::pair<std::string, std::string>> map = {
        {"SINGLETON", "N"},
        {"IMMUTABLE", "I"},
        {"FACTORY", "F"},
        {"OBSERVABLE", "O"},
        {"SERVICE", "S"},
        {"STRATEGY", "Y"},
        {"ACTOR", "A"},
        {"FINAL", "Z"}
    };
    return map;
}

const std::vector<std::pair<std::string, std::string>>& getFunctionModifierCodeMap() {
    static std::vector<std::pair<std::string, std::string>> map = {
        {"PUBLIC", "Pb"},
        {"PRIVATE", "Pv"},
        {"ASYNC", "Ay"}
    };
    return map;
}

std::string typeNameToCode(const std::string& typeName) {
    for (const auto& pair : getTypeCodeMap()) {
        if (pair.first == typeName) {
            return pair.second;
        }
    }
    return "o";
}

std::string codeToTypeName(const std::string& code) {
    for (const auto& pair : getTypeCodeMap()) {
        if (pair.second == code) {
            return pair.first;
        }
    }
    return "unknown";
}

std::string encodeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (std::isalnum(c) || c == '_') {
            result += c;
        } else {
            result += '_';
            result += "0123456789abcdef"[(static_cast<unsigned char>(c) >> 4) & 0xF];
            result += "0123456789abcdef"[static_cast<unsigned char>(c) & 0xF];
            result += '_';
        }
    }
    return result;
}

std::string decodeString(const std::string& str) {
    std::string result;
    size_t i = 0;
    while (i < str.size()) {
        if (str[i] == '_' && i + 2 < str.size() && str[i + 2] == '_') {
            int highVal = (str[i + 1] >= '0' && str[i + 1] <= '9') ? str[i + 1] - '0' :
                       (str[i + 1] >= 'a' && str[i + 1] <= 'f') ? str[i + 1] - 'a' + 10 : 0;
            int lowVal = (str[i + 2] >= '0' && str[i + 2] <= '9') ? str[i + 2] - '0' :
                        (str[i + 2] >= 'a' && str[i + 2] <= 'f') ? str[i + 2] - 'a' + 10 : 0;
            result += static_cast<char>((highVal << 4) | lowVal);
            i += 3;
        } else {
            result += str[i];
            i++;
        }
    }
    return result;
}

std::string encodeComponent(const std::string& component) {
    if (component.empty()) return "_";
    
    bool needsEncoding = false;
    for (char c : component) {
        if (!std::isalnum(c) && c != '_') {
            needsEncoding = true;
            break;
        }
    }
    
    if (needsEncoding) {
        return "E" + encodeString(component) + "E";
    }
    return component;
}

std::string decodeComponent(const std::string& encoded) {
    if (encoded.size() >= 2 && encoded.front() == 'E' && encoded.back() == 'E') {
        return decodeString(encoded.substr(1, encoded.size() - 2));
    }
    return encoded;
}

std::string SymbolMangler::mangleFunctionName(const MangledFunctionParams& params) {
    std::ostringstream oss;
    oss << "_F_";
    
    if (!params.className.empty()) {
        oss << encodeComponent(params.className) << "_";
        
        if (!params.baseClassName.empty()) {
            oss << encodeComponent(params.baseClassName) << "_";
        }
        
        if (!params.classModifiers.empty()) {
            for (const auto& mod : params.classModifiers) {
                auto codeIt = std::find_if(getModifierCodeMap().begin(), getModifierCodeMap().end(),
                    [&mod](const auto& pair) { return pair.first == mod; });
                if (codeIt != getModifierCodeMap().end()) {
                    oss << codeIt->second << "_";
                }
            }
        }
        
        if (params.isConstructor) {
            oss << "CT_";
        } else if (params.isDestructor) {
            oss << "DT_";
        } else {
            oss << encodeComponent(params.functionName) << "_";
        }
        
        if (params.isStatic) oss << "static_";
        if (params.isVirtual) oss << "virtual_";
        
        if (!params.functionModifiers.empty()) {
            for (const auto& mod : params.functionModifiers) {
                auto codeIt = std::find_if(getFunctionModifierCodeMap().begin(), getFunctionModifierCodeMap().end(),
                    [&mod](const auto& pair) { return pair.first == mod; });
                if (codeIt != getFunctionModifierCodeMap().end()) {
                    oss << codeIt->second << "_";
                }
            }
        }
        
        if (!params.returnType.empty()) {
            oss << typeNameToCode(params.returnType) << "_";
        }
        
        for (const auto& param : params.parameterTypes) {
            oss << typeNameToCode(param) << "_";
        }
    } else {
        oss << encodeComponent(params.functionName) << "_";
        
        if (params.isStatic) oss << "static_";
        if (params.isVirtual) oss << "virtual_";
        
        if (!params.functionModifiers.empty()) {
            for (const auto& mod : params.functionModifiers) {
                auto codeIt = std::find_if(getFunctionModifierCodeMap().begin(), getFunctionModifierCodeMap().end(),
                    [&mod](const auto& pair) { return pair.first == mod; });
                if (codeIt != getFunctionModifierCodeMap().end()) {
                    oss << codeIt->second << "_";
                }
            }
        }
        
        if (!params.returnType.empty()) {
            oss << typeNameToCode(params.returnType) << "_";
        }
        
        for (const auto& param : params.parameterTypes) {
            oss << typeNameToCode(param) << "_";
        }
    }
    
    std::string result = oss.str();
    if (!result.empty() && result.back() == '_') {
        result.pop_back();
    }
    
    return result;
}

std::string SymbolMangler::mangleModuleSymbol(const std::vector<std::string>& modulePath,
                                             const std::string& symbolName) {
    std::ostringstream ss;
    ss << "_H_";
    
    for (const auto& part : modulePath) {
        ss << encodeComponent(part) << "_";
    }
    
    ss << encodeComponent(symbolName);
    
    return ss.str();
}

DemangledSymbol SymbolMangler::demangleSymbol(const std::string& mangledName) {
    DemangledSymbol result;
    result.originalName = mangledName;
    
    if (mangledName.empty()) {
        return result;
    }
    
    if (mangledName.find("_F_") == 0) {
        std::string content = mangledName.substr(3);
        std::vector<std::string> components;
        
        size_t pos = 0;
        while (pos < content.size()) {
            std::string comp;
            if (content[pos] == 'E') {
                size_t start = pos;
                pos++;
                while (pos < content.size() && !(content[pos] == 'E' && (pos + 1 >= content.size() || content[pos + 1] != 'E'))) {
                    pos++;
                }
                if (pos < content.size()) pos++;
                comp = decodeComponent(content.substr(start, pos - start));
            } else {
                size_t start = pos;
                while (pos < content.size() && content[pos] != '_') {
                    pos++;
                }
                if (pos < content.size()) pos++;
                comp = content.substr(start, pos - start - 1);
            }
            if (!comp.empty()) {
                components.push_back(comp);
            }
        }
        
        size_t i = 0;
        bool isMemberFunction = false;
        
        if (!components.empty()) {
            const std::string& first = components[0];
            if (first != "CT" && first != "DT" && first != "static" && first != "virtual" && 
                first != "Pb" && first != "Pv" && first != "Ay" &&
                first != "N" && first != "I" && first != "F" && first != "O" && first != "S" && 
                first != "Y" && first != "A" && first != "Z" &&
                first != "i1" && first != "i8" && first != "f" && first != "d" && 
                first != "b" && first != "c" && first != "s" && first != "v" && first != "p") {
                result.className = first;
                isMemberFunction = true;
                i++;
            }
        }
        
        while (i < components.size()) {
            const std::string& comp = components[i];
            
            if (comp == "CT") {
                result.isConstructor = true;
            } else if (comp == "DT") {
                result.isDestructor = true;
            } else if (comp == "static") {
                result.isStatic = true;
            } else if (comp == "virtual") {
                result.isVirtual = true;
            } else if (comp == "Pb") {
                result.functionModifiers.push_back("PUBLIC");
            } else if (comp == "Pv") {
                result.functionModifiers.push_back("PRIVATE");
            } else if (comp == "Ay") {
                result.functionModifiers.push_back("ASYNC");
            } else if (comp == "N") {
                result.classModifiers.push_back("SINGLETON");
            } else if (comp == "I") {
                result.classModifiers.push_back("IMMUTABLE");
            } else if (comp == "F") {
                result.classModifiers.push_back("FACTORY");
            } else if (comp == "O") {
                result.classModifiers.push_back("OBSERVABLE");
            } else if (comp == "S") {
                result.classModifiers.push_back("SERVICE");
            } else if (comp == "Y") {
                result.classModifiers.push_back("STRATEGY");
            } else if (comp == "A") {
                result.classModifiers.push_back("ACTOR");
            } else if (comp == "Z") {
                result.classModifiers.push_back("FINAL");
            } else if (comp == "i1" || comp == "i8" || comp == "f" || comp == "d" || 
                     comp == "b" || comp == "c" || comp == "s" || comp == "v" || comp == "p") {
                if (result.returnType.empty()) {
                    result.returnType = codeToTypeName(comp);
                } else {
                    result.parameterTypes.push_back(codeToTypeName(comp));
                }
            } else if (isMemberFunction && result.baseClassName.empty()) {
                result.baseClassName = comp;
                i++;
                break;
            } else {
                break;
            }
            i++;
        }
        
        if (i < components.size()) {
            const std::string& comp = components[i];
            if (!result.isConstructor && !result.isDestructor) {
                result.functionName = comp;
            }
        }
    }
    
    return result;
}

std::string SymbolMangler::demangle(const std::string& mangledName) {
    auto symbol = demangleSymbol(mangledName);
    
    if (!symbol.className.empty()) {
        std::ostringstream oss;
        oss << "func ";
        
        if (!symbol.functionModifiers.empty()) {
            for (const auto& mod : symbol.functionModifiers) {
                oss << mod << " ";
            }
        }
        
        if (symbol.isStatic) oss << "static ";
        if (symbol.isVirtual) oss << "virtual ";
        if (symbol.isConstructor) {
            oss << symbol.className << "(";
        } else if (symbol.isDestructor) {
            oss << "~" << symbol.className << "(";
        } else {
            if (!symbol.returnType.empty()) {
                oss << symbol.returnType << " ";
            }
            oss << symbol.className << "_" << symbol.originalName << "(";
        }
        
        for (size_t i = 0; i < symbol.parameterTypes.size(); i++) {
            if (i > 0) oss << ", ";
            oss << symbol.parameterTypes[i];
        }
        oss << ")";
        
        if (!symbol.baseClassName.empty()) {
            oss << " extends " << symbol.baseClassName;
        }
        
        if (!symbol.classModifiers.empty()) {
            oss << " // modifiers: ";
            for (const auto& mod : symbol.classModifiers) {
                oss << mod << " ";
            }
        }
        
        return oss.str();
    }
    
    return mangledName;
}

std::string SymbolMangler::typeKindToMangledString(const std::string& typeName) {
    return typeNameToCode(typeName);
}

std::string SymbolMangler::demangleType(const std::string& mangledType) {
    return codeToTypeName(mangledType);
}

std::string SymbolMangler::mangleType(const std::string& typeName) {
    return typeNameToCode(typeName);
}

} // namespace hooc