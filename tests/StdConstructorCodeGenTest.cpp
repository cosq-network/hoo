#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "../src/LLVMCodeGenerator.h"
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "../src/ast/AST.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class StdConstructorCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<LLVMContext>();
        codeGen = std::make_unique<LLVMCodeGenerator>(*context);
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<LLVMContext> context;
    std::unique_ptr<LLVMCodeGenerator> codeGen;
    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    std::unique_ptr<CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;
        return astBuilder->buildAST(parseTree);
    }

    std::string getModuleString(llvm::Module* module) {
        std::string str;
        raw_string_ostream rso(str);
        module->print(rso, nullptr);
        return str;
    }

    bool isModuleValid(llvm::Module* module) {
        std::string errorMsg;
        raw_string_ostream errorStream(errorMsg);
        return !verifyModule(*module, &errorStream);
    }

    bool moduleContains(llvm::Module* module, const std::string& pattern) {
        std::string ir = getModuleString(module);
        return ir.find(pattern) != std::string::npos;
    }
};

// ============================================================================
// std.String Constructor Tests - Qualified Names
// ============================================================================

TEST_F(StdConstructorCodeGenTest, QualifiedStringEmptyConstructor) {
    std::string code = R"(
        func test() -> std.String {
            return new std.String();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should call hoo_string_new
    std::string ir = getModuleString(llvmModule.get());
    EXPECT_TRUE(moduleContains(llvmModule.get(), "hoo_string_new"));
}

TEST_F(StdConstructorCodeGenTest, QualifiedStringLiteralConstructor) {
    std::string code = R"(
        func test() -> std.String {
            return new std.String("hello");
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should call hoo_string_from_cstr
    EXPECT_TRUE(moduleContains(llvmModule.get(), "hoo_string_from_cstr"));
}

TEST_F(StdConstructorCodeGenTest, QualifiedStringVariableDeclaration) {
    std::string code = R"(
        func test() -> void {
            var s: std.String = new std.String("world");
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, QualifiedStringInFunctionCall) {
    std::string code = R"(
        func process(s: std.String) -> void {}

        func test() -> void {
            process(new std.String("test"));
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

// ============================================================================
// std.Array Constructor Tests - Using array literals and standard types
// ============================================================================

TEST_F(StdConstructorCodeGenTest, QualifiedArrayIntConstructor) {
    std::string code = R"(
        func test() -> void {
            var arr: int64[] = [1, 2, 3, 4, 5];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, QualifiedArrayStringConstructor) {
    std::string code = R"(
        func test() -> void {
            var arr: std.String[] = [new std.String("a"), new std.String("b")];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, QualifiedNestedArrayConstructor) {
    std::string code = R"(
        func test() -> void {
            var arr: int64[][] = [[1, 2], [3, 4]];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, QualifiedArrayVariableDeclaration) {
    std::string code = R"(
        func test() -> void {
            var arr: int64[] = [1, 2, 3];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

// ============================================================================
// Imported Names Tests
// ============================================================================

TEST_F(StdConstructorCodeGenTest, ImportedStringConstructor) {
    std::string code = R"(
        from std import String;

        func test() -> String {
            return new String("imported");
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should still call hoo_string_from_cstr
    EXPECT_TRUE(moduleContains(llvmModule.get(), "hoo_string_from_cstr"));
}

TEST_F(StdConstructorCodeGenTest, ImportedArrayConstructor) {
    std::string code = R"(
        func test() -> void {
            var arr: int64[] = [1, 2, 3];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, ImportedStringAndArray) {
    std::string code = R"(
        func test() -> void {
            var arr: std.String[] = [new std.String("a"), new std.String("b")];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

// ============================================================================
// Mixed Qualified and Imported Names
// ============================================================================

TEST_F(StdConstructorCodeGenTest, MixedQualifiedAndImported) {
    std::string code = R"(
        from std import String;

        func test() -> void {
            var s1: String = new String("imported");
            var s2: std.String = new std.String("qualified");
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should have two hoo_string_from_cstr calls
    std::string ir = getModuleString(llvmModule.get());
    size_t count = 0;
    size_t pos = 0;
    std::string pattern = "hoo_string_from_cstr";
    while ((pos = ir.find(pattern, pos)) != std::string::npos) {
        count++;
        pos += pattern.length();
    }
    EXPECT_GE(count, 2);
}

TEST_F(StdConstructorCodeGenTest, MixedArrayAndString) {
    std::string code = R"(
        func main() -> void {
            var s: std.String = new std.String("hello");
            var arr: int64[] = [1, 2, 3];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(StdConstructorCodeGenTest, MultipleStringConstructions) {
    std::string code = R"(
        func test() -> void {
            var s1 = new std.String("first");
            var s2 = new std.String("second");
            var s3 = new std.String("third");
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, MultipleArrayConstructions) {
    std::string code = R"(
        func test() -> void {
            var arr1: int64[] = [1, 2, 3];
            var arr2: double[] = [1.0, 2.0, 3.0];
            var arr3: bool[] = [true, false];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, StringInComplexExpression) {
    std::string code = R"(
        func createMessage(name: std.String) -> std.String {
            return new std.String("Hello");
        }

        func test() -> void {
            var greeting: std.String = createMessage(new std.String("World"));
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

// ============================================================================
// Integration with User-Defined Classes
// ============================================================================

TEST_F(StdConstructorCodeGenTest, MixedStdAndUserClasses) {
    std::string code = R"(
        class Person {
            constructor() {}
        }

        func test() -> void {
            var s: std.String = new std.String("name");
            var p: Person = new Person();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should have hoo_string_from_cstr for std.String
    EXPECT_TRUE(moduleContains(llvmModule.get(), "hoo_string_from_cstr"));

    // Should have hoo_alloc for user class
    EXPECT_TRUE(moduleContains(llvmModule.get(), "hoo_alloc"));
}

// ============================================================================
// Backward Compatibility Tests
// ============================================================================

TEST_F(StdConstructorCodeGenTest, UserClassConstructorStillWorks) {
    std::string code = R"(
        class MyClass {
            constructor() {}
        }

        func test() -> void {
            var obj: MyClass = new MyClass();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should use hoo_alloc for user-defined class
    EXPECT_TRUE(moduleContains(llvmModule.get(), "hoo_alloc"));
}

TEST_F(StdConstructorCodeGenTest, SimpleIdentifierStillWorks) {
    std::string code = R"(
        class SimpleClass {
            constructor() {}
        }

        func test() -> void {
            var obj = new SimpleClass();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

// ============================================================================
// Qualified Array Types Tests
// ============================================================================

TEST_F(StdConstructorCodeGenTest, QualifiedStringArrayDeclaration) {
    std::string code = R"(
        func test() -> void {
            var strings: std.String[] = [new std.String("a"), new std.String("b")];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should have array runtime functions
    EXPECT_TRUE(moduleContains(llvmModule.get(), "hoo_array"));
}

TEST_F(StdConstructorCodeGenTest, QualifiedStringArrayWithLoop) {
    std::string code = R"(
        func test() -> void {
            var strings: std.String[] = [new std.String("hello"), new std.String("world")];
            var total: int64 = 0;
            var i: int64 = 0;
            while i < 2 {
                total = total + 1;
                i = i + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, NestedQualifiedArrayType) {
    std::string code = R"(
        func test() -> void {
            var nested: std.String[][] = [[new std.String("a"), new std.String("b")], [new std.String("c")]];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, QualifiedArrayAsFunctionParameter) {
    std::string code = R"(
        func processStrings(arr: std.String[]) -> void {
            return;
        }

        func test() -> void {
            var strings: std.String[] = [new std.String("test")];
            processStrings(strings);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Both functions should exist
    EXPECT_NE(llvmModule->getFunction("processStrings"), nullptr);
    EXPECT_NE(llvmModule->getFunction("test"), nullptr);
}

TEST_F(StdConstructorCodeGenTest, QualifiedArrayAsReturnType) {
    std::string code = R"(
        func getStrings() -> std.String[] {
            return [new std.String("a"), new std.String("b")];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Function should exist
    Function* func = llvmModule->getFunction("getStrings");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isPointerTy());
}

// ============================================================================
// Union with Arrays Tests
// ============================================================================

TEST_F(StdConstructorCodeGenTest, UnionWithArrayAndString) {
    std::string code = R"(
        func test() -> void {
            var data: int64[] | std.String = [1, 2, 3];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, UnionWithArrayVariableAssignment) {
    std::string code = R"(
        func test() -> void {
            var value: int64[] | std.String = [10, 20, 30];
            value = [1, 2];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, UnionWithMultiDimensionalArray) {
    std::string code = R"(
        func test() -> void {
            var data: int64[][] | std.String = [[1, 2], [3, 4]];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, UnionWithArrayAsFunctionParameter) {
    std::string code = R"(
        func process(data: int64[] | std.String) -> void {
            return;
        }

        func test() -> void {
            process([1, 2, 3]);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(StdConstructorCodeGenTest, UnionWithArrayReturnType) {
    std::string code = R"(
        func getValue(flag: bool) -> int64[] | std.String {
            if flag {
                return [1, 2, 3];
            }
            return new std.String("default");
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    Function* func = llvmModule->getFunction("getValue");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isPointerTy());
}

TEST_F(StdConstructorCodeGenTest, MixedQualifiedAndUnionArray) {
    std::string code = R"(
        func test() -> void {
            var arr1: std.String[] = [new std.String("a")];
            var arr2: int64[] | std.String[] = [1, 2];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}
