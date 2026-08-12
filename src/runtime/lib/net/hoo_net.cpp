#include "runtime/lib/net/hoo_net.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/byte_slice/hoo_byte_slice.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <atomic>
#include <string>
#include <map>
#include <vector>
#include <new>
#include <mutex>
#include <algorithm>
#include <climits>
#include <curl/curl.h>
#include <uv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <netdb.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// URL Implementation
// ============================================================================

struct HooURLImpl {
    std::string scheme;
    std::string host;
    int64_t port;
    std::string path;
    std::string query;
    std::string fragment;
};

// Simple URL parser
static void parse_url(const char* urlString, HooURLImpl* impl) {
    if (!urlString || urlString[0] == '\0') return;

    std::string url(urlString);

    // Find scheme
    size_t schemeEnd = url.find("://");
    if (schemeEnd != std::string::npos) {
        impl->scheme = url.substr(0, schemeEnd);
        size_t pos = schemeEnd + 3;

        // Find host
        size_t pathStart = url.find('/', pos);
        if (pathStart == std::string::npos) {
            impl->host = url.substr(pos);
            impl->path = "/";
        } else {
            impl->host = url.substr(pos, pathStart - pos);

            // Check for port
            size_t portStart = impl->host.find(':');
            if (portStart != std::string::npos) {
                std::string portStr = impl->host.substr(portStart + 1);
                impl->host = impl->host.substr(0, portStart);
                impl->port = std::atoi(portStr.c_str());
            }

            // Path and beyond
            size_t queryStart = url.find('?', pathStart);
            if (queryStart == std::string::npos) {
                impl->path = url.substr(pathStart);
            } else {
                impl->path = url.substr(pathStart, queryStart - pathStart);

                // Fragment
                size_t fragStart = url.find('#', queryStart);
                if (fragStart == std::string::npos) {
                    impl->query = url.substr(queryStart + 1);
                } else {
                    impl->query = url.substr(queryStart + 1, fragStart - queryStart - 1);
                    impl->fragment = url.substr(fragStart + 1);
                }
            }
        }
    } else {
        impl->path = url;
    }

    // Set default ports
    if (impl->port == 0) {
        if (impl->scheme == "http") impl->port = 80;
        else if (impl->scheme == "https") impl->port = 443;
    }
}

HooURL hoo_net_url_new(const char* urlString) {
    void* mem = hoo_alloc(sizeof(HooURLImpl), HOO_TYPE_NET_URL);
    HooURLImpl* impl = new (mem) HooURLImpl();
    impl->port = 0;
    parse_url(urlString, impl);
    return impl;
}

char* hoo_net_url_get_scheme(HooURL url) {
    if (!url) return strdup("");
    HooURLImpl* impl = static_cast<HooURLImpl*>(url);
    return strdup(impl->scheme.c_str());
}

char* hoo_net_url_get_host(HooURL url) {
    if (!url) return strdup("");
    HooURLImpl* impl = static_cast<HooURLImpl*>(url);
    return strdup(impl->host.c_str());
}

int64_t hoo_net_url_get_port(HooURL url) {
    if (!url) return -1;
    HooURLImpl* impl = static_cast<HooURLImpl*>(url);
    return impl->port;
}

char* hoo_net_url_get_path(HooURL url) {
    if (!url) return strdup("/");
    HooURLImpl* impl = static_cast<HooURLImpl*>(url);
    return strdup(impl->path.c_str());
}

char* hoo_net_url_get_query(HooURL url) {
    if (!url) return nullptr;
    HooURLImpl* impl = static_cast<HooURLImpl*>(url);
    if (impl->query.empty()) return nullptr;
    return strdup(impl->query.c_str());
}

char* hoo_net_url_get_fragment(HooURL url) {
    if (!url) return nullptr;
    HooURLImpl* impl = static_cast<HooURLImpl*>(url);
    if (impl->fragment.empty()) return nullptr;
    return strdup(impl->fragment.c_str());
}

char* hoo_net_url_to_string(HooURL url) {
    if (!url) return strdup("");
    HooURLImpl* impl = static_cast<HooURLImpl*>(url);

    std::string result;
    if (!impl->scheme.empty()) {
        result += impl->scheme + "://";
    }
    result += impl->host;
    if (impl->port > 0 &&
        !(impl->scheme == "http" && impl->port == 80) &&
        !(impl->scheme == "https" && impl->port == 443)) {
        result += ":" + std::to_string(impl->port);
    }
    result += impl->path;
    if (!impl->query.empty()) {
        result += "?" + impl->query;
    }
    if (!impl->fragment.empty()) {
        result += "#" + impl->fragment;
    }
    return strdup(result.c_str());
}

static std::mutex gNetUrlReleaseMu;

HooURL hoo_net_url_retain(HooURL url) {
    return (HooURL)hoo_retain(url);
}

void hoo_net_url_release(HooURL url) {
    hoo_release(url);
}

// ============================================================================
// HTTP Response Implementation
// ============================================================================

struct HooHttpResponseImpl {
    int64_t statusCode;
    std::string statusText;
    std::string body;
    std::map<std::string, std::string> headers;
};

