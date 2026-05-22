#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include "hvm/ModuleBundle.h"
#include "hvm/HoModuleBase.h"

using namespace hvm;

class ModuleBundleTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ModuleBundleTest, AddModule) {
    ModuleBundle bundle;
    auto module = std::make_shared<HoModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    
    EXPECT_TRUE(bundle.hasModule("test"));
    EXPECT_EQ(bundle.size(), 1);
}

TEST_F(ModuleBundleTest, GetModule) {
    ModuleBundle bundle;
    auto module = std::make_shared<HoModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    
    auto result = bundle.getModule("test");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getName(), "test");
}

TEST_F(ModuleBundleTest, GetModuleNotFound) {
    ModuleBundle bundle;
    
    auto result = bundle.getModule("nonexistent");
    EXPECT_EQ(result, nullptr);
}

TEST_F(ModuleBundleTest, RemoveModule) {
    ModuleBundle bundle;
    auto module = std::make_shared<HoModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    EXPECT_TRUE(bundle.hasModule("test"));
    
    bool removed = bundle.removeModule("test");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(bundle.hasModule("test"));
    EXPECT_EQ(bundle.size(), 0);
}

TEST_F(ModuleBundleTest, RemoveModuleNotFound) {
    ModuleBundle bundle;
    
    bool removed = bundle.removeModule("nonexistent");
    EXPECT_FALSE(removed);
}

TEST_F(ModuleBundleTest, AddMultipleModules) {
    ModuleBundle bundle;
    
    auto module1 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module1");
    auto module2 = std::make_shared<HoModuleBase>(ModuleType::StaticRuntime, "module2");
    auto module3 = std::make_shared<HoModuleBase>(ModuleType::DynamicLibrary, "module3");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    bundle.addModule(module3);
    
    EXPECT_EQ(bundle.size(), 3);
    EXPECT_TRUE(bundle.hasModule("module1"));
    EXPECT_TRUE(bundle.hasModule("module2"));
    EXPECT_TRUE(bundle.hasModule("module3"));
}

TEST_F(ModuleBundleTest, DuplicateModuleNotAdded) {
    ModuleBundle bundle;
    auto module = std::make_shared<HoModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    bundle.addModule(module);
    
    EXPECT_EQ(bundle.size(), 1);
}

TEST_F(ModuleBundleTest, GetModuleNames) {
    ModuleBundle bundle;
    
    auto module1 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "aaa");
    auto module2 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "bbb");
    auto module3 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "ccc");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    bundle.addModule(module3);
    
    auto names = bundle.getModuleNames();
    EXPECT_EQ(names.size(), 3);
}

TEST_F(ModuleBundleTest, GetAllModules) {
    ModuleBundle bundle;
    
    auto module1 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module1");
    auto module2 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module2");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    
    auto modules = bundle.getAllModules();
    EXPECT_EQ(modules.size(), 2);
}

TEST_F(ModuleBundleTest, Clear) {
    ModuleBundle bundle;
    
    auto module1 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module1");
    auto module2 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module2");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    EXPECT_EQ(bundle.size(), 2);
    
    bundle.clear();
    EXPECT_TRUE(bundle.empty());
    EXPECT_EQ(bundle.size(), 0);
}

TEST_F(ModuleBundleTest, Empty) {
    ModuleBundle bundle;
    
    EXPECT_TRUE(bundle.empty());
    EXPECT_EQ(bundle.size(), 0);
    
    auto module = std::make_shared<HoModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    EXPECT_FALSE(bundle.empty());
    EXPECT_EQ(bundle.size(), 1);
}

TEST_F(ModuleBundleTest, NullModuleNotAdded) {
    ModuleBundle bundle;
    
    bundle.addModule(nullptr);
    
    EXPECT_TRUE(bundle.empty());
}

