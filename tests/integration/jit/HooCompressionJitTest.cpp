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
        import hoo.compression;
        func :int64 test() {
            var c = new Compression();
            var original = "Hello, World!";
            var compressed = c.gzipCompress(original.data(), original.length());
            var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
            c.release();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooCompressionJitTest, DeflateRoundTrip) {
    const std::string source = R"(
        import hoo.compression;
        func :int64 test() {
            var c = new Compression();
            var original = "Hello, World!";
            var compressed = c.deflateCompress(original.data(), original.length());
            var decompressed = c.deflateDecompress(compressed.data(), compressed.length());
            c.release();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}
