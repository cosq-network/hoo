#include <gtest/gtest.h>
#include "archive/HAArchiveBuilder.h"
#include "archive/HAArchive.h"
#include "archive/HAManifest.h"
#include <filesystem>
#include <fstream>

using namespace hooc::archive;

// ============================================================================
// Archive build & read round-trip tests
// ============================================================================

TEST(HAArchiveTest, BuildAndReadRoundtrip) {
    std::filesystem::path tempArchive = std::filesystem::temp_directory_path() / "test_archive.ha";
    
    // Clean up before test just in case
    if (std::filesystem::exists(tempArchive)) {
        std::filesystem::remove(tempArchive);
    }
    
    // Build archive
    {
        HAArchiveBuilder builder;
        
        HAManifest manifest;
        HAModuleInfo mInfo;
        mInfo.moduleName = "test_mod";
        mInfo.archivePath = "modules/test_mod.ho";
        mInfo.sha256 = "deadbeef";
        manifest.modules.push_back(mInfo);
        builder.setManifest(manifest);
        
        std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
        builder.addModule("test_mod", "modules/test_mod.ho", payload);
        
        builder.write(tempArchive);
    }
    
    EXPECT_TRUE(std::filesystem::exists(tempArchive));
    
    // Read archive
    {
        HAArchive archive(tempArchive);
        const auto& manifest = archive.getManifest();
        
        ASSERT_EQ(manifest.modules.size(), 1);
        EXPECT_EQ(manifest.modules[0].moduleName, "test_mod");
        EXPECT_EQ(manifest.modules[0].sha256, "deadbeef");
        
        std::vector<uint8_t> readPayload = archive.readModule("modules/test_mod.ho");
        ASSERT_EQ(readPayload.size(), 4);
        EXPECT_EQ(readPayload[0], 0x01);
        EXPECT_EQ(readPayload[1], 0x02);
        EXPECT_EQ(readPayload[2], 0x03);
        EXPECT_EQ(readPayload[3], 0x04);
    }
    
    std::filesystem::remove(tempArchive);
}

TEST(HAArchiveTest, MultipleModulesRoundtrip) {
    std::filesystem::path tempArchive = std::filesystem::temp_directory_path() / "multi_module.ha";
    
    if (std::filesystem::exists(tempArchive)) {
        std::filesystem::remove(tempArchive);
    }
    
    {
        HAArchiveBuilder builder;
        
        HAManifest manifest;
        
        HAModuleInfo m1;
        m1.moduleName = "main_mod";
        m1.archivePath = "modules/main_mod.ho";
        m1.sha256 = "hash_main";
        manifest.modules.push_back(m1);
        
        HAModuleInfo m2;
        m2.moduleName = "pkg.utils";
        m2.archivePath = "modules/pkg_utils.ho";
        m2.sha256 = "hash_utils";
        manifest.modules.push_back(m2);
        
        manifest.moduleIndex["main_mod"] = "modules/main_mod.ho";
        manifest.moduleIndex["pkg.utils"] = "modules/pkg_utils.ho";
        
        HAEntryPoint ep;
        ep.source = "main.hoo";
        ep.moduleName = "main_mod";
        ep.symbol = "_F_M_main_mod_E_main_v";
        manifest.entryPoint = ep;
        
        builder.setManifest(manifest);
        builder.addModule("main_mod", "modules/main_mod.ho", {0xAA, 0xBB});
        builder.addModule("pkg.utils", "modules/pkg_utils.ho", {0xCC, 0xDD, 0xEE});
        
        builder.write(tempArchive);
    }
    
    {
        HAArchive archive(tempArchive);
        const auto& manifest = archive.getManifest();
        
        ASSERT_EQ(manifest.modules.size(), 2);
        EXPECT_EQ(manifest.modules[0].sha256, "hash_main");
        EXPECT_EQ(manifest.modules[1].sha256, "hash_utils");
        
        EXPECT_TRUE(manifest.entryPoint.has_value());
        EXPECT_EQ(manifest.entryPoint->symbol, "_F_M_main_mod_E_main_v");
        
        auto payload1 = archive.readModule("modules/main_mod.ho");
        ASSERT_EQ(payload1.size(), 2);
        EXPECT_EQ(payload1[0], 0xAA);
        
        auto payload2 = archive.readModule("modules/pkg_utils.ho");
        ASSERT_EQ(payload2.size(), 3);
        EXPECT_EQ(payload2[2], 0xEE);
    }
    
    std::filesystem::remove(tempArchive);
}

TEST(HAArchiveTest, MissingModuleThrows) {
    std::filesystem::path tempArchive = std::filesystem::temp_directory_path() / "missing_mod.ha";
    
    if (std::filesystem::exists(tempArchive)) {
        std::filesystem::remove(tempArchive);
    }
    
    {
        HAArchiveBuilder builder;
        HAManifest manifest;
        builder.setManifest(manifest);
        builder.write(tempArchive);
    }
    
    {
        HAArchive archive(tempArchive);
        EXPECT_THROW(archive.readModule("modules/nonexistent.ho"), std::runtime_error);
    }
    
    std::filesystem::remove(tempArchive);
}

// ============================================================================
// Temp-file cleanup test
// ============================================================================

TEST(HAArchiveTest, TempFilesCleanedOnSuccess) {
    std::filesystem::path outPath = std::filesystem::temp_directory_path() / "cleanup_test.ha";
    std::filesystem::path tempZip = std::filesystem::path(outPath.string() + ".tmp.zip");
    std::filesystem::path tempOut = std::filesystem::path(outPath.string() + ".tmp");
    
    // Clean up before test
    std::filesystem::remove(outPath);
    std::filesystem::remove(tempZip);
    std::filesystem::remove(tempOut);
    
    {
        HAArchiveBuilder builder;
        HAManifest manifest;
        builder.setManifest(manifest);
        builder.write(outPath);
    }
    
    // After successful write, temp files should not exist
    EXPECT_TRUE(std::filesystem::exists(outPath));
    EXPECT_FALSE(std::filesystem::exists(tempZip));
    EXPECT_FALSE(std::filesystem::exists(tempOut));
    
    std::filesystem::remove(outPath);
}

TEST(HAArchiveTest, EmptyPayloadRoundtrip) {
    std::filesystem::path tempArchive = std::filesystem::temp_directory_path() / "empty_payload.ha";
    
    if (std::filesystem::exists(tempArchive)) {
        std::filesystem::remove(tempArchive);
    }
    
    {
        HAArchiveBuilder builder;
        HAManifest manifest;
        builder.setManifest(manifest);
        builder.addModule("empty", "modules/empty.ho", {});
        builder.write(tempArchive);
    }
    
    {
        HAArchive archive(tempArchive);
        auto payload = archive.readModule("modules/empty.ho");
        EXPECT_TRUE(payload.empty());
    }
    
    std::filesystem::remove(tempArchive);
}
