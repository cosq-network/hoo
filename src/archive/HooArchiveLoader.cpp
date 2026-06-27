#include "HooArchiveLoader.h"
#include "HAArchive.h"
#include "hvm/HOModule.h"

namespace hooc {
namespace archive {

HooArchiveLoader::HooArchiveLoader(HVMJIT& jit) : jit_(jit) {}

bool HooArchiveLoader::load(const std::filesystem::path& archivePath) {
    try {
        HAArchive archive(archivePath);
        const HAManifest& manifest = archive.getManifest();
        
        for (const auto& mod : manifest.modules) {
            auto payload = archive.readModule(mod.archivePath);
            auto hoModule = hvm::HOModule::parse(payload);
            if (!hoModule) {
                error_ = "Failed to parse module " + mod.moduleName + " from archive payload.";
                return false;
            }
            if (!jit_.loadModule(std::move(hoModule))) {
                error_ = "HVMJIT load failed for " + mod.moduleName + ": " + jit_.getLastError();
                return false;
            }
        }
        
        if (manifest.entryPoint) {
            entryPointSymbol_ = manifest.entryPoint->symbol;
        }
        
        return true;
    } catch (const std::exception& e) {
        error_ = e.what();
        return false;
    }
}

} // namespace archive
} // namespace hooc
