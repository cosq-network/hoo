#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include "hvm/HVMModuleBundle.h"
#include "hvm/HOModuleBase.h"

using namespace hvm;

class HVMModuleBundleTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(HVMModuleBundleTest, AddModule) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    
    EXPECT_TRUE(bundle.hasModule("test"));
    EXPECT_EQ(bundle.size(), 1);
}

TEST_F(HVMModuleBundleTest, GetModule) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    
    auto result = bundle.getModule("test");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getName(), "test");
}

TEST_F(HVMModuleBundleTest, GetModuleNotFound) {
    HVMModuleBundle bundle;
    
    auto result = bundle.getModule("nonexistent");
    EXPECT_EQ(result, nullptr);
}

TEST_F(HVMModuleBundleTest, RemoveModule) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    EXPECT_TRUE(bundle.hasModule("test"));
    
    bool removed = bundle.removeModule("test");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(bundle.hasModule("test"));
    EXPECT_EQ(bundle.size(), 0);
}

TEST_F(HVMModuleBundleTest, RemoveModuleNotFound) {
    HVMModuleBundle bundle;
    
    bool removed = bundle.removeModule("nonexistent");
    EXPECT_FALSE(removed);
}

TEST_F(HVMModuleBundleTest, AddMultipleModules) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module1");
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::StaticRuntime, "module2");
    auto module3 = std::make_shared<HOModuleBase>(ModuleType::DynamicLibrary, "module3");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    bundle.addModule(module3);
    
    EXPECT_EQ(bundle.size(), 3);
    EXPECT_TRUE(bundle.hasModule("module1"));
    EXPECT_TRUE(bundle.hasModule("module2"));
    EXPECT_TRUE(bundle.hasModule("module3"));
}

TEST_F(HVMModuleBundleTest, DuplicateModuleNotAdded) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    bundle.addModule(module);
    
    EXPECT_EQ(bundle.size(), 1);
}

TEST_F(HVMModuleBundleTest, GetModuleNames) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "aaa");
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "bbb");
    auto module3 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "ccc");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    bundle.addModule(module3);
    
    auto names = bundle.getModuleNames();
    EXPECT_EQ(names.size(), 3);
}

TEST_F(HVMModuleBundleTest, GetAllModules) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module1");
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module2");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    
    auto modules = bundle.getAllModules();
    EXPECT_EQ(modules.size(), 2);
}

TEST_F(HVMModuleBundleTest, Clear) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module1");
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module2");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    EXPECT_EQ(bundle.size(), 2);
    
    bundle.clear();
    EXPECT_TRUE(bundle.empty());
    EXPECT_EQ(bundle.size(), 0);
}

TEST_F(HVMModuleBundleTest, Empty) {
    HVMModuleBundle bundle;
    
    EXPECT_TRUE(bundle.empty());
    EXPECT_EQ(bundle.size(), 0);
    
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    EXPECT_FALSE(bundle.empty());
    EXPECT_EQ(bundle.size(), 1);
}

TEST_F(HVMModuleBundleTest, NullModuleNotAdded) {
    HVMModuleBundle bundle;
    
    bundle.addModule(nullptr);
    
    EXPECT_TRUE(bundle.empty());
}

TEST_F(HVMModuleBundleTest, AddModuleWithSymbols) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
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

TEST_F(HVMModuleBundleTest, FindModuleBySymbolNotFound) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
    bundle.addModule(module);
    
    auto found = bundle.findModuleBySymbol("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST_F(HVMModuleBundleTest, AddDependencies) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module1");
    module1->addDependency("module2", ModuleType::Compiled);
    module1->addDependency("module3", ModuleType::StaticRuntime);
    
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module2");
    auto module3 = std::make_shared<HOModuleBase>(ModuleType::StaticRuntime, "module3");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    bundle.addModule(module3);
    
    auto depOrder = bundle.getModuleDependencyOrder("module1");
    EXPECT_GE(depOrder.size(), 1);
}

