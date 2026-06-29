#include "HAArchive.h"
#include <zip.h>
#include <zstd.h>
#include <fstream>
#include <stdexcept>

namespace hooc {
namespace archive {

HAArchive::HAArchive(const std::filesystem::path& archivePath) {
    std::ifstream file(archivePath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open archive: " + archivePath.string());
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> compressedData(size);
    if (!file.read(reinterpret_cast<char*>(compressedData.data()), size)) {
        throw std::runtime_error("Failed to read archive: " + archivePath.string());
    }
    initFromMemory(compressedData);
}

HAArchive::HAArchive(const std::vector<uint8_t>& memoryArchive) {
    initFromMemory(memoryArchive);
}

void HAArchive::initFromMemory(const std::vector<uint8_t>& compressedData) {
    unsigned long long const uncompressedSize = ZSTD_getFrameContentSize(compressedData.data(), compressedData.size());
    if (uncompressedSize == ZSTD_CONTENTSIZE_ERROR) {
        throw std::runtime_error("Not a valid ZSTD compressed archive");
    }
    if (uncompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("ZSTD uncompressed size unknown");
    }

    uncompressedZipData_.resize(uncompressedSize);
    size_t const dSize = ZSTD_decompress(uncompressedZipData_.data(), uncompressedSize, compressedData.data(), compressedData.size());
    if (ZSTD_isError(dSize)) {
        throw std::runtime_error("ZSTD decompression failed: " + std::string(ZSTD_getErrorName(dSize)));
    }

    zip_error_t zerr;
    zip_error_init(&zerr);
    
    zipSource_ = zip_source_buffer_create(uncompressedZipData_.data(), uncompressedZipData_.size(), 0, &zerr);
    if (!zipSource_) {
        std::string errStr = zip_error_strerror(&zerr);
        zip_error_fini(&zerr);
        throw std::runtime_error("Failed to create zip source from memory: " + errStr);
    }
    
    zipArchive_ = zip_open_from_source(static_cast<zip_source_t*>(zipSource_), ZIP_RDONLY, &zerr);
    if (!zipArchive_) {
        std::string errStr = zip_error_strerror(&zerr);
        zip_source_free(static_cast<zip_source_t*>(zipSource_));
        zipSource_ = nullptr;
        zip_error_fini(&zerr);
        throw std::runtime_error("Failed to open zip from memory: " + errStr);
    }
    zip_error_fini(&zerr);

    // Read manifest
    zip_stat_t sb;
    if (zip_stat(zipArchive_, "META-INF/hoo/archive.json", 0, &sb) != 0) {
        throw std::runtime_error("Missing archive manifest (META-INF/hoo/archive.json)");
    }
    
    zip_file_t* zf = zip_fopen_index(zipArchive_, sb.index, 0);
    if (!zf) {
        throw std::runtime_error("Failed to open archive manifest");
    }
    
    std::string manifestJson(sb.size, '\0');
    if (zip_fread(zf, manifestJson.data(), sb.size) != sb.size) {
        zip_fclose(zf);
        throw std::runtime_error("Failed to read archive manifest");
    }
    zip_fclose(zf);

    manifest_ = HAManifest::fromJson(manifestJson);
}

HAArchive::~HAArchive() {
    if (zipArchive_) {
        zip_close(zipArchive_); // This also frees the zip source
    } else if (zipSource_) {
        zip_source_free(static_cast<zip_source_t*>(zipSource_));
    }
}

std::vector<uint8_t> HAArchive::readModule(const std::string& archivePath) const {
    if (!zipArchive_) {
        throw std::runtime_error("Archive not initialized");
    }
    
    zip_stat_t sb;
    if (zip_stat(zipArchive_, archivePath.c_str(), 0, &sb) != 0) {
        throw std::runtime_error("Module not found in archive: " + archivePath);
    }
    
    zip_file_t* zf = zip_fopen_index(zipArchive_, sb.index, 0);
    if (!zf) {
        throw std::runtime_error("Failed to open module in archive: " + archivePath);
    }
    
    std::vector<uint8_t> payload(sb.size);
    if (zip_fread(zf, payload.data(), sb.size) != sb.size) {
        zip_fclose(zf);
        throw std::runtime_error("Failed to read module from archive: " + archivePath);
    }
    zip_fclose(zf);
    
    return payload;
}

} // namespace archive
} // namespace hooc