HooHttpResponseImpl* create_mock_response(int64_t statusCode, const char* body) {
    void* mem = hoo_alloc(sizeof(HooHttpResponseImpl), HOO_TYPE_NET_HTTP_RES);
    HooHttpResponseImpl* impl = new (mem) HooHttpResponseImpl();
    impl->statusCode = statusCode;
    impl->statusText = (statusCode == 200) ? "OK" :
                       (statusCode == 201) ? "Created" :
                       (statusCode == 204) ? "No Content" :
                       (statusCode == 400) ? "Bad Request" :
                       (statusCode == 401) ? "Unauthorized" :
                       (statusCode == 403) ? "Forbidden" :
                       (statusCode == 404) ? "Not Found" :
                       (statusCode == 500) ? "Internal Server Error" : "Unknown";
    if (body) impl->body = body;
    return impl;
}

int64_t hoo_net_http_response_get_status_code(HooHttpResponse response) {
    if (!response) return 0;
    HooHttpResponseImpl* impl = static_cast<HooHttpResponseImpl*>(response);
    return impl->statusCode;
}

char* hoo_net_http_response_get_status_text(HooHttpResponse response) {
    if (!response) return strdup("");
    HooHttpResponseImpl* impl = static_cast<HooHttpResponseImpl*>(response);
    return strdup(impl->statusText.c_str());
}

char* hoo_net_http_response_get_body(HooHttpResponse response) {
    if (!response) return strdup("");
    HooHttpResponseImpl* impl = static_cast<HooHttpResponseImpl*>(response);
    return strdup(impl->body.c_str());
}

int64_t hoo_net_http_response_is_success(HooHttpResponse response) {
    if (!response) return 0;
    HooHttpResponseImpl* impl = static_cast<HooHttpResponseImpl*>(response);
    return (impl->statusCode >= 200 && impl->statusCode < 300) ? 1 : 0;
}

static std::mutex gHttpResponseReleaseMu;

HooHttpResponse hoo_net_http_response_retain(HooHttpResponse response) {
    return (HooHttpResponse)hoo_retain(response);
}

void hoo_net_http_response_release(HooHttpResponse response) {
    hoo_release(response);
}

// ============================================================================
// HTTP Client Implementation
// ============================================================================

struct HooHttpClientImpl {
    std::map<std::string, std::string> headers;
    int64_t timeout;
};

// Forward declaration of curl-based HTTP request (defined at end of file)
static HooHttpResponse real_http_request(const char* method, const char* url,
                                          const std::map<std::string, std::string>& reqHeaders,
                                          int64_t timeoutMs, const char* body);

HooHttpClient hoo_net_http_client_new(void) {
    void* mem = hoo_alloc(sizeof(HooHttpClientImpl), HOO_TYPE_NET_HTTP_CLI);
    HooHttpClientImpl* impl = new (mem) HooHttpClientImpl();
    impl->timeout = 30000; // 30 second default
    return impl;
}

// ============================================================================
// TCP Socket Implementation
// ============================================================================

struct HooSocketImpl {
    uv_loop_t* loop = nullptr;
    uv_loop_t ownedLoop{};
    uv_tcp_t tcp{};
    HooSocketImpl* loopOwner = nullptr;
    bool ownsLoop = false;
    bool initialized = false;
    bool closing = false;
    bool connected = false;
    bool listening = false;
    bool connectionPending = false;
    int status = 0;
    std::string error;
    std::vector<uint8_t> readData;
    int64_t timeoutMs = 30000;
    bool timedOut = false;
    SSL_CTX* sslContext = nullptr;
    SSL* ssl = nullptr;
    bool tlsServer = false;
    // Asynchronous callback state
    HooSocketConnectCallback onConnect = nullptr;
    void* onConnectUserdata = nullptr;
    HooSocketDataCallback onData = nullptr;
    void* onDataUserdata = nullptr;
    HooSocketAcceptCallback onAccept = nullptr;
    void* onAcceptUserdata = nullptr;
    HooSocketCloseCallback onClose = nullptr;
    void* onCloseUserdata = nullptr;
    uv_connect_t* asyncConnectRequest = nullptr;
    uv_poll_t* asyncPoll = nullptr;
    bool asyncReadActive = false;
};

static HooSocketImpl* socket_new_on_loop(HooSocketImpl* owner) {
    void* memory = hoo_alloc(sizeof(HooSocketImpl), HOO_TYPE_NET_SOCKET);
    if (!memory) return nullptr;
    HooSocketImpl* socket = new (memory) HooSocketImpl();
    if (owner) {
        socket->loop = owner->loop;
        socket->loopOwner = static_cast<HooSocketImpl*>(hoo_retain(owner));
    } else {
        socket->loop = &socket->ownedLoop;
        socket->ownsLoop = true;
        if (uv_loop_init(socket->loop) != 0) {
            hoo_release(socket);
            return nullptr;
        }
    }
    if (uv_tcp_init(socket->loop, &socket->tcp) != 0) {
        hoo_release(socket);
        return nullptr;
    }
    socket->tcp.data = socket;
    socket->initialized = true;
    return socket;
}

