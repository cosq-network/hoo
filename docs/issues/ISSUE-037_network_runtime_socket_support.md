# ISSUE-037: Runtime Networking API Is Missing Full Socket Support

## 1. Overview
The runtime networking header declares the `hoo.net` surface and includes URL parsing, HTTP client, and response helpers, but the implementation note explicitly says that full socket support is still planned for future work.

This means the public runtime API is incomplete for lower-level network use cases, even though higher-level URL and HTTP helpers are already modeled in the docs and tests.

## 2. Technical Analysis
The relevant declaration is in `src/runtime/lib/hoo_net.h`:

```cpp
// Note: Full socket support is planned for future implementation.
```

The current runtime surface appears to support:
- URL construction and accessors,
- HTTP client request/response abstractions,
- possibly mock-based testing for common cases.

However, there is no documented or implemented full socket API for:
- creating raw sockets,
- connecting and binding,
- sending and receiving byte streams,
- non-blocking I/O, or
- TLS/SSL transport flows.

This gap is separate from the higher-level URL/HTTP APIs already described in the runtime docs.

## 3. Requirements
1. Define the missing low-level socket primitives for the runtime API.
2. Decide whether the runtime should expose:
   - a direct socket abstraction,
   - stream-oriented wrappers, or
   - platform-specific bindings under a common Hoo API.
3. Document ownership and lifetime behavior for buffers and connection handles.
4. Add tests covering success and failure cases for the socket layer once implemented.

## 4. Impact
- Prevents full network stack support beyond URL parsing and HTTP helpers.
- Leaves the runtime unable to implement protocols that require raw socket access.
- Limits integration with servers, custom protocols, and low-level networking scenarios.

## 5. Status
- **Date**: 2026-08-09
- **Status**: **IMPLEMENTED**
- **Priority**: Medium (runtime capability gap)
- **Audit 2026-06-21**: URL parsing and HTTP-oriented runtime pieces exist, but the runtime header still marks full socket support as planned. Raw socket APIs remain unimplemented.
- **Resolution 2026-08-09**: Added libuv-backed TCP creation, DNS-aware connect, IPv4 bind/listen/accept, byte-slice send, Buffer receive, configurable operation timeouts, TLS client connect with optional peer verification, TLS server configuration from PEM certificate/key files, error reporting, retain/release, and close operations. Protocol-level event-loop callbacks remain a future extension.
