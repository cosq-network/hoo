#include <gtest/gtest.h>
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/overload/hoo_overload.h"
#include <string>

class HooOverloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        hoo_overload_init();
    }
    
    void TearDown() override {
        hoo_overload_shutdown();
    }
};

TEST_F(HooOverloadTest, MathAbsOverload) {
    int64_t args_int64[] = { HOO_TYPE_INT64 };
    const char* sym_int64 = hoo_resolve_overload("abs", "Math", HOO_OVERLOAD_STATIC_METHOD, args_int64, 1);
    EXPECT_NE(sym_int64, nullptr);
    EXPECT_STREQ(sym_int64, "hoo_math_abs_int64");

    int64_t args_double[] = { HOO_TYPE_FLOAT64 };
    const char* sym_double = hoo_resolve_overload("abs", "Math", HOO_OVERLOAD_STATIC_METHOD, args_double, 1);
    EXPECT_NE(sym_double, nullptr);
    EXPECT_STREQ(sym_double, "hoo_math_abs_double");
    
    // Widening from int8 -> int64
    int64_t args_int8[] = { HOO_TYPE_INT8 };
    const char* sym_int8 = hoo_resolve_overload("abs", "Math", HOO_OVERLOAD_STATIC_METHOD, args_int8, 1);
    EXPECT_NE(sym_int8, nullptr);
    EXPECT_STREQ(sym_int8, "hoo_math_abs_int8");
}

TEST_F(HooOverloadTest, AmbiguityAndNoMatch) {
    // Math.min with a string should fail to match
    int64_t args_bad[] = { HOO_TYPE_STRING, HOO_TYPE_STRING };
    const char* sym = hoo_resolve_overload("min", "Math", HOO_OVERLOAD_STATIC_METHOD, args_bad, 2);
    EXPECT_EQ(sym, nullptr);
}

TEST_F(HooOverloadTest, StringFromAnyFallback) {
    int64_t args_array[] = { HOO_TYPE_ARRAY };
    const char* sym = hoo_resolve_overload("from", "String", HOO_OVERLOAD_STATIC_METHOD, args_array, 1);
    EXPECT_NE(sym, nullptr);
    EXPECT_STREQ(sym, "hoo_string_from_any");
}

TEST_F(HooOverloadTest, RegexCompileArity) {
    int64_t args_1[] = { HOO_TYPE_STRING };
    const char* sym_1 = hoo_resolve_overload("compile", "Regex", HOO_OVERLOAD_STATIC_METHOD, args_1, 1);
    EXPECT_NE(sym_1, nullptr);
    EXPECT_STREQ(sym_1, "hoo_regex_compile");

    int64_t args_2[] = { HOO_TYPE_STRING, HOO_TYPE_STRING };
    const char* sym_2 = hoo_resolve_overload("compile", "Regex", HOO_OVERLOAD_STATIC_METHOD, args_2, 2);
    EXPECT_NE(sym_2, nullptr);
    EXPECT_STREQ(sym_2, "hoo_regex_compile_with_flags");
}

TEST_F(HooOverloadTest, BufferConstructor) {
    const char* sym_empty = hoo_resolve_overload("new", "Buffer", HOO_OVERLOAD_CONSTRUCTOR, nullptr, 0);
    EXPECT_NE(sym_empty, nullptr);
    EXPECT_STREQ(sym_empty, "hoo_buffer_new_empty");

    int64_t args_1[] = { HOO_TYPE_INT64 };
    const char* sym_cap = hoo_resolve_overload("new", "Buffer", HOO_OVERLOAD_CONSTRUCTOR, args_1, 1);
    EXPECT_NE(sym_cap, nullptr);
    EXPECT_STREQ(sym_cap, "hoo_buffer_new");
}
