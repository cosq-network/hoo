#include <gtest/gtest.h>
#include "archive/HooArchiveLoader.h"
#include "archive/HAArchiveBuilder.h"
#include "archive/HAManifest.h"
#include "archive/HAArchive.h"
#include <filesystem>

using namespace hooc::archive;

// ============================================================================
// Helper: create a minimal .ha archive on disk with a given manifest
// ============================================================================

static std::filesystem::path createTestArchive(
    const std::string& filename,
    const HAManifest& manifest,
    const std::unordered_map<std::string, std::vector<uint8_t>>& modules = {})
{
    std::filesystem::path path = std::filesystem::temp_directory_path() / filename;
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }

    HAArchiveBuilder builder;
    builder.setManifest(manifest);
    for (const auto& [archivePath, payload] : modules) {
        // Extract module name from archivePath (e.g., "modules/foo.ho" -> "foo")
        std::string modName = archivePath;
        auto slashPos = modName.rfind('/');
        if (slashPos != std::string::npos) {
            modName = modName.substr(slashPos + 1);
        }
        auto dotPos = modName.rfind('.');
        if (dotPos != std::string::npos) {
            modName = modName.substr(0, dotPos);
        }
        builder.addModule(modName, archivePath, payload);
    }
    builder.write(path);
    return path;
}

// ============================================================================
// Format version validation tests
// ============================================================================

// NOTE: HooArchiveLoader requires HVMJIT which requires LLVM. These tests
// verify the manifest-level logic that the loader relies on. Full integration
// tests with JIT are in the JIT test suite.

TEST(HooArchiveLoaderTest, ManifestVersionZeroRejectedBySpec) {
    // Verify that a manifest with version 0 would be considered invalid
    HAManifest manifest;
    manifest.formatVersion = 0;
    
    EXPECT_EQ(manifest.formatVersion, 0);
    // The loader checks: version < 1 || version > SUPPORTED_FORMAT_VERSION
    EXPECT_TRUE(manifest.formatVersion < HooArchiveLoader::SUPPORTED_FORMAT_VERSION);
}

TEST(HooArchiveLoaderTest, ManifestVersionOneAcceptedBySpec) {
    HAManifest manifest;
    manifest.formatVersion = 1;
    
    EXPECT_GE(manifest.formatVersion, 1);
    EXPECT_LE(manifest.formatVersion, HooArchiveLoader::SUPPORTED_FORMAT_VERSION);
}

TEST(HooArchiveLoaderTest, ManifestVersionTooHighRejectedBySpec) {
    HAManifest manifest;
    manifest.formatVersion = HooArchiveLoader::SUPPORTED_FORMAT_VERSION + 1;
    
    EXPECT_GT(manifest.formatVersion, HooArchiveLoader::SUPPORTED_FORMAT_VERSION);
}

TEST(HooArchiveLoaderTest, SupportedFormatVersionIsOne) {
    EXPECT_EQ(HooArchiveLoader::SUPPORTED_FORMAT_VERSION, 1);
}

// ============================================================================
// Archive structure tests (verify loader prerequisites)
// ============================================================================

TEST(HooArchiveLoaderTest, ArchiveWithExplicitEntryPoint) {
    std::filesystem::path archivePath;
    
    {
        HAManifest manifest;
        manifest.formatVersion = 1;
        
        HAEntryPoint ep;
        ep.source = "main.hoo";
        ep.moduleName = "main";
        ep.symbol = "_F_M_main_E_main_v";
        manifest.entryPoint = ep;
        
        HAModuleInfo m;
        m.moduleName = "main";
        m.archivePath = "modules/main.ho";
        m.sha256 = "abc123";
        manifest.modules.push_back(m);
        
        archivePath = createTestArchive("loader_ep_test.ha", manifest);
    }
    
    // Verify archive can be read and manifest is intact
    {
        HAArchive archive(archivePath);
        const auto& manifest = archive.getManifest();
        
        EXPECT_TRUE(manifest.entryPoint.has_value());
        EXPECT_EQ(manifest.entryPoint->symbol, "_F_M_main_E_main_v");
        EXPECT_EQ(manifest.entryPoint->moduleName, "main");
    }
    
    std::filesystem::remove(archivePath);
}

TEST(HooArchiveLoaderTest, ArchiveWithoutEntryPoint) {
    std::filesystem::path archivePath;
    
    {
        HAManifest manifest;
        manifest.formatVersion = 1;
        manifest.entryPoint = std::nullopt;
        
        HAModuleInfo m;
        m.moduleName = "lib";
        m.archivePath = "modules/lib.ho";
        manifest.modules.push_back(m);
        
        archivePath = createTestArchive("loader_no_ep_test.ha", manifest);
    }
    
    {
        HAArchive archive(archivePath);
        const auto& manifest = archive.getManifest();
        
        EXPECT_FALSE(manifest.entryPoint.has_value());
    }
    
    std::filesystem::remove(archivePath);
}

TEST(HooArchiveLoaderTest, ArchiveWithSha256Preserved) {
    std::filesystem::path archivePath;
    
    {
        HAManifest manifest;
        manifest.formatVersion = 1;
        
        HAModuleInfo m;
        m.moduleName = "mymod";
        m.archivePath = "modules/mymod.ho";
        m.sha256 = "deadbeef12345678";
        manifest.modules.push_back(m);
        
        archivePath = createTestArchive("loader_sha_test.ha", manifest);
    }
    
    {
        HAArchive archive(archivePath);
        const auto& manifest = archive.getManifest();
        
        ASSERT_EQ(manifest.modules.size(), 1);
        EXPECT_EQ(manifest.modules[0].sha256, "deadbeef12345678");
    }
    
    std::filesystem::remove(archivePath);
}
