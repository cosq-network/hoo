#include <gtest/gtest.h>
#include <memory>
#include "../src/LLVMCodeGenerator.h"
#include "../src/ProcessIsolatedParser.h"
#include "../src/SimpleASTBuilder.h"
#include "HoocParser.h"
#include "../src/ast/AST.h"
#include "../src/ast/Type.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for generic type name mangling.
 *
 * This suite tests:
 * - Name mangling for generic classes (Array<int64> -> Array_int64)
 * - Name mangling for generic functions (swap<string, int64> -> swap_string_int64)
 * - Type parameter scope stack management (push/pop/resolve)
 * - Mangling of various type constructs (primitives, user-defined types, nested generics)
 */
class GenericNameManglingTest : public ::testing::Test {
protected:
    void SetUp() override {
        context_ = std::make_unique<llvm::LLVMContext>();
        codeGen_ = std::make_unique<LLVMCodeGenerator>(*context_);
        parser_ = std::make_unique<ProcessIsolatedParser>();
        astBuilder_ = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<LLVMCodeGenerator> codeGen_;
    std::unique_ptr<ProcessIsolatedParser> parser_;
    std::unique_ptr<SimpleASTBuilder> astBuilder_;

    antlr4::tree::ParseTree* parseCode(const std::string& code) {
        return parser_->parseForAST(code);
    }

    HoocParser::CompilationUnitContext* getCompilationUnit(antlr4::tree::ParseTree* tree) {
        return dynamic_cast<HoocParser::CompilationUnitContext*>(tree);
    }

    std::unique_ptr<CompilationUnit> buildAST(const std::string& code) {
        auto* parseTree = parseCode(code);
        if (!parseTree) return nullptr;
        return astBuilder_->buildAST(getCompilationUnit(parseTree));
    }
};

// Test 1: Mangle simple primitive type
TEST_F(GenericNameManglingTest, ManglePrimitiveType) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create type arguments: [int64]
    std::vector<std::unique_ptr<Type>> typeArgs;
    auto primType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto baseType = std::make_unique<BaseType>(std::move(primType));
    typeArgs.push_back(std::move(baseType));

    std::string mangled = codeGen_->mangleClassName("Box", typeArgs);
    EXPECT_EQ(mangled, "Box_int64") << "Should mangle Box with int64 as Box_int64";
}

// Test 2: Mangle with multiple type arguments
TEST_F(GenericNameManglingTest, MangleMultipleTypeArguments) {
    std::string code = R"(
        class Pair<K, V> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create type arguments: [string, int64]
    std::vector<std::unique_ptr<Type>> typeArgs;

    auto stringId = std::make_unique<BaseType>("string");
    typeArgs.push_back(std::move(stringId));

    auto primType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto int64Type = std::make_unique<BaseType>(std::move(primType));
    typeArgs.push_back(std::move(int64Type));

    std::string mangled = codeGen_->mangleClassName("Pair", typeArgs);
    EXPECT_EQ(mangled, "Pair_string_int64") << "Should mangle Pair<string, int64> as Pair_string_int64";
}