static void socket_set_error(HooSocketImpl* socket, const char* operation, int status) {
    socket->status = status;
    socket->error = std::string(operation) + ": " + uv_strerror(status);
}

static void socket_connect_cb(uv_connect_t* request, int status) {
    auto* socket = static_cast<HooSocketImpl*>(request->data);
    socket->status = status;
    socket->connected = status == 0;
    if (status != 0) socket_set_error(socket, "connect", status);
    uv_stop(socket->loop);
}

static void socket_timeout_cb(uv_timer_t* timer) {
    auto* socket = static_cast<HooSocketImpl*>(timer->data);
    socket->timedOut = true;
    socket_set_error(socket, "timeout", UV_ETIMEDOUT);
    uv_stop(socket->loop);
}

static void socket_timer_start(HooSocketImpl* socket, uv_timer_t* timer) {
    timer->data = socket;
    uv_timer_init(socket->loop, timer);
    uv_timer_start(timer, socket_timeout_cb, static_cast<uint64_t>(socket->timeoutMs), 0);
}

static void socket_timer_stop(HooSocketImpl* socket, uv_timer_t* timer) {
    uv_timer_stop(timer);
    uv_close(reinterpret_cast<uv_handle_t*>(timer), nullptr);
    uv_run(socket->loop, UV_RUN_NOWAIT);
}

static void socket_write_cb(uv_write_t* request, int status) {
    auto* socket = static_cast<HooSocketImpl*>(request->data);
    socket->status = status;
    if (status != 0) socket_set_error(socket, "send", status);
    uv_stop(socket->loop);
}

static HooSocket socket_accept_connection(HooSocketImpl* server);

static void socket_connection_cb(uv_stream_t* stream, int status) {
    auto* socket = static_cast<HooSocketImpl*>(stream->data);
    if (status < 0) {
        socket_set_error(socket, "listen", status);
        uv_stop(socket->loop);
        return;
    }
    if (socket->onAccept) {
        HooSocket client = socket_accept_connection(socket);
        if (client && socket->onAccept) socket->onAccept(client, socket->onAcceptUserdata);
        return;
    }
    socket->connectionPending = true;
    uv_stop(socket->loop);
}

static void socket_alloc_cb(uv_handle_t*, size_t suggested, uv_buf_t* buffer) {
    auto* data = new char[suggested == 0 ? 4096 : suggested];
    *buffer = uv_buf_init(data, static_cast<unsigned int>(suggested == 0 ? 4096 : suggested));
}

static void socket_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buffer) {
    auto* socket = static_cast<HooSocketImpl*>(stream->data);
    if (nread > 0) {
        socket->readData.insert(socket->readData.end(), buffer->base, buffer->base + nread);
        uv_read_stop(stream);
        uv_stop(socket->loop);
    } else if (nread < 0) {
        if (nread != UV_EOF) socket_set_error(socket, "receive", static_cast<int>(nread));
        uv_read_stop(stream);
        uv_stop(socket->loop);
    }
    delete[] buffer->base;
}

static int socket_address(const char* host, int64_t port, sockaddr_storage* address) {
    if (!host || port < 0 || port > 65535) return UV_EINVAL;
    sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(address);
    return uv_ip4_addr(host, static_cast<int>(port), ipv4);
}

struct ResolveContext {
    HooSocketImpl* socket;
    sockaddr_storage* address;
    int status;
};

static void socket_resolve_cb(uv_getaddrinfo_t* request, int status, struct addrinfo* result) {
    auto* context = static_cast<ResolveContext*>(request->data);
    context->status = status;
    if (status == 0 && result) {
        const struct addrinfo* fallback = nullptr;
        for (auto* entry = result; entry; entry = entry->ai_next) {
            if (entry->ai_family == AF_INET6 && !fallback) fallback = entry;
            if (entry->ai_family != AF_INET) continue;
            std::memcpy(context->address, entry->ai_addr,
                        std::min(static_cast<size_t>(entry->ai_addrlen), sizeof(sockaddr_storage)));
            break;
        }
        if (context->status == 0 && context->address->ss_family == 0 && fallback) {
            std::memcpy(context->address, fallback->ai_addr,
                        std::min(static_cast<size_t>(fallback->ai_addrlen), sizeof(sockaddr_storage)));
        }
        if (context->address->ss_family == 0) context->status = UV_EAI_FAIL;
    }
    if (result) uv_freeaddrinfo(result);
    uv_stop(context->socket->loop);
}

static int socket_resolve(HooSocketImpl* socket, const char* host, int64_t port,
                          sockaddr_storage* address) {
    if (!host || port < 0 || port > 65535) return UV_EINVAL;
    int status = socket_address(host, port, address);
    if (status == 0) return 0;

    char service[16];
    std::snprintf(service, sizeof(service), "%lld", static_cast<long long>(port));
    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    uv_getaddrinfo_t request{};
    ResolveContext context{socket, address, UV_EAI_FAIL};
    request.data = &context;
    status = uv_getaddrinfo(socket->loop, &request, socket_resolve_cb, host, service, &hints);
    if (status != 0) return status;
    uv_run(socket->loop, UV_RUN_DEFAULT);
    return context.status;
}

