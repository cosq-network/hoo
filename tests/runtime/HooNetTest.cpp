#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <atomic>
#include <condition_variable>
#include <string>
#include <vector>
#include "runtime/lib/hoo_net.h"
#include "runtime/lib/hoo_encoding.h"
#include "runtime/lib/hoo_hashing.h"
#include "runtime/lib/hoo_compression.h"
#include <thread>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

class HooNetTest : public ::testing::Test {
};

namespace {

// ---------------------------------------------------------------------------
// Self-signed certificate fixture generation (cross-platform, no CLI).
// ---------------------------------------------------------------------------

static std::string tempDirectoryPath() {
    const char* base = std::getenv("TMPDIR");
#ifdef _WIN32
    if (!base || !*base) base = std::getenv("TEMP");
    if (!base || !*base) base = "C:\\Windows\\Temp";
#else
    if (!base || !*base) base = "/tmp";
#endif
    return base;
}

static bool writeSelfSignedCertificate(const std::string& commonName, std::string* certPath,
                                       std::string* keyPath) {
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!pctx) return false;
    if (EVP_PKEY_keygen_init(pctx) <= 0 || EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048) <= 0 ||
        EVP_PKEY_keygen(pctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return false;
    }
    EVP_PKEY_CTX_free(pctx);

    X509* cert = X509_new();
    if (!cert) {
        EVP_PKEY_free(pkey);
        return false;
    }
    X509_set_version(cert, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 0x1001);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 60L * 60 * 24 * 30);
    X509_set_pubkey(cert, pkey);
    X509_NAME* name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char*>(commonName.c_str()),
                               static_cast<int>(commonName.size()), -1, 0);
    X509_set_issuer_name(cert, name);
    if (X509_sign(cert, pkey, EVP_sha256()) <= 0) {
        X509_free(cert);
        EVP_PKEY_free(pkey);
        return false;
    }

    const std::string dir = tempDirectoryPath();
    *keyPath = dir + "/hoo-tls-" + commonName + ".key.pem";
    *certPath = dir + "/hoo-tls-" + commonName + ".cert.pem";

    bool ok = false;
    FILE* keyFile = std::fopen(keyPath->c_str(), "w");
    if (keyFile) {
        ok = PEM_write_PrivateKey(keyFile, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 0;
        std::fclose(keyFile);
    }
    FILE* certFile = std::fopen(certPath->c_str(), "w");
    if (ok && certFile) {
        ok = PEM_write_X509(certFile, cert) != 0;
        std::fclose(certFile);
    }
    X509_free(cert);
    EVP_PKEY_free(pkey);
    return ok;
}

// ---------------------------------------------------------------------------
// Shared event flags for asynchronous socket tests.
// ---------------------------------------------------------------------------

struct AsyncSocketEvents {
    std::atomic<bool> connected{false};
    std::atomic<bool> accepted{false};
    std::atomic<bool> serverGotData{false};
    std::atomic<bool> clientGotData{false};
    std::atomic<bool> clientClosed{false};
    std::atomic<bool> serverDone{false};
    std::atomic<HooSocket> acceptedClient{nullptr};
    std::mutex mutex;
    std::condition_variable cv;
    std::string serverReceived;
    std::string clientReceived;
};

static void asyncConnectCallback(int64_t status, void* userdata) {
    auto* events = static_cast<AsyncSocketEvents*>(userdata);
    if (status == 0) events->connected.store(true);
}

static void asyncAcceptCallback(HooSocket client, void* userdata) {
    auto* events = static_cast<AsyncSocketEvents*>(userdata);
    events->acceptedClient.store(client);
    events->accepted.store(true);
    hoo_net_socket_async_start_read(client, [](const void* data, int64_t length, void* inner) {
        auto* events2 = static_cast<AsyncSocketEvents*>(inner);
        if (length > 0 && data) {
            {
                std::lock_guard<std::mutex> lock(events2->mutex);
                events2->serverReceived.assign(static_cast<const char*>(data), length);
            }
            events2->serverGotData.store(true);
        }
    }, userdata);
}

static void asyncClientDataCallback(const void* data, int64_t length, void* userdata) {
    auto* events = static_cast<AsyncSocketEvents*>(userdata);
    if (length > 0 && data) {
        {
            std::lock_guard<std::mutex> lock(events->mutex);
            events->clientReceived.assign(static_cast<const char*>(data), length);
        }
        events->clientGotData.store(true);
    } else {
        events->clientClosed.store(true);
    }
}

static bool pumpUntil(HooSocket socket, const std::atomic<bool>& done, int maxIterations = 5000) {
    for (int i = 0; i < maxIterations && !done.load(); ++i) {
        hoo_net_socket_run_nowait(socket);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return done.load();
}

}  // namespace

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

