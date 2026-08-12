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

// Exercise HttpClient through the JIT. No network is required: the runtime
// serves mock responses for URLs containing "example" (see real_http_request
// in src/runtime/lib/net/hoo_net.cpp).
TEST_F(HooNetJitTest, HttpStatusOk) {
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

TEST_F(HooNetJitTest, SocketTcpSendReceive) {
    const std::string source = R"(
        import hoo.net;
        import hoo.buffer;
        func :int64 test() {
            var server = net_socket_new();
            if (net_socket_bind(server, "127.0.0.1", 0) != 0) { return 1; }
            var port = net_socket_local_port(server);
            if (port <= 0) { return 2; }
            if (net_socket_listen(server, 4) != 0) { return 3; }
            var client = net_socket_new();
            if (net_socket_connect(client, "127.0.0.1", port) != 0) { return 4; }
            var accepted = net_socket_accept(server);
            if (accepted == 0) { return 5; }
            var payload = buffer_fromBytes("ok", 2);
            if (net_socket_send(client, payload) != 2) { return 6; }
            var received = net_socket_receive(accepted, 16);
            if (received == 0) { return 7; }
            if (received.length() != 2) { return 8; }
            if (received.byteAt(0) != 111) { return 9; }
            if (received.byteAt(1) != 107) { return 10; }
            net_socket_release(accepted);
            net_socket_release(client);
            net_socket_release(server);
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooNetJitTest, SocketLastErrorDescribesFailure) {
    const std::string source = R"(
        import hoo.net;
        func :int64 test() {
            var server = net_socket_new();
            if (net_socket_bind(server, "127.0.0.1", 0) != 0) { return 1; }
            var port = net_socket_local_port(server);
            if (port <= 0) { return 2; }
            net_socket_close(server);
            net_socket_release(server);
            var client = net_socket_new();
            net_socket_connect(client, "127.0.0.1", port);
            var err = net_socket_last_error(client);
            if (err.length() <= 0) { return 3; }
            net_socket_release(client);
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto result = jit.run("_F_M_test_E_test_i8");
    if (result == -1) printf("JIT ERROR: %s\n", jit.getLastError().c_str());
    EXPECT_EQ(result, 0);
}