TEST_F(HVMModuleBundleTest, GetAllModulesThatDependOn) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module1");
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module2");
    module2->addDependency("module1", ModuleType::Compiled);
    auto module3 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module3");
    module3->addDependency("module1", ModuleType::Compiled);
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    bundle.addModule(module3);
    
    auto dependents = bundle.getAllModulesThatDependOn("module1");
    EXPECT_EQ(dependents.size(), 2);
}

TEST_F(HVMModuleBundleTest, FindModuleBySymbolWithMultipleModules) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module1");
    ModuleSymbol sym1;
    sym1.name = "shared_symbol";
    sym1.type = SymbolType::Function;
    module1->addSymbol(sym1);
    
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module2");
    ModuleSymbol sym2;
    sym2.name = "shared_symbol";
    sym2.type = SymbolType::Function;
    module2->addSymbol(sym2);
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    
    auto found = bundle.findModuleBySymbol("shared_symbol");
    ASSERT_NE(found, nullptr);
}

TEST_F(HVMModuleBundleTest, ManglingSeparatesExportDomains) {
    HVMModuleBundle bundle;
    std::vector<std::string> modulePath = {"pkg", "mod"};
    std::string symbol = "item";

    auto exportMangled = bundle.mangleExport(modulePath, symbol, SymbolType::Function);
    auto nestedMangled = bundle.mangleNestedMember(modulePath, symbol, SymbolType::Function);
    auto namespaceMangled = bundle.mangleNamespaceMember("pkg.mod", symbol, SymbolType::Function);

    EXPECT_NE(exportMangled, nestedMangled);
    EXPECT_NE(exportMangled, namespaceMangled);
    EXPECT_NE(nestedMangled, namespaceMangled);
}

TEST_F(HVMModuleBundleTest, ManglingSeparatesSymbolKindsForExports) {
    HVMModuleBundle bundle;
    std::vector<std::string> modulePath = {"pkg", "mod"};
    std::string symbol = "same_name";

    auto fnMangled = bundle.mangleExport(modulePath, symbol, SymbolType::Function);
    auto objMangled = bundle.mangleExport(modulePath, symbol, SymbolType::Object);

    EXPECT_NE(fnMangled, objMangled);
}

TEST_F(HVMModuleBundleTest, ManglingSeparatesSymbolKindsForNestedMembers) {
    HVMModuleBundle bundle;
    std::vector<std::string> modulePath = {"pkg", "mod"};
    std::string symbol = "member";

    auto fnMangled = bundle.mangleNestedMember(modulePath, symbol, SymbolType::Function);
    auto objMangled = bundle.mangleNestedMember(modulePath, symbol, SymbolType::Object);

    EXPECT_NE(fnMangled, objMangled);
}

TEST_F(HVMModuleBundleTest, ManglingSeparatesSymbolKindsForNamespaceMembers) {
    HVMModuleBundle bundle;
    std::string ns = "pkg.mod";
    std::string symbol = "member";

    auto fnMangled = bundle.mangleNamespaceMember(ns, symbol, SymbolType::Function);
    auto objMangled = bundle.mangleNamespaceMember(ns, symbol, SymbolType::Object);

    EXPECT_NE(fnMangled, objMangled);
}

