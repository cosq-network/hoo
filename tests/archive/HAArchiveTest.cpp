#include <gtest/gtest.h>
#include "archive/HAArchiveBuilder.h"
#include "archive/HAArchive.h"
#include <filesystem>
#include <fstream>

using namespace hooc::archive;

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
        
        std::vector<uint8_t> readPayload = archive.readModule("modules/test_mod.ho");
        ASSERT_EQ(readPayload.size(), 4);
        EXPECT_EQ(readPayload[0], 0x01);
        EXPECT_EQ(readPayload[1], 0x02);
        EXPECT_EQ(readPayload[2], 0x03);
        EXPECT_EQ(readPayload[3], 0x04);
    }
    
    std::filesystem::remove(tempArchive);
}
