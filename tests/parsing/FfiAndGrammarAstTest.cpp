#include <gtest/gtest.h>

#include "src/ast/AST.h"
#include "antlr4-runtime.h"
#include "HoocLexer.h"
#include "HoocParser.h"

#include "src/parsing/ProcessIsolatedParser.h"

using namespace hooc;
using namespace hooc::ast;

namespace {

bool compilationUnitAccepted(const std::string& code) {
    static ProcessIsolatedParser parser;
    try {
        return parser.parseForAST(code) != nullptr;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace

TEST(FfiAndGrammarAstTest, BuildsFfiLibraryImportNode) {
    FFILibraryImportDeclaration decl("libmath.so", "mathlib");
    EXPECT_EQ(decl.getLibraryPath(), "libmath.so");
    EXPECT_EQ(decl.getAlias(), "mathlib");
    EXPECT_TRUE(decl.hasAlias());
    EXPECT_NE(decl.toString().find("FFILibraryImport"), std::string::npos);
}

TEST(FfiAndGrammarAstTest, BuildsFfiLinkNode) {
    auto module = std::make_unique<ModulePath>(std::vector<std::string>{"foo", "bar"});
    FFILinkDeclaration decl(std::move(module), 1, 5, {"./lib", "/usr/local/lib"});
    ASSERT_NE(decl.getModulePath(), nullptr);
    EXPECT_EQ(decl.getModulePath()->getComponents().size(), 2U);
    EXPECT_EQ(*decl.getVersionMin(), 1);
    EXPECT_EQ(*decl.getVersionMax(), 5);
    EXPECT_EQ(decl.getSearchPaths().size(), 2U);
}

TEST(FfiAndGrammarAstTest, BuildsFfiFunctionTypeTree) {
    std::vector<std::unique_ptr<FFIType>> params;
    params.push_back(std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::INT64));
    params.push_back(std::make_unique<FFIPointerType>(
        std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::INT8)));

    auto fnType = std::make_unique<FFIFunctionType>(
        std::move(params),
        std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::INT64));

    EXPECT_EQ(fnType->getParams().size(), 2U);
    ASSERT_NE(fnType->getReturnType(), nullptr);
    EXPECT_NE(fnType->toString().find("FFIFunctionType"), std::string::npos);
}

TEST(FfiAndGrammarAstTest, BuildsExternNativeFunctionDeclarationNode) {
    std::vector<std::unique_ptr<FFIParameter>> ffiParams;
    ffiParams.push_back(std::make_unique<FFIParameter>(
        "p", std::make_unique<FFIPointerType>(
                 std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::INT8))));

    auto intType = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));
    auto retType = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));

    FFINativeFunctionDeclaration decl(
        true, nullptr, std::move(intType), "process",
        std::move(ffiParams), std::move(retType),
        std::vector<FunctionModifier>{FunctionModifier::ASYNC});

    EXPECT_TRUE(decl.isExtern());
    EXPECT_EQ(decl.getSymbolName(), "process");
    ASSERT_EQ(decl.getFfiParameters().size(), 1U);
    EXPECT_EQ(decl.getFfiParameters()[0]->getName(), "p");
}

TEST(FfiAndGrammarAstTest, BuildsNativeVariableDeclarationNode) {
    auto init = std::make_unique<PrimaryExpression>(std::make_unique<IntegerLiteral>(1));
    auto type = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));
    auto var = std::make_unique<VariableDeclaration>(std::move(type), "sym", std::move(init));

    FFINativeVariableDeclaration decl(false, std::move(var));
    EXPECT_FALSE(decl.isExtern());
    ASSERT_NE(decl.getVariable(), nullptr);
    EXPECT_EQ(decl.getVariable()->getName(), "sym");
}

TEST(FfiAndGrammarAstTest, LexerRecognizesFunctionKeywordToken) {
    antlr4::ANTLRInputStream input("function");
    HoocLexer lexer(&input);
    auto token = lexer.nextToken();
    EXPECT_EQ(token->getType(), HoocLexer::FUNCTION);
}

TEST(FfiAndGrammarAstTest, LexerRecognizesMultilineStringToken) {
    antlr4::ANTLRInputStream input("\"\"\"hello\nworld\"\"\"");
    HoocLexer lexer(&input);
    auto token = lexer.nextToken();
    EXPECT_EQ(token->getType(), HoocLexer::MULTILINE_STRING);
}

TEST(FfiAndGrammarAstTest, ParsesVersionRangeTextMalformedReturnsEmpty) {
    EXPECT_FALSE(compilationUnitAccepted("link dynamic foo.bar@[a..5];"));
}

TEST(FfiAndGrammarAstTest, RejectsDoubleSemicolonAfterFfiDeclaration) {
    EXPECT_FALSE(compilationUnitAccepted("library \"x\";;"));
}