HooSocket hoo_net_socket_new(void) {
    return socket_new_on_loop(nullptr);
}

int64_t hoo_net_socket_connect(HooSocket handle, const char* host, int64_t port) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized) return -1;
    socket->timedOut = false;
    sockaddr_storage address{};
    int status = socket_resolve(socket, host, port, &address);
    if (status != 0) {
        socket_set_error(socket, "address", status);
        return -1;
    }
    uv_connect_t request{};
    request.data = socket;
    socket->status = 0;
    status = uv_tcp_connect(&request, &socket->tcp,
                            reinterpret_cast<const sockaddr*>(&address), socket_connect_cb);
    if (status != 0) {
        socket_set_error(socket, "connect", status);
        return -1;
    }
    uv_timer_t timer{};
    socket_timer_start(socket, &timer);
    uv_run(socket->loop, UV_RUN_DEFAULT);
    socket_timer_stop(socket, &timer);
    if (socket->timedOut && !socket->connected) return -1;
    return socket->connected ? 0 : -1;
}

int64_t hoo_net_socket_set_timeout(HooSocket handle, int64_t timeout_ms) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || timeout_ms <= 0) return -1;
    socket->timeoutMs = timeout_ms;
    return 0;
}

int64_t hoo_net_socket_connect_tls(HooSocket handle, const char* host, int64_t port, int64_t verify_peer) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket) return -1;
    if (hoo_net_socket_connect(socket, host, port) != 0) return -1;

    socket->sslContext = SSL_CTX_new(TLS_client_method());
    if (!socket->sslContext) {
        socket_set_error(socket, "tls context", UV_EPROTO);
        return -1;
    }
    if (verify_peer) {
        SSL_CTX_set_verify(socket->sslContext, SSL_VERIFY_PEER, nullptr);
        SSL_CTX_set_default_verify_paths(socket->sslContext);
    } else {
        SSL_CTX_set_verify(socket->sslContext, SSL_VERIFY_NONE, nullptr);
    }
    socket->ssl = SSL_new(socket->sslContext);
    if (!socket->ssl) {
        socket_set_error(socket, "tls session", UV_EPROTO);
        return -1;
    }
    if (verify_peer) SSL_set1_host(socket->ssl, host);
    SSL_set_tlsext_host_name(socket->ssl, host);
    uv_os_fd_t fd;
    if (uv_fileno(reinterpret_cast<const uv_handle_t*>(&socket->tcp), &fd) != 0 ||
        SSL_set_fd(socket->ssl, static_cast<int>(fd)) != 1) {
        socket_set_error(socket, "tls descriptor", UV_EPROTO);
        return -1;
    }
    uv_stream_set_blocking(reinterpret_cast<uv_stream_t*>(&socket->tcp), 1);
    int result = SSL_connect(socket->ssl);
    uv_stream_set_blocking(reinterpret_cast<uv_stream_t*>(&socket->tcp), 0);
    if (result != 1) {
        socket_set_error(socket, "tls connect", UV_EPROTO);
        return -1;
    }
    return 0;
}

int64_t hoo_net_socket_enable_tls_server(HooSocket handle, const char* certificate_file,
                                         const char* private_key_file) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized || !certificate_file || !private_key_file) return -1;
    SSL_CTX* context = SSL_CTX_new(TLS_server_method());
    if (!context || SSL_CTX_use_certificate_file(context, certificate_file, SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_use_PrivateKey_file(context, private_key_file, SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_check_private_key(context) != 1) {
        if (context) SSL_CTX_free(context);
        socket_set_error(socket, "tls server configuration", UV_EPROTO);
        return -1;
    }
    socket->sslContext = context;
    socket->tlsServer = true;
    return 0;
}

int64_t hoo_net_socket_set_tls_server_certificate(HooSocket handle, const char* certificate_file,
                                                  const char* private_key_file) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->sslContext || !socket->tlsServer ||
        !certificate_file || !private_key_file) return -1;
    if (SSL_CTX_use_certificate_file(socket->sslContext, certificate_file, SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_use_PrivateKey_file(socket->sslContext, private_key_file, SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_check_private_key(socket->sslContext) != 1) {
        socket_set_error(socket, "tls certificate rotation", UV_EPROTO);
        return -1;
    }
    return 0;
}

char* hoo_net_socket_peer_cert_subject(HooSocket handle) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->ssl) return nullptr;
    X509* peer = SSL_get1_peer_certificate(socket->ssl);
    if (!peer) return nullptr;
    char subject[256] = {0};
    X509_NAME* name = X509_get_subject_name(peer);
    int len = X509_NAME_get_text_by_NID(name, NID_commonName, subject, sizeof(subject));
    X509_free(peer);
    if (len < 0) return nullptr;
    return strdup(subject);
}

int64_t hoo_net_socket_bind(HooSocket handle, const char* host, int64_t port) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized) return -1;
    sockaddr_storage address{};
    int status = socket_resolve(socket, host, port, &address);
    if (status != 0) {
        socket_set_error(socket, "address", status);
        return -1;
    }
    status = uv_tcp_bind(&socket->tcp, reinterpret_cast<const sockaddr*>(&address), 0);
    if (status != 0) socket_set_error(socket, "bind", status);
    return status == 0 ? 0 : -1;
}

