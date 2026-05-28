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
        func :int64 test() { return map_new(1); }
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
            var m = map_new(1);
            map_set_int64_int64(m, 42, 100);
            return map_get_int64_int64(m, 42);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 100);
}

TEST_F(HooMapJitTest, MapLength) {
    const std::string source = R"(
        func :int64 test() {
            var m = map_new(4);
            map_set_string_int64(m, "a", 1);
            map_set_string_int64(m, "b", 2);
            return map_length(m);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooMapJitTest, ContainsKey) {
    const std::string source = R"(
        func :int64 test() {
            var m = map_new(1);
            map_set_int64_int64(m, 1, 10);
            return map_contains_int64(m, 1);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMapJitTest, NotContainsKey) {
    const std::string source = R"(
        func :int64 test() {
            var m = map_new(1);
            map_set_int64_int64(m, 1, 10);
            return map_contains_int64(m, 2);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooMapJitTest, RemoveKey) {
    const std::string source = R"(
        func :int64 test() {
            var m = map_new(2);
            map_set_int64_int64(m, 1, 10);
            map_set_int64_int64(m, 2, 20);
            map_remove_int64(m, 1);
            return map_length(m);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMapJitTest, Clear) {
    const std::string source = R"(
        func :int64 test() {
            var m = map_new(1);
            map_set_int64_int64(m, 1, 10);
            map_clear(m);
            return map_length(m);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooMapJitTest, Empty) {
    const std::string source = R"(
        func :int64 test() {
            var m = map_new(1);
            return map_empty(m);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
