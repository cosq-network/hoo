#pragma once

#include <stdint.h>
#include "runtime/lib/byte_slice/hoo_byte_slice.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooNet - Network Operations
// ============================================================================
//
// Provides network operations for the hoo.net module.
// Includes URL parsing, HTTP client, and HTTP response handling.
// Raw sockets use libuv handles underneath. Calls are synchronous at the C
// ABI boundary, but all I/O is performed through non-blocking libuv streams.

// ============================================================================
// URL Class
// ============================================================================

typedef void* HooURL;

/**
 * Create a new URL from string
 * @param urlString URL string
 * @return New HooURL with refcount=1
 */
HooURL hoo_net_url_new(const char* urlString);

/**
 * Get the scheme (protocol)
 * @param url URL (may be NULL)
 * @return Scheme string (must be released by caller)
 */
char* hoo_net_url_get_scheme(HooURL url);

/**
 * Get the host
 * @param url URL (may be NULL)
 * @return Host string (must be released by caller)
 */
char* hoo_net_url_get_host(HooURL url);

/**
 * Get the port
 * @param url URL (may be NULL)
 * @return Port number, or -1 if not specified
 */
int64_t hoo_net_url_get_port(HooURL url);

/**
 * Get the path
 * @param url URL (may be NULL)
 * @return Path string (must be released by caller)
 */
char* hoo_net_url_get_path(HooURL url);

/**
 * Get the query string
 * @param url URL (may be NULL)
 * @return Query string (must be released by caller), or NULL
 */
char* hoo_net_url_get_query(HooURL url);

/**
 * Get the fragment
 * @param url URL (may be NULL)
 * @return Fragment string (must be released by caller), or NULL
 */
char* hoo_net_url_get_fragment(HooURL url);

/**
 * Convert URL to string representation
 * @param url URL (may be NULL)
 * @return String representation (must be released by caller)
 */
char* hoo_net_url_to_string(HooURL url);

/**
 * Retain URL
 * @param url URL
 * @return Same URL
 */
HooURL hoo_net_url_retain(HooURL url);

/**
 * Release URL
 * @param url URL
 */
void hoo_net_url_release(HooURL url);

// ============================================================================
// HTTP Response Class
// ============================================================================

typedef void* HooHttpResponse;

/**
 * Get HTTP status code
 * @param response HTTP response (may be NULL)
 * @return Status code, or 0 if invalid
 */
int64_t hoo_net_http_response_get_status_code(HooHttpResponse response);

/**
 * Get HTTP status text
 * @param response HTTP response (may be NULL)
 * @return Status text (must be released by caller)
 */
char* hoo_net_http_response_get_status_text(HooHttpResponse response);

/**
 * Get response body
 * @param response HTTP response (may be NULL)
 * @return Body string (must be released by caller)
 */
char* hoo_net_http_response_get_body(HooHttpResponse response);

/**
 * Check if response indicates success (2xx)
 * @param response HTTP response (may be NULL)
 * @return 1 if success, 0 otherwise
 */
int64_t hoo_net_http_response_is_success(HooHttpResponse response);

/**
 * Retain HTTP response
 * @param response HTTP response
 * @return Same response
 */
HooHttpResponse hoo_net_http_response_retain(HooHttpResponse response);

/**
 * Release HTTP response
 * @param response HTTP response
 */
void hoo_net_http_response_release(HooHttpResponse response);

// ============================================================================
// HTTP Client Class
// ============================================================================

typedef void* HooHttpClient;

/**
 * Create a new HTTP client
 * @return New HooHttpClient with refcount=1
 */
HooHttpClient hoo_net_http_client_new(void);

/**
 * Set a request header
 * @param client HTTP client
 * @param key Header name
 * @param value Header value
 * @return 1 on success, 0 on failure
 */
int64_t hoo_net_http_client_set_header(HooHttpClient client, const char* key, const char* value);

/**
 * Set request timeout in milliseconds
 * @param client HTTP client
 * @param timeout Timeout in milliseconds
 */
void hoo_net_http_client_set_timeout(HooHttpClient client, int64_t timeout);

/**
 * Perform GET request
 * @param client HTTP client
 * @param url Target URL
 * @return HTTP response (must be released by caller)
 */
HooHttpResponse hoo_net_http_client_get(HooHttpClient client, const char* url);

/**
 * Perform POST request
 * @param client HTTP client
 * @param url Target URL
 * @param body Request body
 * @return HTTP response (must be released by caller)
 */