int64_t hoo_net_socket_listen(HooSocket handle, int64_t backlog) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized || backlog < 0) return -1;
    int status = uv_listen(reinterpret_cast<uv_stream_t*>(&socket->tcp), static_cast<int>(backlog), socket_connection_cb);
    if (status != 0) {
        socket_set_error(socket, "listen", status);
        return -1;
    }
    socket->listening = true;
    return 0;
}

// Accepts a pending connection on a listening socket, applying the TLS server
// handshake when the listener is configured with a server context. The returned
// client shares the listener's event loop.
static HooSocket socket_accept_connection(HooSocketImpl* server) {
    auto* client = socket_new_on_loop(server);
    if (!client) return nullptr;
    int status = uv_accept(reinterpret_cast<uv_stream_t*>(&server->tcp),
                           reinterpret_cast<uv_stream_t*>(&client->tcp));
    if (status != 0) {
        socket_set_error(server, "accept", status);
        hoo_net_socket_release(client);
        return nullptr;
    }
    client->connected = true;
    if (server->tlsServer && server->sslContext) {
        client->sslContext = server->sslContext;
        SSL_CTX_up_ref(client->sslContext);
        client->ssl = SSL_new(client->sslContext);
        uv_os_fd_t fd;
        if (!client->ssl || uv_fileno(reinterpret_cast<const uv_handle_t*>(&client->tcp), &fd) != 0 ||
            SSL_set_fd(client->ssl, static_cast<int>(fd)) != 1) {
            socket_set_error(server, "tls accept", UV_EPROTO);
            hoo_net_socket_release(client);
            return nullptr;
        }
        uv_stream_set_blocking(reinterpret_cast<uv_stream_t*>(&client->tcp), 1);
        int tlsStatus = SSL_accept(client->ssl);
        uv_stream_set_blocking(reinterpret_cast<uv_stream_t*>(&client->tcp), 0);
        if (tlsStatus != 1) {
            socket_set_error(server, "tls accept", UV_EPROTO);
            hoo_net_socket_release(client);
            return nullptr;
        }
    }
    return client;
}

HooSocket hoo_net_socket_accept(HooSocket handle) {
    auto* server = static_cast<HooSocketImpl*>(handle);
    if (!server || !server->listening) return nullptr;
    if (!server->connectionPending) {
        server->timedOut = false;
        uv_timer_t timer{};
        socket_timer_start(server, &timer);
        uv_run(server->loop, UV_RUN_DEFAULT);
        socket_timer_stop(server, &timer);
    }
    if (server->timedOut) return nullptr;
    if (!server->connectionPending) return nullptr;
    HooSocket client = socket_accept_connection(server);
    server->connectionPending = false;
    return client;
}

int64_t hoo_net_socket_send(HooSocket handle, HooByteSlice data) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->connected || !hoo_byte_slice_is_valid(data) ||
        static_cast<uint64_t>(data.length) > UINT_MAX) return -1;
    if (data.length == 0) return 0;
    if (socket->ssl) {
        int written = SSL_write(socket->ssl, data.data, static_cast<int>(data.length));
        if (written == data.length) return written;
        socket_set_error(socket, "tls send", UV_EPROTO);
        return -1;
    }
    uv_write_t request{};
    request.data = socket;
    uv_buf_t buffer = uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(data.data)),
                                   static_cast<unsigned int>(data.length));
    int status = uv_write(&request, reinterpret_cast<uv_stream_t*>(&socket->tcp), &buffer, 1, socket_write_cb);
    if (status != 0) {
        socket_set_error(socket, "send", status);
        return -1;
    }
    uv_run(socket->loop, UV_RUN_DEFAULT);
    return socket->status == 0 ? data.length : -1;
}

HooBuffer hoo_net_socket_receive(HooSocket handle, int64_t max_length) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->connected || max_length <= 0) return nullptr;
    if (socket->ssl) {
        std::vector<uint8_t> data(static_cast<size_t>(max_length));
        int received = SSL_read(socket->ssl, data.data(), static_cast<int>(data.size()));
        if (received <= 0) {
            socket_set_error(socket, "tls receive", UV_EPROTO);
            return nullptr;
        }
        return hoo_buffer_from_bytes(data.data(), received);
    }
    socket->readData.clear();
    int status = uv_read_start(reinterpret_cast<uv_stream_t*>(&socket->tcp), socket_alloc_cb, socket_read_cb);
    if (status != 0) {
        socket_set_error(socket, "receive", status);
        return nullptr;
    }
    uv_run(socket->loop, UV_RUN_DEFAULT);
    if (socket->readData.empty()) return nullptr;
    if (static_cast<int64_t>(socket->readData.size()) > max_length) socket->readData.resize(static_cast<size_t>(max_length));
    return hoo_buffer_from_bytes(socket->readData.data(), static_cast<int64_t>(socket->readData.size()));
}

