#include <gtest/gtest.h>
#include "src/parsing/HooParserWrapper.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/ast/AST.h"

using namespace hooc;

class QualifiedIdentifierParsingTest : public ::testing::Test {
protected:
    HooParserWrapper parser;
    SimpleASTBuilder builder;

    std::unique_ptr<ast::CompilationUnit> parseCode(const std::string& code) {
        auto parseTree = parser.parseForAST(code);
        if (!parseTree) return nullptr;
        return builder.buildAST(parseTree);
    }
};

// Test qualified identifiers in type contexts
TEST_F(QualifiedIdentifierParsingTest, SimpleQualifiedTypeInVariableDeclaration) {
    std::string code = R"(
        func test() {
            var x: hoo.String;
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, NestedQualifiedTypeInVariableDeclaration) {
    std::string code = R"(
        func test() {
            var x: hoo.io.File;
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, DeeplyNestedQualifiedType) {
    std::string code = R"(
        func test() {
            var x: hoo.collections.generic.Buffer;
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, QualifiedTypeAsReturnType) {
    std::string code = R"(
        func:hoo.String getValue() {
            return nullptr;
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, QualifiedTypeAsFunctionParameter) {
    std::string code = R"(
        func process(value: hoo.String) {
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, MultipleQualifiedTypeParameters) {
    std::string code = R"(
        func process(a: hoo.String, b: hoo.io.File) {
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}



TEST_F(QualifiedIdentifierParsingTest, SimpleQualifiedTypeInArrayType) {
    std::string code = R"(
        func test() {
            var arr: hoo.String[];
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, QualifiedTypeWithNullable) {
    std::string code = R"(
        func test() {
            var opt: hoo.String?;
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, SimpleQualifiedTypeInClassField) {
    std::string code = R"(
        class Person {
            var name: hoo.String;
            constructor() {}
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, QualifiedTypeInClassMethod) {
    std::string code = R"(
        class Person {
            constructor() {}
            func:hoo.String getName() {
                return nullptr;
            }
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

// Test backward compatibility - simple identifiers should still work
TEST_F(QualifiedIdentifierParsingTest, SimpleIdentifierStillWorks) {
    std::string code = R"(
        func test() {
            var x: String;
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedIdentifierParsingTest, SimpleIdentifierAsReturnType) {
    std::string code = R"(
        func:String getValue() {
            return nullptr;
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}


