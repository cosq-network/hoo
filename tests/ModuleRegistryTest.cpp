#include <gtest/gtest.h>
#include "../src/ModuleSystem.h"

using namespace hooc;

class ModuleRegistryTest : public ::testing::Test {
protected:
    ModuleRegistry registry;
};

// Test basic std module initialization
TEST_F(ModuleRegistryTest, StdModuleInitialized) {
    auto stdModule = registry.getStdModule();
    ASSERT_NE(stdModule, nullptr);
    EXPECT_EQ(stdModule->getName(), "std");
}

// Test that std module contains String export
TEST_F(ModuleRegistryTest, StdModuleHasStringExport) {
    auto stdModule = registry.getStdModule();
    ASSERT_NE(stdModule, nullptr);

    auto stringExport = stdModule->getExport("String");
    ASSERT_NE(stringExport, nullptr);
    EXPECT_EQ(stringExport->name, "String");
    EXPECT_EQ(stringExport->kind, ModuleExport::Kind::CLASS);
    EXPECT_EQ(stringExport->runtimeClassName, "HooString");
    EXPECT_FALSE(stringExport->isGeneric);
}

// Test that std module contains Array export
TEST_F(ModuleRegistryTest, StdModuleHasArrayExport) {
    auto stdModule = registry.getStdModule();
    ASSERT_NE(stdModule, nullptr);

    auto arrayExport = stdModule->getExport("Array");
    ASSERT_NE(arrayExport, nullptr);
    EXPECT_EQ(arrayExport->name, "Array");
    EXPECT_EQ(arrayExport->kind, ModuleExport::Kind::CLASS);
    EXPECT_EQ(arrayExport->runtimeClassName, "HooArray");
    EXPECT_TRUE(arrayExport->isGeneric);
}

// Test resolveQualifiedName with std.String
TEST_F(ModuleRegistryTest, ResolveQualifiedNameStdString) {
    auto qid = ast::QualifiedIdentifier({"std", "String"});
    auto export_ = registry.resolveQualifiedName(qid);

    ASSERT_NE(export_, nullptr);
    EXPECT_EQ(export_->name, "String");
    EXPECT_EQ(export_->runtimeClassName, "HooString");
}

// Test resolveQualifiedName with std.Array
TEST_F(ModuleRegistryTest, ResolveQualifiedNameStdArray) {
    auto qid = ast::QualifiedIdentifier({"std", "Array"});
    auto export_ = registry.resolveQualifiedName(qid);

    ASSERT_NE(export_, nullptr);
    EXPECT_EQ(export_->name, "Array");
    EXPECT_EQ(export_->runtimeClassName, "HooArray");
}

// Test resolveQualifiedName with invalid name
TEST_F(ModuleRegistryTest, ResolveQualifiedNameInvalid) {
    auto qid = ast::QualifiedIdentifier({"std", "InvalidClass"});
    auto export_ = registry.resolveQualifiedName(qid);

    EXPECT_EQ(export_, nullptr);
}

// Test resolveQualifiedName with invalid module
TEST_F(ModuleRegistryTest, ResolveQualifiedNameInvalidModule) {
    auto qid = ast::QualifiedIdentifier({"invalid", "String"});
    auto export_ = registry.resolveQualifiedName(qid);

    EXPECT_EQ(export_, nullptr);
}

// Test resolveQualifiedName with single component (should fail without root module)
TEST_F(ModuleRegistryTest, ResolveQualifiedNameSingleComponent) {
    auto qid = ast::QualifiedIdentifier({"String"});
    auto export_ = registry.resolveQualifiedName(qid);

    // Single component without root module should fail
    EXPECT_EQ(export_, nullptr);
}

// Test resolveQualifiedName with nested path (std.io.File) - currently not in registry
TEST_F(ModuleRegistryTest, ResolveQualifiedNameNestedNotFound) {
    auto qid = ast::QualifiedIdentifier({"std", "io", "File"});
    auto export_ = registry.resolveQualifiedName(qid);

    EXPECT_EQ(export_, nullptr);
}

// Test resolveModulePath for std
TEST_F(ModuleRegistryTest, ResolveModulePathStd) {
    auto module = registry.resolveModulePath({"std"});

    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->getName(), "std");
}

// Test resolveModulePath for invalid path
TEST_F(ModuleRegistryTest, ResolveModulePathInvalid) {
    auto module = registry.resolveModulePath({"invalid"});

    EXPECT_EQ(module, nullptr);
}

