#include "HooArchiveCompiler.h"
#include "core/HooCompiler.h"
#include "hvm/HOModule.h"
#include "HAArchiveBuilder.h"
#include "HAManifest.h"
#include <stdexcept>

namespace hooc {
namespace archive {

HooBuildPlanner::HooBuildPlanner(IOProvider& ioProvider) : ioProvider_(ioProvider) {}

std::vector<ResolvedModule> HooBuildPlanner::plan(const std::filesystem::path& rootSourcePath) {
    LocalImportResolver resolver(ioProvider_);
    return resolver.resolve(rootSourcePath);
}

HooArchiveCompiler::HooArchiveCompiler(IOProvider& ioProvider) : ioProvider_(ioProvider) {}

void HooArchiveCompiler::compile(const std::vector<ResolvedModule>& modules, const std::filesystem::path& outPath) {
    HAArchiveBuilder builder;
    HAManifest manifest;
    
    for (const auto& modInfo : modules) {
        auto sourceCode = ioProvider_.readFile(modInfo.sourcePath.string());
        if (!sourceCode) {
            throw std::runtime_error("Could not read " + modInfo.sourcePath.string());
        }

        HooCompiler compiler;
        auto hoModule = compiler.compile(modInfo.moduleName, *sourceCode);
        if (!hoModule) {
            throw std::runtime_error("Compile error in " + modInfo.sourcePath.string() + ": " + compiler.getLastError());
        }

        std::vector<uint8_t> payload;
        if (!hoModule->serialize(payload)) {
            throw std::runtime_error("Failed to serialize module " + modInfo.moduleName);
        }

        std::string archivePath = "modules/" + modInfo.moduleName + ".ho";
        
        builder.addModule(modInfo.moduleName, archivePath, payload);

        HAModuleInfo hmInfo;
        hmInfo.moduleName = modInfo.moduleName;
        hmInfo.sourcePath = modInfo.sourcePath.string();
        hmInfo.archivePath = archivePath;
        hmInfo.imports = modInfo.localImports;
        
        // Extract symbols
        for (const auto& sym : hoModule->getSymbols()) {
            if (sym.binding == hvm::Symbol::STB_GLOBAL) {
                HASymbolInfo sInfo;
                sInfo.mangled = sym.name;
                sInfo.name = sym.name; // In a real scenario we'd demangle
                sInfo.visibility = "public";
                if (sym.type == hvm::Symbol::STT_FUNC) {
                    sInfo.kind = "function";
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
