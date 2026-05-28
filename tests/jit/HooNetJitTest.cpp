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
            var url = net_url_new("https://example.com/path?q=1#frag");
            var scheme = net_url_get_scheme(url);
            var host = net_url_get_host(url);
            var port = net_url_get_port(url);
            var path = net_url_get_path(url);
            var query = net_url_get_query(url);
            var frag = net_url_get_fragment(url);
            net_url_release(url);
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooNetJitTest, UrlScheme) {
    const std::string source = R"(
        func :int64 test() {
            var url = net_url_new("https://example.com");
            var s = net_url_get_scheme(url);
            var len = string_length(s);
            net_url_release(url);
            return len;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooNetJitTest, UrlPort) {
    const std::string source = R"(
        func :int64 test() {
            var url = net_url_new("https://example.com:8080/path");
            var p = net_url_get_port(url);
            net_url_release(url);
            return p;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 8080);
}

TEST_F(HooNetJitTest, UrlNoPort) {
    const std::string source = R"(
        func :int64 test() {
            var url = net_url_new("https://example.com/path");
            var p = net_url_get_port(url);
            net_url_release(url);
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
            var client = net_http_client_new();
            net_http_client_set_timeout(client, 10000);
            var resp = net_http_client_get(client, "https://example.com/");
            var code = net_http_response_get_status_code(resp);
            net_http_response_release(resp);
            net_http_client_release(client);
            return code;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 200);
}
