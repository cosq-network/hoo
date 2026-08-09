#include "HooArchiveCompiler.h"
#include "core/HooCompiler.h"
#include "hvm/HOModule.h"
#include "HAArchiveBuilder.h"
#include "HAManifest.h"
#include "core/SymbolMangler.h"
#include <stdexcept>
#include <sstream>
#include <unordered_map>

namespace hooc {
namespace archive {

namespace {
std::string rawMangledFunctionStem(const std::string& symbolName) {
    if (symbolName.rfind("_F_", 0) != 0) {
        return symbolName;
    }

    std::string content = symbolName.substr(3);
    if (content.rfind("M_", 0) == 0) {
        const size_t moduleEnd = content.find("_E_");
        if (moduleEnd != std::string::npos) {
            content = content.substr(moduleEnd + 3);
        }
    }

    std::vector<std::string> parts;
    std::stringstream ss(content);
    std::string part;
    while (std::getline(ss, part, '_')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    while (!parts.empty() && SymbolMangler::demangleType(parts.back()) != "unknown") {
        parts.pop_back();
    }

    std::string stem;
    for (const auto& token : parts) {
        if (!stem.empty()) {
            stem += "_";
        }
        stem += token;
    }
    return stem;
}
}

HooBuildPlanner::HooBuildPlanner(IOProvider& ioProvider) : ioProvider_(ioProvider) {}

std::vector<ResolvedModule> HooBuildPlanner::plan(const std::filesystem::path& rootSourcePath) {
    LocalImportResolver resolver(ioProvider_);
    return resolver.resolve(rootSourcePath);
}

HooArchiveCompiler::HooArchiveCompiler(IOProvider& ioProvider) : ioProvider_(ioProvider) {}

void HooArchiveCompiler::compile(const std::vector<ResolvedModule>& modules, const std::filesystem::path& outPath) {
    HAArchiveBuilder builder;
    HAManifest manifest;
    std::unordered_map<std::string, ExternalFunctionMetadataSets> exportsByModule;
    
    for (const auto& modInfo : modules) {
        auto sourceCode = ioProvider_.readFile(modInfo.sourcePath.string());
        if (!sourceCode) {
            throw std::runtime_error("Could not read " + modInfo.sourcePath.string());
        }

        HooCompiler compiler;
        ExternalFunctionMetadataSets visibleExternalFunctions;
        for (const auto& importName : modInfo.localImports) {
            auto exportIt = exportsByModule.find(importName);
            if (exportIt == exportsByModule.end()) {
                continue;
            }
            for (const auto& [functionName, overloads] : exportIt->second) {
                auto& destination = visibleExternalFunctions[functionName];
                destination.insert(destination.end(), overloads.begin(), overloads.end());
            }
        }
        compiler.setExternalFunctionMetadataSets(visibleExternalFunctions);
        auto hoModule = compiler.compile(modInfo.moduleName, *sourceCode);
        if (!hoModule) {
            throw std::runtime_error("Compile error in " + modInfo.sourcePath.string() + ": " + compiler.getLastError());
        }

        std::vector<uint8_t> payload;
        if (!hoModule->serialize(payload)) {
            throw std::runtime_error("Failed to serialize module " + modInfo.moduleName);
        }

        std::string archivePath = "modules/" + modInfo.moduleName + ".ho";
        
        // Compute SHA-256 of the serialized module payload
        std::string payloadSha256 = computeSha256(payload);
        
        builder.addModule(modInfo.moduleName, archivePath, payload);

        HAModuleInfo hmInfo;
        hmInfo.moduleName = modInfo.moduleName;
        hmInfo.sourcePath = modInfo.sourcePath.string();
        hmInfo.archivePath = archivePath;
        hmInfo.sha256 = payloadSha256;
        hmInfo.imports = modInfo.localImports;
        
        // Extract symbols
        const auto& sourceMetadata = compiler.getExportedFunctionMetadataSets();
        for (const auto& sym : hoModule->getSymbols()) {
            if (sym.binding == hvm::Symbol::STB_GLOBAL) {
                HASymbolInfo sInfo;
                sInfo.mangled = sym.name;
                sInfo.name = sym.name; // In a real scenario we'd demangle
                sInfo.visibility = "public";
                if (sym.type == hvm::Symbol::STT_FUNC) {
                    sInfo.kind = "function";
                    auto demangled = SymbolMangler::demangleSymbol(sym.name);
                    std::string functionName = rawMangledFunctionStem(sym.name);
                    if (functionName.empty()) {
                        functionName = demangled.functionName.empty()
                            ? demangled.className
                            : demangled.functionName;
                    }
                    if (!functionName.empty()) {
                        ExternalFunctionInfo info;
                        auto metadataIt = sourceMetadata.find(functionName);
                        if (metadataIt != sourceMetadata.end() && !metadataIt->second.empty()) {
                            info = metadataIt->second.front();
                        }
                        info.modulePath = modInfo.moduleName;
                        if (info.returnType.empty()) info.returnType = demangled.returnType;
                        if (info.parameterTypes.empty()) info.parameterTypes = demangled.parameterTypes;
                        auto& exportedOverloads = exportsByModule[modInfo.moduleName][functionName];
                        bool alreadyPresent = false;
                        for (const auto& existing : exportedOverloads) {
                            if (existing.parameterTypes == info.parameterTypes &&
                                existing.returnType == info.returnType) {
                                alreadyPresent = true;
                                break;
                            }
                        }
                        if (!alreadyPresent) exportedOverloads.push_back(std::move(info));
                    }
                }
                hmInfo.symbols.push_back(sInfo);
            }
        }
        
        manifest.modules.push_back(hmInfo);
        manifest.moduleIndex[modInfo.moduleName] = archivePath;
        for (const auto& sym : hmInfo.symbols) {
            manifest.symbolIndex[sym.mangled] = archivePath;
        }
    }
    
    // Find entry point main if one exists
    for (const auto& m : manifest.modules) {
        for (const auto& s : m.symbols) {
            if (s.mangled.find("_F_M_") == 0 && s.mangled.find("_E_main") != std::string::npos) {
                HAEntryPoint ep;
                ep.source = m.sourcePath;
                ep.moduleName = m.moduleName;
                ep.symbol = s.mangled;
                manifest.entryPoint = ep;
                break;
            }
        }
        if (manifest.entryPoint) break;
    }

    builder.setManifest(manifest);
    builder.write(outPath);
}

} // namespace archive
} // namespace hooc
