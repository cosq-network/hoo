#include "SymbolMangler.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>

namespace hooc {
namespace {
std::string trimSpaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            out.push_back(c);
        }
    }
    return out;
}

std::string toHex(const std::string& value) {
    static const char* kHex = "0123456789abcdef";
    std::string out;
    out.reserve(value.size() * 2);
    for (unsigned char c : value) {
        out.push_back(kHex[(c >> 4) & 0xF]);
        out.push_back(kHex[c & 0xF]);
    }
    return out;
}

bool fromHex(const std::string& hex, std::string& out) {
    if (hex.size() % 2 != 0) {
        return false;
    }
    out.clear();
    out.reserve(hex.size() / 2);
    auto hexVal = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hexVal(hex[i]);
        int lo = hexVal(hex[i + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return true;
}
} // namespace

const std::vector<std::pair<std::string, std::string>>& getTypeCodeMap() {
    static std::vector<std::pair<std::string, std::string>> map = {
        {"int8", "i1"},
        {"byte", "i1"},
        {"int64", "i8"},
        {"int", "i8"},
        {"float", "f"},
        {"double", "d"},
        {"f64", "d"},
        {"f8", "e"},
        {"bit", "x"},
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
        {"SERVICE", "S"},
        {"FINAL", "Z"}
    };
    return map;
}

const std::vector<std::pair<std::string, std::string>>& getFunctionModifierCodeMap() {
    static std::vector<std::pair<std::string, std::string>> map = {
        {"PUBLIC", "Pb"},
        {"PRIVATE", "Pv"}
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
        if (str[i] == '_' && i + 3 < str.size() && str[i + 3] == '_') {
            int highVal = (str[i + 1] >= '0' && str[i + 1] <= '9') ? str[i + 1] - '0' :
                       (str[i + 1] >= 'a' && str[i + 1] <= 'f') ? str[i + 1] - 'a' + 10 : 0;
            int lowVal = (str[i + 2] >= '0' && str[i + 2] <= '9') ? str[i + 2] - '0' :
                        (str[i + 2] >= 'a' && str[i + 2] <= 'f') ? str[i + 2] - 'a' + 10 : 0;
            result += static_cast<char>((highVal << 4) | lowVal);
            i += 4;
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
    
    // 1. Module Path (Unambiguous marking)
    if (!params.modulePath.empty()) {
        oss << "M_";
        for (const auto& part : params.modulePath) {
            oss << encodeComponent(part) << "_";
        }
        oss << "E_";
    }
    
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
            oss << SymbolMangler::mangleType(params.returnType) << "_";
        }
        
        for (const auto& param : params.parameterTypes) {
            oss << SymbolMangler::mangleType(param) << "_";
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
            oss << SymbolMangler::mangleType(params.returnType) << "_";
        }
        
        for (const auto& param : params.parameterTypes) {
            oss << SymbolMangler::mangleType(param) << "_";
        }
    }
    
    std::string result = oss.str();
    if (!result.empty() && result.back() == '_') {
        result.pop_back();
    }
    
    return result;
}

std::string SymbolMangler::mangleModuleSymbol(const std::vector<std::string>& modulePath,
                                             const std::string& symbolName,
                                             const std::string& kindTag) {
    std::ostringstream ss;
    ss << "_H_";
    
    for (const auto& part : modulePath) {
        ss << encodeComponent(part) << "_";
    }
    
    ss << encodeComponent(symbolName);

    if (!kindTag.empty()) {
        ss << "_" << kindTag;
    }
    
    return ss.str();
}

DemangledSymbol SymbolMangler::demangleSymbol(const std::string& mangledName) {
    DemangledSymbol result;
    result.originalName = mangledName;
    
    if (mangledName.empty()) {
        return result;
    }

    // Helper to strip kind tags (_fn, _ob, _ty, _tls, _nt, _uk)
    auto stripKindTag = [](std::string& name) {
        static const std::vector<std::string> kindTags = {"_fn", "_ob", "_ty", "_tls", "_nt", "_uk"};
        for (const auto& tag : kindTags) {
            if (name.length() >= tag.length()) {
                std::string suffix = name.substr(name.length() - tag.length());
                if (suffix == tag) {
                    name = name.substr(0, name.length() - tag.length());
                    return;
                }
            }
        }
    };

    stripKindTag(result.originalName);

    auto splitComponents = [](const std::string& content) {
        std::vector<std::string> components;
        size_t pos = 0;
        while (pos < content.size()) {
            std::string comp;
            if (content[pos] == 'E') {
                size_t start = pos;
                size_t endEncoded = std::string::npos;
                size_t searchPos = pos + 1;
                while (searchPos < content.size()) {
                    size_t ePos = content.find('E', searchPos);
                    if (ePos == std::string::npos) break;
                    if (ePos + 1 == content.size() || content[ePos + 1] == '_') {
                        endEncoded = ePos;
                        break;
                    }
                    searchPos = ePos + 1;
                }
                if (endEncoded == std::string::npos) {
                    size_t nextDelim = content.find('_', pos);
                    if (nextDelim == std::string::npos) { comp = content.substr(pos); pos = content.size(); }
                    else { comp = content.substr(pos, nextDelim - pos); pos = nextDelim + 1; }
                } else {
                    comp = decodeComponent(content.substr(start, endEncoded - start + 1));
                    pos = endEncoded + 1;
                    if (pos < content.size() && content[pos] == '_') pos++;
                }
            } else {
                size_t start = pos;
                while (pos < content.size() && content[pos] != '_') pos++;
                size_t end = pos;
                if (pos < content.size() && content[pos] == '_') pos++;
                comp = content.substr(start, end - start);
            }
            if (!comp.empty()) components.push_back(comp);
        }
        return components;
    };

    if (mangledName.find("_F_") == 0) {
        std::string content = mangledName.substr(3);
        stripKindTag(content);
        std::vector<std::string> components = splitComponents(content);
        
        auto isFunctionModifierCode = [](const std::string& comp) {
            return comp == "Pb" || comp == "Pv";
        };
        auto isClassModifierCode = [](const std::string& comp) {
            return comp == "N" || comp == "I" ||
                   comp == "S" || comp == "Z";
        };
        auto pushClassModifier = [&](const std::string& comp) {
            if (comp == "N") result.classModifiers.push_back("SINGLETON");
            else if (comp == "I") result.classModifiers.push_back("IMMUTABLE");
            else if (comp == "S") result.classModifiers.push_back("SERVICE");
            else if (comp == "Z") result.classModifiers.push_back("FINAL");
        };
        auto pushFunctionModifier = [&](const std::string& comp) {
            if (comp == "Pb") result.functionModifiers.push_back("PUBLIC");
            else if (comp == "Pv") result.functionModifiers.push_back("PRIVATE");
        };

        size_t i = 0;
        // 1. Module Path (with markers)
        if (i < components.size() && components[i] == "M") {
            i++;
            while (i < components.size() && components[i] != "E") {
                result.modulePath.push_back(components[i]);
                i++;
            }
            if (i < components.size() && components[i] == "E") i++;
        }

        auto isSpecialToken = [&](const std::string& comp) {
            return comp == "CT" || comp == "DT" || comp == "static" || comp == "virtual" ||
                   isFunctionModifierCode(comp) || isClassModifierCode(comp) ||
                   demangleType(comp) != "unknown";
        };

        // 2. Extract potential names (ClassName, BaseClassName, FunctionName)
        std::vector<std::string> names;
        size_t nameStartIdx = i;
        while (i < components.size() && !isSpecialToken(components[i])) {
            names.push_back(components[i]);
            i++;
        }

        // 3. Class Modifiers (must be before CT/DT or FunctionName)
        while (i < components.size() && isClassModifierCode(components[i])) {
            pushClassModifier(components[i]);
            i++;
        }

        // 4. Handle CT/DT or FunctionName
        bool hasSpecialCtor = false;
        if (i < components.size()) {
            if (components[i] == "CT") {
                result.isConstructor = true;
                hasSpecialCtor = true;
                i++;
            } else if (components[i] == "DT") {
                result.isDestructor = true;
                hasSpecialCtor = true;
                i++;
            }
        }

        // 5. Categorize names
        if (hasSpecialCtor || !result.classModifiers.empty()) {
            // It's definitely a class member.
            if (names.size() >= 2) {
                result.className = names[0];
                result.baseClassName = names[1];
                if (names.size() > 2) result.functionName = names[2];
            } else if (names.size() == 1) {
                result.className = names[0];
            }
        } else {
            // Ambiguous or plain function.
            if (names.size() >= 3) {
                // Class _ Base _ Func
                result.className = names[0];
                result.baseClassName = names[1];
                result.functionName = names[2];
            } else if (names.size() == 2) {
                // Class _ Func
                result.className = names[0];
                result.functionName = names[1];
            } else if (names.size() == 1) {
                // Legacy/Simple case: treat as className
                result.className = names[0];
            }
        }

        // 6. Function Modifiers & Signature
        while (i < components.size()) {
            const std::string& comp = components[i];
            if (comp == "static") result.isStatic = true;
            else if (comp == "virtual") result.isVirtual = true;
            else if (isFunctionModifierCode(comp)) pushFunctionModifier(comp);
            else {
                std::string demangledType = demangleType(comp);
                if (demangledType == "unknown") break;
                if (result.returnType.empty()) result.returnType = demangledType;
                else result.parameterTypes.push_back(demangledType);
            }
            i++;
        }
    } else if (mangledName.find("_H_") == 0) {
        std::string content = mangledName.substr(3);
        stripKindTag(content);
        std::vector<std::string> components = splitComponents(content);
        if (!components.empty()) {
            result.functionName = components.back();
            components.pop_back();
            result.modulePath = components;
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
    if (mangledType.empty()) {
        return "unknown";
    }

    // Backward-compatible primitive decoding path.
    std::string primitive = codeToTypeName(mangledType);
    if (primitive != "unknown") {
        return primitive;
    }

    size_t pos = 0;
    std::function<std::string()> parseType = [&]() -> std::string {
        if (pos >= mangledType.size()) {
            return "unknown";
        }

        // Primitive tokens used inside structured encodings.
        if (mangledType.compare(pos, 2, "i8") == 0) {
            pos += 2;
            return "int64";
        }
        if (mangledType.compare(pos, 2, "i1") == 0) {
            pos += 2;
            return "int8";
        }
        if (mangledType[pos] == 'f') {
            pos++;
            return "float";
        }
        if (mangledType[pos] == 'd') {
            pos++;
            return "double";
        }
        if (mangledType[pos] == 'e') {
            pos++;
            return "f8";
        }
        if (mangledType[pos] == 'x') {
            pos++;
            return "bit";
        }
        if (mangledType[pos] == 'b') {
            pos++;
            return "bool";
        }
        if (mangledType[pos] == 'c') {
            pos++;
            return "char";
        }
        if (mangledType[pos] == 's') {
            pos++;
            return "string";
        }
        if (mangledType[pos] == 'v') {
            pos++;
            return "void";
        }
        if (mangledType[pos] == 'p') {
            pos++;
            return "ptr";
        }

        if (mangledType[pos] == 'O') {
            pos++;
            std::string inner = parseType();
            if (inner == "unknown") {
                return "unknown";
            }
            return inner + "?";
        }

        if (mangledType[pos] == 'A') {
            pos++;
            std::string inner = parseType();
            if (inner == "unknown") {
                return "unknown";
            }
            return inner + "[]";
        }

        if (mangledType[pos] == 'M') {
            pos++;
            std::string key = parseType();
            std::string value = parseType();
            if (key == "unknown" || value == "unknown") {
                return "unknown";
            }
            return "map[" + key + "," + value + "]";
        }

        if (mangledType[pos] == 'Q') {
            pos++;
            size_t end = mangledType.find('Z', pos);
            if (end == std::string::npos) {
                return "unknown";
            }
            std::string raw;
            if (!fromHex(mangledType.substr(pos, end - pos), raw)) {
                return "unknown";
            }
            pos = end + 1;
            return raw;
        }

        return "unknown";
    };

    std::string decoded = parseType();
    if (decoded == "unknown" || pos != mangledType.size()) {
        return "unknown";
    }
    return decoded;
}

std::string SymbolMangler::mangleType(const std::string& typeName) {
    std::string normalized = trimSpaces(typeName);
    if (normalized.empty()) {
        return "o";
    }
    std::string primitive = typeNameToCode(normalized);
    if (primitive != "o") {
        return primitive;
    }

    size_t pos = 0;
    std::function<std::string()> parseType = [&]() -> std::string {
        // map[K,V]
        if (normalized.compare(pos, 4, "map[") == 0) {
            pos += 4;
            std::string key = parseType();
            if (key.empty() || pos >= normalized.size() || normalized[pos] != ',') {
                return "";
            }
            pos++;
            std::string value = parseType();
            if (value.empty() || pos >= normalized.size() || normalized[pos] != ']') {
                return "";
            }
            pos++;
            std::string out = "M" + key + value;
            if (pos < normalized.size() && normalized[pos] == '?') {
                pos++;
                out = "O" + out;
            }
            return out;
        }

        // base identifier / qualified identifier
        size_t start = pos;
        while (pos < normalized.size()) {
            char c = normalized[pos];
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.') {
                pos++;
            } else {
                break;
            }
        }
        if (start == pos) {
            return "";
        }

        std::string base = normalized.substr(start, pos - start);
        std::string code = typeNameToCode(base);
        if (code == "o") {
            code = "Q" + toHex(base) + "Z";
        }

        // array suffixes
        while (pos + 1 < normalized.size() && normalized[pos] == '[' && normalized[pos + 1] == ']') {
            pos += 2;
            code = "A" + code;
        }

        // nullable suffix
        if (pos < normalized.size() && normalized[pos] == '?') {
            pos++;
            code = "O" + code;
        }
        return code;
    };

    std::string encoded = parseType();
    if (encoded.empty() || pos != normalized.size()) {
        return "Q" + toHex(normalized) + "Z";
    }
    return encoded;
}

} // namespace hooc