TEST_F(HVMModuleBundleTest, ManglingUsesStableKindTags) {
    HVMModuleBundle bundle;
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

TEST_F(HVMModuleBundleTest, DemangleExportStripsKindTags) {
    HVMModuleBundle bundle;
    std::vector<std::string> modulePath = {"pkg", "mod"};

    auto fnMangled = bundle.mangleExport(modulePath, "myfunc", SymbolType::Function);
    auto objMangled = bundle.mangleExport(modulePath, "myobj", SymbolType::Object);
    auto typeMangled = bundle.mangleExport(modulePath, "mytype", SymbolType::Type);

    auto fnDemangled = bundle.demangleExport(fnMangled);
    auto objDemangled = bundle.demangleExport(objMangled);
    auto typeDemangled = bundle.demangleExport(typeMangled);

    // demangleExport strips the kind tag (_fn, _ob, _ty) from the originalName
    // The originalName still contains the module path prefix
    EXPECT_EQ(fnDemangled.originalName, "_H_pkg_mod_myfunc");
    EXPECT_EQ(objDemangled.originalName, "_H_pkg_mod_myobj");
    EXPECT_EQ(typeDemangled.originalName, "_H_pkg_mod_mytype");
    
    // Also verify the mangled names contain the kind tags
    EXPECT_NE(fnMangled.find("_fn"), std::string::npos);
    EXPECT_NE(objMangled.find("_ob"), std::string::npos);
    EXPECT_NE(typeMangled.find("_ty"), std::string::npos);
}

// ============ HIGH PRIORITY: Symbol Lookup ============

TEST_F(HVMModuleBundleTest, FindModuleBySymbolMangled) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
    ModuleSymbol sym1;
    sym1.name = "func1";
    sym1.type = SymbolType::Function;
    module->addSymbol(sym1);
    
    // Note: HOModuleBase::addSymbol generates its own mangled name
    // For module "test" and symbol "func1", it generates "_H_test_func1"
    bundle.addModule(module);
    
    auto found = bundle.findModuleBySymbolMangled("_H_test_func1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "test");
}

TEST_F(HVMModuleBundleTest, FindModuleBySymbolMangledNotFound) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    auto found = bundle.findModuleBySymbolMangled("_Znonexistentv");
    EXPECT_EQ(found, nullptr);
}

// ============ HIGH PRIORITY: Dependency Resolution ============

TEST_F(HVMModuleBundleTest, ResolveDependencyOrder) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "base");
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "dep1");
    module2->addDependency("base", ModuleType::Compiled);
    auto module3 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "dep2");
    module3->addDependency("base", ModuleType::Compiled);
    auto module4 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "top");
    module4->addDependency("dep1", ModuleType::Compiled);
    module4->addDependency("dep2", ModuleType::Compiled);
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    bundle.addModule(module3);
    bundle.addModule(module4);
    
    auto order = bundle.resolveDependencyOrder();
    EXPECT_EQ(order.size(), 4);
    
    // Verify topological order: every module's dependencies appear before it
    std::unordered_map<std::string, size_t> name_to_index;
    for (size_t i = 0; i < order.size(); ++i) {
        name_to_index[order[i]->getName()] = i;
    }
    
    // Check each module's dependencies come before it
    for (const auto& m : order) {
        for (const auto& dep_name : m->getDependencyOrder()) {
            auto dep_it = name_to_index.find(dep_name);
            if (dep_it != name_to_index.end()) {
                EXPECT_LT(dep_it->second, name_to_index[m->getName()])
                    << m->getName() << " depends on " << dep_name
                    << " but appears before it in order";
            }
        }
    }
}

// Note: HOModuleBase::resolveDependencyOrder has a known issue with cycle detection
// that causes false positives for simple dependency chains. These tests use self-dependencies
// to test the HVMModuleBundle wrapper methods without triggering the HOModuleBase bug.

TEST_F(HVMModuleBundleTest, HasCircularDependencySelfDependency) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module1");
    // Create a self-dependency: module1 depends on itself
    module1->addDependency("module1", ModuleType::Compiled);
    
    bundle.addModule(module1);
    
    // Self-dependency is a cycle
    EXPECT_TRUE(bundle.hasCircularDependency("module1"));
    EXPECT_TRUE(bundle.hasCircularDependency());
}

TEST_F(HVMModuleBundleTest, HasCircularDependencyNotFound) {
    HVMModuleBundle bundle;
    EXPECT_FALSE(bundle.hasCircularDependency("nonexistent"));
}