// Test 3: Non-generic class name unchanged
TEST_F(GenericNameManglingTest, NonGenericClassNameUnchanged) {
    std::string code = R"(
        class Widget {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create empty type arguments
    std::vector<std::unique_ptr<Type>> typeArgs;

    std::string mangled = codeGen_->mangleClassName("Widget", typeArgs);
    EXPECT_EQ(mangled, "Widget") << "Non-generic class name should remain unchanged";
}

// Test 4: Function name mangling
TEST_F(GenericNameManglingTest, MangleFunctionName) {
    std::string code = R"(
        func swap<T>(a: T, b: T) -> void {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create type arguments: [string]
    std::vector<std::unique_ptr<Type>> typeArgs;
    auto stringType = std::make_unique<BaseType>("string");
    typeArgs.push_back(std::move(stringType));

    std::string mangled = codeGen_->mangleFunctionNameWithTypes("swap", typeArgs);
    EXPECT_EQ(mangled, "swap_string") << "Should mangle swap<string> as swap_string";
}

// Test 5: Function name mangling with multiple type arguments
TEST_F(GenericNameManglingTest, MangleFunctionNameMultipleArgs) {
    std::string code = R"(
        func combine<T, U>(a: T, b: U) -> void {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create type arguments: [int64, string]
    std::vector<std::unique_ptr<Type>> typeArgs;

    auto primType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto int64Type = std::make_unique<BaseType>(std::move(primType));
    typeArgs.push_back(std::move(int64Type));

    auto stringType = std::make_unique<BaseType>("string");
    typeArgs.push_back(std::move(stringType));

    std::string mangled = codeGen_->mangleFunctionNameWithTypes("combine", typeArgs);
    EXPECT_EQ(mangled, "combine_int64_string") << "Should mangle combine<int64, string> as combine_int64_string";
}

// Test 6: Mangle user-defined type
TEST_F(GenericNameManglingTest, MangleUserDefinedType) {
    std::string code = R"(
        class Container<T> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create type arguments: [MyClass]
    std::vector<std::unique_ptr<Type>> typeArgs;
    auto myType = std::make_unique<BaseType>("MyClass");
    typeArgs.push_back(std::move(myType));

    std::string mangled = codeGen_->mangleClassName("Container", typeArgs);
    EXPECT_EQ(mangled, "Container_MyClass") << "Should mangle Container<MyClass> as Container_MyClass";
}

// Test 7: Type parameter scope push/pop
TEST_F(GenericNameManglingTest, TypeParameterScopePushPop) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create LLVM types for binding
    llvm::Type* int64Ty = llvm::Type::getInt64Ty(*context_);
    llvm::Type* doubleTy = llvm::Type::getDoubleTy(*context_);

    // Push first scope: T -> int64
    std::vector<std::string> params1 = {"T"};
    std::vector<llvm::Type*> types1 = {int64Ty};
    codeGen_->pushTypeParameterScope(params1, types1);

    // Verify T resolves to int64
    EXPECT_EQ(codeGen_->resolveTypeParameter("T"), int64Ty);

    // Pop scope
    codeGen_->popTypeParameterScope();

    // Verify T no longer resolves
    EXPECT_EQ(codeGen_->resolveTypeParameter("T"), nullptr);
}

// Test 8: Type parameter scope nesting
TEST_F(GenericNameManglingTest, TypeParameterScopeNesting) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    llvm::Type* int64Ty = llvm::Type::getInt64Ty(*context_);
    llvm::Type* doubleTy = llvm::Type::getDoubleTy(*context_);
    llvm::Type* boolTy = llvm::Type::getInt1Ty(*context_);

    // Push first scope: T -> int64
    codeGen_->pushTypeParameterScope({"T"}, {int64Ty});
    EXPECT_EQ(codeGen_->resolveTypeParameter("T"), int64Ty);

    // Push second scope: T -> double (shadows outer T), U -> bool
    codeGen_->pushTypeParameterScope({"T", "U"}, {doubleTy, boolTy});
    EXPECT_EQ(codeGen_->resolveTypeParameter("T"), doubleTy) << "Inner scope T should shadow outer";
    EXPECT_EQ(codeGen_->resolveTypeParameter("U"), boolTy);

    // Pop inner scope
    codeGen_->popTypeParameterScope();
    EXPECT_EQ(codeGen_->resolveTypeParameter("T"), int64Ty) << "Outer scope T should be visible";
    EXPECT_EQ(codeGen_->resolveTypeParameter("U"), nullptr) << "U should no longer be visible";

    // Pop outer scope
    codeGen_->popTypeParameterScope();
    EXPECT_EQ(codeGen_->resolveTypeParameter("T"), nullptr);
}

// Test 9: Type parameter scope with empty pop
TEST_F(GenericNameManglingTest, TypeParameterScopeEmptyPop) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Pop from empty stack should not crash
    codeGen_->popTypeParameterScope();
    codeGen_->popTypeParameterScope();

    // Should be able to push after empty pops
    llvm::Type* int64Ty = llvm::Type::getInt64Ty(*context_);
    codeGen_->pushTypeParameterScope({"T"}, {int64Ty});
    EXPECT_EQ(codeGen_->resolveTypeParameter("T"), int64Ty);

    // Clean up
    codeGen_->popTypeParameterScope();
}

// Test 10: Resolve non-existent type parameter
TEST_F(GenericNameManglingTest, ResolveNonExistentTypeParameter) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    llvm::Type* int64Ty = llvm::Type::getInt64Ty(*context_);

    // Push scope with T
    codeGen_->pushTypeParameterScope({"T"}, {int64Ty});

    // Try to resolve non-existent parameter
    EXPECT_EQ(codeGen_->resolveTypeParameter("U"), nullptr) << "Non-existent parameter should return nullptr";
    EXPECT_EQ(codeGen_->resolveTypeParameter("V"), nullptr) << "Non-existent parameter should return nullptr";

    // Pop scope
    codeGen_->popTypeParameterScope();
}

// Test 11: Nested generic type mangling
TEST_F(GenericNameManglingTest, MangleNestedGenericType) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create type arguments: [Array<int64>]
    std::vector<std::unique_ptr<Type>> typeArgs;

    // Inner type: int64
    auto primType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto int64Type = std::make_unique<BaseType>(std::move(primType));

    // Inner type arguments: [int64]
    std::vector<std::unique_ptr<Type>> innerArgs;
    innerArgs.push_back(std::move(int64Type));

    // Outer type: Array<int64>
    auto arrayType = std::make_unique<BaseType>("Array", std::move(innerArgs));
    typeArgs.push_back(std::move(arrayType));

    std::string mangled = codeGen_->mangleClassName("Box", typeArgs);
    EXPECT_EQ(mangled, "Box_Array_int64") << "Should mangle Box<Array<int64>> as Box_Array_int64";
}

// Test 12: Complex nested generic with multiple type parameters
TEST_F(GenericNameManglingTest, MangleComplexNestedGeneric) {
    std::string code = R"(
        class Map<K, V> {
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);

    // Create type arguments: [string, Array<int64>]
    std::vector<std::unique_ptr<Type>> typeArgs;

    // First arg: string
    auto stringType = std::make_unique<BaseType>("string");
    typeArgs.push_back(std::move(stringType));

    // Second arg: Array<int64>
    auto primType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto int64Type = std::make_unique<BaseType>(std::move(primType));

    std::vector<std::unique_ptr<Type>> innerArgs;
    innerArgs.push_back(std::move(int64Type));

    auto arrayType = std::make_unique<BaseType>("Array", std::move(innerArgs));
    typeArgs.push_back(std::move(arrayType));

    std::string mangled = codeGen_->mangleClassName("Map", typeArgs);
    EXPECT_EQ(mangled, "Map_string_Array_int64") << "Should mangle Map<string, Array<int64>> as Map_string_Array_int64";
}
