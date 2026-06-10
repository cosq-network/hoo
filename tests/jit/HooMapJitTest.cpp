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

TEST_F(HooMapJitTest, NewMap) {
    const std::string source = R"(
        func :int64 test() { return Map.new(1); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooMap m = (HooMap)r;
    EXPECT_EQ(0, hoo_map_length(m));
    hoo_map_release(m);
}

TEST_F(HooMapJitTest, SetGetInt64) {
    const std::string source = R"(
        func :int64 test() {
            var m = Map.new(1);
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
            var m = Map.new(4);
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
            var m = Map.new(1);
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
            var m = Map.new(1);
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
            var m = Map.new(2);
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
            var m = Map.new(1);
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
            var m = Map.new(1);
            return m.empty();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