TEST_F(HooNetTest, ByteSliceIsBorrowedAndBufferConvertible) {
    const uint8_t bytes[] = {1, 2, 3};
    HooByteSlice raw = hoo_byte_slice_from_bytes(bytes, 3);
    ASSERT_EQ(hoo_byte_slice_is_valid(raw), 1);
    EXPECT_EQ(raw.data, bytes);
    EXPECT_EQ(raw.length, 3);

    HooBuffer buffer = hoo_buffer_from_bytes(bytes, 3);
    ASSERT_NE(buffer, nullptr);
    HooByteSlice view = hoo_byte_slice_from_buffer(buffer);
    EXPECT_EQ(view.data, hoo_buffer_data(buffer));
    EXPECT_EQ(view.length, 3);
    hoo_buffer_release(buffer);
}

TEST_F(HooNetTest, ByteSliceHandleFeedsEncodingHashingAndCompression) {
    const uint8_t bytes[] = {'h', 'i'};
    HooBuffer buffer = hoo_buffer_from_bytes(bytes, 2);
    HooByteSliceHandle slice = hoo_byte_slice_from_buffer_handle(buffer);
    ASSERT_NE(slice, nullptr);

    char* encoded = hoo_encoding_base64_encode_slice(slice);
    ASSERT_NE(encoded, nullptr);
    EXPECT_STREQ(encoded, "aGk=");
    hoo_encoding_free_string(encoded);

    char* digest = hoo_hashing_sha256_slice(slice);
    ASSERT_NE(digest, nullptr);
    EXPECT_EQ(std::strlen(digest), 64u);
    hoo_hashing_free_string(digest);

    HooBuffer compressed = hoo_compression_gzip_compress_slice(slice);
    ASSERT_NE(compressed, nullptr);
    EXPECT_GT(hoo_buffer_length(compressed), 0);
    hoo_buffer_release(compressed);
    hoo_byte_slice_release(slice);
    hoo_buffer_release(buffer);
}

TEST_F(HooNetTest, TcpSocketConnectSendReceiveAndAccept) {
    constexpr int64_t port = 39765;
    HooSocket server = hoo_net_socket_new();
    ASSERT_NE(server, nullptr);
    ASSERT_EQ(hoo_net_socket_bind(server, "127.0.0.1", port), 0)
        << hoo_net_socket_last_error(server);
    ASSERT_EQ(hoo_net_socket_listen(server, 1), 0)
        << hoo_net_socket_last_error(server);

    std::thread clientThread([&] {
        HooSocket client = hoo_net_socket_new();
        ASSERT_NE(client, nullptr);
        ASSERT_EQ(hoo_net_socket_set_timeout(client, 1000), 0);
        ASSERT_EQ(hoo_net_socket_connect(client, "localhost", port), 0)
            << hoo_net_socket_last_error(client);
        const uint8_t payload[] = {'o', 'k'};
        EXPECT_EQ(hoo_net_socket_send(client, hoo_byte_slice_from_bytes(payload, 2)), 2);
        hoo_net_socket_release(client);
    });

    HooSocket accepted = hoo_net_socket_accept(server);
    ASSERT_NE(accepted, nullptr) << hoo_net_socket_last_error(server);
    HooBuffer received = hoo_net_socket_receive(accepted, 16);
    ASSERT_NE(received, nullptr) << hoo_net_socket_last_error(accepted);
    ASSERT_EQ(hoo_buffer_length(received), 2);
    EXPECT_EQ(hoo_buffer_byte_at(received, 0), 'o');
    EXPECT_EQ(hoo_buffer_byte_at(received, 1), 'k');

    hoo_buffer_release(received);
    hoo_net_socket_release(accepted);
    clientThread.join();
    hoo_net_socket_release(server);
}

TEST_F(HooNetTest, TlsServerConfigurationRejectsMissingCertificate) {
    HooSocket socket = hoo_net_socket_new();
    ASSERT_NE(socket, nullptr);
    EXPECT_EQ(hoo_net_socket_enable_tls_server(socket, "missing-cert.pem", "missing-key.pem"), -1);
    EXPECT_NE(std::strstr(hoo_net_socket_last_error(socket), "tls server configuration"), nullptr);
    hoo_net_socket_release(socket);
}

