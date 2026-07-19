#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "hvm/HVMJIT.h"

namespace hooc {
namespace archive {

class HooArchiveLoader {
public:
    /// Supported archive format version. Bump when the manifest schema changes.
    static constexpr int SUPPORTED_FORMAT_VERSION = 1;

    explicit HooArchiveLoader(HVMJIT& jit);
    bool load(const std::filesystem::path& archivePath);
    
    std::string getLastError() const { return error_; }
    const std::string& getEntryPointSymbol() const { return entryPointSymbol_; }
    int getLoadedFormatVersion() const { return loadedFormatVersion_; }

private:
    /// Resolve entry point when manifest.entryPoint is absent.
    /// Scans loaded modules for exported main functions.
    /// Sets error_ if zero or more than one main is found.
    /// @param moduleNames  names of loaded modules in topological order.
    bool resolveEntryPointFromModules(const std::vector<std::string>& moduleNames);

    HVMJIT& jit_;
    std::string error_;
    std::string entryPointSymbol_;
    int loadedFormatVersion_ = 0;
};

} // namespace archive
} // namespace hooc