// Test resolveModulePath for non-existent nested path
TEST_F(ModuleRegistryTest, ResolveModulePathNested) {
    auto module = registry.resolveModulePath({"std", "io"});

    EXPECT_EQ(module, nullptr);
}

// Test Module::hasExport
TEST_F(ModuleRegistryTest, ModuleHasExport) {
    auto stdModule = registry.getStdModule();
    ASSERT_NE(stdModule, nullptr);

    EXPECT_TRUE(stdModule->hasExport("String"));
    EXPECT_TRUE(stdModule->hasExport("Array"));
    EXPECT_FALSE(stdModule->hasExport("NonExistent"));
}

// Test Module::hasSubmodule
TEST_F(ModuleRegistryTest, ModuleHasSubmodule) {
    auto stdModule = registry.getStdModule();
    ASSERT_NE(stdModule, nullptr);

    // By default, std has no submodules
    EXPECT_FALSE(stdModule->hasSubmodule("io"));
}

// Test adding custom module to registry
TEST_F(ModuleRegistryTest, AddCustomModule) {
    auto customModule = std::make_unique<Module>("custom");
    customModule->addExport(ModuleExport(
        ModuleExport::Kind::CLASS,
        "MyClass",
        "MyRuntimeClass",
        false
    ));

    // Note: addModule is called before, but we're testing after registry creation
    // In actual usage, you'd add modules before/after registry creation
    // This test just verifies the module structure works
    EXPECT_TRUE(customModule->hasExport("MyClass"));
    auto export_ = customModule->getExport("MyClass");
    ASSERT_NE(export_, nullptr);
    EXPECT_EQ(export_->runtimeClassName, "MyRuntimeClass");
}

// Test QualifiedIdentifier integration with module resolution
TEST_F(ModuleRegistryTest, QualifiedIdentifierIntegration) {
    // Create a QualifiedIdentifier for std.String
    std::vector<std::string> components = {"std", "String"};
    auto qid = ast::QualifiedIdentifier(components);

    // Verify the QualifiedIdentifier properties
    EXPECT_EQ(qid.getFullName(), "std.String");
    EXPECT_EQ(qid.getName(), "String");

    auto modulePath = qid.getModulePath();
    ASSERT_EQ(modulePath.size(), 1);
    EXPECT_EQ(modulePath[0], "std");

    // Resolve through registry
    auto export_ = registry.resolveQualifiedName(qid);
    ASSERT_NE(export_, nullptr);
    EXPECT_EQ(export_->name, "String");
}

// Test deeply nested qualified identifiers
TEST_F(ModuleRegistryTest, DeeplyNestedQualifiedIdentifier) {
    auto qid = ast::QualifiedIdentifier({"std", "collections", "generic", "List"});

    EXPECT_EQ(qid.getFullName(), "std.collections.generic.List");
    EXPECT_EQ(qid.getName(), "List");

    auto modulePath = qid.getModulePath();
    ASSERT_EQ(modulePath.size(), 3);
    EXPECT_EQ(modulePath[0], "std");
    EXPECT_EQ(modulePath[1], "collections");
    EXPECT_EQ(modulePath[2], "generic");

    // Should not be found in registry (not added in initializeStdModule)
    auto export_ = registry.resolveQualifiedName(qid);
    EXPECT_EQ(export_, nullptr);
}

// Test that the registry can resolve multiple different qualified names
TEST_F(ModuleRegistryTest, MultipleQualifiedNameResolution) {
    auto qid1 = ast::QualifiedIdentifier({"std", "String"});
    auto qid2 = ast::QualifiedIdentifier({"std", "Array"});

    auto export1 = registry.resolveQualifiedName(qid1);
    auto export2 = registry.resolveQualifiedName(qid2);

    ASSERT_NE(export1, nullptr);
    ASSERT_NE(export2, nullptr);

    EXPECT_EQ(export1->name, "String");
    EXPECT_EQ(export2->name, "Array");
    EXPECT_NE(export1->runtimeClassName, export2->runtimeClassName);
}

// Test Module's export iteration
TEST_F(ModuleRegistryTest, ModuleExportIteration) {
    auto stdModule = registry.getStdModule();
    ASSERT_NE(stdModule, nullptr);

    const auto& exports = stdModule->getExports();
    EXPECT_GE(exports.size(), 2);  // At least String and Array

    bool hasString = false;
    bool hasArray = false;

    for (const auto& pair : exports) {
        if (pair.first == "String") hasString = true;
        if (pair.first == "Array") hasArray = true;
    }

    EXPECT_TRUE(hasString);
    EXPECT_TRUE(hasArray);
}