TEST_F(HooNetTest, AsyncSocketCallbacksDeliverConnectDataAndClose) {
    AsyncSocketEvents events;

    HooSocket server = hoo_net_socket_new();
    ASSERT_NE(server, nullptr);
    ASSERT_EQ(hoo_net_socket_set_timeout(server, 5000), 0);
    ASSERT_EQ(hoo_net_socket_bind(server, "127.0.0.1", 0), 0) << hoo_net_socket_last_error(server);
    int64_t port = hoo_net_socket_local_port(server);
    ASSERT_GT(port, 0);
    ASSERT_EQ(hoo_net_socket_listen(server, 4), 0) << hoo_net_socket_last_error(server);
    ASSERT_EQ(hoo_net_socket_async_accept(server, asyncAcceptCallback, &events), 0);

    HooSocket client = hoo_net_socket_new();
    ASSERT_NE(client, nullptr);
    ASSERT_EQ(hoo_net_socket_set_timeout(client, 5000), 0);
    ASSERT_EQ(hoo_net_socket_async_connect(client, "localhost", port, asyncConnectCallback, &events), 0);

    // Pump both loops until the connection has been established and accepted.
    for (int i = 0; i < 5000 && !(events.connected.load() && events.accepted.load()); ++i) {
        hoo_net_socket_run_nowait(server);
        hoo_net_socket_run_nowait(client);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ASSERT_TRUE(events.connected.load()) << "async connect callback never fired";
    ASSERT_TRUE(events.accepted.load()) << "async accept callback never fired";
    HooSocket accepted = events.acceptedClient.load();
    ASSERT_NE(accepted, nullptr);

    // Client sends payload; server's async read delivers it.
    ASSERT_EQ(hoo_net_socket_async_start_read(client, asyncClientDataCallback, &events), 0);
    const uint8_t payload[] = {'h', 'e', 'l', 'l', 'o'};
    EXPECT_EQ(hoo_net_socket_send(client, hoo_byte_slice_from_bytes(payload, 5)), 5);
    ASSERT_TRUE(pumpUntil(server, events.serverGotData)) << "server async read never fired";
    {
        std::lock_guard<std::mutex> lock(events.mutex);
        EXPECT_EQ(events.serverReceived, "hello");
    }

    // Server echoes back over the accepted client.
    HooSocket echo = accepted;
    const uint8_t response[] = {'w', 'o', 'r', 'l', 'd'};
    EXPECT_EQ(hoo_net_socket_send(echo, hoo_byte_slice_from_bytes(response, 5)), 5);

    ASSERT_TRUE(pumpUntil(client, events.clientGotData)) << "client async read never fired";
    {
        std::lock_guard<std::mutex> lock(events.mutex);
        EXPECT_EQ(events.clientReceived, "world");
    }

    // Close the client asynchronously; the close callback must fire.
    ASSERT_EQ(hoo_net_socket_async_close(client, [](void* userdata) {
        static_cast<AsyncSocketEvents*>(userdata)->clientClosed.store(true);
    }, &events), 0);
    ASSERT_TRUE(pumpUntil(client, events.clientClosed)) << "async close callback never fired";

    // Close the accepted client and the listener to drain the server loop.
    ASSERT_EQ(hoo_net_socket_async_close(accepted, nullptr, nullptr), 0);
    ASSERT_EQ(hoo_net_socket_async_close(server, nullptr, nullptr), 0);
    hoo_net_socket_run_nowait(server);
    hoo_net_socket_run_nowait(client);

    hoo_net_socket_release(client);
    hoo_net_socket_release(accepted);
    hoo_net_socket_release(server);
}

TEST_F(HooNetTest, TlsServerCertificateRotationChangesServedIdentity) {
    std::string cert1, key1, cert2, key2;
    ASSERT_TRUE(writeSelfSignedCertificate("tls-first", &cert1, &key1));
    ASSERT_TRUE(writeSelfSignedCertificate("tls-second", &cert2, &key2));

    HooSocket server = hoo_net_socket_new();
    ASSERT_NE(server, nullptr);
    ASSERT_EQ(hoo_net_socket_set_timeout(server, 5000), 0);
    ASSERT_EQ(hoo_net_socket_enable_tls_server(server, cert1.c_str(), key1.c_str()), 0)
        << hoo_net_socket_last_error(server);
    ASSERT_EQ(hoo_net_socket_bind(server, "127.0.0.1", 0), 0) << hoo_net_socket_last_error(server);
    int64_t port = hoo_net_socket_local_port(server);
    ASSERT_GT(port, 0);
    ASSERT_EQ(hoo_net_socket_listen(server, 4), 0) << hoo_net_socket_last_error(server);

    std::vector<HooSocket> acceptedClients;
    std::mutex acceptMutex;
    std::condition_variable acceptCv;
    std::atomic<bool> acceptDone{false};
    std::thread acceptThread([&] {
        for (int i = 0; i < 2; ++i) {
            HooSocket accepted = hoo_net_socket_accept(server);
            if (!accepted) break;
            std::lock_guard<std::mutex> lock(acceptMutex);
            acceptedClients.push_back(accepted);
            acceptCv.notify_all();
        }
        std::unique_lock<std::mutex> lock(acceptMutex);
        acceptCv.wait(lock, [&] { return acceptDone.load(); });
        for (HooSocket client : acceptedClients) {
            hoo_net_socket_close(client);
            hoo_net_socket_run_nowait(client);
            hoo_net_socket_release(client);
        }
    });

    // First connection: verify the peer presents the original certificate.
    HooSocket client1 = hoo_net_socket_new();
    ASSERT_NE(client1, nullptr);
    ASSERT_EQ(hoo_net_socket_set_timeout(client1, 5000), 0);
    ASSERT_EQ(hoo_net_socket_connect_tls(client1, "127.0.0.1", port, 0), 0)
        << hoo_net_socket_last_error(client1);
    char* subject1 = hoo_net_socket_peer_cert_subject(client1);
    ASSERT_NE(subject1, nullptr);
    EXPECT_STREQ(subject1, "tls-first");
    std::free(subject1);
    hoo_net_socket_release(client1);

    // Rotate the served certificate on the live listener.
    ASSERT_EQ(hoo_net_socket_set_tls_server_certificate(server, cert2.c_str(), key2.c_str()), 0)
        << hoo_net_socket_last_error(server);

    // Second connection: the rotated identity must be presented.
    HooSocket client2 = hoo_net_socket_new();
    ASSERT_NE(client2, nullptr);
    ASSERT_EQ(hoo_net_socket_set_timeout(client2, 5000), 0);
    ASSERT_EQ(hoo_net_socket_connect_tls(client2, "127.0.0.1", port, 0), 0)
        << hoo_net_socket_last_error(client2);
    char* subject2 = hoo_net_socket_peer_cert_subject(client2);
    ASSERT_NE(subject2, nullptr);
    EXPECT_STREQ(subject2, "tls-second");
    std::free(subject2);
    hoo_net_socket_release(client2);

    acceptDone.store(true);
    acceptCv.notify_all();
    acceptThread.join();
    hoo_net_socket_release(server);
}

TEST_F(HooNetTest, AsyncTlsReadDeliversDecryptedData) {
    std::string cert, key;
    ASSERT_TRUE(writeSelfSignedCertificate("tls-async", &cert, &key));

    AsyncSocketEvents events;
    HooSocket server = hoo_net_socket_new();
    ASSERT_NE(server, nullptr);
    ASSERT_EQ(hoo_net_socket_set_timeout(server, 5000), 0);
    ASSERT_EQ(hoo_net_socket_enable_tls_server(server, cert.c_str(), key.c_str()), 0)
        << hoo_net_socket_last_error(server);
    ASSERT_EQ(hoo_net_socket_bind(server, "127.0.0.1", 0), 0) << hoo_net_socket_last_error(server);
    int64_t port = hoo_net_socket_local_port(server);
    ASSERT_GT(port, 0);
    ASSERT_EQ(hoo_net_socket_listen(server, 4), 0) << hoo_net_socket_last_error(server);
    ASSERT_EQ(hoo_net_socket_async_accept(server, asyncAcceptCallback, &events), 0);

    // Server loop runs on a dedicated thread so the blocking client handshake
    // can proceed while accepted connections are serviced asynchronously.
    std::thread serverThread([&] {
        while (!events.serverDone.load()) {
            hoo_net_socket_run_nowait(server);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        HooSocket accepted = events.acceptedClient.load();
        if (accepted) {
            hoo_net_socket_async_close(accepted, nullptr, nullptr);
            hoo_net_socket_run_nowait(server);
            hoo_net_socket_release(accepted);
        }
    });

    HooSocket client = hoo_net_socket_new();
    ASSERT_NE(client, nullptr);
    ASSERT_EQ(hoo_net_socket_set_timeout(client, 5000), 0);
    ASSERT_EQ(hoo_net_socket_connect_tls(client, "localhost", port, 0), 0)
        << hoo_net_socket_last_error(client);

    const uint8_t payload[] = {'t', 'l', 's', '!', '!'};
    EXPECT_EQ(hoo_net_socket_send(client, hoo_byte_slice_from_bytes(payload, 5)), 5);

    // The server loop is pumped continuously by serverThread, so wait for the
    // callback flag without pumping the shared loop from this thread.
    for (int i = 0; i < 8000 && !events.serverGotData.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ASSERT_TRUE(events.serverGotData.load()) << "TLS async read never fired";
    {
        std::lock_guard<std::mutex> lock(events.mutex);
        EXPECT_EQ(events.serverReceived, "tls!!");
    }

    hoo_net_socket_release(client);
    events.serverDone.store(true);
    serverThread.join();
    // Release the listener on this thread; sockets are TLAB-allocated and must
    // be freed on the thread that allocated them.
    hoo_net_socket_release(server);
}
