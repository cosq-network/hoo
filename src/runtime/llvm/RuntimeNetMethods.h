#pragma once

#include "RuntimeMethodRegistry.h"

BEGIN_RUNTIME_CLASS(HttpClient, "hoo.net.HttpClient")
    RUNTIME_METHOD(get, "hoo_net_http_client_get")
    RUNTIME_METHOD(post, "hoo_net_http_client_post")
    RUNTIME_METHOD(put, "hoo_net_http_client_put")
    RUNTIME_METHOD(delete, "hoo_net_http_client_delete")
    RUNTIME_METHOD(setHeader, "hoo_net_http_client_set_header")
    RUNTIME_METHOD(setTimeout, "hoo_net_http_client_set_timeout")
END_RUNTIME_CLASS(HttpClient, "hoo.net.HttpClient")

BEGIN_RUNTIME_CLASS(HttpResponse, "hoo.net.HttpResponse")
    RUNTIME_METHOD(getStatusCode, "hoo_net_http_response_get_status_code")
    RUNTIME_METHOD(getStatusText, "hoo_net_http_response_get_status_text")
    RUNTIME_METHOD(getBody, "hoo_net_http_response_get_body")
    RUNTIME_METHOD(isSuccess, "hoo_net_http_response_is_success")
END_RUNTIME_CLASS(HttpResponse, "hoo.net.HttpResponse")

BEGIN_RUNTIME_CLASS(URL, "hoo.net.URL")
    RUNTIME_METHOD(getScheme, "hoo_net_url_get_scheme")
    RUNTIME_METHOD(getHost, "hoo_net_url_get_host")
    RUNTIME_METHOD(getPort, "hoo_net_url_get_port")
    RUNTIME_METHOD(getPath, "hoo_net_url_get_path")
    RUNTIME_METHOD(getQuery, "hoo_net_url_get_query")
    RUNTIME_METHOD(getFragment, "hoo_net_url_get_fragment")
    RUNTIME_METHOD(toString, "hoo_net_url_to_string")
END_RUNTIME_CLASS(URL, "hoo.net.URL")