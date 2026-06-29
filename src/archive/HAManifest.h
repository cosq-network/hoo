#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace hooc {
namespace archive {

struct HASymbolInfo {
    std::string name;
    std::string mangled;
    std::string kind;
    std::string returnType;
    std::string visibility;
};

struct HAModuleInfo {
    std::string moduleName;
    std::string sourcePath;
    std::string archivePath;
    std::string sha256;
    std::vector<std::string> imports;
    std::vector<HASymbolInfo> symbols;
};

struct HAEntryPoint {
    std::string source;
    std::string moduleName;
    std::string symbol;
};

struct HACreatedBy {
    std::string tool;
    std::string version;
};

class HAManifest {
public:
    std::string format;
    int formatVersion;
    HACreatedBy createdBy;
    std::optional<HAEntryPoint> entryPoint;
    
    std::vector<HAModuleInfo> modules;
    std::unordered_map<std::string, std::string> symbolIndex; // mangled name -> archivePath
    std::unordered_map<std::string, std::string> moduleIndex; // moduleName -> archivePath

    HAManifest();

    // Serialize to minified JSON string
    std::string toJson() const;

    // Deserialize from JSON string, throws runtime_error on invalid format
    static HAManifest fromJson(const std::string& jsonString);
};

} // namespace archive
} // namespace hooc