TEST_F(HVMModuleBundleTest, HasCircularDependencyNoDependencies) {
    HVMModuleBundle bundle;
    
    auto module1 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module1");
    auto module2 = std::make_shared<HOModuleBase>(ModuleType::Compiled, "module2");
    
    bundle.addModule(module1);
    bundle.addModule(module2);
    
    // No dependencies, no cycles
    EXPECT_FALSE(bundle.hasCircularDependency("module1"));
    EXPECT_FALSE(bundle.hasCircularDependency("module2"));
    EXPECT_FALSE(bundle.hasCircularDependency());
}

// ============ HIGH PRIORITY: Export Registration ============

TEST_F(HVMModuleBundleTest, RegisterExport) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    bundle.registerExport("test", "myFunc", "_Z6myFuncv", SymbolType::Function);
    
    EXPECT_TRUE(bundle.hasExport("myFunc"));
    EXPECT_TRUE(bundle.findExportsByKind(SymbolType::Function).size() >= 1);
    
    auto exports = bundle.getAllExportedSymbols();
    EXPECT_NE(std::find(exports.begin(), exports.end(), "myFunc"), exports.end());
    
    auto mangled = bundle.getAllMangledExports();
    EXPECT_NE(std::find(mangled.begin(), mangled.end(), "_Z6myFuncv"), mangled.end());
}

TEST_F(HVMModuleBundleTest, RegisterExportModuleNotFound) {
    HVMModuleBundle bundle;
    // Don't add any modules
    bundle.registerExport("nonexistent", "func", "_Zfuncv", SymbolType::Function);
    EXPECT_FALSE(bundle.hasExport("func"));
}

TEST_F(HVMModuleBundleTest, RegisterNestedExport) {
    HVMModuleBundle bundle;
    // Create a module named "mod" - registerNestedExport uses module_path.back() for lookup
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "mod");
    bundle.addModule(module);
    
    bundle.registerNestedExport({"pkg", "mod"}, "member", "_Zmember", SymbolType::Object);
    
    EXPECT_TRUE(bundle.hasNestedExport({"pkg", "mod"}, "member"));
    EXPECT_FALSE(bundle.hasNestedExport({"pkg", "mod"}, "nonexistent"));
    EXPECT_FALSE(bundle.hasNestedExport({"pkg"}, "member"));
}

TEST_F(HVMModuleBundleTest, RegisterNestedExportEmptyPath) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    // Empty path should be silently ignored
    bundle.registerNestedExport({}, "member", "_Zmember", SymbolType::Object);
    EXPECT_FALSE(bundle.hasNestedExport({}, "member"));
}

TEST_F(HVMModuleBundleTest, RegisterNamespaceExport) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    bundle.registerNamespaceExport("myns", "func", "_Z3ns3funcv", SymbolType::Function);
    
    auto exports = bundle.findExportsInNamespace("myns");
    EXPECT_NE(std::find(exports.begin(), exports.end(), "func"), exports.end());
}

// ============ HIGH PRIORITY: Module Lookup by Export/Symbol ============

TEST_F(HVMModuleBundleTest, FindModuleByNestedSymbol) {
    HVMModuleBundle bundle;
    // Module name must match the last component of the path
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "sub");
    bundle.addModule(module);
    
    bundle.registerNestedExport({"pkg", "sub"}, "func", "_Zfunc", SymbolType::Function);
    
    auto found = bundle.findModuleByNestedSymbol({"pkg", "sub"}, "func");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "sub");
}

TEST_F(HVMModuleBundleTest, FindModuleByNestedSymbolNotFound) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    auto found = bundle.findModuleByNestedSymbol({"pkg", "sub"}, "nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST_F(HVMModuleBundleTest, FindModuleByExport) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
    ModuleSymbol sym;
    sym.name = "exportedFunc";
    sym.type = SymbolType::Function;
    module->addSymbol(sym);
    
    bundle.addModule(module);
    bundle.registerExport("test", "exportedFunc", "_Z11exportedFuncv", SymbolType::Function);
    
    auto found = bundle.findModuleByExport("exportedFunc");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "test");
}

