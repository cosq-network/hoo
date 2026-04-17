#include <gtest/gtest.h>
#include <memory>
#include "src/parsing/ProcessIsolatedParser.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/ast/AST.h"

using namespace hooc;
using namespace hooc::ast;

class ImportStatementParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    std::unique_ptr<CompilationUnit> parseCode(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;
        return astBuilder->buildAST(parseTree);
    }
};

// Basic import tests
TEST_F(ImportStatementParsingTest, BasicImport) {
    std::string code = R"(
        import os;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* basicImport = dynamic_cast<const BasicImport*>(imports[0].get());
    ASSERT_NE(basicImport, nullptr);

    auto* modulePath = basicImport->getModule();
    ASSERT_NE(modulePath, nullptr);
    EXPECT_EQ(modulePath->toString(), "os");
    EXPECT_FALSE(basicImport->hasAlias());
}

TEST_F(ImportStatementParsingTest, BasicImportWithAlias) {
    std::string code = R"(
        import math as m;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* basicImport = dynamic_cast<const BasicImport*>(imports[0].get());
    ASSERT_NE(basicImport, nullptr);

    EXPECT_EQ(basicImport->getModule()->toString(), "math");
    EXPECT_TRUE(basicImport->hasAlias());
    EXPECT_EQ(basicImport->getAlias(), "m");
}

TEST_F(ImportStatementParsingTest, DottedModuleImport) {
    std::string code = R"(
        import os.path;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* basicImport = dynamic_cast<const BasicImport*>(imports[0].get());
    ASSERT_NE(basicImport, nullptr);

    EXPECT_EQ(basicImport->getModule()->toString(), "os.path");
    EXPECT_FALSE(basicImport->hasAlias());
}

TEST_F(ImportStatementParsingTest, DottedModuleImportWithAlias) {
    std::string code = R"(
        import os.path as ospath;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* basicImport = dynamic_cast<const BasicImport*>(imports[0].get());
    ASSERT_NE(basicImport, nullptr);

    EXPECT_EQ(basicImport->getModule()->toString(), "os.path");
    EXPECT_TRUE(basicImport->hasAlias());
    EXPECT_EQ(basicImport->getAlias(), "ospath");
}

// From import tests
TEST_F(ImportStatementParsingTest, FromImportSingleName) {
    std::string code = R"(
        from os import path;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* fromImport = dynamic_cast<const FromImport*>(imports[0].get());
    ASSERT_NE(fromImport, nullptr);

    EXPECT_EQ(fromImport->getModule()->toString(), "os");

    const auto& items = fromImport->getItems();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0]->getName(), "path");
    EXPECT_FALSE(items[0]->hasAlias());
}

TEST_F(ImportStatementParsingTest, FromImportSingleNameWithAlias) {
    std::string code = R"(
        from os import path as p;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* fromImport = dynamic_cast<const FromImport*>(imports[0].get());
    ASSERT_NE(fromImport, nullptr);

    const auto& items = fromImport->getItems();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0]->getName(), "path");
    EXPECT_TRUE(items[0]->hasAlias());
    EXPECT_EQ(items[0]->getAlias(), "p");
}

TEST_F(ImportStatementParsingTest, FromImportMultipleNames) {
    std::string code = R"(
        from std.io import read, write, close;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* fromImport = dynamic_cast<const FromImport*>(imports[0].get());
    ASSERT_NE(fromImport, nullptr);

    EXPECT_EQ(fromImport->getModule()->toString(), "std.io");

    const auto& items = fromImport->getItems();
    ASSERT_EQ(items.size(), 3u);
    EXPECT_EQ(items[0]->getName(), "read");
    EXPECT_EQ(items[1]->getName(), "write");
    EXPECT_EQ(items[2]->getName(), "close");
}

TEST_F(ImportStatementParsingTest, FromImportMixedAliases) {
    std::string code = R"(
        from lib import x, y as Y, z;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* fromImport = dynamic_cast<const FromImport*>(imports[0].get());
    ASSERT_NE(fromImport, nullptr);

    const auto& items = fromImport->getItems();
    ASSERT_EQ(items.size(), 3u);

    EXPECT_EQ(items[0]->getName(), "x");
    EXPECT_FALSE(items[0]->hasAlias());

    EXPECT_EQ(items[1]->getName(), "y");
    EXPECT_TRUE(items[1]->hasAlias());
    EXPECT_EQ(items[1]->getAlias(), "Y");

    EXPECT_EQ(items[2]->getName(), "z");
    EXPECT_FALSE(items[2]->hasAlias());
}