HooHttpResponse hoo_net_http_client_post(HooHttpClient client, const char* url, const char* body);

/**
 * Perform PUT request
 * @param client HTTP client
 * @param url Target URL
 * @param body Request body
 * @return HTTP response (must be released by caller)
 */
HooHttpResponse hoo_net_http_client_put(HooHttpClient client, const char* url, const char* body);

/**
 * Perform DELETE request
 * @param client HTTP client
 * @param url Target URL
 * @return HTTP response (must be released by caller)
 */
HooHttpResponse hoo_net_http_client_delete(HooHttpClient client, const char* url);

/**
 * Retain HTTP client
 * @param client HTTP client
 * @return Same client
 */
HooHttpClient hoo_net_http_client_retain(HooHttpClient client);

/**
 * Release HTTP client
 * @param client HTTP client
 */
void hoo_net_http_client_release(HooHttpClient client);

// ============================================================================
// TCP Socket API
// ============================================================================

typedef void* HooSocket;

HooSocket hoo_net_socket_new(void);
int64_t hoo_net_socket_connect(HooSocket socket, const char* host, int64_t port);
int64_t hoo_net_socket_connect_tls(HooSocket socket, const char* host, int64_t port, int64_t verify_peer);
int64_t hoo_net_socket_enable_tls_server(HooSocket socket, const char* certificate_file, const char* private_key_file);
int64_t hoo_net_socket_set_timeout(HooSocket socket, int64_t timeout_ms);
int64_t hoo_net_socket_bind(HooSocket socket, const char* host, int64_t port);
int64_t hoo_net_socket_listen(HooSocket socket, int64_t backlog);
HooSocket hoo_net_socket_accept(HooSocket socket);
int64_t hoo_net_socket_send(HooSocket socket, HooByteSlice data);
HooBuffer hoo_net_socket_receive(HooSocket socket, int64_t max_length);
const char* hoo_net_socket_last_error(HooSocket socket);
int64_t hoo_net_socket_local_port(HooSocket socket);
int64_t hoo_net_socket_close(HooSocket socket);
HooSocket hoo_net_socket_retain(HooSocket socket);
void hoo_net_socket_release(HooSocket socket);

// ----------------------------------------------------------------------------
// TLS server certificate rotation
// ----------------------------------------------------------------------------

/**
 * Replace the PEM certificate/key served by an already-configured TLS server
 * socket. New accepted connections use the rotated certificate while existing
 * connections keep the previous one.
 * @return 0 on success, -1 on failure
 */
int64_t hoo_net_socket_set_tls_server_certificate(HooSocket socket, const char* certificate_file, const char* private_key_file);

/**
 * Return the subject common name of the peer certificate presented during the
 * TLS handshake (must be released by the caller), or NULL when unavailable.
 */
char* hoo_net_socket_peer_cert_subject(HooSocket socket);

// ----------------------------------------------------------------------------
// Protocol-level asynchronous socket callbacks
//
// These functions register event callbacks driven by the socket's libuv event
// loop instead of blocking at the C boundary. After registering callbacks, call
// hoo_net_socket_run() to pump the loop until no handles remain active, or
// hoo_net_socket_run_nowait() to process ready events once. The data callback
// receives 0 for length on EOF/error, and NULL data on a closed stream.
// ----------------------------------------------------------------------------

typedef void (*HooSocketConnectCallback)(int64_t status, void* userdata);
typedef void (*HooSocketDataCallback)(const void* data, int64_t length, void* userdata);
typedef void (*HooSocketAcceptCallback)(HooSocket client, void* userdata);
typedef void (*HooSocketCloseCallback)(void* userdata);

int64_t hoo_net_socket_async_connect(HooSocket socket, const char* host, int64_t port,
                                     HooSocketConnectCallback callback, void* userdata);
int64_t hoo_net_socket_async_accept(HooSocket socket, HooSocketAcceptCallback callback, void* userdata);
int64_t hoo_net_socket_async_start_read(HooSocket socket, HooSocketDataCallback callback, void* userdata);
int64_t hoo_net_socket_async_close(HooSocket socket, HooSocketCloseCallback callback, void* userdata);
int64_t hoo_net_socket_run(HooSocket socket);
int64_t hoo_net_socket_run_nowait(HooSocket socket);

#ifdef __cplusplus
}
#endif