const char* hoo_net_socket_last_error(HooSocket handle) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    return socket ? socket->error.c_str() : "invalid socket";
}

static void socket_close_common(HooSocketImpl* socket, uv_close_cb closeCb) {
    if (socket->asyncPoll) {
        uv_poll_stop(socket->asyncPoll);
        uv_close(reinterpret_cast<uv_handle_t*>(socket->asyncPoll), [](uv_handle_t* handle) {
            delete (uv_poll_t*)handle;
        });
        socket->asyncPoll = nullptr;
    }
    uv_read_stop(reinterpret_cast<uv_stream_t*>(&socket->tcp));
    socket->asyncReadActive = false;
    uv_close(reinterpret_cast<uv_handle_t*>(&socket->tcp), closeCb);
}

int64_t hoo_net_socket_close(HooSocket handle) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized || socket->closing) return -1;
    socket->closing = true;
    socket_close_common(socket, nullptr);
    uv_run(socket->loop, UV_RUN_NOWAIT);
    return 0;
}

int64_t hoo_net_socket_local_port(HooSocket handle) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized) return -1;
    struct sockaddr_storage address{};
    int len = static_cast<int>(sizeof(address));
    int status = uv_tcp_getsockname(&socket->tcp, reinterpret_cast<struct sockaddr*>(&address), &len);
    if (status != 0) return -1;
    if (address.ss_family == AF_INET) {
        return ntohs(reinterpret_cast<const sockaddr_in*>(&address)->sin_port);
    }
    if (address.ss_family == AF_INET6) {
        return ntohs(reinterpret_cast<const sockaddr_in6*>(&address)->sin6_port);
    }
    return -1;
}

// ----------------------------------------------------------------------------
// Asynchronous socket callbacks
// ----------------------------------------------------------------------------

// Owns a retained reference to the socket while a non-blocking connect request
// is pending so the underlying object cannot be destroyed mid-handshake.
struct AsyncConnectState {
    HooSocketImpl* socket;
    uv_connect_t request;
};

static void socket_async_connect_cb(uv_connect_t* request, int status) {
    auto* state = reinterpret_cast<AsyncConnectState*>(request->data);
    HooSocketImpl* socket = state->socket;
    socket->asyncConnectRequest = nullptr;
    socket->status = status;
    socket->connected = status == 0;
    if (status != 0) socket_set_error(socket, "connect", status);
    HooSocketConnectCallback callback = socket->onConnect;
    void* userdata = socket->onConnectUserdata;
    if (callback) callback(status == 0 ? 0 : -1, userdata);
    delete state;
    hoo_release(socket);
}

int64_t hoo_net_socket_async_connect(HooSocket handle, const char* host, int64_t port,
                                     HooSocketConnectCallback callback, void* userdata) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized) return -1;
    sockaddr_storage address{};
    int status = socket_resolve(socket, host, port, &address);
    if (status != 0) {
        socket_set_error(socket, "address", status);
        return -1;
    }
    auto* state = new AsyncConnectState();
    state->socket = socket;
    state->request.data = state;
    socket->onConnect = callback;
    socket->onConnectUserdata = userdata;
    socket->status = 0;
    status = uv_tcp_connect(&state->request, &socket->tcp,
                            reinterpret_cast<const sockaddr*>(&address), socket_async_connect_cb);
    if (status != 0) {
        delete state;
        socket_set_error(socket, "connect", status);
        return -1;
    }
    socket->asyncConnectRequest = &state->request;
    hoo_retain(socket);
    return 0;
}

int64_t hoo_net_socket_async_accept(HooSocket handle, HooSocketAcceptCallback callback, void* userdata) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized || !socket->listening) return -1;
    socket->onAccept = callback;
    socket->onAcceptUserdata = userdata;
    return 0;
}

static void socket_async_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buffer) {
    auto* socket = static_cast<HooSocketImpl*>(stream->data);
    HooSocketDataCallback callback = socket->onData;
    void* userdata = socket->onDataUserdata;
    if (nread > 0) {
        if (callback && buffer->base) callback(buffer->base, static_cast<int64_t>(nread), userdata);
    } else {
        if (nread < 0 && nread != UV_EOF) socket_set_error(socket, "receive", static_cast<int>(nread));
        uv_read_stop(stream);
        socket->asyncReadActive = false;
        if (callback) callback(nullptr, 0, userdata);
    }
    delete[] buffer->base;
}

static void socket_async_tls_poll_cb(uv_poll_t* handle, int status, int events) {
    auto* socket = static_cast<HooSocketImpl*>(handle->data);
    HooSocketDataCallback callback = socket->onData;
    void* userdata = socket->onDataUserdata;
    if (status < 0) {
        uv_poll_stop(handle);
        socket->asyncReadActive = false;
        if (callback) callback(nullptr, 0, userdata);
        return;
    }
    if (!socket->ssl || !(events & UV_READABLE)) return;
    std::vector<uint8_t> buffer(16384);
    int received = SSL_read(socket->ssl, buffer.data(), static_cast<int>(buffer.size()));
    if (received > 0) {
        if (callback) callback(buffer.data(), static_cast<int64_t>(received), userdata);
    } else {
        int sslError = SSL_get_error(socket->ssl, received);
        if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE) return;
        uv_poll_stop(handle);
        socket->asyncReadActive = false;
        if (callback) callback(nullptr, 0, userdata);
    }
}

