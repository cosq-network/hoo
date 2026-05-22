#include <gtest/gtest.h>
#include "src/modules/ModuleSystem.h"
#include <vector>
#include <string>

using namespace hooc;

class ModuleRegistryTest : public ::testing::Test {
protected:
    ModuleRegistry registry;
};

TEST_F(ModuleRegistryTest, StdModuleInitialized) {
    HooModule* hoo = registry.resolveModulePath({"hoo"});
    EXPECT_NE(hoo, nullptr);
    EXPECT_EQ(hoo->getName(), "hoo");
}

TEST_F(ModuleRegistryTest, StdModuleHasStringExport) {
    const ModuleExport* exp = registry.resolveQualifiedName({"hoo", "String"});
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->name, "String");
    EXPECT_EQ(exp->kind, ModuleExport::Kind::CLASS);
}

TEST_F(ModuleRegistryTest, StdModuleHasArrayExport) {
    const ModuleExport* exp = registry.resolveQualifiedName({"hoo", "Array"});
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->name, "Array");
}

TEST_F(ModuleRegistryTest, ResolveQualifiedNameStdString) {
    std::vector<std::string> path = {"hoo", "String"};
    const ModuleExport* exp = registry.resolveQualifiedName(path);
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->name, "String");
}

TEST_F(ModuleRegistryTest, ResolveQualifiedNameStdArray) {
    std::vector<std::string> path = {"hoo", "Array"};
    const ModuleExport* exp = registry.resolveQualifiedName(path);
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->name, "Array");
}

TEST_F(ModuleRegistryTest, ResolveQualifiedNameInvalid) {
    std::vector<std::string> path = {"hoo", "NonExistent"};
    const ModuleExport* exp = registry.resolveQualifiedName(path);
    EXPECT_EQ(exp, nullptr);
}

TEST_F(ModuleRegistryTest, ResolveQualifiedNameInvalidModule) {
    std::vector<std::string> path = {"invalid", "String"};
    const ModuleExport* exp = registry.resolveQualifiedName(path);
    EXPECT_EQ(exp, nullptr);
}

TEST_F(ModuleRegistryTest, ResolveQualifiedNameSingleComponent) {
    std::vector<std::string> path = {"hoo"};
    const ModuleExport* exp = registry.resolveQualifiedName(path);
    EXPECT_EQ(exp, nullptr); // Path must include export name
}

TEST_F(ModuleRegistryTest, ResolveQualifiedNameNestedNotFound) {
    std::vector<std::string> path = {"hoo", "io", "File"};
    const ModuleExport* exp = registry.resolveQualifiedName(path);
    EXPECT_EQ(exp, nullptr);
}

TEST_F(ModuleRegistryTest, ResolveModulePathStd) {
    HooModule* mod = registry.resolveModulePath({"hoo"});
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->getName(), "hoo");
}

TEST_F(ModuleRegistryTest, ResolveModulePathInvalid) {
    HooModule* mod = registry.resolveModulePath({"nonexistent"});
    EXPECT_EQ(mod, nullptr);
}

TEST_F(ModuleRegistryTest, ResolveModulePathNested) {
    HooModule* mod = registry.resolveModulePath({"hoo", "io"});
    EXPECT_EQ(mod, nullptr); // Not initialized by default
}

TEST_F(ModuleRegistryTest, ModuleHasExport) {
    HooModule mod("test");
    mod.addExport(ModuleExport(ModuleExport::Kind::FUNCTION, "func1"));
    
    const ModuleExport* exp = mod.getExport("func1");
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->name, "func1");
    EXPECT_EQ(exp->kind, ModuleExport::Kind::FUNCTION);
}

TEST_F(ModuleRegistryTest, ModuleHasSubmodule) {
    HooModule mod("parent");
    auto sub = std::make_unique<HooModule>("child");
    mod.addSubmodule(std::move(sub));
    
    HooModule* fetched = mod.getSubmodule("child");
    ASSERT_NE(fetched, nullptr);
    EXPECT_EQ(fetched->getName(), "child");
}

TEST_F(ModuleRegistryTest, AddCustomModule) {
    auto myMod = std::make_unique<HooModule>("utils");
    myMod->addExport(ModuleExport(ModuleExport::Kind::FUNCTION, "help"));
    
    registry.addModule({"myapp", "utils"}, std::move(myMod));
    
    HooModule* mod = registry.resolveModulePath({"myapp", "utils"});
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->getName(), "utils");
    
    const ModuleExport* exp = registry.resolveQualifiedName({"myapp", "utils", "help"});
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->name, "help");
}