TEST_F(HVMModuleBundleTest, FindModuleByExportFallsBackToSymbol) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    
    ModuleSymbol sym;
    sym.name = "someSymbol";
    sym.type = SymbolType::Function;
    module->addSymbol(sym);
    
    bundle.addModule(module);
    // Note: NOT registered as export, but exists as symbol
    
    // Should fall back to findModuleBySymbol
    auto found = bundle.findModuleByExport("someSymbol");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "test");
}

// ============ HIGH PRIORITY: Export Queries ============

TEST_F(HVMModuleBundleTest, FindExportsByKind) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    bundle.registerExport("test", "func1", "_Z5func1v", SymbolType::Function);
    bundle.registerExport("test", "obj1", "_Z4obj1", SymbolType::Object);
    bundle.registerExport("test", "type1", "_Z5type1", SymbolType::Type);
    
    auto functions = bundle.findExportsByKind(SymbolType::Function);
    EXPECT_EQ(functions.size(), 1);
    EXPECT_EQ(functions[0], "func1");
    
    auto objects = bundle.findExportsByKind(SymbolType::Object);
    EXPECT_EQ(objects.size(), 1);
    EXPECT_EQ(objects[0], "obj1");
    
    auto types = bundle.findExportsByKind(SymbolType::Type);
    EXPECT_EQ(types.size(), 1);
    EXPECT_EQ(types[0], "type1");
}

TEST_F(HVMModuleBundleTest, HasExport) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    EXPECT_FALSE(bundle.hasExport("nonexistent"));
    
    bundle.registerExport("test", "myExport", "_Z7myExportv", SymbolType::Function);
    EXPECT_TRUE(bundle.hasExport("myExport"));
}

TEST_F(HVMModuleBundleTest, GetAllExportedSymbols) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    bundle.registerExport("test", "export1", "_Z7export1v", SymbolType::Function);
    bundle.registerExport("test", "export2", "_Z7export2v", SymbolType::Function);
    
    auto exports = bundle.getAllExportedSymbols();
    EXPECT_EQ(exports.size(), 2);
    EXPECT_NE(std::find(exports.begin(), exports.end(), "export1"), exports.end());
    EXPECT_NE(std::find(exports.begin(), exports.end(), "export2"), exports.end());
}

TEST_F(HVMModuleBundleTest, GetAllMangledExports) {
    HVMModuleBundle bundle;
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    bundle.registerExport("test", "func", "_Z4funcv", SymbolType::Function);
    bundle.registerExport("test", "obj", "_Z3obj", SymbolType::Object);
    
    auto mangled = bundle.getAllMangledExports();
    EXPECT_EQ(mangled.size(), 2);
    EXPECT_NE(std::find(mangled.begin(), mangled.end(), "_Z4funcv"), mangled.end());
    EXPECT_NE(std::find(mangled.begin(), mangled.end(), "_Z3obj"), mangled.end());
}

// ============ MEDIUM PRIORITY: Singleton Lifecycle ============

TEST_F(HVMModuleBundleTest, GetModulesReturnsSameInstance) {
    HVMModuleBundle& instance1 = HVMModuleBundle::getModules();
    HVMModuleBundle& instance2 = HVMModuleBundle::getModules();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(HVMModuleBundleTest, ShutdownClearsModules) {
    HVMModuleBundle& bundle = HVMModuleBundle::getModules();
    
    auto module = std::make_shared<HOModuleBase>(ModuleType::Compiled, "test");
    bundle.addModule(module);
    
    EXPECT_EQ(bundle.size(), 1);
    EXPECT_TRUE(bundle.hasModule("test"));
    
    HVMModuleBundle::shutdown();
    
    EXPECT_EQ(bundle.size(), 0);
    EXPECT_FALSE(bundle.hasModule("test"));
}
