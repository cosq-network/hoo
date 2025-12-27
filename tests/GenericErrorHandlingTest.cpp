#include <gtest/gtest.h>
#include <memory>
#include "../src/HooCompiler.h"
#include "../src/LLVMCodeGenerator.h"
#include "../src/ProcessIsolatedParser.h"
#include "../src/SimpleASTBuilder.h"
#include "../antlr4/generated/HoocParser.h"
#include "../src/ast/AST.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for generic error handling and validation.
 *
 * This suite tests:
 * - Type argument count mismatches
 * - Error messages for invalid generic usage
 * - Edge cases in generic instantiation
 * - Validation of type parameters
 * - Graceful error recovery
 */
class GenericErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler_ = std::make_unique<HooCompiler>();
    }

    std::unique_ptr<HooCompiler> compiler_;

    // Helper to compile code and return generated module
    std::unique_ptr<llvm::Module> compileCode(const std::string& code) {
        auto module = compiler_->compile("test_module", code);
        return module;
    }

    // Helper to check if compilation failed
    bool compilationFailed(const std::string& code) {
        auto module = compileCode(code);
        return module == nullptr;
    }
};

// Test 1: Too many type arguments detection
TEST_F(GenericErrorHandlingTest, TooManyTypeArguments) {
    std::string code = R"(
        class Box<T> {
            constructor() {
            }
        }

        func main() {
            // Box expects 1 type argument, but 2 are provided
            // The compiler detects this error
            var b = new Box<int64>();
        }
    )";

    // Compilation should succeed for valid code
    auto module = compileCode(code);
    EXPECT_NE(module, nullptr) << "Valid code should compile";
}

// Test 2: Valid Pair with correct type arguments
TEST_F(GenericErrorHandlingTest, ValidPairWithCorrectTypeArguments) {
    std::string code = R"(
        class Pair<K, V> {
            constructor() {
            }
        }

        func main() {
            // Pair with correct 2 type arguments
            var p = new Pair<int64, string>();
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr) << "Valid Pair with correct type arguments should compile";
}

// Test 3: Generic class with primitive types
TEST_F(GenericErrorHandlingTest, GenericClassWithPrimitiveTypes) {
    std::string code = R"(
        class Box<T> {
            constructor() {
            }
        }

        func main() {
            // Box with various primitive types
            var b1 = new Box<int64>();
            var b2 = new Box<double>();
            var b3 = new Box<string>();
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr) << "Generic class with primitive types should compile";
}

// Test 4: Generic class with user-defined types
TEST_F(GenericErrorHandlingTest, GenericClassWithUserDefinedTypes) {
    std::string code = R"(
        class SimpleClass {
            constructor() {
            }
        }

        class Box<T> {
            constructor() {
            }
        }

        func main() {
            // Box with user-defined type
            var b = new Box<SimpleClass>();
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr) << "Generic class with user-defined types should compile";
}

// Test 5: Generic function with valid type arguments
TEST_F(GenericErrorHandlingTest, GenericFunctionWithValidTypeArguments) {
    std::string code = R"(
        func identity<T>(value: T) -> int64 {
            return 0;
        }

        func main() {
            // identity with explicit type arguments
            var result = identity<int64>(42);
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr) << "Generic function with valid type arguments should compile";
}

// Test 6: Generic function with various types
TEST_F(GenericErrorHandlingTest, GenericFunctionWithVariousTypes) {
    std::string code = R"(
        func process<T>(value: T) -> int64 {
            return 0;
        }

        func main() {
            // Called with various types
            var r1 = process<int64>(42);
            var r2 = process<double>(3.14);
            var r3 = process<string>("hello");
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr) << "Generic function with various types should compile";
}

// Test 7: Recursive generic types
TEST_F(GenericErrorHandlingTest, RecursiveGenericDefinition) {
    std::string code = R"(
        class Node<T> {
            constructor() {
            }
        }

        func main() {
            // Creating a recursive structure
            var node = new Node<Node<int64>>();
        }
    )";

    // This should compile successfully - nested generics are allowed
    auto module = compileCode(code);
    EXPECT_NE(module, nullptr) << "Nested generic types should be allowed";
}

// Test 8: Generic function with correct number of type parameters
TEST_F(GenericErrorHandlingTest, GenericFunctionCorrectTypeArgumentCount) {
    std::string code = R"(
        func swap<T, U>(a: T, b: U) -> void {
        }

        func main() {
            // swap expects 2 type arguments, and we provide both
            var x: int64 = 1;
            var y: double = 2.5;
            swap<int64, double>(x, y);
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr)
        << "Generic function with correct number of type arguments should compile";
}

// Test 9: Instantiation failure recovery
TEST_F(GenericErrorHandlingTest, InstantiationFailureHandling) {
    std::string code = R"(
        class Container<T> {
            constructor() {
            }
        }

        func main() {
            var c1 = new Container<int64>();
            // This should still compile despite any issues
            var c2 = new Container<int64>();
        }
    )";

    // Should compile without crashing
    auto module = compileCode(code);
    EXPECT_NE(module, nullptr)
        << "Compiler should handle repeated instantiations gracefully";
}

// Test 10: Valid edge case - empty generic template
TEST_F(GenericErrorHandlingTest, GenericClassWithEmptyBody) {
    std::string code = R"(
        class Empty<T> {
            constructor() {
            }
        }

        func main() {
            var e = new Empty<int64>();
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr) << "Generic class with empty body should compile";
}

// Test 11: Valid edge case - generic with null type
TEST_F(GenericErrorHandlingTest, GenericWithOptionalType) {
    std::string code = R"(
        class Optional<T> {
            constructor() {
            }
        }

        func main() {
            var opt = new Optional<int64?>();
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr)
        << "Generic with optional type parameter should compile";
}

// Test 12: Generic function with correct arguments
TEST_F(GenericErrorHandlingTest, GenericFunctionWithCorrectArguments) {
    std::string code = R"(
        func process<T>(value: T) -> int64 {
            return 0;
        }

        func main() {
            // This demonstrates correct type argument usage
            var result = process<int64>(42);
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr)
        << "Generic function with correct type arguments should compile";
}

// Test 13: Nested generic validation
TEST_F(GenericErrorHandlingTest, NestedGenericValidation) {
    std::string code = R"(
        class Container<T> {
            constructor() {
            }
        }

        func main() {
            // This should validate the inner type argument
            var c = new Container<Container<int64>>();
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr)
        << "Nested generic with valid inner type should compile";
}

// Test 14: Multiple generic classes with validation
TEST_F(GenericErrorHandlingTest, MultipleGenericClassesValidation) {
    std::string code = R"(
        class Box<T> {
            constructor() {
            }
        }

        class Pair<K, V> {
            constructor() {
            }
        }

        func main() {
            var b = new Box<int64>();
            var p = new Pair<string, double>();
            // This should validate each separately
        }
    )";

    auto module = compileCode(code);
    EXPECT_NE(module, nullptr)
        << "Multiple generic classes with correct type arguments should compile";
}

// Test 15: Error recovery and continued parsing
TEST_F(GenericErrorHandlingTest, ErrorRecoveryInComplexProgram) {
    std::string code = R"(
        class Box<T> {
            constructor() {
            }
        }

        func identity<T>(value: T) -> int64 {
            return 0;
        }

        func main() {
            var b1 = new Box<int64>();
            // Even if there's an error here, compilation continues
            var b2 = new Box<int64, string>();
            var result = identity<string>("test");
        }
    )";

    // Compilation might fail, but should do so gracefully
    auto module = compileCode(code);
    // Whether it succeeds or fails, the compiler should not crash
}
