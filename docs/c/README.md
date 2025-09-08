# libhttp C API Documentation (Beta)

This document describes the C interface of libhttp: session management, configuration options, request execution, response access, and error handling.

## Headers and linking

- Include: `libhttp.h`
- Link: your program must link against `libhttp.a` (or `-lhttp`) and platform sockets if needed (Windows: `-lws2_32`).

## Core types

- `typedef struct http_session_struct *http_session;` opaque handle for a connection/session
- `HTTPSOCKET` connection file descriptor/handle (int on POSIX, SOCKET on Windows)

## Return codes and errors

- Success: `HTTP_OK` (0)
- Failure: `HTTP_ERROR` (-1)
- Get details: `int http_get_error_code(http_session)`, `const char* http_get_error(http_session)`

Selected error codes (`http_get_error_code`):
- `HTTP_INVALID_URL`, `HTTP_NO_URL`
- `HTTP_CONNECTION_RESET`, `HTTP_FD_NOT_CONNECTED`
- `HTTP_SSL_ERROR`, `HTTP_SSL_CONN_FAILED`, `HTTP_CERT_VP_FAILED`
- `HTTP_RES_TIMEOUT`

## HTTP status codes
Use `http_get_status_code(session)` to read the numeric status. Constants for common statuses are available in `enum http_status_code`.

## Session lifecycle

1. Create: `http_session s = http_new();`
2. Configure options: `http_options_set(s, HTTP_OPTIONS_..., value)`
3. Connect or perform: either:
   - Manual: `http_connect(s); http_session_start(s);`
   - Helper: `http_perform_req(s);`
4. Read response: headers/body/status
5. Disconnect: `http_disconnect(s);`
6. Free: `http_free(s);`

Helpers exist for proxy sessions: `http_proxy_connect`, `http_proxy_send_request`, `http_proxy_session_start`, `http_proxy_perform_req`, `http_proxy_disconnect`.

## Options
Set or get options via:
- `int http_options_set(http_session, enum http_options option, const void* value);`
- `int http_options_get(http_session, enum http_options option, char** value);`

Important options (`enum http_options`):
- Connection and URL parts: `HTTP_OPTIONS_URL`, `HTTP_OPTIONS_HOSTNAME`, `HTTP_OPTIONS_PORT`, `HTTP_OPTIONS_PATH`, `HTTP_OPTIONS_QUERY`
- Method and protocol: `HTTP_OPTIONS_REQUEST_METHOD` (`enum http_requests`), `HTTP_OPTIONS_HTTP_VERSION`, `HTTP_OPTIONS_TLS_VERSION`
- Headers: `HTTP_OPTIONS_HEADERS`, `HTTP_OPTIONS_HEADERS_INCLUDE`, `HTTP_OPTIONS_USER_AGENT`, `HTTP_OPTIONS_CONNECTION_HEADER`, `HTTP_OPTIONS_CONTENT_TYPE_HEADER`
- Bodies: `HTTP_OPTIONS_POST_BODY`, `HTTP_OPTIONS_POST_BODY_FILE`, `HTTP_OPTIONS_PUT_BODY`, `HTTP_OPTIONS_PUT_BODY_FILE`, `HTTP_OPTIONS_PATCH_BODY`, `HTTP_OPTIONS_PATCH_BODY_FILE`
- Cookies: `HTTP_OPTIONS_LOAD_COOKIES`, `HTTP_OPTIONS_LOAD_COOKIES_FILE`
- Redirects: `HTTP_OPTIONS_REDIRECTS` (`enum http_redirects`), `HTTP_OPTIONS_MAX_REDIRECT`
- Behavior: `HTTP_OPTIONS_VERBOSITY` (`enum http_verbosity`), `HTTP_OPTIONS_RESPONSE_TIMEOUT`, `HTTP_OPTIONS_LOGGING_FP`
- Proxy: `HTTP_OPTIONS_PROXY_URL`, `HTTP_OPTIONS_PROXY_HOSTNAME`, `HTTP_OPTIONS_PROXY_PORT`

