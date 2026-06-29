#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "HAManifest.h"
#include <memory>

struct zip;

namespace hooc {
namespace archive {

class HAArchive {
public:
    explicit HAArchive(const std::filesystem::path& archivePath);
    explicit HAArchive(const std::vector<uint8_t>& memoryArchive);
    ~HAArchive();

    // Prevent copying
    HAArchive(const HAArchive&) = delete;
    HAArchive& operator=(const HAArchive&) = delete;

    // Get the parsed manifest
    const HAManifest& getManifest() const { return manifest_; }

    // Read a module payload by its archive path (e.g. "modules/main.ho")
    std::vector<uint8_t> readModule(const std::string& archivePath) const;

private:
    void initFromMemory(const std::vector<uint8_t>& compressedData);
    
    zip* zipArchive_ = nullptr;
    void* zipSource_ = nullptr;
    std::vector<uint8_t> uncompressedZipData_;
    HAManifest manifest_;
};

} // namespace archive
} // namespace hooc
