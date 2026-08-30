#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "HAManifest.h"
#include <unordered_map>

// Windows' <io.h> (and the forced-include test compat header) map the POSIX
// name `write` to `_write` via a macro. That would corrupt the member function
// below into `_write`, producing a mismatch between the declaration seen by
// callers and the definition compiled into hoo-core. Undefine it so the member
// name `write` is preserved across translation units.
#ifdef write
#undef write
#endif

namespace hooc {
namespace archive {

class HAArchiveBuilder {
public:
    HAArchiveBuilder() = default;

    // Add a compiled .ho module payload
    void addModule(const std::string& moduleName, const std::string& archivePath, const std::vector<uint8_t>& payload);

    // Set the archive manifest
    void setManifest(const HAManifest& manifest);

    // Write the full .ha archive atomically (ZIP container + ZSTD compression)
    void write(const std::filesystem::path& outPath);

private:
    HAManifest manifest_;
    std::unordered_map<std::string, std::vector<uint8_t>> modules_; // archivePath -> payload
};

} // namespace archive
} // namespace hooc
