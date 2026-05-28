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
            var obj = json_parse("{\"key\":\"value\"}");
            var type = json_type(obj);
            json_release(obj);
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
            var obj = json_parse("{\"name\":\"Alice\"}");
            var val = json_get_string(obj, "name");
            var len = string_length(val);
            json_release(obj);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooJsonJitTest, GetInt) {
    const std::string source = R"(
        func :int64 test() {
            var obj = json_parse("{\"age\":30}");
            var age = json_get_int(obj, "age");
            json_release(obj);
            return age;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 30);
}

TEST_F(HooJsonJitTest, ArrayAccess) {
    const std::string source = R"(
        func :int64 test() {
            var arr = json_parse("[10, 20, 30]");
            var len = json_array_length(arr);
            json_release(arr);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooJsonJitTest, BuildObject) {
    const std::string source = R"(
        func :int64 test() {
            var obj = json_new_object();
            var name = json_new_string("Bob");
            json_set(obj, "name", name);
            var age = json_new_int(25);
            json_set(obj, "age", age);
            var out = json_stringify(obj);
            var len = string_length(out);
            json_release(obj);
            json_release(name);
            json_release(age);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_TRUE(jit.run("_F_M_test_E_test_i8") > 0);
}

TEST_F(HooJsonJitTest, BuildArray) {
    const std::string source = R"(
        func :int64 test() {
            var arr = json_new_array();
            var one = json_new_int(1);
            var two = json_new_int(2);
            json_array_push(arr, one);
            json_array_push(arr, two);
            var len = json_array_length(arr);
            json_release(arr);
            json_release(one);
            json_release(two);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooJsonJitTest, NullAndBool) {
    const std::string source = R"(
        func :int64 test() {
            var n = json_new_null();
            var t = json_new_bool(1);
            var f = json_new_bool(0);
            var nt = json_type(n);
            var tt = json_type(t);
            var ft = json_type(f);
            json_release(n);
            json_release(t);
            json_release(f);
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
            var obj = json_parse("{\"user\":{\"name\":\"Alice\",\"scores\":[95,87,92]}}");
            var user = json_get(obj, "user");
            var name = json_get_string(user, "name");
            var len = string_length(name);
            json_release(obj);
            json_release(user);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}
