#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooNetJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooNetJitTest, UrlNew) {
    const std::string source = R"(
        import hoo.net;
        func :int64 test() {
            var url = new URL("https://example.com/path?q=1#frag");
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooNetJitTest, UrlScheme) {
    const std::string source = R"(
        import hoo.net;
        func :int64 test() {
            var url = new URL("https://example.com");
            var s = url.getScheme();
            var len = s.length();
            url.release();
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooNetJitTest, UrlPort) {
    const std::string source = R"(
        import hoo.net;
        func :int64 test() {
            var url = new URL("https://example.com:8080/path");
            var p = url.getPort();
            url.release();
            return p;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 8080);
}

TEST_F(HooNetJitTest, UrlNoPort) {
    const std::string source = R"(
        import hoo.net;
        func :int64 test() {
            var url = new URL("https://example.com/path");
            var p = url.getPort();
            url.release();
            return p;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Default HTTPS port is 443 when no explicit port specified
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 443);
}

TEST_F(HooNetJitTest, DISABLED_HttpStatusOk) {
    const std::string source = R"(
        import hoo.net;
        func :int64 test() {
            var client = new HttpClient();
            client.setTimeout(10000);
            var resp = client.get("https://example.com/");
            var code = resp.statusCode();
            resp.release();
            client.release();
            return code;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 200);
}

TEST_F(HooNetJitTest, UrlJustRelease) {
    const std::string source = R"(
        import hoo.net;
        func :int64 test() {
            var url = new URL("x");
            url.release();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooNetJitTest, ProcessThroughRedirectWorks) {
    const std::string source = R"(
        import hoo.process;
        func :int64 test() { return process_self_pid(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooNetJitTest, UrlNoString) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            return 42;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooNetJitTest, UrlNewWithString) {
    const std::string source = R"(
        import hoo.net;
        func :int64 test() {
            var url = new URL("x");
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
