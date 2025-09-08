# httpc — libhttp-based HTTP client

## NAME
httpc — command-line HTTP/HTTPS client using libhttp

## SYNOPSIS
```
httpc [options] <url>
```

## DESCRIPTION
httpc is a compact curl-like tool powered by libhttp. It supports common HTTP verbs, headers, proxying, redirects, timeouts, and file outputs. TLS and HTTP versions are configurable.

## OPTIONS
- -X, --request <METHOD>
  Specify request method: GET, POST, PUT, PATCH, DELETE, HEAD, OPTIONS, TRACE.

- -H, --header <LINE>
  Add request header, e.g. "Header: value". May be used multiple times.

- -A, --user-agent <UA>
  Set the User-Agent header value.

- -d, --data <DATA>
  Provide request body as a string. Defaults method to POST if unspecified.

- --data-file <FILE>
  Read request body from file. Works with POST, PUT, PATCH depending on method.

- --content-type <TYPE>
  Set Content-Type header (e.g., application/json).

- -x, --proxy <URL>
  Use HTTP proxy (e.g. http://127.0.0.1:8080).

- -o, --output <FILE>
  Write response body to FILE.

- -D, --dump-header <FILE>
  Write response headers to FILE.

- -i, --include
  Include response headers in stdout output (friendly format).

- -v, --verbose
  Enable verbose logging.

- -I, --head
  Fetch headers only (HEAD request).

- -L, --location
  Follow redirects.

- --max-redirs <N>
  Maximum number of redirects (default 10).

- --max-time <SEC>
  Response timeout in seconds.

- --http1.0 | --http1.1 | --http2
  Force HTTP protocol version.

- --tlsv1.0 | --tlsv1.1 | --tlsv1.2 | --tlsv1.3
  Force TLS protocol version.

- --fail
  Exit non-zero if HTTP status is >= 400.

## EXAMPLES
- Basic GET with headers printed:
```
httpc -i http://example.com
```

- POST JSON data:
```
httpc -X POST --content-type application/json -d '{"name":"test"}' https://httpbin.org/post
```

- Upload from file and save response:
```
httpc --data-file payload.bin --content-type application/octet-stream https://example.com/upload -o out.bin -D headers.txt
```

- Use HTTP proxy and show only headers:
```
httpc -x http://127.0.0.1:8080 -I https://example.com
```

- Follow redirects with timeout:
```
httpc -L --max-redirs 5 --max-time 10 http://example.com
```

- Force HTTP/2 over TLS 1.3:
```
httpc --http2 --tlsv1.3 https://example.com
```

## EXIT STATUS
- 0: Success
- 1: Runtime error (network/libhttp)
- 2: Usage or I/O error
- 22: --fail used and HTTP status >= 400

## SEE ALSO
libhttp C API (`docs/c/README.md`) and C++ wrapper (`docs/cpp/README.md`). 