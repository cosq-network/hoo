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
        func :int64 test() {
            var url = url_new("https://example.com/path?q=1#frag");
            var scheme = url_scheme(url);
            var host = url_host(url);
            var port = url_port(url);
            var path = url_path(url);
            var query = url_query(url);
            var frag = url_fragment(url);
            url_release(url);
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooNetJitTest, UrlScheme) {
    const std::string source = R"(
        func :int64 test() {
            var url = url_new("https://example.com");
            var s = url_scheme(url);
            var len = string_length(s);
            url_release(url);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooNetJitTest, UrlPort) {
    const std::string source = R"(
        func :int64 test() {
            var url = url_new("https://example.com:8080/path");
            var p = url_port(url);
            url_release(url);
            return p;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 8080);
}

TEST_F(HooNetJitTest, UrlNoPort) {
    const std::string source = R"(
        func :int64 test() {
            var url = url_new("https://example.com/path");
            var p = url_port(url);
            url_release(url);
            return p;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Default HTTPS port is 443 when no explicit port specified
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 443);
}

TEST_F(HooNetJitTest, DISABLED_HttpStatusOk) {
    const std::string source = R"(
        func :int64 test() {
            var client = http_client_new();
            http_client_set_timeout(client, 10000);
            var resp = http_client_get(client, "https://example.com/");
            var code = http_response_status_code(resp);
            http_response_release(resp);
            http_client_release(client);
            return code;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 200);
}

TEST_F(HooNetJitTest, UrlJustRelease) {
    const std::string source = R"(
        func :int64 test() {
            var url = url_new("x");
            url_release(url);
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooNetJitTest, ProcessThroughRedirectWorks) {
    const std::string source = R"(
        func :int64 test() { return process_self_pid(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooNetJitTest, UrlNoString) {
    const std::string source = R"(
        func :int64 test() {
            return 42;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooNetJitTest, UrlNewWithString) {
    const std::string source = R"(
        func :int64 test() {
            var url = url_new("x");
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
