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
 * Test suite for generic function code generation and monomorphization.
 *
 * This suite tests:
 * - Generic function instantiation with type substitution
 * - Function call code generation with explicit type arguments
 * - Multiple instantiations of same generic function
 * - Proper return type handling with type parameters
 * - Parameter type substitution in function bodies
 */
class GenericFunctionCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        context_ = std::make_unique<llvm::LLVMContext>();
        compiler_ = std::make_unique<HooCompiler>();
    }

    std::unique_ptr<llvm::LLVMContext> context_;
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
};

// Test 1: Simple generic function instantiation
TEST_F(GenericFunctionCodeGenTest, SimpleGenericFunctionInstantiation) {
    std::string code = R"(
        func identity<T>(value: T) -> T {
            return value;
        }

        func main() {
            var result = identity<int64>(42);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated function was created
    EXPECT_TRUE(functionExists(module.get(), "identity_int64"))
        << "Should have generated identity_int64 function";
}

// Test 2: Generic function with multiple type parameters
TEST_F(GenericFunctionCodeGenTest, GenericFunctionWithMultipleTypeParameters) {
    std::string code = R"(
        func swap<T, U>(a: T, b: U) -> void {
        }

        func main() {
            var x: int64 = 1;
            var y: double = 2.5;
            swap<int64, double>(x, y);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated function was created
    EXPECT_TRUE(functionExists(module.get(), "swap_int64_double"))
        << "Should have generated swap_int64_double function";
}

// Test 3: Multiple instantiations of same generic function
TEST_F(GenericFunctionCodeGenTest, MultipleInstantiationsOfGenericFunction) {
    std::string code = R"(
        func identity<T>(value: T) -> T {
            return value;
        }

        func main() {
            var i = identity<int64>(42);
            var s = identity<string>("hello");
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that both instantiated functions were created
    EXPECT_TRUE(functionExists(module.get(), "identity_int64"))
        << "Should have generated identity_int64 function";

    EXPECT_TRUE(functionExists(module.get(), "identity_string"))
        << "Should have generated identity_string function";
}

// Test 4: Generic function returning type parameter
TEST_F(GenericFunctionCodeGenTest, GenericFunctionReturningTypeParameter) {
    std::string code = R"(
        func getValue<T>() -> T {
            return 0;
        }

        func main() {
            var x = getValue<int64>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated function was created
    EXPECT_TRUE(functionExists(module.get(), "getValue_int64"))
        << "Should have generated getValue_int64 function";
}

// Test 5: Generic function with type parameter in parameter
TEST_F(GenericFunctionCodeGenTest, GenericFunctionWithTypeParameterInParameter) {
    std::string code = R"(
        func process<T>(value: T) -> int64 {
            return 42;
        }

        func main() {
            var result = process<int64>(10);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated function was created
    EXPECT_TRUE(functionExists(module.get(), "process_int64"))
        << "Should have generated process_int64 function";
}

// Test 6: Non-generic function not affected
TEST_F(GenericFunctionCodeGenTest, NonGenericFunctionNotAffected) {
    std::string code = R"(
        func add(a: int64, b: int64) -> int64 {
            return a + b;
        }

        func main() {
            var result = add(5, 10);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the non-generic function was created
    EXPECT_TRUE(functionExists(module.get(), "add"))
        << "Should have generated add function";
}

// Test 7: Generic function with array parameter
TEST_F(GenericFunctionCodeGenTest, GenericFunctionWithArrayParameter) {
    std::string code = R"(
        func getLength<T>(arr: T[]) -> int64 {
            return 0;
        }

        func main() {
            var nums = [1, 2, 3];
            var len = getLength<int64>(nums);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated function was created
    EXPECT_TRUE(functionExists(module.get(), "getLength_int64"))
        << "Should have generated getLength_int64 function";
}

// Test 8: Same generic function instantiated twice reuses same function
TEST_F(GenericFunctionCodeGenTest, SameInstantiationReused) {
    std::string code = R"(
        func identity<T>(value: T) -> T {
            return value;
        }

        func main() {
            var x = identity<int64>(42);
            var y = identity<int64>(100);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated function was created
    EXPECT_TRUE(functionExists(module.get(), "identity_int64"))
        << "Should have generated identity_int64 function";

    // Should only have one identity_int64 function, not two
    auto funcCount = 0;
    for (auto& func : *module) {
        if (func.getName() == "identity_int64") {
            funcCount++;
        }
    }
    EXPECT_EQ(funcCount, 1) << "Should only have one identity_int64 function";
}

// Test 9: Generic function with primitive type argument
TEST_F(GenericFunctionCodeGenTest, GenericFunctionWithPrimitiveTypeArgument) {
    std::string code = R"(
        func wrap<T>(value: T) -> void {
        }

        func main() {
            wrap<double>(3.14);
            wrap<int64>(42);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that both instantiated functions were created
    EXPECT_TRUE(functionExists(module.get(), "wrap_double"))
        << "Should have generated wrap_double function";

    EXPECT_TRUE(functionExists(module.get(), "wrap_int64"))
        << "Should have generated wrap_int64 function";
}

// Test 10: Generic function with user-defined type argument
TEST_F(GenericFunctionCodeGenTest, GenericFunctionWithUserDefinedTypeArgument) {
    std::string code = R"(
        class Box {
            constructor() {}
        }

        func process<T>(obj: T) -> void {
        }

        func main() {
            var b = new Box();
            process<Box>(b);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the generic function was instantiated with Box type
    EXPECT_TRUE(functionExists(module.get(), "process_Box"))
        << "Should have generated process_Box function";
}

// Test 11: Generic function with nested type parameters
TEST_F(GenericFunctionCodeGenTest, GenericFunctionWithComplexTypeParameter) {
    std::string code = R"(
        func convert<T>(arr: T[]) -> int64 {
            return 0;
        }

        func main() {
            var data = [1, 2, 3];
            var result = convert<int64>(data);
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated function was created
    EXPECT_TRUE(functionExists(module.get(), "convert_int64"))
        << "Should have generated convert_int64 function";
}

// Test 12: Generic function with explicit void return
TEST_F(GenericFunctionCodeGenTest, GenericFunctionWithVoidReturn) {
    std::string code = R"(
        func process<T>(value: T) -> void {
        }

        func main() {
            process<int64>(42);
            process<string>("hello");
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that both instantiated functions were created
    EXPECT_TRUE(functionExists(module.get(), "process_int64"))
        << "Should have generated process_int64 function";

    EXPECT_TRUE(functionExists(module.get(), "process_string"))
        << "Should have generated process_string function";
}