TEST_F(ModuleBundleTest, AddModuleWithSymbols) {
    ModuleBundle bundle;
    auto module = std::make_shared<HoModuleBase>(ModuleType::Compiled, "test");
    
    ModuleSymbol sym1;
    sym1.name = "func1";
    sym1.mangled_name = "_Z5func1v";
    sym1.type = SymbolType::Function;
    sym1.address = 0x1000;
    module->addSymbol(sym1);
    
    ModuleSymbol sym2;
    sym2.name = "var1";
    sym2.mangled_name = "var1";
    sym2.type = SymbolType::Object;
    sym2.address = 0x2000;
    module->addSymbol(sym2);
    
    bundle.addModule(module);
    
    auto found = bundle.findModuleBySymbol("func1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "test");
}

TEST_F(ModuleBundleTest, FindModuleBySymbolNotFound) {
    ModuleBundle bundle;
    auto module = std::make_shared<HoModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    
    auto found = bundle.findModuleBySymbol("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST_F(ModuleBundleTest, AddDependencies) {
    ModuleBundle bundle;
    
    auto module1 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module1");
    module1->addDependency("module2", ModuleType::Compiled);
    module1->addDependency("module3", ModuleType::StaticRuntime);
    
    auto module2 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module2");
    auto module3 = std::make_shared<HoModuleBase>(ModuleType::StaticRuntime, "module3");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    bundle.addModule(module3);
    
    auto depOrder = bundle.getModuleDependencyOrder("module1");
    EXPECT_GE(depOrder.size(), 1);
}

TEST_F(ModuleBundleTest, FindModuleBySymbolWithMultipleModules) {
    ModuleBundle bundle;
    
    auto module1 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module1");
    ModuleSymbol sym1;
    sym1.name = "shared_symbol";
    sym1.type = SymbolType::Function;
    module1->addSymbol(sym1);
    
    auto module2 = std::make_shared<HoModuleBase>(ModuleType::Compiled, "module2");
    ModuleSymbol sym2;
    sym2.name = "shared_symbol";
    sym2.type = SymbolType::Function;
    module2->addSymbol(sym2);
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    
    auto found = bundle.findModuleBySymbol("shared_symbol");
    ASSERT_NE(found, nullptr);
}

TEST_F(ModuleBundleTest, ManglingSeparatesExportDomains) {
    ModuleBundle bundle;
    std::vector<std::string> modulePath = {"pkg", "mod"};
    std::string symbol = "item";

    auto exportMangled = bundle.mangleExport(modulePath, symbol, SymbolType::Function);
    auto nestedMangled = bundle.mangleNestedMember(modulePath, symbol, SymbolType::Function);
    auto namespaceMangled = bundle.mangleNamespaceMember("pkg.mod", symbol, SymbolType::Function);

    EXPECT_NE(exportMangled, nestedMangled);
    EXPECT_NE(exportMangled, namespaceMangled);
    EXPECT_NE(nestedMangled, namespaceMangled);
}

TEST_F(ModuleBundleTest, ManglingSeparatesSymbolKindsForExports) {
    ModuleBundle bundle;
    std::vector<std::string> modulePath = {"pkg", "mod"};
    std::string symbol = "same_name";

    auto fnMangled = bundle.mangleExport(modulePath, symbol, SymbolType::Function);
    auto objMangled = bundle.mangleExport(modulePath, symbol, SymbolType::Object);

    EXPECT_NE(fnMangled, objMangled);
}

TEST_F(ModuleBundleTest, ManglingSeparatesSymbolKindsForNestedMembers) {
    ModuleBundle bundle;
    std::vector<std::string> modulePath = {"pkg", "mod"};
    std::string symbol = "member";

    auto fnMangled = bundle.mangleNestedMember(modulePath, symbol, SymbolType::Function);
    auto objMangled = bundle.mangleNestedMember(modulePath, symbol, SymbolType::Object);

    EXPECT_NE(fnMangled, objMangled);
}

TEST_F(ModuleBundleTest, ManglingSeparatesSymbolKindsForNamespaceMembers) {
    ModuleBundle bundle;
    std::string ns = "pkg.mod";
    std::string symbol = "member";

    auto fnMangled = bundle.mangleNamespaceMember(ns, symbol, SymbolType::Function);
    auto objMangled = bundle.mangleNamespaceMember(ns, symbol, SymbolType::Object);

    EXPECT_NE(fnMangled, objMangled);
}

TEST_F(ModuleBundleTest, ManglingUsesStableKindTags) {
    ModuleBundle bundle;
    std::vector<std::string> modulePath = {"pkg", "mod"};

    auto fnMangled = bundle.mangleExport(modulePath, "sym", SymbolType::Function);
    auto objMangled = bundle.mangleExport(modulePath, "sym", SymbolType::Object);
    auto typeMangled = bundle.mangleExport(modulePath, "sym", SymbolType::Type);
    auto tlsMangled = bundle.mangleExport(modulePath, "sym", SymbolType::TLS);
    auto noTypeMangled = bundle.mangleExport(modulePath, "sym", SymbolType::NoType);

    EXPECT_NE(fnMangled.find("_fn"), std::string::npos);
    EXPECT_NE(objMangled.find("_ob"), std::string::npos);
    EXPECT_NE(typeMangled.find("_ty"), std::string::npos);
    EXPECT_NE(tlsMangled.find("_tls"), std::string::npos);
    EXPECT_NE(noTypeMangled.find("_nt"), std::string::npos);
}
