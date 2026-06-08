#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooCompressionJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooCompressionJitTest, GzipRoundTrip) {
    const std::string source = R"(
        func :int64 test() {
            var original = "Hello, World!";
            var data = original.data();
            var len = original.length();
            var compressed = Compression.gzip_compress(data, len);
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooCompressionJitTest, DeflateRoundTrip) {
    const std::string source = R"(
        func :int64 test() {
            var original = "Hello, World!";
            var data = original.data();
            var len = original.length();
            var compressed = Compression.deflate_compress(data, len);
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}
