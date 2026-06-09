#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooStandardLibraryJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooStandardLibraryJitTest, SystemHostname) {
    const std::string source = R"(
        func:int64 test() {
            var name = System.hostname();
            return name.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Assuming hostname length is > 0
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooStandardLibraryJitTest, FsExists) {
    const std::string source = R"(
        func:int64 test() {
            // Check if current directory exists, should be true (1)
            return Fs.exists(".");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooStandardLibraryJitTest, RegexCompile) {
    const std::string source = R"(
        func:int64 test() {
            var re = Regex.compile("[a-z]+");
            var result = Regex.match(re, "hello");
            return result;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooStandardLibraryJitTest, UuidV4) {
    const std::string source = R"(
        func:int64 test() {
            var id = Uuid.v4();
            var str = Uuid.to_string(id);
            return str.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 36); // UUID is 36 chars long
}

TEST_F(HooStandardLibraryJitTest, EncodingBase64) {
    const std::string source = R"(
        func:int64 test() {
            var str = "Hello";
            var bytes = str.data();
            var len = str.length();
            var b64 = Encoding.base64_encode(bytes, len);
            return b64.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // "Hello" is 5 bytes -> base64 is 8 bytes
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 8);
}
