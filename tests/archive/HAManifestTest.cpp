#include <gtest/gtest.h>
#include "archive/HAManifest.h"

using namespace hooc::archive;

TEST(HAManifestTest, SerializationRoundTrip) {
    HAManifest manifest;
    
    HAModuleInfo m1;
    m1.moduleName = "a";
    m1.sourcePath = "a.hoo";
    m1.archivePath = "modules/a.ho";
    m1.imports = {"b"};
    
    HASymbolInfo s1;
    s1.name = "main";
    s1.mangled = "_F_M_a_E_main_v";
    s1.kind = "function";
    s1.returnType = "int64";
    s1.visibility = "public";
    
    m1.symbols.push_back(s1);
    
    manifest.modules.push_back(m1);
    manifest.moduleIndex["a"] = "modules/a.ho";
    manifest.symbolIndex["_F_M_a_E_main_v"] = "modules/a.ho";
    
    HAEntryPoint ep;
    ep.source = "a.hoo";
    ep.moduleName = "a";
    ep.symbol = "_F_M_a_E_main_v";
    manifest.entryPoint = ep;
    
    std::string jsonStr = manifest.toJson();
    EXPECT_FALSE(jsonStr.empty());
    
    HAManifest parsed = HAManifest::fromJson(jsonStr);
    
    EXPECT_EQ(parsed.format, "hoo-archive");
    EXPECT_EQ(parsed.formatVersion, 1);
    EXPECT_EQ(parsed.createdBy.tool, "hoo");
    EXPECT_TRUE(parsed.entryPoint.has_value());
    EXPECT_EQ(parsed.entryPoint->moduleName, "a");
    EXPECT_EQ(parsed.entryPoint->symbol, "_F_M_a_E_main_v");
    
    ASSERT_EQ(parsed.modules.size(), 1);
    EXPECT_EQ(parsed.modules[0].moduleName, "a");
    EXPECT_EQ(parsed.modules[0].sourcePath, "a.hoo");
    EXPECT_EQ(parsed.modules[0].archivePath, "modules/a.ho");
    EXPECT_EQ(parsed.modules[0].imports.size(), 1);
    EXPECT_EQ(parsed.modules[0].imports[0], "b");
    
    ASSERT_EQ(parsed.modules[0].symbols.size(), 1);
    EXPECT_EQ(parsed.modules[0].symbols[0].name, "main");
    EXPECT_EQ(parsed.modules[0].symbols[0].mangled, "_F_M_a_E_main_v");
    EXPECT_EQ(parsed.modules[0].symbols[0].kind, "function");
    
    EXPECT_EQ(parsed.moduleIndex.at("a"), "modules/a.ho");
    EXPECT_EQ(parsed.symbolIndex.at("_F_M_a_E_main_v"), "modules/a.ho");
}
