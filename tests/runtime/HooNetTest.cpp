#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include "runtime/lib/hoo_net.h"

class HooNetTest : public ::testing::Test {
};

TEST_F(HooNetTest, UrlNewAndParse) {
    HooURL url = hoo_net_url_new("https://example.com:8080/path/to/page?query=val#frag");
    ASSERT_NE(url, nullptr);

    char* scheme = hoo_net_url_get_scheme(url);
    ASSERT_NE(scheme, nullptr);
    EXPECT_STREQ(scheme, "https");
    free(scheme);

    char* host = hoo_net_url_get_host(url);
    ASSERT_NE(host, nullptr);
    EXPECT_STREQ(host, "example.com");
    free(host);

    EXPECT_EQ(hoo_net_url_get_port(url), 8080);

    char* path = hoo_net_url_get_path(url);
    ASSERT_NE(path, nullptr);
    EXPECT_STREQ(path, "/path/to/page");
    free(path);

    char* qs = hoo_net_url_get_query(url);
    ASSERT_NE(qs, nullptr);
    EXPECT_STREQ(qs, "query=val");
    free(qs);

    char* frag = hoo_net_url_get_fragment(url);
    ASSERT_NE(frag, nullptr);
    EXPECT_STREQ(frag, "frag");
    free(frag);

    hoo_net_url_release(url);
}

TEST_F(HooNetTest, UrlToString) {
    HooURL url = hoo_net_url_new("http://example.com/path");
    ASSERT_NE(url, nullptr);

    char* str = hoo_net_url_to_string(url);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "http://example.com/path");
    free(str);

    hoo_net_url_release(url);
}

TEST_F(HooNetTest, UrlDefaultPorts) {
    HooURL http_url = hoo_net_url_new("http://example.com");
    ASSERT_NE(http_url, nullptr);
    EXPECT_EQ(hoo_net_url_get_port(http_url), 80);
    hoo_net_url_release(http_url);

    HooURL https_url = hoo_net_url_new("https://example.com");
    ASSERT_NE(https_url, nullptr);
    EXPECT_EQ(hoo_net_url_get_port(https_url), 443);
    hoo_net_url_release(https_url);
}

TEST_F(HooNetTest, UrlNoQueryOrFragment) {
    HooURL url = hoo_net_url_new("http://example.com/path");
    ASSERT_NE(url, nullptr);

    char* qs = hoo_net_url_get_query(url);
    EXPECT_EQ(qs, nullptr);

    char* frag = hoo_net_url_get_fragment(url);
    EXPECT_EQ(frag, nullptr);

    hoo_net_url_release(url);
}

TEST_F(HooNetTest, UrlRetainRelease) {
    HooURL url = hoo_net_url_new("http://example.com");
    ASSERT_NE(url, nullptr);

    HooURL retained = hoo_net_url_retain(url);
    EXPECT_EQ(retained, url);

    hoo_net_url_release(url);
    hoo_net_url_release(url); // Should not crash
}

TEST_F(HooNetTest, HttpClientCreateAndConfigure) {
    HooHttpClient client = hoo_net_http_client_new();
    ASSERT_NE(client, nullptr);

    int64_t ret = hoo_net_http_client_set_header(client, "User-Agent", "hoo-test/1.0");
    EXPECT_EQ(ret, 1);

    hoo_net_http_client_set_timeout(client, 5000);

    // Make a GET request to a realistic URL pattern
    HooHttpResponse resp = hoo_net_http_client_get(client, "http://success.example/api");
    ASSERT_NE(resp, nullptr);

    int64_t code = hoo_net_http_response_get_status_code(resp);
    EXPECT_EQ(code, 200);

    EXPECT_TRUE(hoo_net_http_response_is_success(resp));

    char* body = hoo_net_http_response_get_body(resp);
    ASSERT_NE(body, nullptr);
    EXPECT_GT(strlen(body), 0);
    free(body);

    hoo_net_http_response_release(resp);
    hoo_net_http_client_release(client);
}

TEST_F(HooNetTest, HttpErrors) {
    HooHttpClient client = hoo_net_http_client_new();
    ASSERT_NE(client, nullptr);

    // Test 404 response
    HooHttpResponse resp = hoo_net_http_client_get(client, "http://notfound.example/404");
    ASSERT_NE(resp, nullptr);

    EXPECT_EQ(hoo_net_http_response_get_status_code(resp), 404);
    EXPECT_FALSE(hoo_net_http_response_is_success(resp));

    char* body = hoo_net_http_response_get_body(resp);
    ASSERT_NE(body, nullptr);
    EXPECT_GT(strlen(body), 0);
    free(body);

    hoo_net_http_response_release(resp);
    hoo_net_http_client_release(client);
}

TEST_F(HooNetTest, HttpPostAndPut) {
    HooHttpClient client = hoo_net_http_client_new();
    ASSERT_NE(client, nullptr);

    HooHttpResponse resp = hoo_net_http_client_post(client, "http://created.example/api", "{\"key\":\"val\"}");
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(hoo_net_http_response_get_status_code(resp), 201);
    hoo_net_http_response_release(resp);

    resp = hoo_net_http_client_put(client, "http://success.example/api/1", "{\"key\":\"updated\"}");
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(hoo_net_http_response_get_status_code(resp), 200);
    hoo_net_http_response_release(resp);

    hoo_net_http_client_release(client);
}

TEST_F(HooNetTest, HttpDelete) {
    HooHttpClient client = hoo_net_http_client_new();
    ASSERT_NE(client, nullptr);

    HooHttpResponse resp = hoo_net_http_client_delete(client, "http://success.example/api/1");
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(hoo_net_http_response_get_status_code(resp), 200);

    hoo_net_http_response_release(resp);
    hoo_net_http_client_release(client);
}

TEST_F(HooNetTest, NullHandling) {
    // hoo_net_url_new(nullptr) returns an empty URL, not nullptr
    HooURL null_url = hoo_net_url_new(nullptr);
    ASSERT_NE(null_url, nullptr);
    EXPECT_STREQ(hoo_net_url_get_scheme(null_url), "");
    hoo_net_url_release(null_url);
    EXPECT_STREQ(hoo_net_url_get_scheme(nullptr), "");
    EXPECT_STREQ(hoo_net_url_get_host(nullptr), "");
    EXPECT_EQ(hoo_net_url_get_port(nullptr), -1);
    EXPECT_STREQ(hoo_net_url_get_path(nullptr), "/");
    EXPECT_EQ(hoo_net_url_get_query(nullptr), nullptr);
    EXPECT_EQ(hoo_net_url_get_fragment(nullptr), nullptr);
    EXPECT_EQ(hoo_net_http_response_get_status_code(nullptr), 0);
    EXPECT_EQ(hoo_net_http_response_is_success(nullptr), 0);
}
