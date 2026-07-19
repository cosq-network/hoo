#include "HAManifest.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sstream>
#include <iomanip>

#if __APPLE__
#include <CommonCrypto/CommonDigest.h>
#define HOO_SHA256_CTX CC_SHA256_CTX
#define HOO_SHA256_Init CC_SHA256_Init
#define HOO_SHA256_Update CC_SHA256_Update
#define HOO_SHA256_Final CC_SHA256_Final
#define HOO_SHA256_DIGEST_LENGTH CC_SHA256_DIGEST_LENGTH
#else
#include <openssl/sha.h>
#define HOO_SHA256_CTX SHA256_CTX
#define HOO_SHA256_Init SHA256_Init
#define HOO_SHA256_Update SHA256_Update
#define HOO_SHA256_Final SHA256_Final
#define HOO_SHA256_DIGEST_LENGTH SHA256_DIGEST_LENGTH
#endif

namespace hooc {
namespace archive {

std::string computeSha256(const uint8_t* data, size_t len) {
    HOO_SHA256_CTX ctx;
    HOO_SHA256_Init(&ctx);
    HOO_SHA256_Update(&ctx, data, len);
    uint8_t hash[HOO_SHA256_DIGEST_LENGTH];
    HOO_SHA256_Final(hash, &ctx);

    std::ostringstream oss;
    for (int i = 0; i < HOO_SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string computeSha256(const std::vector<uint8_t>& data) {
    return computeSha256(data.data(), data.size());
}

using json = nlohmann::json;

#ifndef HOO_VERSION
#define HOO_VERSION "0.0.0"
#endif

HAManifest::HAManifest() : format("hoo-archive"), formatVersion(1) {
    createdBy.tool = "hoo";
    createdBy.version = HOO_VERSION;
}

std::string HAManifest::toJson() const {
    json j;
    j["format"] = format;
    j["formatVersion"] = formatVersion;
    j["createdBy"]["tool"] = createdBy.tool;
    j["createdBy"]["version"] = createdBy.version;
    
    if (entryPoint) {
        j["entryPoint"]["source"] = entryPoint->source;
        j["entryPoint"]["module"] = entryPoint->moduleName;
        j["entryPoint"]["symbol"] = entryPoint->symbol;
    }

    json modulesArray = json::array();
    for (const auto& mod : modules) {
        json modObj;
        modObj["module"] = mod.moduleName;
        modObj["sourcePath"] = mod.sourcePath;
        modObj["archivePath"] = mod.archivePath;
        modObj["sha256"] = mod.sha256;
        modObj["imports"] = mod.imports;
        
        json symArray = json::array();
        for (const auto& sym : mod.symbols) {
            json symObj;
            symObj["name"] = sym.name;
            symObj["mangled"] = sym.mangled;
            symObj["kind"] = sym.kind;
            symObj["returnType"] = sym.returnType;
            symObj["visibility"] = sym.visibility;
            symArray.push_back(symObj);
        }
        modObj["symbols"] = symArray;
        modulesArray.push_back(modObj);
    }
    j["modules"] = modulesArray;

    json symIndex;
    for (const auto& pair : symbolIndex) {
        symIndex[pair.first]["archivePath"] = pair.second;
        // The issue specifies {"module": "a", "archivePath": "..."} but module name can be inferred or added to index
    }
    j["symbolIndex"] = symIndex;

    json modIndex;
    for (const auto& pair : moduleIndex) {
        modIndex[pair.first] = pair.second;
    }
    j["moduleIndex"] = modIndex;

    return j.dump(); // Minified
}

HAManifest HAManifest::fromJson(const std::string& jsonString) {
    HAManifest manifest;
    try {
        json j = json::parse(jsonString);
        
        manifest.format = j.value("format", "");
        if (manifest.format != "hoo-archive") {
            throw std::runtime_error("Invalid format: " + manifest.format);
        }
        manifest.formatVersion = j.value("formatVersion", 0);
        
        if (j.contains("createdBy")) {
            manifest.createdBy.tool = j["createdBy"].value("tool", "");
            manifest.createdBy.version = j["createdBy"].value("version", "");
        }

        if (j.contains("entryPoint")) {
            HAEntryPoint ep;
            ep.source = j["entryPoint"].value("source", "");
            ep.moduleName = j["entryPoint"].value("module", "");
            ep.symbol = j["entryPoint"].value("symbol", "");
            manifest.entryPoint = ep;
        }

        if (j.contains("modules") && j["modules"].is_array()) {
            for (const auto& modObj : j["modules"]) {
                HAModuleInfo mod;
                mod.moduleName = modObj.value("module", "");
                mod.sourcePath = modObj.value("sourcePath", "");
                mod.archivePath = modObj.value("archivePath", "");
                mod.sha256 = modObj.value("sha256", "");
                
                if (modObj.contains("imports") && modObj["imports"].is_array()) {
                    for (const auto& imp : modObj["imports"]) {
                        mod.imports.push_back(imp.get<std::string>());
                    }
                }
                
                if (modObj.contains("symbols") && modObj["symbols"].is_array()) {
                    for (const auto& symObj : modObj["symbols"]) {
                        HASymbolInfo sym;
                        sym.name = symObj.value("name", "");
                        sym.mangled = symObj.value("mangled", "");
                        sym.kind = symObj.value("kind", "");
                        sym.returnType = symObj.value("returnType", "");
                        sym.visibility = symObj.value("visibility", "");
                        mod.symbols.push_back(sym);
                    }
                }
                manifest.modules.push_back(mod);
            }
        }

        if (j.contains("symbolIndex") && j["symbolIndex"].is_object()) {
            for (const auto& item : j["symbolIndex"].items()) {
                manifest.symbolIndex[item.key()] = item.value().value("archivePath", "");
            }
        }

        if (j.contains("moduleIndex") && j["moduleIndex"].is_object()) {
            for (const auto& item : j["moduleIndex"].items()) {
                manifest.moduleIndex[item.key()] = item.value().get<std::string>();
            }
        }

    } catch (const json::exception& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }
    
    return manifest;
}

} // namespace archive
} // namespace hooc
