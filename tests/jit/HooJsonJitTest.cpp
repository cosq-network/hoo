#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooJsonJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooJsonJitTest, ParseAndStringify) {
    const std::string source = R"(
        func :int64 test() {
            var obj = Json.parse("{\"key\":\"value\"}");
            var type = Json.type(obj);
            Json.release(obj);
            return type;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // JSON_OBJECT = 5
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooJsonJitTest, GetString) {
    const std::string source = R"(
        func :int64 test() {
            var obj = Json.parse("{\"name\":\"Alice\"}");
            var val = Json.get_string(obj, "name");
            var len = val.length();
            Json.release(obj);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooJsonJitTest, GetInt) {
    const std::string source = R"(
        func :int64 test() {
            var obj = Json.parse("{\"age\":30}");
            var age = Json.get_int(obj, "age");
            Json.release(obj);
            return age;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 30);
}

TEST_F(HooJsonJitTest, ArrayAccess) {
    const std::string source = R"(
        func :int64 test() {
            var arr = Json.parse("[10, 20, 30]");
            var len = Json.array_length(arr);
            Json.release(arr);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooJsonJitTest, BuildObject) {
    const std::string source = R"(
        func :int64 test() {
            var obj = Json.new_object();
            var name = Json.new_string("Bob");
            Json.set(obj, "name", name);
            var age = Json.new_int(25);
            Json.set(obj, "age", age);
            var out = Json.stringify(obj);
            var len = out.length();
            Json.release(obj);
            Json.release(name);
            Json.release(age);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_TRUE(jit.run("_F_M_test_E_test_i8") > 0);
}

TEST_F(HooJsonJitTest, BuildArray) {
    const std::string source = R"(
        func :int64 test() {
            var arr = Json.new_array();
            var one = Json.new_int(1);
            var two = Json.new_int(2);
            Json.array_push(arr, one);
            Json.array_push(arr, two);
            var len = Json.array_length(arr);
            Json.release(arr);
            Json.release(one);
            Json.release(two);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooJsonJitTest, NullAndBool) {
    const std::string source = R"(
        func :int64 test() {
            var n = Json.new_null();
            var t = Json.new_bool(1);
            var f = Json.new_bool(0);
            var nt = Json.type(n);
            var tt = Json.type(t);
            var ft = Json.type(f);
            Json.release(n);
            Json.release(t);
            Json.release(f);
            return nt;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // JSON_NULL = 0
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooJsonJitTest, NestedObject) {
    const std::string source = R"(
        func :int64 test() {
            var obj = Json.parse("{\"user\":{\"name\":\"Alice\",\"scores\":[95,87,92]}}");
            var user = Json.get(obj, "user");
            var name = Json.get_string(user, "name");
            var len = name.length();
            Json.release(obj);
            Json.release(user);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}
