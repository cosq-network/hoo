#include <gtest/gtest.h>

#include "core/DefaultIOProvider.h"
#include "hvm/HVMJIT.h"

using namespace hooc;

class HooAnyCollectionsJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooAnyCollectionsJitTest, ListLiteralLength) {
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

TEST_F(HooAnyCollectionsJitTest, ListPushSetAndLength) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new List();
            values.push(5);
            values.push(8);
            values[0] = 7;
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooAnyCollectionsJitTest, ListCapacityConstructorAndLength) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new List();
            values.push(31);
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, DictFixedSubscriptSetGet) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            return m[10] + m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 44);
}

TEST_F(HooAnyCollectionsJitTest, DictAnySubscriptSetAndCount) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<byte, any> = new Dict<byte, any>();
            m[1] = 100;
            m[2] = 5;
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooAnyCollectionsJitTest, DictRemove) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            m.remove(10);
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, DictClear) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            m.clear();
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooAnyCollectionsJitTest, DictFixedValuesReadBack) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            m[10] = 42;
            m[11] = 8;
            return m[10] + m[11];
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 50);
}

TEST_F(HooAnyCollectionsJitTest, DictInt64StringSetGet) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int64, string> = new Dict<int64, string>();
            m[1] = "hello";
            m[2] = "world";
            var s = m[1];
            return s.equals("hello");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, DictInt64StringCount) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int64, string> = new Dict<int64, string>();
            m[1] = "hello";
            m[2] = "world";
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooAnyCollectionsJitTest, DictAnyMixedTypes) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            m[1] = 42;
            m[2] = "hello";
            m[3] = 3.14;
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooAnyCollectionsJitTest, DictAnyOverwriteValue) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            m[1] = 42;
            m[1] = 99;
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, ListSetAndGet) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new List();
            values.push(10);
            values.push(20);
            values[0] = 99;
            return values[0];
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 99);
}

TEST_F(HooAnyCollectionsJitTest, ListPopAndLength) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new List();
            values.push(10);
            values.push(20);
            var popped = values.pop();
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooAnyCollectionsJitTest, ListMixedTypes) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = new List();
            values.push(42);
            values.push("hello");
            values.push(3.14);
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooAnyCollectionsJitTest, ListLiteralIndexRead) {
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

TEST_F(HooAnyCollectionsJitTest, ListHomogeneousStringElementChainedLength) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = ["ab", "cdef"]any;
            return values[0].length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooAnyCollectionsJitTest, ListHomogeneousIntElementInfersScalar) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = [10, 20, 30]any;
            var v = values[1];
            return v;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 20);
}

TEST_F(HooAnyCollectionsJitTest, ListMixedElementsStayDynamic) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = ["ab", 42]any;
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooAnyCollectionsJitTest, ListEmptyLiteralStaysDynamic) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var values = []any;
            values.push("x");
            return values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
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

TEST_F(HooAnyCollectionsJitTest, DictInt8KeyAnyValue) {
    const std::string source = R"(
        import hoo.collections;
        func :int64 test() {
            var m: Dict<int8, any> = new Dict<int8, any>();
            m[1] = 42;
            m[2] = 99;
            return m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}
