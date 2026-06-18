#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/hoo_map.h"
#include "runtime/lib/hoo_string.h"
#include <cstring>

using namespace hooc;

class HooMapJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

// === Int64 key, Int64 value ===

TEST_F(HooMapJitTest, NewMap) {
    const std::string source = R"(
        func :int64 test() { return new Map(2, 1); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooMap m = (HooMap)r;
    EXPECT_EQ(0, hoo_map_count(m));
    EXPECT_EQ(HOO_MAP_KEY_INT64, hoo_map_key_type(m));
    EXPECT_EQ(HOO_MAP_VAL_INT64, hoo_map_value_type(m));
    hoo_map_release(m);
}

TEST_F(HooMapJitTest, SetGetInt64Int64) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 1);
            m.setInt64Int64(42, 100);
            return m.getInt64Int64(42);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 100);
}

TEST_F(HooMapJitTest, MapLength) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(4, 1);
            m.setStringInt64("a", 1);
            m.setStringInt64("b", 2);
            return m.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooMapJitTest, ContainsKey) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            return m.containsInt64(1);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMapJitTest, NotContainsKey) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            return m.containsInt64(2);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooMapJitTest, RemoveKey) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            m.setInt64Int64(2, 20);
            m.removeInt64(1);
            return m.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMapJitTest, Clear) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 1);
            m.setInt64Int64(1, 10);
            m.clear();
            return m.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooMapJitTest, Empty) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 1);
            return m.empty();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

// === Int64 key, Double value ===

TEST_F(HooMapJitTest, SetGetInt64Double) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 2);
            m.setInt64Double(10, 3.14);
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMapJitTest, GetInt64DoubleValue) {
    const std::string source = R"(
        func :double test() {
            var m = new Map(2, 2);
            m.setInt64Double(10, 3.14);
            return m.getInt64Double(10);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto raw = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &raw, sizeof(double));
    EXPECT_DOUBLE_EQ(val, 3.14);
}

// === Int64 key, String value ===

TEST_F(HooMapJitTest, SetGetInt64String) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 4);
            m.setInt64String(1, "hello");
            var s = m.getInt64String(1);
            return s.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooMapJitTest, GetInt64StringContent) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 4);
            m.setInt64String(1, "world");
            var s = m.getInt64String(1);
            return s.equals("world");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

// === Int64 key, Bool value ===

TEST_F(HooMapJitTest, SetGetInt64Bool) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 3);
            m.setInt64Bool(1, 1);
            return m.getInt64Bool(1);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMapJitTest, GetInt64BoolFalse) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 3);
            m.setInt64Bool(1, 0);
            return m.getInt64Bool(1);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

// === String key, Double value ===

TEST_F(HooMapJitTest, SetGetStringDouble) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(4, 2);
            m.setStringDouble("pi", 3.14159);
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

// === String key, String value ===

TEST_F(HooMapJitTest, SetGetStringString) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(4, 4);
            m.setStringString("greeting", "hello");
            var s = m.getStringString("greeting");
            return s.equals("hello");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

// === String key, Bool value ===

TEST_F(HooMapJitTest, SetGetStringBool) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(4, 3);
            m.setStringBool("flag", 1);
            return m.getStringBool("flag");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

// === Int8 key, Int64 value ===

TEST_F(HooMapJitTest, SetGetInt8Int64) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(1, 1);
            m.setInt8Int64(7, 77);
            return m.getInt8Int64(7);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 77);
}

// === Key type and value type queries ===

TEST_F(HooMapJitTest, KeyTypeQuery) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2, 1);
            return m.keyType();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), HOO_MAP_KEY_INT64);
}

TEST_F(HooMapJitTest, ValueTypeQuery) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(4, 4);
            return m.valueType();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), HOO_MAP_VAL_STRING);
}

TEST_F(HooMapJitTest, ValueTypeDefault) {
    const std::string source = R"(
        func :int64 test() {
            var m = new Map(2);
            return m.valueType();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), HOO_MAP_VAL_ANY);
}
