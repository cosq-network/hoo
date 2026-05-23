#include <gtest/gtest.h>
#include "ast/AST.h"
#include "ast/FFIDeclaration.h"

using namespace hooc::ast;

class FFIDeclarationTest : public ::testing::Test {};

TEST_F(FFIDeclarationTest, FFIPrimitiveType) {
    FFIPrimitiveType fpt(PrimitiveTypeKind::INT64);
    EXPECT_EQ(fpt.getKind(), PrimitiveTypeKind::INT64);
    EXPECT_EQ(fpt.toString(), "FFIPrimitiveType(int64)");
}

TEST_F(FFIDeclarationTest, FFIQualifiedType) {
    auto qi = std::make_unique<QualifiedIdentifier>(std::vector<std::string>{"std", "String"});
    FFIQualifiedType fqt(std::move(qi));
    EXPECT_NE(fqt.getTypeName(), nullptr);
    EXPECT_EQ(fqt.getTypeName()->getFullName(), "std.String");
    EXPECT_EQ(fqt.toString(), "FFIQualifiedType(std.String)");
}

TEST_F(FFIDeclarationTest, FFIPointerType) {
    auto inner = std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::INT64);
    FFIPointerType fpt(std::move(inner));
    EXPECT_NE(fpt.getPointee(), nullptr);
    EXPECT_EQ(fpt.toString(), "FFIPointerType(FFIPrimitiveType(int64))");
}

TEST_F(FFIDeclarationTest, FFIArrayType) {
    auto elem = std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::BYTE);
    FFIArrayType fat(256, std::move(elem));
    EXPECT_EQ(fat.getSize(), 256);
    EXPECT_NE(fat.getElementType(), nullptr);
    EXPECT_EQ(fat.toString(), "FFIArrayType[256]");
}

TEST_F(FFIDeclarationTest, FFIFunctionType) {
    std::vector<std::unique_ptr<FFIType>> params;
    params.push_back(std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::INT64));
    params.push_back(std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::DOUBLE));
    auto ret = std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::VOID);
    FFIFunctionType fft(std::move(params), std::move(ret));
    EXPECT_EQ(fft.getParams().size(), 2);
    EXPECT_NE(fft.getReturnType(), nullptr);
    EXPECT_EQ(fft.toString(), "FFIFunctionType(params=2)");
}

TEST_F(FFIDeclarationTest, FFIParameter) {
    auto type = std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::INT64);
    FFIParameter fp("count", std::move(type));
    EXPECT_EQ(fp.getName(), "count");
    EXPECT_NE(fp.getType(), nullptr);
    EXPECT_EQ(fp.toString(), "FFIParameter(count)");
}

TEST_F(FFIDeclarationTest, FFILibraryImportWithoutAlias) {
    FFILibraryImportDeclaration flid("libfoo.so", "");
    EXPECT_EQ(flid.getLibraryPath(), "libfoo.so");
    EXPECT_FALSE(flid.hasAlias());
    EXPECT_EQ(flid.toString(), "FFILibraryImport(libfoo.so)");
}

TEST_F(FFIDeclarationTest, FFILibraryImportWithAlias) {
    FFILibraryImportDeclaration flid("libmath.dylib", "math");
    EXPECT_TRUE(flid.hasAlias());
    EXPECT_EQ(flid.getAlias(), "math");
    EXPECT_EQ(flid.toString(), "FFILibraryImport(libmath.dylib as math)");
}

TEST_F(FFIDeclarationTest, FFILinkDeclaration) {
    auto mp = std::make_unique<ModulePath>(std::vector<std::string>{"foo", "bar"});
    FFILinkDeclaration fld(std::move(mp), std::nullopt, std::nullopt, {});
    EXPECT_NE(fld.getModulePath(), nullptr);
    EXPECT_FALSE(fld.getVersionMin().has_value());
    EXPECT_FALSE(fld.getVersionMax().has_value());
    EXPECT_TRUE(fld.getSearchPaths().empty());
    EXPECT_EQ(fld.toString(), "FFILink(foo.bar)");
}

TEST_F(FFIDeclarationTest, FFILinkDeclarationWithVersions) {
    auto mp = std::make_unique<ModulePath>(std::vector<std::string>{"mod"});
    FFILinkDeclaration fld(std::move(mp), 1, 5, {"/usr/lib", "/usr/local/lib"});
    EXPECT_EQ(fld.getVersionMin().value(), 1);
    EXPECT_EQ(fld.getVersionMax().value(), 5);
    EXPECT_EQ(fld.getSearchPaths().size(), 2);
}

TEST_F(FFIDeclarationTest, FFINativeFunctionDeclaration) {
    auto paramType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto param = std::make_unique<Parameter>(std::move(paramType), "x");
    std::vector<std::unique_ptr<Parameter>> funcParams;
    funcParams.push_back(std::move(param));
    auto retType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Statement>>{});
    auto nativeFn = std::make_unique<FunctionDeclaration>("add", std::move(funcParams),
        std::move(retType), std::move(body));

    auto symType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    std::vector<std::unique_ptr<FFIParameter>> ffiParams;
    ffiParams.push_back(std::make_unique<FFIParameter>("x",
        std::make_unique<FFIPrimitiveType>(PrimitiveTypeKind::INT64)));
    auto ffiRet = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);

    FFINativeFunctionDeclaration fnfd(true, std::move(nativeFn), std::move(symType),
        "add_impl", std::move(ffiParams), std::move(ffiRet), {});
    EXPECT_TRUE(fnfd.isExtern());
    EXPECT_NE(fnfd.getNativeFunction(), nullptr);
    EXPECT_EQ(fnfd.getSymbolName(), "add_impl");
    EXPECT_EQ(fnfd.getFfiParameters().size(), 1);
    EXPECT_EQ(fnfd.toString(), "FFINativeFunction(add)");
}

TEST_F(FFIDeclarationTest, FFINativeVariableDeclaration) {
    auto type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto var = std::make_unique<VariableDeclaration>(std::move(type), "global_flag", nullptr, true);
    FFINativeVariableDeclaration fnvd(true, std::move(var));
    EXPECT_TRUE(fnvd.isExtern());
    EXPECT_NE(fnvd.getVariable(), nullptr);
    EXPECT_EQ(fnvd.getVariable()->getName(), "global_flag");
    EXPECT_EQ(fnvd.toString(), "FFINativeVariable(global_flag)");
}
