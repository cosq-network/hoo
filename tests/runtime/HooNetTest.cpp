#include <gtest/gtest.h>
#include "runtime/lib/hoo_net.h"
#include <cstring>
#include <cstdlib>

class HooNetTest : public ::testing::Test {
};

TEST_F(HooNetTest, URLParsing) {
    HooURL url = hoo_net_url_new("https://example.com:8080/path?q=1#frag");
    ASSERT_NE(url, nullptr);
    
    char* scheme = hoo_net_url_get_scheme(url);
    EXPECT_STREQ(scheme, "https");
    free(scheme);
    
    char* host = hoo_net_url_get_host(url);
    EXPECT_STREQ(host, "example.com");
    free(host);
    
    EXPECT_EQ(hoo_net_url_get_port(url), 8080);
    
    char* path = hoo_net_url_get_path(url);
    EXPECT_STREQ(path, "/path");
    free(path);
    
    char* query = hoo_net_url_get_query(url);
    EXPECT_STREQ(query, "q=1");
    free(query);
    
    char* frag = hoo_net_url_get_fragment(url);
    EXPECT_STREQ(frag, "frag");
    free(frag);
    
    hoo_net_url_release(url);
}

TEST_F(HooNetTest, HTTPClientMock) {
    HooHttpClient client = hoo_net_http_client_new();
    ASSERT_NE(client, nullptr);
    
    hoo_net_http_client_set_header(client, "User-Agent", "HooTest");
    
    HooHttpResponse resp = hoo_net_http_client_get(client, "http://example.com/success");
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(hoo_net_http_response_get_status_code(resp), 200);
    EXPECT_EQ(hoo_net_http_response_is_success(resp), 1);
    
    char* body = hoo_net_http_response_get_body(resp);
    EXPECT_TRUE(strstr(body, "Success") != nullptr);
    free(body);
    
    hoo_net_http_response_release(resp);
    hoo_net_http_client_release(client);
}

TEST_F(HooNetTest, ARC) {
    HooURL url = hoo_net_url_new("http://test.com");
    hoo_net_url_retain(url);
    hoo_net_url_release(url);
    hoo_net_url_release(url);
}
