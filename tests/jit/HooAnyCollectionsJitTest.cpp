#include <gtest/gtest.h>

#include "core/DefaultIOProvider.h"
#include "hvm/HVMJIT.h"

using namespace hooc;

class HooAnyCollectionsJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooAnyCollectionsJitTest, AnyArrayLiteralLengthAndSubscript) {
    const std::string source = R"(
        func :int64 test() {
            var values = [10, 20, 30]any;
            return values.length() + values[1];
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 23);
}

TEST_F(HooAnyCollectionsJitTest, AnyArrayPushSetAndSubscript) {
    const std::string source = R"(
        func :int64 test() {
            var values = new AnyArray();
            values.push(5);
            values.push(8);
            values[0] = 7;
            return values[0] + values[1];
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 15);
}

TEST_F(HooAnyCollectionsJitTest, AnyArrayCapacityConstructor) {
    const std::string source = R"(
        func :int64 test() {
            var values = new AnyArray(8);
            values.push(31);
            return values.length() + values[0];
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 32);
}

TEST_F(HooAnyCollectionsJitTest, HashMapFixedSubscriptSetGet) {
    const std::string source = R"(
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

TEST_F(HooAnyCollectionsJitTest, HashMapAnySubscriptSetGet) {
    const std::string source = R"(
        func :int64 test() {
            var m: HashMap<byte, any> = new HashMap<byte, any>();
            m[1] = 100;
            m[2] = 5;
            return m[1] + m.count();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 102);
}