Other utility functions:
- `void http_options_clear(http_session)` — clear all options
- `void http_options_copy(http_session dest, http_session src)` — copy options between sessions

## Performing requests

- Manual flow:
```c
http_connect(s);
http_session_start(s);
```
- Convenience:
```c
http_perform_req(s);
```
Set method with `HTTP_OPTIONS_REQUEST_METHOD` to one of:
`HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_HEAD, HTTP_OPTIONS, HTTP_DELETE, HTTP_TRACE`.

## Response access
- Status: `int http_get_status_code(s);`
- Headers (full): `const char* http_get_headers(s);`
- Header by name: `const char* http_get_header(s, "Content-Type");`
- Body: `const char* http_get_body(s);`
- Write helpers:
  - `http_write_res_fp(s, FILE*)`
  - `http_write_res_headers_fp(s, FILE*)`
  - `http_write_res_body_fp(s, FILE*)`
  - Friendly formatted variants: `http_write_res_fp_friendly`, `http_write_res_headers_fp_friendly`, `http_write_res_body_fp_friendly`

## Proxy sessions
- Connect: `http_proxy_connect(s);`
- Send CONNECT: `http_proxy_send_request(s);`
- Start request: `http_proxy_session_start(s);`
- One-shot helper: `http_proxy_perform_req(s);`
- Disconnect: `http_proxy_disconnect(s);`
- FD access: `HTTPSOCKET http_get_proxy_fd(s);`

## TLS and certificates
- Select TLS version with `HTTP_OPTIONS_TLS_VERSION` (`HTTP_TLS_1_0..HTTP_TLS_1_3`)
- Inspect peer certificate names: `http_get_certificate_subject(s)`, `http_get_certificate_issuer(s)`

## Utilities
- URL encode: `char* http_url_encode(const char* str, size_t len)` — returns allocated string; free with `LIBHTTP_free`
- Base64 encode: `char* http_base64_encode(unsigned char* str, size_t len)` — returns allocated string; free with `LIBHTTP_free`
- Encoded size helper: `size_t htttp_bs64encode_size(size_t len)`
- FD access: `HTTPSOCKET http_get_fd(s)`
- Library version: `const char* libhttp_get_version(void)`

## Examples

### Basic GET
```c
http_session s = http_new();
int method = HTTP_GET;
http_options_set(s, HTTP_OPTIONS_URL, "http://example.com/");
http_options_set(s, HTTP_OPTIONS_REQUEST_METHOD, &method);
if (http_perform_req(s) != HTTP_OK) {
    fprintf(stderr, "%s\n", http_get_error(s));
}
http_write_res_fp_friendly(s, stdout);
http_disconnect(s);
http_free(s);
```

### POST body from file
```c
int method = HTTP_POST;
http_options_set(s, HTTP_OPTIONS_URL, "https://example.com/upload");
http_options_set(s, HTTP_OPTIONS_REQUEST_METHOD, &method);
http_options_set(s, HTTP_OPTIONS_CONTENT_TYPE_HEADER, "application/octet-stream");
http_options_set(s, HTTP_OPTIONS_POST_BODY_FILE, "payload.bin");
http_perform_req(s);
```

### Via HTTP proxy
```c
http_options_set(s, HTTP_OPTIONS_PROXY_URL, "http://127.0.0.1:8080");
http_proxy_perform_req(s);
```

## Memory management
- Most getters return internal pointers valid for the lifetime of the session unless options are cleared. Do not free them.
- Functions returning allocated strings (`http_url_encode`, `http_base64_encode`) must be freed with `LIBHTTP_free`.

## Notes and limitations (Beta)
- HTTP/2 support is experimental.
- Authentication schemes are reserved; Basic may be added in future (`http_userauth_basic` placeholder). 