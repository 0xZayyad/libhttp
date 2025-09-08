# libhttp (Beta)

A lightweight HTTP/HTTPS client library written in C with a modern C++ wrapper. It provides simple primitives to configure sessions, connect, and perform HTTP methods (GET, POST, PUT, PATCH, HEAD, OPTIONS, DELETE), with optional proxy support, TLS selection, redirects, and convenience helpers for writing responses and encoding. Also comes with a small curl-like client tool to demostrute usage (see below).

- **Status**: Beta
- **Languages**: C API + C++ wrapper (namespace `nsh`)
- **Platforms**: Linux, macOS, Windows

## Features
- **HTTP methods**: GET, POST, PUT, PATCH, HEAD, OPTIONS, DELETE, TRACE
- **TLS**: Select TLS 1.0–1.3 (where supported)
- **HTTP version**: HTTP/1.0, HTTP/1.1 (default), HTTP/2 (beta)
- **Proxy support**: HTTP CONNECT and proxied requests
- **Redirect control** with max-redirects
- **Headers and User-Agent** customization
- **Body sources**: inline or file for POST/PUT/PATCH
- **Cookie loading** from file
- **Timeouts and verbosity** options
- **Convenience I/O**: write full response, headers, or body to a FILE*
- **Encoding helpers**: URL-encode and Base64-encode

## Requirements
- C compiler (GCC/Clang on Unix, MSVC/MinGW on Windows)
- Standard sockets: POSIX (Unix) or Winsock2 (Windows)

## Building from source

### Unix-like (Linux/macOS)

The provided Makefile builds a static archive in `bin/`:

```bash
make
```
Outputs:
- `bin/libhttp.o` (object)
- `bin/libhttp.a` (static library)

To install system-wide (requires sudo), copy headers and archive to standard locations (adjust as needed):

```bash
sudo install -d /usr/local/include/libhttp /usr/local/lib
sudo install lib/libhttp.h lib/libhttp.hpp lib/libhttp_version.h /usr/local/include/libhttp
sudo install bin/libhttp.a /usr/local/lib
sudo ldconfig || true
```

To link in your project (GCC example):

```bash
gcc your_program.c -I/usr/local/include/libhttp -L/usr/local/lib -lhttp -o your_program
```

### Windows (MSVC)

- Open a Developer Command Prompt for VS.
- Compile the C library to a static library:

```bat
cl /c /EHsc /I lib lib\libhttp.c
lib /OUT:libhttp.lib libhttp.obj
```

- Include headers from `lib\` in your project, and link against `libhttp.lib`.
- For Winsock, add `Ws2_32.lib` to your linker inputs.

MSVC example link:

```bat
cl /EHsc /I lib your_program.c libhttp.lib Ws2_32.lib
```

### Windows (MinGW)

```bash
gcc -c -I lib lib/libhttp.c -o libhttp.o
ar rcs libhttp.a libhttp.o
# link with -lws2_32
gcc your_program.c -I lib -L . -lhttp -lws2_32 -o your_program.exe
```

## Quickstart

### C (basic GET)

```c
#include "libhttp.h"
#include <stdio.h>

int main() {
    http_session s = http_new();
    int method = HTTP_GET;
    http_options_set(s, HTTP_OPTIONS_URL, "http://example.com/");
    http_options_set(s, HTTP_OPTIONS_REQUEST_METHOD, &method);

    if (http_perform_req(s) != HTTP_OK) {
        fprintf(stderr, "error: %s\n", http_get_error(s));
        return 1;
    }

    http_write_res_fp_friendly(s, stdout);
    http_disconnect(s);
    http_free(s);
}
```

### C++ (exceptions enabled by default)

```cpp
#include "lib/libhttp.hpp"
#include <iostream>
using nsh::HTTPSession; using nsh::HTTPException;

int main() {
    try {
        HTTPSession http{"http://example.com"};
        http.connect();
        http.httpGet();
        http.writeResponseFriendly(stdout);
    } catch (HTTPException& e) {
        std::cerr << "Error: " << e.getError() << "\n";
        return 1;
    }
}
```

## API surface (high-level)

- C handle: `http_session`
- Core ops: `http_new`, `http_free`, `http_connect`, `http_session_start`, `http_perform_req`, `http_disconnect`
- Options: `http_options_set/get` with `enum http_options` (URL, HOSTNAME, PORT, PATH, QUERY, VERBOSITY, REQUEST_METHOD, HEADERS, USER_AGENT, CONNECTION_HEADER, PROXY_URL/HOSTNAME/PORT, HTTP_VERSION, TLS_VERSION, POST/PUT/PATCH bodies and files, COOKIES, REDIRECTS, CONTENT_TYPE, TIMEOUT, LOGGING_FP, MAX_REDIRECT)
- Response: `http_get_status_code`, `http_get_headers`, `http_get_body`, `http_get_header`
- Error and info: `http_get_error`, `http_get_error_code`, `libhttp_get_version`, certificate getters
- Helpers: `http_url_encode`, `http_base64_encode`
- C++ wrapper: `nsh::HTTPSession`, `nsh::HTTPProxySession`, optional `HTTP_NO_CPP_EXCEPTIONS`

## Examples
See `examples/c` and `examples/cpp` for more end-to-end samples, including proxy sessions and file output.

## Command-line client: httpc
A small curl-like client is included in `bin/httpc` after building.

- Build:
```bash
make httpc
```

- Usage:
```bash
bin/httpc [options] <url>
```

Common examples:
```bash
# Basic GET with headers printed
bin/httpc -i http://example.com

# POST JSON
bin/httpc -X POST --content-type application/json -d '{"name":"test"}' https://httpbin.org/post

# Download to file and dump headers
bin/httpc -o out.bin -D headers.txt https://example.com

# Use HTTP proxy and show only headers
bin/httpc -x http://127.0.0.1:8080 -I https://example.com

# Follow redirects with timeout
bin/httpc -L --max-redirs 5 --max-time 10 http://example.com
```

Refer to `docs/httpc.md` for a complete reference.

## License
TBD.
