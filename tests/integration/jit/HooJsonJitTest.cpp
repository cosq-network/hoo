#include <gtest/gtest.h>

#include "core/DefaultIOProvider.h"
#include "hvm/HVMJIT.h"

using namespace hooc;

class HooJsonJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooJsonJitTest, SerializeHashMapFreeFunction) {
    const std::string source = R"(
        import hoo.collections;
        import hoo.json;
        func :int64 test() {
            var m: HashMap<int64, int64> = new HashMap<int64, int64>();
            m[1] = 42;
            var json = json_serialize_hashmap(m);
            return json.contains("\"1\":42");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooJsonJitTest, SerializeAnyArrayFreeFunction) {
    const std::string source = R"(
        import hoo.collections;
        import hoo.json;
        func :int64 test() {
            var values = [1, 2, 3]any;
            var json = json_serialize_anyarray(values);
            return json.equals("[1,2,3]");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooJsonJitTest, MinifyFreeFunction) {
    const std::string source = R"(
        import hoo.json;
        func :int64 test() {
            var json = json_minify("{ \"a\" : [ 1, true ] }");
            return json.equals("{\"a\":[1,true]}");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooJsonJitTest, DeserializeHashMapFreeFunction) {
    const std::string source = R"(
        import hoo.collections;
        import hoo.json;
        func :int64 test() {
            var m = json_deserialize_hashmap("{\"1\":42,\"2\":[7]}");
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooJsonJitTest, DeserializeAnyArrayFreeFunction) {
    const std::string source = R"(
        import hoo.collections;
        import hoo.json;
        func :int64 test() {
            var values = json_deserialize_anyarray("[1,\"two\",true]");
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooJsonJitTest, BeautifyFreeFunction) {
    const std::string source = R"(
        import hoo.json;
        func :int64 test() {
            var json = json_beautify("{\"a\":1}");
            return json.contains("\n  \"a\": 1");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooJsonJitTest, ClassStyleJsonApiIsNotSupported) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var obj = Json.parse("{\"key\":\"value\"}");
            return 0;
        }
    )";
    EXPECT_FALSE(jit.loadSourceCode("test", source));
}
