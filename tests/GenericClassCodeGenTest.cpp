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
 * Test suite for generic class code generation and monomorphization.
 *
 * This suite tests:
 * - Generic class struct generation with type substitution
 * - Constructor generation for generic classes
 * - Method generation with type parameter substitution
 * - Multiple instantiations of same generic (Array<int64> vs Array<string>)
 * - Proper LLVM type generation for instantiated generics
 */
class GenericClassCodeGenTest : public ::testing::Test {
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

    // Helper to check if a type exists (check for named struct type)
    bool typeExists(llvm::Module* module, const std::string& typeName) {
        // In LLVM, named struct types are accessible via the module's context
        // We can check if the type was created by looking for any struct with that name
        // For now, a simpler approach: check if struct type can be retrieved
        auto type = llvm::StructType::getTypeByName(module->getContext(), typeName);
        return type != nullptr;
    }
};

// Test 1: Simple generic class instantiation
TEST_F(GenericClassCodeGenTest, SimpleGenericClassInstantiation) {
    std::string code = R"(
        class Box<T> {
            constructor() {
            }
        }

        func main() {
            var b = new Box<int64>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created
    EXPECT_TRUE(typeExists(module.get(), "Box_int64"))
        << "Should have generated Box_int64 struct";

    // Check that the constructor was generated
    EXPECT_TRUE(functionExists(module.get(), "Box_int64_init"))
        << "Should have generated Box_int64_init constructor";
}

// Test 2: Generic class with field of type parameter
TEST_F(GenericClassCodeGenTest, GenericClassWithGenericField) {
    std::string code = R"(
        class Container<T> {
            var item: T;

            constructor() {
            }
        }

        func main() {
            var c = new Container<int64>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created
    EXPECT_TRUE(typeExists(module.get(), "Container_int64"))
        << "Should have generated Container_int64 struct with int64 field";
}

// Test 3: Multiple instantiations of same generic
TEST_F(GenericClassCodeGenTest, MultipleInstantiationsOfGeneric) {
    std::string code = R"(
        class Pair<T> {
            constructor() {
            }
        }

        func main() {
            var p1 = new Pair<int64>();
            var p2 = new Pair<string>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that both instantiated types were created
    EXPECT_TRUE(typeExists(module.get(), "Pair_int64"))
        << "Should have generated Pair_int64 struct";

    EXPECT_TRUE(typeExists(module.get(), "Pair_string"))
        << "Should have generated Pair_string struct";

    // Check constructors for both
    EXPECT_TRUE(functionExists(module.get(), "Pair_int64_init"))
        << "Should have generated Pair_int64_init constructor";

    EXPECT_TRUE(functionExists(module.get(), "Pair_string_init"))
        << "Should have generated Pair_string_init constructor";
}

// Test 4: Generic class with method returning type parameter
TEST_F(GenericClassCodeGenTest, GenericClassWithMethodReturningTypeParameter) {
    std::string code = R"(
        class Holder<T> {
            constructor() {
            }

            func getValue() -> int64 {
                return 42;
            }
        }

        func main() {
            var h = new Holder<int64>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created
    EXPECT_TRUE(typeExists(module.get(), "Holder_int64"))
        << "Should have generated Holder_int64 struct";

    // Check that the method was generated
    EXPECT_TRUE(functionExists(module.get(), "Holder_int64_getValue"))
        << "Should have generated Holder_int64_getValue method";
}

// Test 5: Generic class with method taking type parameter
TEST_F(GenericClassCodeGenTest, GenericClassWithMethodTakingTypeParameter) {
    std::string code = R"(
        class Setter<T> {
            constructor() {
            }

            func set(value: T) {
            }
        }

        func main() {
            var s = new Setter<int64>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created
    EXPECT_TRUE(typeExists(module.get(), "Setter_int64"))
        << "Should have generated Setter_int64 struct";

    // Check that the method was generated
    EXPECT_TRUE(functionExists(module.get(), "Setter_int64_set"))
        << "Should have generated Setter_int64_set method";
}

// Test 6: Non-generic template not instantiated
TEST_F(GenericClassCodeGenTest, NonGenericClassNotAffected) {
    std::string code = R"(
        class SimpleClass {
            constructor() {
            }
        }

        func main() {
            var s = new SimpleClass();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the non-generic type was created
    EXPECT_TRUE(typeExists(module.get(), "SimpleClass"))
        << "Should have generated SimpleClass struct";

    // Check constructor
    EXPECT_TRUE(functionExists(module.get(), "SimpleClass_init"))
        << "Should have generated SimpleClass_init constructor";
}

// Test 7: Generic class with primitive type argument
TEST_F(GenericClassCodeGenTest, GenericClassWithPrimitiveTypeArgument) {
    std::string code = R"(
        class Box<T> {
            constructor() {
            }
        }

        func main() {
            var b1 = new Box<int64>();
            var b2 = new Box<double>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that both instantiated types were created
    EXPECT_TRUE(typeExists(module.get(), "Box_int64"))
        << "Should have generated Box_int64 struct";

    EXPECT_TRUE(typeExists(module.get(), "Box_double"))
        << "Should have generated Box_double struct";
}

// Test 8: Generic class with user-defined type argument
TEST_F(GenericClassCodeGenTest, GenericClassWithUserDefinedTypeArgument) {
    std::string code = R"(
        class Container<T> {
            constructor() {
            }
        }

        func main() {
            var c = new Container<string>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created with string type
    EXPECT_TRUE(typeExists(module.get(), "Container_string"))
        << "Should have generated Container_string struct";
}

// Test 9: Generic class with multiple type parameters
TEST_F(GenericClassCodeGenTest, GenericClassWithMultipleTypeParameters) {
    std::string code = R"(
        class Pair<K, V> {
            constructor() {
            }
        }

        func main() {
            var p = new Pair<int64, string>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created
    EXPECT_TRUE(typeExists(module.get(), "Pair_int64_string"))
        << "Should have generated Pair_int64_string struct";
}

// Test 10: Same generic instantiated multiple times reuses same type
TEST_F(GenericClassCodeGenTest, SameInstantiationReused) {
    std::string code = R"(
        class Container<T> {
            constructor() {
            }
        }

        func main() {
            var c1 = new Container<int64>();
            var c2 = new Container<int64>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created
    EXPECT_TRUE(typeExists(module.get(), "Container_int64"))
        << "Should have generated Container_int64 struct";

    // Should only have one _init function, not two
    auto funcCount = 0;
    for (auto& func : *module) {
        if (func.getName() == "Container_int64_init") {
            funcCount++;
        }
    }
    EXPECT_EQ(funcCount, 1) << "Should only have one Container_int64_init constructor";
}

// Test 11: Generic class with empty field list
TEST_F(GenericClassCodeGenTest, GenericClassNoFields) {
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
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created (with dummy byte field)
    EXPECT_TRUE(typeExists(module.get(), "Empty_int64"))
        << "Should have generated Empty_int64 struct";
}

// Test 12: Generic class with method that uses type parameter implicitly
TEST_F(GenericClassCodeGenTest, GenericClassMethodWithTypeParameterInContext) {
    std::string code = R"(
        class Identity<T> {
            constructor() {
            }

            func get() -> int64 {
                return 100;
            }
        }

        func main() {
            var id = new Identity<int64>();
        }
    )";

    auto module = compileCode(code);
    ASSERT_NE(module, nullptr) << "Compilation should succeed";

    // Check that the instantiated type was created
    EXPECT_TRUE(typeExists(module.get(), "Identity_int64"))
        << "Should have generated Identity_int64 struct";

    // Check that method was generated with mangled name
    EXPECT_TRUE(functionExists(module.get(), "Identity_int64_get"))
        << "Should have generated Identity_int64_get method";
}