TEST_F(ImportStatementParsingTest, FromImportDottedModule) {
    std::string code = R"(
        from os.path import join, split;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* fromImport = dynamic_cast<const FromImport*>(imports[0].get());
    ASSERT_NE(fromImport, nullptr);

    EXPECT_EQ(fromImport->getModule()->toString(), "os.path");

    const auto& items = fromImport->getItems();
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0]->getName(), "join");
    EXPECT_EQ(items[1]->getName(), "split");
}

// Multiple imports tests
TEST_F(ImportStatementParsingTest, MultipleImportStatements) {
    std::string code = R"(
        import os;
        import math as m;
        from sys import argv;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 3u);

    // First import: import os;
    auto* basic1 = dynamic_cast<const BasicImport*>(imports[0].get());
    ASSERT_NE(basic1, nullptr);
    EXPECT_EQ(basic1->getModule()->toString(), "os");

    // Second import: import math as m;
    auto* basic2 = dynamic_cast<const BasicImport*>(imports[1].get());
    ASSERT_NE(basic2, nullptr);
    EXPECT_EQ(basic2->getModule()->toString(), "math");
    EXPECT_EQ(basic2->getAlias(), "m");

    // Third import: from sys import argv;
    auto* from = dynamic_cast<const FromImport*>(imports[2].get());
    ASSERT_NE(from, nullptr);
    EXPECT_EQ(from->getModule()->toString(), "sys");
}

TEST_F(ImportStatementParsingTest, ImportsBeforeDeclarations) {
    std::string code = R"(
        import os;
        import sys;

        func main() -> void {
        }

        func helper() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    EXPECT_EQ(ast->getImports().size(), 2u);
    EXPECT_EQ(ast->getDeclarations().size(), 2u);
}

// Complex scenarios
TEST_F(ImportStatementParsingTest, ComplexDottedPath) {
    std::string code = R"(
        import a.b.c.d.e;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* basicImport = dynamic_cast<const BasicImport*>(imports[0].get());
    ASSERT_NE(basicImport, nullptr);

    EXPECT_EQ(basicImport->getModule()->toString(), "a.b.c.d.e");
}

TEST_F(ImportStatementParsingTest, FromImportAllAliased) {
    std::string code = R"(
        from m import a as A, b as B, c as C;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    auto* fromImport = dynamic_cast<const FromImport*>(imports[0].get());
    ASSERT_NE(fromImport, nullptr);

    const auto& items = fromImport->getItems();
    ASSERT_EQ(items.size(), 3u);

    for (size_t i = 0; i < items.size(); i++) {
        EXPECT_TRUE(items[i]->hasAlias());
    }

    EXPECT_EQ(items[0]->getAlias(), "A");
    EXPECT_EQ(items[1]->getAlias(), "B");
    EXPECT_EQ(items[2]->getAlias(), "C");
}

TEST_F(ImportStatementParsingTest, MixedImportStyles) {
    std::string code = R"(
        import os;
        from sys import argv, path;
        import json as j;
        from os.path import join as path_join;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 4u);

    // Verify mix of BasicImport and FromImport
    EXPECT_NE(dynamic_cast<const BasicImport*>(imports[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<const FromImport*>(imports[1].get()), nullptr);
    EXPECT_NE(dynamic_cast<const BasicImport*>(imports[2].get()), nullptr);
    EXPECT_NE(dynamic_cast<const FromImport*>(imports[3].get()), nullptr);
}

TEST_F(ImportStatementParsingTest, EmptyFileWithImports) {
    std::string code = R"(
        import os;
        from sys import argv;
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    EXPECT_EQ(ast->getImports().size(), 2u);
    EXPECT_EQ(ast->getDeclarations().size(), 0u);
}

TEST_F(ImportStatementParsingTest, LargeNumberOfImports) {
    std::string code = R"(
        import a;
        import b;
        import c;
        import d;
        import e;
        from f import x, y;
        from g import p, q, r;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    EXPECT_EQ(imports.size(), 7u);
}

TEST_F(ImportStatementParsingTest, ToStringBasicImport) {
    std::string code = R"(
        import os;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    std::string str = imports[0]->toString();
    EXPECT_TRUE(str.find("import") != std::string::npos);
    EXPECT_TRUE(str.find("os") != std::string::npos);
}

TEST_F(ImportStatementParsingTest, ToStringFromImport) {
    std::string code = R"(
        from sys import argv, path;

        func main() -> void {
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    const auto& imports = ast->getImports();
    ASSERT_EQ(imports.size(), 1u);

    std::string str = imports[0]->toString();
    EXPECT_TRUE(str.find("from") != std::string::npos);
    EXPECT_TRUE(str.find("sys") != std::string::npos);
    EXPECT_TRUE(str.find("import") != std::string::npos);
}
