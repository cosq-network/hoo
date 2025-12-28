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
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class StringCodeGenTest : public ::testing::Test {
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

    bool verifyModuleIsValid(llvm::Module* module) {
        std::string errorMsg;
        raw_string_ostream errorStream(errorMsg);
        return !verifyModule(*module, &errorStream);
    }
};

// ============================================================================
// String Literal Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringLiteralSimple) {
    std::string code = R"(
        func test() -> void {
            var greeting = "Hello";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    // Should generate hoo_string_from_cstr call
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_from_cstr") != std::string::npos);
    // Modern LLVM uses opaque pointer syntax (ptr instead of i8*)
    EXPECT_TRUE(irString.find("alloca ptr") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringLiteralMultiple) {
    std::string code = R"(
        func test() -> void {
            var greeting = "Hello";
            var name = "World";
            var punct = "!";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    // Should generate multiple hoo_string_from_cstr calls
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_from_cstr") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringLiteralEmpty) {
    std::string code = R"(
        func test() -> void {
            var empty = "";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_from_cstr") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringLiteralWithSpaces) {
    std::string code = R"(
        func test() -> void {
            var message = "Hello, World!";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_from_cstr") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// String Concatenation Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringConcatenationSimple) {
    std::string code = R"(
        func test() -> void {
            var greeting = "Hello";
            var name = "World";
            var message = greeting + name;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    // Should generate hoo_string_concat call
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_concat") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringConcatenationLiteral) {
    std::string code = R"(
        func test() -> void {
            var message = "Hello" + "World";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_from_cstr") != std::string::npos);
    EXPECT_TRUE(irString.find("@hoo_string_concat") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringConcatenationChained) {
    std::string code = R"(
        func test() -> void {
            var result = "Hello" + ", " + "World" + "!";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Should generate multiple concat calls for chained operations
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_concat") != std::string::npos);

    // Count occurrences of concat call (should be at least 3 for 4 strings)
    size_t pos = 0;
    int concatCount = 0;
    while ((pos = irString.find("@hoo_string_concat", pos)) != std::string::npos) {
        concatCount++;
        pos += 18;
    }
    EXPECT_GE(concatCount, 3);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringConcatenationThreeOperands) {
    std::string code = R"(
        func test() -> void {
            var greeting = "Hello";
            var comma = ", ";
            var name = "World";
            var message = greeting + comma + name;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_concat") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// String Equality Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringEqualityComparison) {
    std::string code = R"(
        func test() -> void {
            var s1 = "hello";
            var s2 = "hello";
            if (s1 == s2) {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    // Should generate hoo_string_equals call
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp ne i64") != std::string::npos); // Type conversion
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringEqualityWithLiterals) {
    std::string code = R"(
        func test() -> void {
            var name = "Alice";
            if (name == "Alice") {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringEqualityTwoLiterals) {
    std::string code = R"(
        func test() -> void {
            if ("hello" == "hello") {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// String Inequality Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringInequalityComparison) {
    std::string code = R"(
        func test() -> void {
            var s1 = "hello";
            var s2 = "world";
            if (s1 != s2) {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp eq i64") != std::string::npos); // Inverted for !=
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringInequalityWithLiterals) {
    std::string code = R"(
        func test() -> void {
            if ("hello" != "world") {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// String Less Than Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringLessThanComparison) {
    std::string code = R"(
        func test() -> void {
            var s1 = "apple";
            var s2 = "banana";
            if (s1 < s2) {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_compare") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp slt i64") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringLessThanLiterals) {
    std::string code = R"(
        func test() -> void {
            if ("apple" < "banana") {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_compare") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// String Greater Than Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringGreaterThanComparison) {
    std::string code = R"(
        func test() -> void {
            var s1 = "zebra";
            var s2 = "apple";
            if (s1 > s2) {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_compare") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp sgt i64") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// String Less Than Or Equal Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringLessThanOrEqual) {
    std::string code = R"(
        func test() -> void {
            var s1 = "apple";
            var s2 = "apple";
            if (s1 <= s2) {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_compare") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp sle i64") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// String Greater Than Or Equal Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringGreaterThanOrEqual) {
    std::string code = R"(
        func test() -> void {
            var s1 = "zebra";
            var s2 = "zebra";
            if (s1 >= s2) {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_compare") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp sge i64") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// Complex Expression Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringComplexExpression) {
    std::string code = R"(
        func test() -> void {
            var greeting = "Hello";
            var name = "World";
            var message = greeting + ", " + name + "!";
            if (message == "Hello, World!") {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_concat") != std::string::npos);
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// String Function Call Tests
// ============================================================================

TEST_F(StringCodeGenTest, StringPassedToFunction) {
    std::string code = R"(
        func print_string(s: string) -> void {
            return;
        }

        func test() -> void {
            var msg = "Hello";
            print_string(msg);
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* printFunc = module->getFunction("print_string");
    ASSERT_NE(printFunc, nullptr);

    // String parameter should be i8* (pointer)
    EXPECT_EQ(printFunc->arg_size(), 1);
    EXPECT_TRUE(printFunc->arg_begin()->getType()->isPointerTy());
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringReturnType) {
    std::string code = R"(
        func get_greeting() -> string {
            var msg = "Hello";
            return msg;
        }

        func test() -> void {
            var greeting = get_greeting();
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* getFunc = module->getFunction("get_greeting");
    ASSERT_NE(getFunc, nullptr);

    // Return type should be i8* (pointer)
    EXPECT_TRUE(getFunc->getReturnType()->isPointerTy());
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// Multiple Operations Tests
// ============================================================================

TEST_F(StringCodeGenTest, MultipleStringOperations) {
    std::string code = R"(
        func test() -> void {
            var s1 = "hello";
            var s2 = "world";

            if (s1 != s2) {
                var combined = s1 + " " + s2;
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(irString.find("@hoo_string_concat") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringVariableReuse) {
    std::string code = R"(
        func test() -> void {
            var msg = "test";

            if (msg == "test") {
                if (msg != "other") {
                    if (msg < "zzz") {
                        return;
                    }
                }
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(irString.find("@hoo_string_compare") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(StringCodeGenTest, EmptyStringLiteral) {
    std::string code = R"(
        func test() -> void {
            var empty = "";
            if (empty == "") {
                return;
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, LongStringLiteral) {
    std::string code = R"(
        func test() -> void {
            var longStr = "This is a very long string with many characters and spaces";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, SpecialCharactersInString) {
    std::string code = R"(
        func test() -> void {
            var special = "!@#$%^&*()_+-=[]{}|;:,.<>?";
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringInWhileLoop) {
    std::string code = R"(
        func test() -> void {
            var flag = true;
            var msg = "hello";

            while (flag) {
                if (msg == "hello") {
                    flag = false;
                }
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

TEST_F(StringCodeGenTest, StringInForLoop) {
    std::string code = R"(
        func test() -> void {
            var name = "Alice";

            for i in 0..5 {
                if (name == "Alice") {
                    return;
                }
            }
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}

// ============================================================================
// All Operators Together
// ============================================================================

TEST_F(StringCodeGenTest, AllStringOperators) {
    std::string code = R"(
        func test() -> void {
            var a = "apple";
            var b = "banana";
            var c = "apple";

            // Concatenation
            var combined = a + b;

            // Equality
            if (a == c) {
                return;
            }

            // Inequality
            if (a != b) {
                return;
            }

            // Ordering
            if (a < b) {
                return;
            }

            if (b > a) {
                return;
            }

            if (a <= c) {
                return;
            }

            if (c >= a) {
                return;
            }

            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string irString = getModuleString(module.get());

    // Verify all functions are called
    EXPECT_TRUE(irString.find("@hoo_string_from_cstr") != std::string::npos);
    EXPECT_TRUE(irString.find("@hoo_string_concat") != std::string::npos);
    EXPECT_TRUE(irString.find("@hoo_string_equals") != std::string::npos);
    EXPECT_TRUE(irString.find("@hoo_string_compare") != std::string::npos);

    // Verify LLVM IR is valid
    EXPECT_TRUE(verifyModuleIsValid(module.get()));
}
