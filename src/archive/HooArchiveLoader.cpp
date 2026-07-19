#include "HooArchiveLoader.h"
#include "HAArchive.h"
#include "hvm/HOModule.h"
#include "core/SymbolMangler.h"
#include <algorithm>

namespace hooc {
namespace archive {

HooArchiveLoader::HooArchiveLoader(HVMJIT& jit) : jit_(jit) {}

bool HooArchiveLoader::load(const std::filesystem::path& archivePath) {
    try {
        HAArchive archive(archivePath);
        const HAManifest& manifest = archive.getManifest();

        // --- Format version validation ---
        loadedFormatVersion_ = manifest.formatVersion;
        if (manifest.formatVersion < 1 || manifest.formatVersion > SUPPORTED_FORMAT_VERSION) {
            error_ = "Unsupported Hoo archive format version " +
                     std::to_string(manifest.formatVersion) +
                     " (supported: 1)";
            return false;
        }

        // --- Load all modules into JIT, collecting module names in topological order ---
        std::vector<std::string> moduleNames;
        for (const auto& mod : manifest.modules) {
            auto payload = archive.readModule(mod.archivePath);
            auto hoModule = hvm::HOModule::parse(payload);
            if (!hoModule) {
                error_ = "Failed to parse module " + mod.moduleName + " from archive payload.";
                return false;
            }
            if (hoModule->getName().empty() || hoModule->getName() == "unnamed_module") {
                hoModule->setName(mod.moduleName);
            }
            moduleNames.push_back(mod.moduleName);
            if (!jit_.loadModule(std::move(hoModule))) {
                error_ = "HVMJIT load failed for " + mod.moduleName + ": " + jit_.getLastError();
                return false;
            }
        }

        // --- Resolve entry point ---
        if (manifest.entryPoint) {
            entryPointSymbol_ = manifest.entryPoint->symbol;
        } else {
            // No explicit entry point in manifest — scan loaded modules for exported main
            if (!resolveEntryPointFromModules(moduleNames)) {
                return false;  // error_ already set
            }
        }

        return true;
    } catch (const std::exception& e) {
        error_ = e.what();
        return false;
    }
}

bool HooArchiveLoader::resolveEntryPointFromModules(const std::vector<std::string>& moduleNames) {
    // Strategy: probe the JIT for exported main functions in each loaded module.
    // Main functions follow the mangled pattern: _F_M_<module>_E_main_<returnType>
    // We also check the legacy unqualified pattern: _F_main_<returnType>
    //
    // We accept exactly one match. Zero matches or more than one match is an error.

    // Common return type suffixes in Hoo's mangling scheme:
    //   v  = void, i8 = int64, S = string, p = ptr/any, f64 = float64
    static const std::string_view kReturnTypeSuffixes[] = {"v", "i8", "S", "p", "f64"};

    std::string resolvedSymbol;
    int matchCount = 0;

    // 1. Check legacy unqualified main (single-file programs)
    for (const auto& suffix : kReturnTypeSuffixes) {
        std::string candidate = "_F_main_" + std::string(suffix);
        if (jit_.getSymbolAddress(candidate)) {
            resolvedSymbol = candidate;
            ++matchCount;
        }
    }

    // 2. Check module-qualified main for each loaded module
    for (const auto& moduleName : moduleNames) {
        for (const auto& suffix : kReturnTypeSuffixes) {
            // Module names can contain dots (e.g., "pkg.math_utils"). In mangled form,
            // dots are replaced with underscores and the module is prefixed with _M_ and
            // suffixed with _E_.  Example: _F_M_pkg_math_utils_E_main_v
            std::string mangledModule = moduleName;
            std::replace(mangledModule.begin(), mangledModule.end(), '.', '_');

            std::string candidate = "_F_M_" + mangledModule + "_E_main_" + std::string(suffix);
            if (jit_.getSymbolAddress(candidate)) {
                // Avoid double-counting if legacy and module-qualified resolve to the
                // same symbol (shouldn't happen in practice, but be safe).
                if (candidate != resolvedSymbol) {
                    resolvedSymbol = candidate;
                    ++matchCount;
                }
            }
        }
    }

    if (matchCount == 0) {
        error_ = "Execution failed: No entry point found in archive. "
                 "Archive does not define an explicit entryPoint in its manifest, "
                 "and no exported main function could be resolved.";
        return false;
    }

    if (matchCount > 1) {
        error_ = "Archive contains multiple exported main functions; "
                 "entry point is ambiguous. Add an explicit entryPoint to the archive manifest.";
        return false;
    }

    entryPointSymbol_ = resolvedSymbol;
    return true;
}

} // namespace archive
} // namespace hooc