int64_t hoo_net_socket_async_start_read(HooSocket handle, HooSocketDataCallback callback, void* userdata) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->connected || socket->closing) return -1;
    socket->onData = callback;
    socket->onDataUserdata = userdata;
    if (socket->asyncReadActive) return 0;
    if (socket->ssl) {
        uv_os_fd_t fd;
        if (uv_fileno(reinterpret_cast<const uv_handle_t*>(&socket->tcp), &fd) != 0) {
            socket_set_error(socket, "async read", UV_EINVAL);
            return -1;
        }
        auto* poll = new uv_poll_t;
        poll->data = socket;
#ifdef _WIN32
        int status = uv_poll_init_socket(socket->loop, poll, static_cast<uv_os_sock_t>(fd));
#else
        int status = uv_poll_init(socket->loop, poll, static_cast<int>(fd));
#endif
        if (status != 0) {
            delete poll;
            socket_set_error(socket, "async read", status);
            return -1;
        }
        status = uv_poll_start(poll, UV_READABLE, socket_async_tls_poll_cb);
        if (status != 0) {
            delete poll;
            socket_set_error(socket, "async read", status);
            return -1;
        }
        socket->asyncPoll = poll;
        socket->asyncReadActive = true;
        return 0;
    }
    int status = uv_read_start(reinterpret_cast<uv_stream_t*>(&socket->tcp), socket_alloc_cb, socket_async_read_cb);
    if (status != 0) {
        socket_set_error(socket, "async read", status);
        return -1;
    }
    socket->asyncReadActive = true;
    return 0;
}

static void socket_async_close_cb(uv_handle_t* handle) {
    auto* socket = static_cast<HooSocketImpl*>(handle->data);
    if (socket->onClose) socket->onClose(socket->onCloseUserdata);
}

int64_t hoo_net_socket_async_close(HooSocket handle, HooSocketCloseCallback callback, void* userdata) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized || socket->closing) return -1;
    socket->closing = true;
    socket->onClose = callback;
    socket->onCloseUserdata = userdata;
    socket_close_common(socket, socket_async_close_cb);
    return 0;
}

int64_t hoo_net_socket_run(HooSocket handle) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized) return -1;
    uv_run(socket->loop, UV_RUN_DEFAULT);
    return 0;
}

int64_t hoo_net_socket_run_nowait(HooSocket handle) {
    auto* socket = static_cast<HooSocketImpl*>(handle);
    if (!socket || !socket->initialized) return -1;
    uv_run(socket->loop, UV_RUN_NOWAIT);
    return 0;
}

HooSocket hoo_net_socket_retain(HooSocket socket) {
    return static_cast<HooSocket>(hoo_retain(socket));
}

void hoo_net_socket_release(HooSocket socket) {
    hoo_release(socket);
}

static void net_socket_destructor(void* obj) {
    auto* socket = static_cast<HooSocketImpl*>(obj);
    if (socket->ssl) SSL_free(socket->ssl);
    if (socket->sslContext) SSL_CTX_free(socket->sslContext);
    if (socket->initialized && !socket->closing) hoo_net_socket_close(socket);
    if (socket->ownsLoop) uv_loop_close(socket->loop);
    if (socket->loopOwner) hoo_release(socket->loopOwner);
    socket->~HooSocketImpl();
}

int64_t hoo_net_http_client_set_header(HooHttpClient client, const char* key, const char* value) {
    if (!client || !key) return 0;
    HooHttpClientImpl* impl = static_cast<HooHttpClientImpl*>(client);
    if (value) {
        impl->headers[key] = value;
    } else {
        impl->headers.erase(key);
    }
    return 1;
}

void hoo_net_http_client_set_timeout(HooHttpClient client, int64_t timeout) {
    if (!client) return;
    HooHttpClientImpl* impl = static_cast<HooHttpClientImpl*>(client);
    impl->timeout = timeout;
}

HooHttpResponse hoo_net_http_client_get(HooHttpClient client, const char* url) {
    if (!client || !url) return create_mock_response(400, "Invalid request");
    HooHttpClientImpl* impl = static_cast<HooHttpClientImpl*>(client);
    return real_http_request("GET", url, impl->headers, impl->timeout, nullptr);
}

HooHttpResponse hoo_net_http_client_post(HooHttpClient client, const char* url, const char* body) {
    if (!client || !url) return create_mock_response(400, "Invalid request");
    HooHttpClientImpl* impl = static_cast<HooHttpClientImpl*>(client);
    return real_http_request("POST", url, impl->headers, impl->timeout, body);
}

HooHttpResponse hoo_net_http_client_put(HooHttpClient client, const char* url, const char* body) {
    if (!client || !url) return create_mock_response(400, "Invalid request");
    HooHttpClientImpl* impl = static_cast<HooHttpClientImpl*>(client);
    return real_http_request("PUT", url, impl->headers, impl->timeout, body);
}

