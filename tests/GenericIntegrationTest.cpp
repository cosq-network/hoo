#include <gtest/gtest.h>
#include <memory>
#include "../src/HooCompiler.h"
#include "../src/LLVMCodeGenerator.h"
#include "../src/ProcessIsolatedParser.h"
#include "../src/SimpleASTBuilder.h"
#include "HoocParser.h"
#include "../src/ast/AST.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Integration test suite for generic classes and functions.
 *
 * This suite tests realistic end-to-end scenarios:
 * - Generic containers with methods
 * - Generic utility functions
 * - Nested generic types
 * - Multiple instantiations in a single program
 * - Generic functions working with generic classes
 */
class GenericIntegrationTest : public ::testing::Test {
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

    // Helper to check if a function exists in the module
    bool functionExists(llvm::Module* module, const std::string& funcName) {
        return module->getFunction(funcName) != nullptr;
    }

    // Helper to check if a type exists in the module
    bool typeExists(llvm::Module* module, const std::string& typeName) {
        auto type = llvm::StructType::getTypeByName(module->getContext(), typeName);
        return type != nullptr;
    }
};

// Test 1: Generic container class with multiple instantiations
TEST_F(GenericIntegrationTest, GenericContainerWithMultipleInstantiations) {
    std::string code = R"(
        class Container<T> {
            constructor() {
            }

            func add(item: T) -> void {
            }

            func getCount() -> int64 {
                return 0;
            }
        }

        func main() {
            var intContainer = new Container<int64>();
            var stringContainer = new Container<string>();
            var doubleContainer = new Container<double>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that all instantiated types were created
    EXPECT_TRUE(typeExists(module.get(), "Container_int64"))
        << "Should have generated Container_int64";
    EXPECT_TRUE(typeExists(module.get(), "Container_string"))
        << "Should have generated Container_string";
    EXPECT_TRUE(typeExists(module.get(), "Container_double"))
        << "Should have generated Container_double";

    // Check that all constructors were generated
    EXPECT_TRUE(functionExists(module.get(), "Container_int64_init"));
    EXPECT_TRUE(functionExists(module.get(), "Container_string_init"));
    EXPECT_TRUE(functionExists(module.get(), "Container_double_init"));

    // Check that all methods were generated
    EXPECT_TRUE(functionExists(module.get(), "Container_int64_add"));
    EXPECT_TRUE(functionExists(module.get(), "Container_string_add"));
    EXPECT_TRUE(functionExists(module.get(), "Container_int64_getCount"));
}

// Test 2: Generic function working with different types
TEST_F(GenericIntegrationTest, GenericFunctionWithMultipleTypes) {
    std::string code = R"(
        func process<T>(value: T) -> int64 {
            return 0;
        }

        func main() {
            var a = process<int64>(42);
            var b = process<double>(3.14);
            var c = process<string>("hello");
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that all instantiated functions were created
    EXPECT_TRUE(functionExists(module.get(), "process_int64"));
    EXPECT_TRUE(functionExists(module.get(), "process_double"));
    EXPECT_TRUE(functionExists(module.get(), "process_string"));
}

// Test 3: Generic class with generic method-like behavior
TEST_F(GenericIntegrationTest, GenericClassWithTypedMethods) {
    std::string code = R"(
        class Wrapper<T> {
            constructor() {
            }

            func getValue() -> int64 {
                return 100;
            }

            func transform(input: T) -> int64 {
                return 42;
            }
        }

        func main() {
            var intWrapper = new Wrapper<int64>();
            var stringWrapper = new Wrapper<string>();

            var result1 = intWrapper.getValue();
            var result2 = intWrapper.transform(5);

            var result3 = stringWrapper.getValue();
            var result4 = stringWrapper.transform("test");
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check both instantiations exist
    EXPECT_TRUE(typeExists(module.get(), "Wrapper_int64"));
    EXPECT_TRUE(typeExists(module.get(), "Wrapper_string"));

    // Check methods for both instantiations
    EXPECT_TRUE(functionExists(module.get(), "Wrapper_int64_getValue"));
    EXPECT_TRUE(functionExists(module.get(), "Wrapper_int64_transform"));
    EXPECT_TRUE(functionExists(module.get(), "Wrapper_string_getValue"));
    EXPECT_TRUE(functionExists(module.get(), "Wrapper_string_transform"));
}

// Test 4: Nested generic types
TEST_F(GenericIntegrationTest, NestedGenericTypes) {
    std::string code = R"(
        class Pair<T> {
            constructor() {
            }
        }

        class Container<T> {
            constructor() {
            }
        }

        func main() {
            var x = new Container<Pair<int64>>();
            var y = new Container<Pair<string>>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that nested generic types were created
    // The inner Pair<int64> and Pair<string> should exist
    EXPECT_TRUE(typeExists(module.get(), "Pair_int64"));
    EXPECT_TRUE(typeExists(module.get(), "Pair_string"));

    // The outer Container<Pair_int64> and Container<Pair_string> should exist
    EXPECT_TRUE(typeExists(module.get(), "Container_Pair_int64"));
    EXPECT_TRUE(typeExists(module.get(), "Container_Pair_string"));
}

// Test 5: Generic function with explicit type arguments and usage
TEST_F(GenericIntegrationTest, GenericFunctionWithExplicitTypeArguments) {
    std::string code = R"(
        func identity<T>(value: T) -> int64 {
            return 0;
        }

        class Holder<T> {
            constructor() {
            }
        }

        func main() {
            var b1 = new Holder<int64>();
            var b2 = new Holder<string>();
            var i1 = identity<int64>(42);
            var i2 = identity<string>("hello");
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that both Holder instantiations exist
    EXPECT_TRUE(typeExists(module.get(), "Holder_int64"));
    EXPECT_TRUE(typeExists(module.get(), "Holder_string"));

    // Check that both function instantiations exist
    EXPECT_TRUE(functionExists(module.get(), "identity_int64"));
    EXPECT_TRUE(functionExists(module.get(), "identity_string"));
}

// Test 6: Multiple generic parameters in same program
TEST_F(GenericIntegrationTest, MultipleGenericParametersInProgram) {
    std::string code = R"(
        func swap<T, U>(a: T, b: U) -> void {
        }

        class Pair<K, V> {
            constructor() {
            }
        }

        func main() {
            var x: int64 = 1;
            var y: double = 2.5;
            swap<int64, double>(x, y);

            var p1 = new Pair<int64, string>();
            var p2 = new Pair<string, double>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check function instantiation
    EXPECT_TRUE(functionExists(module.get(), "swap_int64_double"));

    // Check class instantiations
    EXPECT_TRUE(typeExists(module.get(), "Pair_int64_string"));
    EXPECT_TRUE(typeExists(module.get(), "Pair_string_double"));
}

// Test 7: Generic class with array of type parameter
TEST_F(GenericIntegrationTest, GenericClassWithArrayField) {
    std::string code = R"(
        class List<T> {
            var items: T[];

            constructor() {
            }

            func length() -> int64 {
                return 0;
            }
        }

        func main() {
            var intList = new List<int64>();
            var stringList = new List<string>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that both instantiations exist
    EXPECT_TRUE(typeExists(module.get(), "List_int64"));
    EXPECT_TRUE(typeExists(module.get(), "List_string"));

    // Check that constructors exist
    EXPECT_TRUE(functionExists(module.get(), "List_int64_init"));
    EXPECT_TRUE(functionExists(module.get(), "List_string_init"));

    // Check that methods exist
    EXPECT_TRUE(functionExists(module.get(), "List_int64_length"));
    EXPECT_TRUE(functionExists(module.get(), "List_string_length"));
}

// Test 8: Generic function with array parameter
TEST_F(GenericIntegrationTest, GenericFunctionWithArrayParameter) {
    std::string code = R"(
        func sum<T>(arr: T[]) -> int64 {
            return 0;
        }

        func main() {
            var ints = [1, 2, 3];
            var floats = [1.0, 2.5, 3.14];
            var result1 = sum<int64>(ints);
            var result2 = sum<double>(floats);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that both instantiations exist
    EXPECT_TRUE(functionExists(module.get(), "sum_int64"));
    EXPECT_TRUE(functionExists(module.get(), "sum_double"));
}

// Test 9: Mix of generic and non-generic code
TEST_F(GenericIntegrationTest, MixGenericAndNonGeneric) {
    std::string code = R"(
        class SimpleClass {
            constructor() {
            }

            func getValue() -> int64 {
                return 42;
            }
        }

        class GenericClass<T> {
            constructor() {
            }

            func process(value: T) -> int64 {
                return 0;
            }
        }

        func simpleFunc(x: int64) -> int64 {
            return x + 1;
        }

        func genericFunc<T>(value: T) -> int64 {
            return 100;
        }

        func main() {
            var simple = new SimpleClass();
            var generic1 = new GenericClass<int64>();
            var generic2 = new GenericClass<string>();

            var a = simpleFunc(5);
            var b = genericFunc<int64>(10);
            var c = genericFunc<string>("hello");
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check non-generic class and function
    EXPECT_TRUE(typeExists(module.get(), "SimpleClass"));
    EXPECT_TRUE(functionExists(module.get(), "SimpleClass_init"));
    EXPECT_TRUE(functionExists(module.get(), "SimpleClass_getValue"));
    EXPECT_TRUE(functionExists(module.get(), "simpleFunc"));

    // Check generic class instantiations
    EXPECT_TRUE(typeExists(module.get(), "GenericClass_int64"));
    EXPECT_TRUE(typeExists(module.get(), "GenericClass_string"));
    EXPECT_TRUE(functionExists(module.get(), "GenericClass_int64_init"));
    EXPECT_TRUE(functionExists(module.get(), "GenericClass_string_process"));

    // Check generic function instantiations
    EXPECT_TRUE(functionExists(module.get(), "genericFunc_int64"));
    EXPECT_TRUE(functionExists(module.get(), "genericFunc_string"));
}

// Test 10: Complex nested generic scenario
TEST_F(GenericIntegrationTest, ComplexNestedGenericScenario) {
    std::string code = R"(
        class Option<T> {
            constructor() {
            }
        }

        class Result<T, E> {
            constructor() {
            }

            func isOk() -> int64 {
                return 0;
            }
        }

        func unwrap<T>(opt: T) -> int64 {
            return 0;
        }

        func main() {
            var opt1 = new Option<int64>();
            var opt2 = new Option<string>();

            var res1 = new Result<int64, string>();
            var res2 = new Result<string, int64>();

            var u1 = unwrap<int64>(0);
            var u2 = unwrap<string>("s");
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check Option instantiations
    EXPECT_TRUE(typeExists(module.get(), "Option_int64"));
    EXPECT_TRUE(typeExists(module.get(), "Option_string"));

    // Check Result instantiations
    EXPECT_TRUE(typeExists(module.get(), "Result_int64_string"));
    EXPECT_TRUE(typeExists(module.get(), "Result_string_int64"));

    // Check constructors
    EXPECT_TRUE(functionExists(module.get(), "Option_int64_init"));
    EXPECT_TRUE(functionExists(module.get(), "Result_int64_string_init"));

    // Check methods
    EXPECT_TRUE(functionExists(module.get(), "Result_int64_string_isOk"));

    // Check generic function instantiations
    EXPECT_TRUE(functionExists(module.get(), "unwrap_int64"));
    EXPECT_TRUE(functionExists(module.get(), "unwrap_string"));
}
