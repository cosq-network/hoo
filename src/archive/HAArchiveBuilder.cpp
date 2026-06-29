#include "HAArchiveBuilder.h"
#include <zip.h>
#include <zstd.h>
#include <fstream>
#include <stdexcept>
#include <random>

namespace hooc {
namespace archive {

void HAArchiveBuilder::addModule(const std::string& moduleName, const std::string& archivePath, const std::vector<uint8_t>& payload) {
    modules_[archivePath] = payload;
}

void HAArchiveBuilder::setManifest(const HAManifest& manifest) {
    manifest_ = manifest;
}

void HAArchiveBuilder::write(const std::filesystem::path& outPath) {
    std::filesystem::path tempZipPath = outPath;
    tempZipPath += ".tmp.zip";
    
    // 1. Create a ZIP archive
    int error = 0;
    zip_t* z = zip_open(tempZipPath.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!z) {
        throw std::runtime_error("Failed to create temporary zip at " + tempZipPath.string());
    }

    // Write manifest
    std::string manifestJson = manifest_.toJson();
    zip_source_t* sManifest = zip_source_buffer(z, manifestJson.c_str(), manifestJson.size(), 0);
    if (!sManifest) {
        zip_close(z);
        throw std::runtime_error("Failed to create zip source for manifest");
    }
    
    // Add META-INF dir (implied by adding a file inside it, but some tools prefer explicit dirs)
    zip_dir_add(z, "META-INF", ZIP_FL_ENC_UTF_8);
    zip_dir_add(z, "META-INF/hoo", ZIP_FL_ENC_UTF_8);
    
    zip_int64_t idx = zip_file_add(z, "META-INF/hoo/archive.json", sManifest, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
    if (idx < 0) {
        zip_source_free(sManifest);
        zip_close(z);
        throw std::runtime_error("Failed to add archive.json to zip");
    }
    zip_set_file_compression(z, idx, ZIP_CM_STORE, 0); // Uncompressed inside ZIP

    // Add modules
    zip_dir_add(z, "modules", ZIP_FL_ENC_UTF_8);
    for (const auto& [archivePath, payload] : modules_) {
        zip_source_t* sMod = zip_source_buffer(z, payload.data(), payload.size(), 0);
        if (!sMod) {
            zip_close(z);
            throw std::runtime_error("Failed to create zip source for module " + archivePath);
        }
        zip_int64_t midx = zip_file_add(z, archivePath.c_str(), sMod, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
        if (midx < 0) {
            zip_source_free(sMod);
            zip_close(z);
            throw std::runtime_error("Failed to add " + archivePath + " to zip");
        }
        zip_set_file_compression(z, midx, ZIP_CM_STORE, 0); // Uncompressed inside ZIP
    }

    if (zip_close(z) < 0) {
        throw std::runtime_error("Failed to close/write temporary zip: " + tempZipPath.string());
    }

    // 2. Read the ZIP file fully into memory
    std::ifstream zipFile(tempZipPath, std::ios::binary | std::ios::ate);
    if (!zipFile) {
        throw std::runtime_error("Failed to read temporary zip");
    }
    std::streamsize zipSize = zipFile.tellg();
    zipFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> zipData(zipSize);
    if (!zipFile.read(reinterpret_cast<char*>(zipData.data()), zipSize)) {
        throw std::runtime_error("Failed to read temporary zip data");
    }
    zipFile.close();

    // 3. Compress with ZSTD
    size_t const cBuffSize = ZSTD_compressBound(zipSize);
    std::vector<uint8_t> cBuff(cBuffSize);
    size_t const cSize = ZSTD_compress(cBuff.data(), cBuffSize, zipData.data(), zipSize, 3);
    if (ZSTD_isError(cSize)) {
        std::filesystem::remove(tempZipPath);
        throw std::runtime_error("ZSTD compression failed: " + std::string(ZSTD_getErrorName(cSize)));
    }

    // 4. Write to final path atomically
    std::filesystem::path tempOutPath = outPath;
    tempOutPath += ".tmp";
    std::ofstream outFile(tempOutPath, std::ios::binary);
    if (!outFile) {
        std::filesystem::remove(tempZipPath);
        throw std::runtime_error("Failed to open output file " + tempOutPath.string());
    }
    outFile.write(reinterpret_cast<const char*>(cBuff.data()), cSize);
    outFile.close();

    std::filesystem::rename(tempOutPath, outPath);
    std::filesystem::remove(tempZipPath); // Clean up temp zip
}

} // namespace archive
} // namespace hooc