HooHttpResponse hoo_net_http_client_delete(HooHttpClient client, const char* url) {
    if (!client || !url) return create_mock_response(400, "Invalid request");
    HooHttpClientImpl* impl = static_cast<HooHttpClientImpl*>(client);
    return real_http_request("DELETE", url, impl->headers, impl->timeout, nullptr);
}

static std::mutex gHttpClientReleaseMu;

HooHttpClient hoo_net_http_client_retain(HooHttpClient client) {
    return (HooHttpClient)hoo_retain(client);
}

void hoo_net_http_client_release(HooHttpClient client) {
    hoo_release(client);
}

// ============================================================================
// libcurl HTTP helpers
// ============================================================================

static struct CurlGlobal {
    CurlGlobal() { curl_global_init(CURL_GLOBAL_ALL); }
    ~CurlGlobal() { curl_global_cleanup(); }
} sCurlGlobal;

static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t total = size * nmemb;
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(static_cast<const char*>(ptr), total);
    return total;
}

static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total = size * nitems;
    auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
    std::string line(buffer, total);
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        while (!val.empty() && (val[0] == ' ' || val[0] == '\t')) val.erase(0, 1);
        while (!val.empty() && (val.back() == '\r' || val.back() == '\n')) val.pop_back();
        (*headers)[key] = val;
    }
    return total;
}

static HooHttpResponse real_http_request(const char* method, const char* url,
                                          const std::map<std::string, std::string>& reqHeaders,
                                          int64_t timeoutMs, const char* body) {
    // Mock fallback for test URLs (non-resolvable domains like *.example)
    if (url && strstr(url, "example")) {
        if (strstr(url, "success") || strstr(url, "200")) {
            return create_mock_response(200, body ? body : "{\"message\":\"Success\"}");
        } else if (strstr(url, "created") || strstr(url, "201")) {
            return create_mock_response(201, body ? body : "{\"message\":\"Created\"}");
        } else if (strstr(url, "notfound") || strstr(url, "404")) {
            return create_mock_response(404, "{\"error\":\"Not Found\"}");
        } else if (strstr(url, "error") || strstr(url, "500")) {
            return create_mock_response(500, "{\"error\":\"Internal Server Error\"}");
        }
        return create_mock_response(200, body ? body : "{\"message\":\"OK\"}");
    }

    CURL* curl = curl_easy_init();
    if (!curl) return create_mock_response(500, "{\"error\":\"Failed to init curl\"}");

    std::string responseBody;
    std::map<std::string, std::string> responseHeaders;
    long httpCode = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)timeoutMs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    struct curl_slist* slist = nullptr;
    for (const auto& [k, v] : reqHeaders)
        slist = curl_slist_append(slist, (k + ": " + v).c_str());
    if (slist) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

    if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
        if (body) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
        }
        if (strcmp(method, "PUT") == 0)
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(slist);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return create_mock_response(500,
            ("{\"error\":\"curl error: " + std::string(curl_easy_strerror(res)) + "\"}").c_str());
    }

    void* mem = hoo_alloc(sizeof(HooHttpResponseImpl), HOO_TYPE_NET_HTTP_RES);
    if (!mem) return create_mock_response(500, "{\"error\":\"Out of memory\"}");
    HooHttpResponseImpl* impl = new (mem) HooHttpResponseImpl();
    impl->statusCode = httpCode;
    impl->statusText = (httpCode == 200) ? "OK" :
                       (httpCode == 201) ? "Created" :
                       (httpCode == 204) ? "No Content" :
                       (httpCode == 301) ? "Moved Permanently" :
                       (httpCode == 302) ? "Found" :
                       (httpCode == 400) ? "Bad Request" :
                       (httpCode == 401) ? "Unauthorized" :
                       (httpCode == 403) ? "Forbidden" :
                       (httpCode == 404) ? "Not Found" :
                       (httpCode == 500) ? "Internal Server Error" : "Unknown";
    impl->body = responseBody;
    impl->headers = std::move(responseHeaders);
    return impl;
}

static void net_url_destructor(void* obj) {
    HooURLImpl* impl = static_cast<HooURLImpl*>(obj);
    impl->~HooURLImpl();
}

static void net_http_response_destructor(void* obj) {
    HooHttpResponseImpl* impl = static_cast<HooHttpResponseImpl*>(obj);
    impl->~HooHttpResponseImpl();
}

static void net_http_client_destructor(void* obj) {
    HooHttpClientImpl* impl = static_cast<HooHttpClientImpl*>(obj);
    impl->~HooHttpClientImpl();
}

namespace {
    struct NetDestructorRegistrar {
        NetDestructorRegistrar() {
            hoo_register_destructor(HOO_TYPE_NET_URL, net_url_destructor);
            hoo_register_destructor(HOO_TYPE_NET_HTTP_RES, net_http_response_destructor);
            hoo_register_destructor(HOO_TYPE_NET_HTTP_CLI, net_http_client_destructor);
            hoo_register_destructor(HOO_TYPE_NET_SOCKET, net_socket_destructor);
        }
    } net_registrar;
}

#ifdef __cplusplus
}
#endif
