#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace hooc {
namespace archive {

/// Compute the hex-encoded SHA-256 hash of a byte buffer.
std::string computeSha256(const uint8_t* data, size_t len);

/// Compute the hex-encoded SHA-256 hash of a vector of bytes.
std::string computeSha256(const std::vector<uint8_t>& data);

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
