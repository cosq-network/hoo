#include <gtest/gtest.h>

#include "core/DefaultIOProvider.h"
#include "hvm/HVMJIT.h"

using namespace hooc;

class HooAnyCollectionsJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooAnyCollectionsJitTest, AnyArrayLiteralLength) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = [10, 20, 30]any;
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooAnyCollectionsJitTest, AnyArrayPushSetAndLength) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new AnyArray();
            values.push(5);
            values.push(8);
            values[0] = 7;
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooAnyCollectionsJitTest, AnyArrayCapacityConstructorAndLength) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new AnyArray();
            values.push(31);
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, HashMapFixedSubscriptSetGet) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int64, int64> = new HashMap<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            return m[10] + m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 44);
}

TEST_F(HooAnyCollectionsJitTest, HashMapAnySubscriptSetAndCount) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<byte, any> = new HashMap<byte, any>();
            m[1] = 100;
            m[2] = 5;
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooAnyCollectionsJitTest, HashMapRemove) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int64, int64> = new HashMap<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            m.remove(10);
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, HashMapClear) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int64, int64> = new HashMap<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            m.clear();
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooAnyCollectionsJitTest, HashMapFixedValuesReadBack) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int64, int64> = new HashMap<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            return m[10] + m[11];
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 50);
}

TEST_F(HooAnyCollectionsJitTest, HashMapInt64StringSetGet) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int64, string> = new HashMap<int64, string>();
            m[1] = "hello";
            m[2] = "world";
            var s = m[1];
            return s.equals("hello");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, HashMapInt64StringCount) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int64, string> = new HashMap<int64, string>();
            m[1] = "hello";
            m[2] = "world";
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooAnyCollectionsJitTest, HashMapAnyMixedTypes) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int64, any> = new HashMap<int64, any>();
            m[1] = 42;
            m[2] = "hello";
            m[3] = 3.14;
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooAnyCollectionsJitTest, HashMapAnyOverwriteValue) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int64, any> = new HashMap<int64, any>();
            m[1] = 42;
            m[1] = 99;
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, AnyArraySetAndGet) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new AnyArray();
            values.push(10);
            values.push(20);
            values[0] = 99;
            return values[0];
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 99);
}

TEST_F(HooAnyCollectionsJitTest, AnyArrayPopAndLength) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new AnyArray();
            values.push(10);
            values.push(20);
            var popped = values.pop();
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, AnyArrayMixedTypes) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new AnyArray();
            values.push(42);
            values.push("hello");
            values.push(3.14);
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooAnyCollectionsJitTest, AnyArrayLiteralIndexRead) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = [10, 20, 30]any;
            return values[1];
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 20);
}

TEST_F(HooAnyCollectionsJitTest, AnyFunctionReturnAndUse) {
    const std::string source = R"(
        import hoo;
        func:any getValue() {
            return 42;
        }
        func :int64 test() {
            var v = getValue();
            return v;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooAnyCollectionsJitTest, AnyFunctionReturnAndCall) {
    // Calling a func:any and using the return value directly should work
    const std::string source = R"(
        import hoo;
        func:any getValue() {
            return 42;
        }
        func :int64 test() {
            return getValue();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooAnyCollectionsJitTest, HashMapInt8KeyAnyValue) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: HashMap<int8, any> = new HashMap<int8, any>();
            m[1] = 42;
            m[2] = 99;
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}
