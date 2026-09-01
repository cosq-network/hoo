#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/text/hoo_string.h"

using namespace hooc;

class InterpolatedStringTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};

    void SetUp() override {
    }
};

TEST_F(InterpolatedStringTest, IntInterpolation) {
    const std::string source = R"(
        import hoo;
        func :string test() {
            return "Value: ${42}";
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    int64_t result = jit.run("_F_M_test_E_test_s");
    ASSERT_NE(0, result) << jit.getLastError();
    
    HooString s = (HooString)result;
    EXPECT_STREQ("Value: 42", hoo_string_data(s));
    
    hoo_string_release(s);
}

TEST_F(InterpolatedStringTest, SimpleInterpolation) {
    const std::string source = R"(
        import hoo;
        func :string test() {
            var name = "World";
            return "Hello, ${name}!";
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    int64_t result = jit.run("_F_M_test_E_test_s");
    ASSERT_NE(0, result) << jit.getLastError();
    
    HooString s = (HooString)result;
    EXPECT_STREQ("Hello, World!", hoo_string_data(s));
    
    hoo_string_release(s);
}

TEST_F(InterpolatedStringTest, MultipleInterpolation) {
    const std::string source = R"(
        import hoo;
        func :string test() {
            var a = "One";
            var b = "Two";
            return "${a} and ${b}";
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    int64_t result = jit.run("_F_M_test_E_test_s");
    ASSERT_NE(0, result);
    
    HooString s = (HooString)result;
    EXPECT_STREQ("One and Two", hoo_string_data(s));
    
    hoo_string_release(s);
}

TEST_F(InterpolatedStringTest, CharInterpolation) {
    const std::string source = R"(
        import hoo;
        func :string test() {
            var c = 'X';
            return "Char: ${c}";
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    int64_t result = jit.run("_F_M_test_E_test_s");
    ASSERT_NE(0, result);
    
    HooString s = (HooString)result;
    EXPECT_STREQ("Char: X", hoo_string_data(s));
    
    hoo_string_release(s);
}

TEST_F(InterpolatedStringTest, VariableInterpolation) {
    const std::string source = R"(
        import hoo;
        func :string test() {
            var x = 123;
            return "Value: ${x}";
        }
    )";

    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    
    int64_t result = jit.run("_F_M_test_E_test_s");
    ASSERT_NE(0, result);
    
    HooString s = (HooString)result;
    EXPECT_STREQ("Value: 123", hoo_string_data(s));
    
    hoo_string_release(s);
}
