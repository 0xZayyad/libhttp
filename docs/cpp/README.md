# libhttp C++ Wrapper (namespace `nsh`)

The C++ API provides a thin, RAII-style wrapper around the C library for convenient usage with exceptions and `std::string`.

- Header: `libhttp.hpp` (includes `libhttp.h`)
- Namespace: `nsh`
- Exceptions: enabled by default; define `HTTP_NO_CPP_EXCEPTIONS` to disable and receive `HTTP_OK/HTTP_ERROR` return codes.

## Classes

- `HTTPSession`
  - Holds an underlying `http_session` and manages its lifetime
  - Exposes request methods (GET/POST/PUT/PATCH/HEAD/OPTIONS/TRACE/DELETE)
  - Exposes option setters, connect, helpers, and response accessors

- `HTTPProxySession : public HTTPSession`
  - Specialized for proxy flows: `connect`, `proxySendRequest`, proxied request methods

- `HTTPException`
  - Thrown when a wrapped C call returns `HTTP_ERROR`
  - Inspect with `.getCode()` and `.getError()`

Macros controlling behavior:
- Default (exceptions):
  - `http_throw(e)` throws on error
  - Wrapped methods return `void`
- With `HTTP_NO_CPP_EXCEPTIONS` defined:
  - `http_throw(e)` returns `HTTP_ERROR`
  - Wrapped methods return `int` (`HTTP_OK` on success)

## HTTPSession API (selected)

Construction:
```cpp
HTTPSession();
HTTPSession(const char* url);
```

Configuration:
```cpp
http_throwable setOption(enum http_options option, const char* value);
http_throwable setOption(enum http_options option, long int value);
http_throwable setOption(enum http_options option, void* value);
void clearOptions();
void copyOptions(HTTPSession dest);
```

Connection and requests:
```cpp
http_throwable connect();
http_throwable performRequest(); // one-shot helper
http_throwable httpGet();
http_throwable httpPost();
http_throwable httpPut();
http_throwable httpPatch();
http_throwable httpHead();
http_throwable httpOptions();
http_throwable httpTrace();
http_throwable httpDelete();
void disconnect();
```

Response and info:
```cpp
int getStatusCode();
std::string getHeaders();
std::string getHeader(const char* name);
std::string getBody();
HTTPSOCKET getFd();
std::string getVersion();
std::string getCertificateSubjectName();
std::string getCertificateIssuerName();
int getErrorCode();
std::string getError();
```

Utilities:
```cpp
std::string urlEncode(char* str, int len);
std::string base64Encode(char* str, int len);
```

## HTTPProxySession API (selected)
```cpp
HTTPProxySession();
HTTPProxySession(const char* proxy_url);
http_throwable connect();
http_throwable proxySendRequest();
http_throwable performRequest();
http_throwable httpGet();
http_throwable httpPost();
http_throwable httpPut();
http_throwable httpPatch();
http_throwable httpHead();
http_throwable httpOptions();
http_throwable httpTrace();
http_throwable httpDelete();
void disconnect();
HTTPSOCKET getFd();
```

## Examples

### Basic GET with exceptions
```cpp
#include "lib/libhttp.hpp"
#include <iostream>
using namespace nsh;

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

### Disable exceptions
```cpp
#define HTTP_NO_CPP_EXCEPTIONS
#include "lib/libhttp.hpp"

int main() {
  nsh::HTTPSession http{"http://example.com"};
  if (http.connect() == HTTP_ERROR) return 1;
  if (http.httpGet() == HTTP_ERROR) return 1;
  http.writeResponseFriendly(stdout);
}
```

### Proxy usage
```cpp
#include "lib/libhttp.hpp"
using namespace nsh;

int main() {
  try {
    HTTPProxySession proxy{"http://127.0.0.1:8080"};
    proxy.setOption(HTTP_OPTIONS_URL, "http://example.com/");
    proxy.connect();                 // connect to proxy
    proxy.proxySendRequest();        // send CONNECT
    proxy.httpGet();                 // perform GET via proxy
    proxy.writeResponseFriendly(stdout);
  } catch (HTTPException& e) {
    fprintf(stderr, "%s\n", e.getError().c_str());
    return 1;
  }
}
```

## Common options from C
Use `setOption` with `enum http_options` values, e.g.:
- `HTTP_OPTIONS_URL`, `HTTP_OPTIONS_REQUEST_METHOD`
- `HTTP_OPTIONS_VERBOSITY` (`HTTP_VERBOSITY_ENABLE`/`DISABLE`)
- `HTTP_OPTIONS_TLS_VERSION` (`HTTP_TLS_1_0`..`HTTP_TLS_1_3`)
- `HTTP_OPTIONS_REDIRECTS` and `HTTP_OPTIONS_MAX_REDIRECT`
- `HTTP_OPTIONS_POST_BODY[_FILE]`, `HTTP_OPTIONS_PUT_BODY[_FILE]`, `HTTP_OPTIONS_PATCH_BODY[_FILE]`
- `HTTP_OPTIONS_HEADERS`, `HTTP_OPTIONS_HEADERS_INCLUDE`, `HTTP_OPTIONS_USER_AGENT`
- `HTTP_OPTIONS_PROXY_URL` (for `HTTPProxySession`)

## Notes (Beta)
- HTTP/2 is experimental; behavior may change.
- Some proxy method assignments in the wrapper may change (ensure correct mapping to `HTTP_OPTIONS_REQUEST_METHOD`). 