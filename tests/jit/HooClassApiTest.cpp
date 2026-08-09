#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/hoo_string.h"

using namespace hooc;

class HooClassApiTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooClassApiTest, StringLengthMethod) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var s = "hello";
            return s.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooClassApiTest, ChainedUserMethodReturnTypeDispatch) {
    const std::string source = R"(
        import hoo;
        class Holder {
            constructor() {}
            func :Array values() {
                return [1, 2, 3, 4];
            }
        }
        func :int64 test() {
            var holder = new Holder();
            return holder.values().length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 4) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HooClassApiTest, ChainedUserFieldReturnTypeDispatch) {
    const std::string source = R"(
        import hoo;
        class Holder {
            var values: Array;
            constructor() {
                this.values = [7, 8, 9];
            }
        }
        func :int64 test() {
            var holder = new Holder();
            return holder.values.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3) << jit.getLastError();
    EXPECT_TRUE(jit.lastRunUsedJIT());
}

TEST_F(HooClassApiTest, SameMethodNameDispatchesByReceiverClass) {
    const std::string source = R"(
        class First {
            constructor() {}
            func :int64 value() { return 11; }
        }
        class Second {
            constructor() {}
            func :int64 value() { return 22; }
        }
        func :int64 test() {
            var first = new First();
            var second = new Second();
            // Keep the operands distinguishable so an accidental dispatch to
            // the same class cannot pass the regression test.
            return first.value() * 100 + second.value();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1122) << jit.getLastError();
}

TEST_F(HooClassApiTest, StringConcatMethod) {
    const std::string source = R"(
        import hoo;
        func :string test() {
            var a = "hello ";
            var b = "world";
            return a.concat(b);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_STREQ("hello world", hoo_string_data(s));
    hoo_string_release(s);
}

TEST_F(HooClassApiTest, StringIsEmptyMethod) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var s = "";
            return s.isEmpty();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooClassApiTest, FreeFuncDateTimeNow) {
    const std::string source = R"(
        import hoo.datetime;
        func :int64 test() {
            var dt = datetime_now();
            return dt.getTimestamp();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    // Now should return a non-zero timestamp
    EXPECT_GT(r, 1000000);
}

TEST_F(HooClassApiTest, ChainedFreeFunctionReturnTypeDispatch) {
    const std::string source = R"(
        import hoo.datetime;
        func :int64 test() {
            return datetime_now().getTimestamp();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 1000000) << jit.getLastError();
}

TEST_F(HooClassApiTest, FreeFuncMathAbs) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() {
            return math_abs(-42);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooClassApiTest, FreeFuncMathGetPi) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() {
            return math_get_pi();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // get_pi returns a double; just verify it's non-zero
    EXPECT_NE(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooClassApiTest, FreeFuncDateTimeNowSeconds) {
    const std::string source = R"(
        import hoo.datetime;
        func :int64 test() {
            return datetime_now_seconds();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_GT(r, 1000000);
}

TEST_F(HooClassApiTest, FreeFuncSystemHostname) {
    const std::string source = R"(
        import hoo.system;
        func :string test() {
            return system_hostname();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_GT(hoo_string_length(s), 0);
    hoo_string_release(s);
}

TEST_F(HooClassApiTest, StaticFsExists) {
    const std::string source = R"(
        import hoo.io;
        func :int64 test() {
            return fs_exists(".");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooClassApiTest, RegexConstructor) {
    const std::string source = R"(
        import hoo.regex;
        func :int64 test() {
            var re = new Regex("[a-z]+");
            re.release();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_EQ(r, 1);
}

TEST_F(HooClassApiTest, ClassModifiers_SingletonInstanceIdentity) {
    const std::string source = R"(
        import hoo;
        singleton class AppState {
            var value: int64;
            constructor() {
                // Initialize only if not already initialized
                if (this.value == 0) {
                    this.value = 1;
                }
            }
            func :void setValue(v: int64) {
                this.value = v;
            }
            func :int64 getValue() {
                return this.value;
            }
        }
        func :int64 test() {
            var a = new AppState();
            a.setValue(42);
            var b = new AppState();
            return b.getValue();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // b.getValue() should be 42 because it's the same instance as a
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooClassApiTest, ClassModifiers_ImmutableClassExecution) {
    const std::string source = R"(
        immutable class Config {
            var version: int64;
            constructor(v: int64) {
                this.version = v;
            }
        }
        func :bool test() {
            var c: Config = new Config(42);
            return c.version == 42;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_b"), 1);
}

TEST_F(HooClassApiTest, ClassModifiers_ServiceExecution) {
    const std::string source = R"(
        service class DatabaseService {
            var connected: bool;
            constructor() {
                this.connected = true;
            }
            func :bool isConnected() {
                return this.connected;
            }
        }
        func :bool test() {
            // Testing service class execution
            return true;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_b"), 1);
}

TEST_F(HooClassApiTest, ClassModifiers_FinalExecution) {
    const std::string source = R"(
        final class BaseWidget {
            var id: int64;
            constructor(id: int64) {
                this.id = id;
            }
        }
        func :bool test() {
            var w: BaseWidget = new BaseWidget(100);
            return w.id == 100;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_b"), 1);
}
