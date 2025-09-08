#include "libhttp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#if defined(_WIN32)
#define IsValidSocket(s) ((s) != INVALID_SOCKET)
#define CloseSocket(s) closesocket(s)
#else
#define IsValidSocket(s) ((s) >= 0)
#define CloseSocket(s) close(s)
#endif /* _WIN32 */

#define MAXREQUEST 4097
#define MAXBUFFER 2048
#define MAXRESPONSE 98567
#define RES_TIMEOUT 6.0

enum response_state
{
    HTTP_RESPONSE_YES,
    HTTP_RESPONSE_NO,
    HTTP_RESPONSE_NO_BODY
};

enum connection_protocol
{
    HTTP = 1,
    HTTPS,
};

// Structure holding http proxy connection info
struct http_proxy
{
    char *url;
    char *hostname;
    char *port;
    int http_send_request_flag;
};

/* structure containing the connection information  */
struct http_conn_info
{
    char *headers;
    char *url;
    char *hostname;
    char *port;
    char *path;
    char *query;
    char *user_agent;
    char *connection;
    char *include_headers;
    char *content_type;
    char req_headers[MAXREQUEST];
    char post_body[MAXREQUEST];
    char put_body[MAXREQUEST];
    char patch_body[MAXREQUEST];
    char cookies[MAXBUFFER];
    int res_timeout;
    int http2InUse;
    int max_redirect;
    int c_redirect_num;
    enum http_version version;
    enum http_requests method;
    enum http_redirects redirects;
    struct http_proxy proxy;
};

struct http_response
{
    char *headers;
    char body[MAXRESPONSE];
    char *status_code;
    enum response_state state;
};

struct openssl_
{
    SSL *ssl;
    SSL *proxy_ssl;
    int alpn_h2_negotiated;
    enum http_tls_version version;
    char *cert_subject;
    char *cert_issuer;
    int is_printed;
};

/* The structure representing the HTTP session*/
struct http_session_struct
{
    struct http_conn_info connection;
    struct http_response response;
    struct openssl_ ssl;
    enum connection_protocol flag;
    enum connection_protocol proxy_flag;
    enum http_verbosity verbose;
    const char *error_msg;
    int error_code;
    int connected;
    int proxy_connected;
    HTTPSOCKET socket;
    HTTPSOCKET proxy_socket;
    struct http_session_struct *next;
    FILE *lfp;
};

// Set error msg
void __set_error_msg(http_session http, const char *str_err, ...)
{

    char formated[1024];
    va_list arg;
    va_start(arg, str_err);
    vsprintf(formated, str_err, arg);
    va_end(arg);

    http->error_msg = strdup(formated);
}

/* Creating a new session */
http_session http_new()
{

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
#ifdef _WIN32
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d))
        return NULL;
#endif
    http_session n;
    n = (http_session)calloc(1, sizeof(struct http_session_struct));
    if (!n)
        return NULL;
    memset(n, 0, sizeof(struct http_session_struct));
    n->error_code = HTTP_SUCCESS;
    __set_error_msg(n, "Success");
    return n;
}

void lfprintf(http_session http, const char *buffer, ...)
{

    char buf[2089];
    va_list arg;
    va_start(arg, buffer);
    vsprintf(buf, buffer, arg);
    va_end(arg);

    fprintf(stdout, "%s", buf);

    if (http->lfp != NULL)
        fprintf(http->lfp, "%s", buf);
}
const char *libhttp_get_version(void)
{
    char v[10];
    sprintf(v, "%d.%d.%d", LIBHTTP_VERSION_MINOR,
            LIBHTTP_VERSION_MAJOR, LIBHTTP_VERSION_MACRO);

    return strdup(v);
}
/* Copy http_session from src to dest*/
void http_options_copy(http_session dest, http_session src)
{

    memcpy(dest, src, sizeof(struct http_session_struct));
    memset(&dest->response, 0, sizeof dest->response);
}

void http_options_clear(http_session http)
{
    memset(http, 0, sizeof(struct http_session_struct));
}
// free alocated resource
void http_free(http_session http) { free(http); }

// Shutdown connection
void http_disconnect(http_session http)
{
    if (http->flag == HTTPS)
    {
        SSL_shutdown(http->ssl.ssl);
        SSL_free(http->ssl.ssl);
    }
#ifdef _WIN32
    WSACleanup();
#endif
    CloseSocket(http->socket);
}

// Shutting down proxy connections
void http_proxy_disconnect(http_session http)
{
    if (http->proxy_flag == HTTPS)
    {
        SSL_shutdown(http->ssl.ssl);
        SSL_free(http->ssl.ssl);
    }
#ifdef _WIN32
    WSACleanup();
#endif
    CloseSocket(http->proxy_socket);
}

// Parsing URL
void __parse_url(http_session http, char **hostname, char **port, char **path, char **query)
{

    int flag = 0;
    char *p;
    *path = "";
    if (strstr(http->connection.url, "https://"))
    {
        flag = 1;
        http->flag = HTTPS;
        p = strdup(strstr(http->connection.url, "https://"));
        p += 8;
    }
    else if (strstr(http->connection.url, "http://"))
    {
        p = strdup(strstr(http->connection.url, "http://"));
        p += 7;
        http->flag = HTTP;
    }
    else
    {
        return;
    }

    *hostname = p;

    while (*p && *p != ':' && *p != '/' && *p != '#')
        *p++;

    if (flag)
        *port = "443";
    else
        *port = "80";

    if (*p == ':')
    {
        *p++ = 0;
        *port = p;
    }

    while (*p && *p != '/' && *p != '#')
        *p++;
    if (*p == '/')
    {
        *p++ = 0;
        *path = p;
    }

    while (*p && *p != '#')
        *p++;
    if (*p == '#')
    {
        *p++ = 0;
    }

    char *k;
    if (strstr(http->connection.url, "?"))
    {
        k = strdup(strstr(http->connection.url, "?"));
        k += 1;
        *query = k;

        while (*k && *k != '#')
            *k++;
        if (*k == '#')
            *k++ = 0;
    }
}

// Parsing proxy URL
void __parse_proxy_url(http_session http, char **hostname, char **port)
{

    int flag = 0;
    char *p;
    if (strstr(http->connection.proxy.url, "https://"))
    {
        flag = 1;
        http->proxy_flag = HTTPS;
        p = strdup(strstr(http->connection.proxy.url, "https://"));
        p += 8;
    }
    else if (strstr(http->connection.proxy.url, "http://"))
    {
        p = strdup(strstr(http->connection.proxy.url, "http://"));
        p += 7;
        http->proxy_flag = HTTP;
    }
    else
    {
        return;
    }

    *hostname = p;

    while (*p && *p != ':' && *p != '/' && *p != '#')
        *p++;

    if (flag)
        *port = "443";
    else
        *port = "80";

    if (*p == ':')
    {
        *p++ = 0;
        *port = p;
    }

    // remove the hash and query information
    // since they're not needed in http connect request method
    while (*p && *p != '#')
        *p++;
    if (*p == '#')
    {
        *p++ = 0;
    }

    while (*p && *p != '?')
        *p++;
    if (*p == '?')
    {
        *p++ = 0;
    }
}

// Frees an allocated resource by a libhttp function
void LIBHTTP_free(void *target)
{
    free(target);
}

size_t url_encode_size(const char *str)
{
    size_t len = 0;
    for (int i = 0; i < strlen(str); i++)
    {
        if (
            ('a' <= str[i] && str[i] <= 'z') ||
            ('A' <= str[i] && str[i] <= 'Z') ||
            ('0' <= str[i] && str[i] <= '9'))
        {
            len += 1;
        }
        else
        {
            len += 3;
        }
    }

    return len;
}

char *http_url_encode(const char *str, size_t str_len)
{

    char *encoded = (char *)malloc(sizeof(char) * str_len * 3 + 1);
    const char *hex = "0123456789ABCDEF";
    int pos = 0;

    for (int i = 0; i < strlen(str); i++)
    {
        if (

            ('a' <= str[i] && str[i] <= 'z') ||
            ('A' <= str[i] && str[i] <= 'Z') ||
            ('0' <= str[i] && str[i] <= '9'))
        {
            encoded[pos++] = str[i];
        }
        else
        {
            encoded[pos++] = '%';
            encoded[pos++] = hex[str[i] >> 4];
            encoded[pos++] = hex[str[i] & 15];
        }
    }

    encoded[pos] = '\0';
    return encoded;
}

// Returns the size needed to encode a string into base64
size_t http_bs64encode_size(size_t str_len)
{

    size_t ret;
    ret = str_len;
    if (str_len % 3 != 0)
        ret += 3 - (str_len % 3);
    ret /= 3;
    ret *= 4;

    return ret;
}

// Checks weather or not a string is a valid base64 encoding
int http_is_valid_base64(unsigned char *base64_str, size_t base64_str_len)
{

    for (size_t i = 0; i < base64_str_len; i++)
    {
        if (
            (base64_str[i] >= 'A' && base64_str[i] <= 'Z') ||
            (base64_str[i] >= 'a' && base64_str[i] <= 'z') ||
            (base64_str[i] >= '0' && base64_str[i] <= '9') ||
            (base64_str[i] == '=') || (base64_str[i] == '+'))
        {
            continue;
        }
        else
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief encodes str to base64
 * @returns an encoded base64 string
 * Must call LIBHTTP_free() to free allocated memory resource
 */
char *http_base64_encode(unsigned char *str, size_t str_len)
{

    if (str == NULL || str_len == 0)
        return NULL;

    const char *bs64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t encode_len = http_bs64encode_size(str_len);
    char *encoded = (char *)malloc(sizeof(char) * encode_len + 1);
    size_t val, i, j, count = 0, index = 0, tmp, no_of_bits = 0, padding = 0, pos = 0;

    for (i = 0; i < str_len; i += 3)
    {

        val = 0, count = 0, no_of_bits = 0;
        for (j = i; j < str_len && j <= i + 2; j++)
        {
            val = val << 8;
            val = val | str[j];
            count++;
        }

        no_of_bits = count * 8;
        padding = no_of_bits % 3;

        while (no_of_bits != 0)
        {

            if (no_of_bits >= 6)
            {
                tmp = no_of_bits - 6;

                index = (val >> tmp) & 0x3F;
                no_of_bits -= 6;
            }
            else
            {
                tmp = 6 - no_of_bits;
                index = (val << tmp) & 0x3F;
                no_of_bits = 0;
            }
            encoded[pos++] = bs64chars[index];
        }
    }

    for (i = 1; i <= padding; i++)
    {
        encoded[pos++] = '=';
    }
    encoded[pos] = '\0';

    return encoded;
}

/** Get system error describtion
 *   @returns an error describtion
 */
const char *__get_error_msg()
{

#if defined(_WIN32)
    static char message[256];
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, WSAGetLastError(), 0, message, 256, 0);
    char *nl = strrchr(message, '\n');
    if (nl)
        *nl = 0;
    return message;
#else
    return strerror(errno);
#endif
}

// setting session options
int http_options_set(http_session http,
                     enum http_options option, const void *value)
{

    char *tmp;
    const int *val;
    if (option != HTTP_OPTIONS_LOGGING_FP)
    {
        tmp = (char *)value;
        val = *((int const **)value);
    }

    if (option == HTTP_OPTIONS_POST_BODY_FILE || option == HTTP_OPTIONS_PUT_BODY_FILE ||
        option == HTTP_OPTIONS_PATCH_BODY_FILE || option == HTTP_OPTIONS_LOAD_COOKIES_FILE)
    {
        FILE *fp;
        fp = fopen(tmp, "r");
        if (fp == NULL)
        {
            __set_error_msg(http, "%s", __get_error_msg());
            http->error_code = errno;
            return HTTP_ERROR;
        }
        char buf[2048];
        int r = fread(buf, 1, 2048, fp);

        while (r)
        {

            if (option == HTTP_OPTIONS_POST_BODY_FILE)
            {
                sprintf(http->connection.post_body +
                            strlen(http->connection.post_body),
                        "%s", buf);
            }
            else if (option == HTTP_OPTIONS_PATCH_BODY_FILE)
            {
                sprintf(http->connection.patch_body +
                            strlen(http->connection.patch_body),
                        "%s", buf);
            }
            else if (option == HTTP_OPTIONS_LOAD_COOKIES_FILE)
            {

                // replace all newline characters with a backfeed
                char *ptr = buf;
                while (1)
                {
                    while (*ptr && *ptr != '\n')
                        *ptr++;
                    if (*ptr == '\n')
                    {
                        *ptr = '\b';
                    }
                    else
                    {
                        break;
                    }
                }
                sprintf(http->connection.cookies +
                            strlen(http->connection.cookies),
                        "%s", buf);
            }
            else
            {
                sprintf(http->connection.put_body +
                            strlen(http->connection.put_body),
                        "%s", buf);
            }
            r = fread(buf, 1, 2048, fp);
        }
        fflush(fp);
        fclose(fp);
        return HTTP_OK;
    }
    switch (option)
    {
    case HTTP_OPTIONS_URL:
        http->connection.url = tmp;
        __parse_url(http, &http->connection.hostname,
                    &http->connection.port, &http->connection.path, &http->connection.query);
        if (!http->connection.hostname || !http->connection.port)
        {
            __set_error_msg(http, "Invalid URL");
            http->error_code = HTTP_INVALID_URL;
            return HTTP_ERROR;
        }
        break;
    case HTTP_OPTIONS_HOSTNAME:
        http->connection.hostname = strdup(tmp);
        break;
    case HTTP_OPTIONS_PORT:
        http->connection.port = strdup(tmp);
        break;
    case HTTP_OPTIONS_PATH:
        http->connection.path = strdup(tmp);
        break;
    case HTTP_OPTIONS_QUERY:
        http->connection.query = strdup(tmp);
        break;
    case HTTP_OPTIONS_REQUEST_METHOD:
        http->connection.method = (enum http_requests)val;
        break;
    case HTTP_OPTIONS_HEADERS:
        http->connection.headers = strdup(tmp);
        break;
    case HTTP_OPTIONS_CONTENT_TYPE_HEADER:
        http->connection.content_type = strdup(tmp);
        break;
    case HTTP_OPTIONS_HTTP_VERSION:
        http->connection.version = (enum http_version)val;
        break;
    case HTTP_OPTIONS_TLS_VERSION:
        http->ssl.version = (enum http_tls_version)val;
        break;
    case HTTP_OPTIONS_REDIRECTS:
        http->connection.redirects = (enum http_redirects)val;
        break;
    case HTTP_OPTIONS_MAX_REDIRECT:
        http->connection.max_redirect = *val;
        break;
    case HTTP_OPTIONS_RESPONSE_TIMEOUT:
        http->connection.res_timeout = *val;
        // Handle
        break;
    case HTTP_OPTIONS_POST_BODY:
        sprintf(http->connection.post_body, "%s", tmp);
        break;
    case HTTP_OPTIONS_PATCH_BODY:
        sprintf(http->connection.patch_body, "%s", tmp);
        break;
    case HTTP_OPTIONS_LOAD_COOKIES:
        sprintf(http->connection.cookies, "%s", tmp);
        break;
    case HTTP_OPTIONS_PUT_BODY:
        sprintf(http->connection.put_body, "%s", tmp);
        break;
    case HTTP_OPTIONS_VERBOSITY:
        http->verbose = (enum http_verbosity)val;
        break;
    case HTTP_OPTIONS_LOGGING_FP:
        http->lfp = (FILE *)value;
        break;
    case HTTP_OPTIONS_PROXY_URL:
        http->connection.proxy.url = strdup(tmp);
        __parse_proxy_url(http, &http->connection.proxy.hostname,
                          &http->connection.proxy.port);
        if (!http->connection.proxy.hostname || !http->connection.proxy.port)
        {
            __set_error_msg(http, "Invalid URL");
            http->error_code = HTTP_INVALID_URL;
            return HTTP_ERROR;
        }
        break;
    case HTTP_OPTIONS_PROXY_HOSTNAME:
        http->connection.proxy.hostname = strdup(tmp);
        break;
    case HTTP_OPTIONS_PROXY_PORT:
        http->connection.proxy.port = strdup(tmp);
        break;
    case HTTP_OPTIONS_HEADERS_INCLUDE:
        http->connection.include_headers = strdup(tmp);
        break;
    case HTTP_OPTIONS_USER_AGENT:
        http->connection.user_agent = strdup(tmp);
        break;
    case HTTP_OPTIONS_CONNECTION_HEADER:
        http->connection.connection = strdup(tmp);
        break;
    default:
        return HTTP_ERROR;
    }

    return HTTP_OK;
}

// Get an HTTP option and write it to value
int http_options_get(http_session http, enum http_options option, char **value)
{

    switch (option)
    {
    case HTTP_OPTIONS_URL:
        *value = http->connection.url;
        break;
    case HTTP_OPTIONS_HOSTNAME:
        *value = http->connection.hostname;
        break;
    case HTTP_OPTIONS_PORT:
        *value = http->connection.port;
        break;
    case HTTP_OPTIONS_PATH:
        *value = http->connection.path;
        break;
    case HTTP_OPTIONS_QUERY:
        *value = http->connection.query;
        break;
    case HTTP_OPTIONS_HEADERS:
        *value = http->connection.headers;
        break;
    case HTTP_OPTIONS_POST_BODY:
        *value = http->connection.post_body;
        break;
    case HTTP_OPTIONS_PATCH_BODY:
        *value = http->connection.patch_body;
        break;
    case HTTP_OPTIONS_CONTENT_TYPE_HEADER:
        *value = http->connection.content_type;
        break;
    case HTTP_OPTIONS_LOAD_COOKIES:
        *value = http->connection.cookies;
        break;
    case HTTP_OPTIONS_PUT_BODY:
        *value = http->connection.put_body;
        break;
    case HTTP_OPTIONS_PROXY_URL:
        *value = http->connection.proxy.url;
        break;
    case HTTP_OPTIONS_PROXY_HOSTNAME:
        *value = http->connection.proxy.hostname;
        break;
    case HTTP_OPTIONS_PROXY_PORT:
        *value = http->connection.proxy.port;
        break;
    default:
        return HTTP_ERROR;
    }

    return HTTP_OK;
}

// Get request method
int http_options_get_request_method(http_session http)
{
    if (http->connection.method < 1)
        return HTTP_GET;
    else
        return http->connection.method;
}

// Get http version
int http_options_get_http_version(http_session http)
{
    if (http->connection.version < 1)
        return HTTP_1_1;
    else
        return http->connection.version;
}
int http_get_error_code(http_session http)
{
    return http->error_code;
};
// Get tls version
int http_options_get_tls_version(http_session http)
{
    return http->ssl.version;
}

// Retrieve the HTTP connection socket
HTTPSOCKET http_get_fd(http_session http)
{
    return http->socket;
}

// Retrieve the HTTP proxy socket
HTTPSOCKET http_get_proxy_fd(http_session http)
{
    return http->proxy_socket;
}

void __request_http2(http_session http)
{

    switch (http->connection.method)
    {
    case HTTP_GET:
        sprintf(http->connection.req_headers, "GET /%s HTTP/1.1\r\n", http->connection.path);
        break;

    case HTTP_POST:
        sprintf(http->connection.req_headers, "POST /%s HTTP/1.1\r\n", http->connection.path);
        break;
    case HTTP_PUT:
        sprintf(http->connection.req_headers, "PUT /%s HTTP/1.1\r\n", http->connection.path);
        break;
    case HTTP_PATCH:
        sprintf(http->connection.req_headers, "PATCH /%s HTTP/1.1\r\n", http->connection.path);
        break;
    case HTTP_HEAD:
        sprintf(http->connection.req_headers, "HEAD /%s HTTP/1.1\r\n", http->connection.path);
        break;
    case HTTP_OPTIONS:
        sprintf(http->connection.req_headers, "OPTIONS /%s HTTP/1.1\r\n", http->connection.path);
        break;
    case HTTP_DELETE:
        sprintf(http->connection.req_headers, "DELETE /%s HTTP/1.1\r\n", http->connection.path);
        break;
    case HTTP_TRACE:
        sprintf(http->connection.req_headers, "TRACE /%s HTTP/1.1\r\n", http->connection.path);
        break;
    default:
        sprintf(http->connection.req_headers, "GET /%s HTTP/1.1\r\n", http->connection.path);
        break;
    }

    // If headers are already provided by the user
    if (http->connection.headers != NULL)
    {
        sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "%s", http->connection.headers);
    }
    else
    {

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "Host: %s:%s\r\n",
                http->connection.hostname, http->connection.port);

        if (http->connection.user_agent)
            sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "User-Agent: %s\r\n", http->connection.user_agent);
        else

            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "User-Agent: libhttp(nshell)/%s\r\n", libhttp_get_version());

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                "Connection: Upgrade, HTTP2-Settings\r\n");

        if (http->flag == HTTPS)
            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "Upgrade: h2\r\n");
        else
            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "Upgrade: h2c\r\n");

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                "HTTP2-Settings: AAMAAABkAAQCAAAAAAIAAAAA\r\n");
        sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                "accept: */*\r\n");

        if (http->connection.include_headers)
        {

            if (strstr(http->connection.include_headers, "\n"))
            {

                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "%s\r\n", http->connection.include_headers);
            }
            else
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "%s\r\n", http->connection.include_headers);
        }

        if (strlen(http->connection.cookies) > 1)

            sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "Cookies: %s\r\n",
                    http->connection.cookies);

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "\r\n");
    }
}

void __construct_h2_request_headers(http_session http)
{

    switch (http->connection.method)
    {
    case HTTP_GET:
        sprintf(http->connection.req_headers, "GET /%s HTTP/2\r\n", http->connection.path);
        break;

    case HTTP_POST:
        sprintf(http->connection.req_headers, "POST /%s HTTP/2\r\n", http->connection.path);
        break;
    case HTTP_PUT:
        sprintf(http->connection.req_headers, "PUT /%s HTTP/2\r\n", http->connection.path);
        break;
    case HTTP_PATCH:
        sprintf(http->connection.req_headers, "PATCH /%s HTTP/2\r\n", http->connection.path);
        break;
    case HTTP_HEAD:
        sprintf(http->connection.req_headers, "HEAD /%s HTTP/2\r\n", http->connection.path);
        break;
    case HTTP_OPTIONS:
        sprintf(http->connection.req_headers, "OPTIONS /%s HTTP/2\r\n", http->connection.path);
        break;
    case HTTP_DELETE:
        sprintf(http->connection.req_headers, "DELETE /%s HTTP/2\r\n", http->connection.path);
        break;
    case HTTP_TRACE:
        sprintf(http->connection.req_headers, "TRACE /%s HTTP/2\r\n", http->connection.path);
        break;
    default:
        sprintf(http->connection.req_headers, "GET /%s HTTP/2\r\n", http->connection.path);
        break;
    }

    // If headers are already provided by the user
    if (http->connection.headers != NULL)
    {
        sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "%s", http->connection.headers);
    }
    else
    {

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "Host: %s:%s\r\n",
                http->connection.hostname, http->connection.port);

        if (http->connection.user_agent)
            sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "user-agent: %s\r\n", http->connection.user_agent);
        else

            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "user-agent: libhttp(nshell)/%s\r\n", libhttp_get_version());

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                "connection: close\r\n");

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                "accept: */*\r\n");

        if (http->connection.include_headers)
        {

            if (strstr(http->connection.include_headers, "\n"))
            {

                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "%s\r\n", http->connection.include_headers);
            }
            else
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "%s\r\n", http->connection.include_headers);
        }

        if (strlen(http->connection.cookies) > 1)

            sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "cookies: %s\r\n",
                    http->connection.cookies);

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "\r\n");
    }
}

// Construct the request headers
void __construct_request_headers(http_session http)
{

    if (http->connection.version == HTTP_2 && !http->connection.http2InUse)
    {
        __request_http2(http);
        return;
    }
    switch (http->connection.method)
    {
    case HTTP_GET:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "GET /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "GET /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "GET /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "GET /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    case HTTP_POST:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "POST /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "POST /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "POST /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "POST /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    case HTTP_PUT:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "PUT /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "PUT /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "PUT /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "PUT /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    case HTTP_HEAD:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "HEAD /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "HEAD /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "HEAD /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "HEAD /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    case HTTP_DELETE:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "DELETE /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "DELETE /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "DELETE /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "DELETE /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    case HTTP_TRACE:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "TRACE /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "TRACE /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "TRACE /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "TRACE /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    case HTTP_PATCH:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "PATCH /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "PATCH /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "PATCH /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "PATCH /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    case HTTP_OPTIONS:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "OPTIONS /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "OPTIONS /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "OPTIONS /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "OPTIONS /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    default:
        switch (http->connection.version)
        {
        case HTTP_1_0:
            sprintf(http->connection.req_headers, "GET /%s HTTP/1.0\r\n", http->connection.path);
            break;
        case HTTP_1_1:
            sprintf(http->connection.req_headers, "GET /%s HTTP/1.1\r\n", http->connection.path);
            break;
        case HTTP_2:
            sprintf(http->connection.req_headers, "GET /%s HTTP/2\r\n", http->connection.path);
            break;
        default:
            sprintf(http->connection.req_headers, "GET /%s HTTP/1.1\r\n", http->connection.path);
            break;
        }
        break;
    }
    // If headers are already provided by the user
    if (http->connection.headers != NULL)
    {
        sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "%s", http->connection.headers);
    }
    else
    {
        sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "Host: %s:%s\r\n",
                http->connection.hostname, http->connection.port);

        if (http->connection.user_agent)
            sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "User-Agent: %s\r\n", http->connection.user_agent);

        else
            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "User-Agent: libhttp(nshell)/%s\r\n", libhttp_get_version());

        sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                "Accept: */*\r\n");

        if (http->connection.include_headers)
        {

            if (strstr(http->connection.include_headers, "\n"))
            {

                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "%s\r\n", http->connection.include_headers);
            }
            else
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "%s\r\n", http->connection.include_headers);
        }

        if (strlen(http->connection.cookies) > 1)

            sprintf(http->connection.req_headers + strlen(http->connection.req_headers), "Cookies: %s\r\n",
                    http->connection.cookies);

        if (http->connection.connection)
            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "Connection: %s\r\n", http->connection.connection);

        else
            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "Connection: close\r\n");

        // Is a POST request
        if (http->connection.method == HTTP_POST)
        {

            // if content type is already provided
            if (http->connection.content_type)
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "Content-Type: %s\r\n", http->connection.content_type);
            else
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "Content-Type: application/x-www-form-urlencoded\r\n");

            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "Content-Length: %d\r\n", strlen(http->connection.post_body));
        }
        else if (http->connection.method == HTTP_PUT)
        {
            // if content type is already provided
            if (http->connection.content_type)
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "Content-Type: %s\r\n", http->connection.content_type);
            else
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "Content-Type: text/html\r\n");

            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "Content-Length: %d\r\n", strlen(http->connection.put_body));
        }
        else if (http->connection.method == HTTP_PATCH)
        {
            // if content type is already provided
            if (http->connection.content_type)
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "Content-Type: %s\r\n", http->connection.content_type);
            else
                sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                        "Content-Type: text/html\r\n");

            sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
                    "Content-Length: %d\r\n", strlen(http->connection.patch_body));
        }
    }
    sprintf(http->connection.req_headers + strlen(http->connection.req_headers),
            "\r\n");
}

// Send the requests to the server
int __send_request_headers(http_session http, int flag)
{

    // memset(http->connection.req_headers, 0, sizeof(MAXREQUEST));
    // *http->connection.req_headers = NULL;
    __construct_request_headers(http);
    int bytes_sent = 0;
    if (flag)
    {
        switch (http->proxy_flag)
        {
        case HTTP:
            bytes_sent = send(http->proxy_socket, http->connection.req_headers,
                              strlen(http->connection.req_headers), 0);
            break;
        case HTTPS:
            bytes_sent = SSL_write(http->ssl.proxy_ssl, http->connection.req_headers,
                                   strlen(http->connection.req_headers));
            break;

        default:
            bytes_sent = send(http->proxy_socket, http->connection.req_headers,
                              strlen(http->connection.req_headers), 0);
            break;
        }
    }
    else
    {
        switch (http->flag)
        {
        case HTTP:
            bytes_sent = send(http->socket, http->connection.req_headers,
                              strlen(http->connection.req_headers), 0);
            break;
        case HTTPS:
            bytes_sent = SSL_write(http->ssl.ssl, http->connection.req_headers,
                                   strlen(http->connection.req_headers));
            break;

        default:
            bytes_sent = send(http->socket, http->connection.req_headers,
                              strlen(http->connection.req_headers), 0);
            break;
        }
    }
    if (bytes_sent < 1)
    {
        __set_error_msg(http, "unfinished request, connection reset by peer\n");
        http->error_code = HTTP_CONNECTION_RESET;
        return HTTP_ERROR;
    }
    if (http->verbose == HTTP_VERBOSITY_ENABLE)
    {
        // printf("%s\n", http->connection.req_headers);
        char *q = strdup(http->connection.req_headers);
        fprintf(stdout, "<| ");
        while (*q)
        {
            if (*q == '\n')
            {
                fprintf(stdout, "\n<| ");
                *q++;
            }
            else
            {
                fprintf(stdout, "%c", *q);
                *q++;
            }
        }
        fprintf(stdout, "\n");
        if (http->lfp != NULL)
            fprintf(http->lfp, "%s", http->connection.req_headers);
    }
    return HTTP_OK;
}

// To send body request of POST, PUT nad PATCH
int __send_request_body(http_session http, int flag)
{

    int bytes_sent = 0;
    char data[MAXREQUEST];

    switch (http->connection.method)
    {

    case HTTP_POST:
        strcpy(data, http->connection.post_body);
        if (flag)
        {
            switch (http->proxy_flag)
            {
            case HTTP:
                bytes_sent = send(http->proxy_socket, http->connection.post_body,
                                  strlen(http->connection.post_body), 0);
                break;
            case HTTPS:
                bytes_sent = SSL_write(http->ssl.proxy_ssl, http->connection.post_body,
                                       strlen(http->connection.post_body));
                break;

            default:
                bytes_sent = send(http->proxy_socket, http->connection.post_body,
                                  strlen(http->connection.post_body), 0);
                break;
            }
        }
        else
        {
            switch (http->flag)
            {

            case HTTP:
                bytes_sent = send(http->socket, http->connection.post_body,
                                  strlen(http->connection.post_body), 0);
                break;
            case HTTPS:
                bytes_sent = SSL_write(http->ssl.ssl, http->connection.post_body,
                                       strlen(http->connection.post_body));
                break;

            default:
                bytes_sent = send(http->socket, http->connection.post_body,
                                  strlen(http->connection.post_body), 0);
                break;
            }
        }
        break;
    case HTTP_PUT:
        strcpy(data, http->connection.put_body);
        if (flag)
        {
            switch (http->flag)
            {
            case HTTP:
                bytes_sent = send(http->proxy_socket, http->connection.put_body,
                                  strlen(http->connection.put_body), 0);
                break;
            case HTTPS:
                bytes_sent = SSL_write(http->ssl.proxy_ssl, http->connection.put_body,
                                       strlen(http->connection.put_body));
                break;

            default:
                bytes_sent = send(http->proxy_socket, http->connection.put_body,
                                  strlen(http->connection.put_body), 0);
                break;
            }
        }
        else
        {
            switch (http->flag)
            {
            case HTTP:
                bytes_sent = send(http->socket, http->connection.put_body,
                                  strlen(http->connection.put_body), 0);
                break;
            case HTTPS:
                bytes_sent = SSL_write(http->ssl.ssl, http->connection.put_body,
                                       strlen(http->connection.put_body));
                break;

            default:
                bytes_sent = send(http->socket, http->connection.put_body,
                                  strlen(http->connection.put_body), 0);
                break;
            }
        }
        break;
    case HTTP_PATCH:
        strcpy(data, http->connection.patch_body);
        if (flag)
        {
            switch (http->flag)
            {
            case HTTP:
                bytes_sent = send(http->proxy_socket, http->connection.patch_body,
                                  strlen(http->connection.patch_body), 0);
                break;
            case HTTPS:
                bytes_sent = SSL_write(http->ssl.proxy_ssl, http->connection.patch_body,
                                       strlen(http->connection.patch_body));
                break;

            default:
                bytes_sent = send(http->proxy_socket, http->connection.patch_body,
                                  strlen(http->connection.patch_body), 0);
                break;
            }
        }
        else
        {
            switch (http->flag)
            {
            case HTTP:
                bytes_sent = send(http->socket, http->connection.patch_body,
                                  strlen(http->connection.patch_body), 0);
                break;
            case HTTPS:
                bytes_sent = SSL_write(http->ssl.ssl, http->connection.patch_body,
                                       strlen(http->connection.patch_body));
                break;

            default:
                bytes_sent = send(http->socket, http->connection.patch_body,
                                  strlen(http->connection.patch_body), 0);
                break;
            }
        }
        break;
    default:
        return HTTP_ERROR;
    } // switch

    if (http->verbose == HTTP_VERBOSITY_ENABLE)
    {

        char *q = strdup(data);
        fprintf(stdout, "<| ");
        while (*q)
        {
            if (*q == '\n')
            {
                fprintf(stdout, "\n<| ");
                *q++;
            }
            else
            {
                fprintf(stdout, "%c", *q);
                *q++;
            }
        }
        fprintf(stdout, "\n");
        if (http->lfp != NULL)
            fprintf(http->lfp, "%s", data);
    }

    return HTTP_OK;
}

// Get status code
int http_get_status_code(http_session http)
{

    char *res = strdup(http->response.headers);
    if (res == NULL)
        return HTTP_OK;
    char *hdr, *code;
    if (strstr(res, "HTTP/1") || strstr(res, "HTTP/2"))
    {

        if (strstr(res, "HTTP/1.0"))
            hdr = strdup(strstr(res, "HTTP/1.0"));
        if (strstr(res, "HTTP/1.1"))
            hdr = strdup(strstr(res, "HTTP/1.1"));
        if (strstr(res, "HTTP/2.0"))
            hdr = strdup(strstr(res, "HTTP/2.0"));
        hdr += 9;
        code = hdr;
        while (*hdr && *hdr < 0 && *hdr > 9)
            *hdr++;
        if (*hdr < 0 && *hdr > 9)
            *hdr = 0;
        return atoi(code);
    }
    return HTTP_OK;
}
// Get the http response headers
const char *http_get_headers(http_session http)
{
    return http->response.headers;
}
// Get the http response body
const char *http_get_body(http_session http)
{
    return http->response.body;
}

// Get a specific header field value
const char *http_get_header(http_session http, const char *header_name)
{

    // Removing extra spaces(Just incase)
    char *q = strdup(header_name);
    char *k = q;
    for (; *q != '\0'; *q++)
        if (*q == ' ')
            *q++ = '\b';

    char *p;
    char tmp[100], *ans;
    sprintf(tmp, "\n%s: ", k);

    if (!http->response.headers)
    {
        return NULL;
    }
    if (strstr(http->response.headers, tmp))
    {

        p = strdup(strstr(http->response.headers, tmp));
        if (!p)
            return NULL;

        p = strchr(p, ' ');
        p += 1;

        ans = strtok(p, "\r\n");
    }
    else
        return NULL;
    return ans;
}

// Get an error describtion
const char *http_get_error(http_session http)
{
    return http->error_msg;
}

// Retrive the server certificate subject name
const char *http_get_certificate_subject(http_session http)
{
    return http->ssl.cert_subject;
}

const char *http_get_certificate_issuer(http_session http)
{
    return http->ssl.cert_issuer;
}

// HTTP connect request
int __http_connect(http_session http)
{

    char *buffer = (char *)malloc(sizeof(char) + MAXREQUEST);
    switch (http->connection.version)
    {
    case HTTP_1_0:
        sprintf(buffer, "CONNECT %s:%s HTTP/1.0\r\n", http->connection.hostname, http->connection.port);
        break;
    case HTTP_1_1:
        sprintf(buffer, "CONNECT %s:%s HTTP/1.1\r\n", http->connection.hostname, http->connection.port);
        break;
    case HTTP_2:
        sprintf(buffer, "CONNECT %s:%s HTTP/2.0\r\n", http->connection.hostname, http->connection.port);
        break;
    default:
        sprintf(buffer, "CONNECT %s:%s HTTP/1.1\r\n", http->connection.hostname, http->connection.port);
        break;
    }

    // If headers are already provided by the user
    if (http->connection.headers != NULL)
    {
        sprintf(buffer + strlen(buffer), "%s", http->connection.headers);
    }
    else
    {

        sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", http->connection.hostname,
                http->connection.port);

        sprintf(buffer + strlen(buffer), "User-Agent: libhttp(nshell)/%s\r\n", libhttp_get_version());

        if (strlen(http->connection.cookies) > 1)
            sprintf(buffer + strlen(buffer), "Cookies: %s\r\n", http->connection.cookies);

        sprintf(buffer + strlen(buffer), "Connection: keep-alive\r\n\r\n");
    }

    int bytes_sent = 0;
    switch (http->proxy_flag)
    {
    case HTTP:
        bytes_sent = send(http->proxy_socket, buffer, strlen(buffer), 0);
        break;
    case HTTPS:
        bytes_sent = SSL_write(http->ssl.proxy_ssl, buffer, strlen(buffer));
        break;

    default:
        bytes_sent = send(http->socket, buffer, strlen(buffer), 0);
        break;
    }

    if (bytes_sent < 1)
    {
        __set_error_msg(http, "Unfinished request, connection reset by peer\n");
        http->error_code = HTTP_CONNECTION_RESET;
        return HTTP_ERROR;
    }

    if (http->verbose == HTTP_VERBOSITY_ENABLE)
    {

        char *q = strdup(buffer);
        fprintf(stdout, "<| ");
        while (*q)
        {
            if (*q == '\n')
            {
                fprintf(stdout, "\n<| ");
                *q++;
            }
            else
            {
                fprintf(stdout, "%c", *q);
                *q++;
            }
        }
        fprintf(stdout, "\n");
        if (http->lfp != NULL)
            fprintf(http->lfp, "%s", buffer);
    }
    free(buffer);
    return HTTP_OK;
}

// Establising connection
int http_connect(http_session http)
{

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints)); // Zero out structure

    hints.ai_family = AF_UNSPEC;     // Address family
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if (!http->connection.url && !http->connection.hostname &&
        !http->connection.port)
    {
        __set_error_msg(http, "No connection URL\n");
        http->error_code = HTTP_NO_URL;
        return HTTP_ERROR;
    }

    struct addrinfo *peer_addr, *rp;
    if (getaddrinfo(http->connection.hostname, http->connection.port,
                    &hints, &peer_addr))
    {
        __set_error_msg(http, "%s", __get_error_msg());
        http->error_code = errno;
        return HTTP_ERROR;
    }

    HTTPSOCKET s;
    int connected = 0;
    char address[100];
    // Connect to each address on the host until one connects successfully
    for (rp = peer_addr; rp != NULL; rp = rp->ai_next)
    {
        if (http->verbose == 1)
        {
            memset(address, 0, sizeof(address));
            getnameinfo(rp->ai_addr, rp->ai_addrlen, address, 100, 0, 0, NI_NUMERICHOST);
            lfprintf(http, "** Trying %s:%s..\n", address, http->connection.port);
        }
        s = socket(rp->ai_family, rp->ai_socktype,
                   rp->ai_protocol);
        if (!IsValidSocket(s))
            continue;

        if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            if (http->verbose == 1)
                lfprintf(http, "** Connected to #%s (%s) #port (%s)\n", http->connection.hostname,
                         address, http->connection.port);
            connected = 1;
            break;
        }
        CloseSocket(s);
    }
    if (!connected)
    {
        __set_error_msg(http, "%s", __get_error_msg());
        http->error_code = errno;
        return HTTP_ERROR;
    }

    http->connected = 1;
    freeaddrinfo(peer_addr);
    http->socket = s;
    if (!IsValidSocket(s))
    {
        __set_error_msg(http, "%s", __get_error_msg());
        http->error_code = errno;
        return HTTP_ERROR;
    }
    if (http->flag == HTTPS)
    {
        SSL *ssl_tmp;
        SSL_CTX *ctx;
        const SSL_METHOD *method;
        switch (http->ssl.version)
        {
        case HTTP_TLS_1_0:
            method = TLSv1_client_method();
            break;
        case HTTP_TLS_1_1:
            method = TLSv1_1_client_method();
            break;
        case HTTP_TLS_1_2:
            method = TLSv1_2_client_method();
            break;
        case HTTP_TLS_1_3:
            method = TLS_client_method();
            break;
        default:
            method = TLS_client_method();
            break;
        }
        ctx = SSL_CTX_new(method);
        // SSL_CTX_set_options(ctx, SSL_CTX_set_timeout(ctx, 0));
        if (!ctx)
        {
            __set_error_msg(http, "SSL error\n");
            http->error_code = HTTP_SSL_ERROR;
            return HTTP_ERROR;
        }
        /**
         * If the user requested to use http/2, then we use
         * SSL_CTX to set the type of protocol we want to negotiate with the server
         */
        if (http->connection.version == HTTP_2)
        {
            if (SSL_CTX_set_alpn_protos(ctx, (const unsigned char *)"\x02h2", 3) != 0)
            {
                __set_error_msg(http, "OpenSSL, ALPN: failed to set protocol 'h2'");
                http->error_code = HTTP_SSL_ERROR;
                return HTTP_ERROR;
            }
        }

        ssl_tmp = SSL_new(ctx);
        if (!ssl_tmp)
        {
            __set_error_msg(http, "SSL error");
            http->error_code = HTTP_SSL_ERROR;
            SSL_CTX_free(ctx);
            return HTTP_ERROR;
        }

        if (!SSL_set_tlsext_host_name(ssl_tmp, http->connection.hostname))
        {
            SSL_CTX_free(ctx);
            SSL_free(ssl_tmp);
            return HTTP_ERROR;
        }
        // SSL_set_timeout(http->ssl.ssl, 5);
        // printf("Timeout %d\n", SSL_get_timeout(http->ssl.ssl));
        SSL_set_fd(ssl_tmp, http->socket);
        if (SSL_connect(ssl_tmp) == -1)
        {
            SSL_CTX_free(ctx);
            SSL_free(ssl_tmp);
            __set_error_msg(http, "Failed to establish SSL/TLS connection");
            http->error_code = HTTP_SSL_CONN_FAILED;
            return HTTP_ERROR;
        }
        http->ssl.ssl = ssl_tmp;

        /**
         * Connection established, check if the user requested to use http/2
         * and that Openssl has negotaited with the server
         */
        if (http->connection.version == HTTP_2)
        {
            const unsigned char *alpn = NULL;
            int alpnlen = 0;

#ifndef OPENSSL_NO_NEXTPROTONEG
            SSL_get0_next_proto_negotiated(http->ssl.ssl, &alpn, &alpnlen);
#endif

            if (alpn == NULL)
            {
                SSL_get0_alpn_selected(http->ssl.ssl, &alpn, &alpnlen);
            }

            if (alpn == NULL || alpnlen == 0 || memcmp("h2", alpn, 2) != 0)
            {
                if (http->verbose == 1)
                {
                    lfprintf(http, "** ALPN: server does'nt accept to use h2, switching to http/1.1\n");
                }
            }
            else
            {
                http->ssl.alpn_h2_negotiated = 1;
                if (http->verbose == 1)
                {
                    lfprintf(http, "** ALPN: server accepted to use h2\n");
                }
                int val = 1;
                setsockopt(http->socket, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val));
                /**
                 * Hex value of:
                 *  PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n
                 *  0x505249202a20485454502f322e300d0a0d0a534d0d0a0d0a
                 * An empty settings frame:
                 *  0x000000040000000000
                 */
                const char *client_preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
                const char *settings_frame = "0x000000040000000000";
                char read[1024];
                SSL_write(http->ssl.ssl, client_preface, strlen(client_preface));
                SSL_write(http->ssl.ssl, settings_frame, strlen(settings_frame));
                int n = SSL_read(http->ssl.ssl, read, 1024);
                if (n < 1)
                {
                    printf("Connection closed\n");
                    exit(0);
                }
                printf("** Server preface: %.*s\n", n, read);
            }
        }
        // Get the server certificate
        X509 *cert = SSL_get_peer_certificate(http->ssl.ssl);
        char *subject, *issuer;
        if ((subject = strdup(X509_NAME_oneline(X509_get_subject_name(cert), 0, 0))) != NULL)
        {
            http->ssl.cert_subject = strdup(subject);
            OPENSSL_free(subject);
        }

        if ((issuer = strdup(X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0))) != NULL)
        {
            http->ssl.cert_issuer = strdup(issuer);
            OPENSSL_free(issuer);
        }
        long verify = SSL_get_verify_result(http->ssl.ssl);
        if (http->verbose == 1 && !http->ssl.is_printed)
        {
            lfprintf(http, "** certificate subject: %s\n", http_get_certificate_subject(http));
            lfprintf(http, "** certificate issuer: %s\n", http_get_certificate_issuer(http));

            if (verify == X509_V_OK)
                lfprintf(http, "** certificate verification successful\n");
            else
                lfprintf(http, "** certificate verification failed\n");
            http->ssl.is_printed = 1;
        }
        // unsigned char vectors[] = {
        //     11, 'h', 't', 't', 'p', '/', '2'
        // };
        // unsigned int len = sizeof(vectors);
    }
    return HTTP_OK;
}

// Connect to a proxy server
int http_proxy_connect(http_session http)
{

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints)); // Zero out structure

    hints.ai_family = AF_UNSPEC;     // Address family
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if (!http->connection.proxy.url && !http->connection.proxy.hostname &&
        !http->connection.proxy.port)
    {
        __set_error_msg(http, "No connection URL\n");
        http->error_code = HTTP_NO_URL;
        return HTTP_ERROR;
    }

    struct addrinfo *peer_addr, *rp;
    if (getaddrinfo(http->connection.proxy.hostname, http->connection.proxy.port,
                    &hints, &peer_addr))
    {
        __set_error_msg(http, "%s", __get_error_msg());
        http->error_code = errno;
        return HTTP_ERROR;
    }

    HTTPSOCKET s;
    int connected = 0;
    char address[100];
    // Connect to each address on the host until one connects successfully
    for (rp = peer_addr; rp != NULL; rp = rp->ai_next)
    {
        if (http->verbose == 1)
        {
            memset(address, 0, sizeof(address));
            getnameinfo(rp->ai_addr, rp->ai_addrlen, address, 100, 0, 0, NI_NUMERICHOST);
            lfprintf(http, "** Trying %s:%s..\n", address, http->connection.proxy.port);
        }
        s = socket(rp->ai_family, rp->ai_socktype,
                   rp->ai_protocol);
        if (!IsValidSocket(s))
            continue;

        if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            if (http->verbose == 1)
                lfprintf(http, "** Connected to #%s (%s) #port (%s)\n", http->connection.proxy.hostname,
                         address, http->connection.proxy.port);
            connected = 1;
            break;
        }
        CloseSocket(s);
    }
    if (!connected)
    {
        __set_error_msg(http, "%s", __get_error_msg());
        http->error_code = errno;
        return HTTP_ERROR;
    }

    http->proxy_connected = 1;
    freeaddrinfo(peer_addr);
    http->proxy_socket = s;
    if (!IsValidSocket(s))
    {
        __set_error_msg(http, "%s", __get_error_msg());
        http->error_code = errno;
        return HTTP_ERROR;
    }
    if (http->proxy_flag == HTTPS)
    {
        SSL *ssl_tmp;
        SSL_CTX *ctx;
        const SSL_METHOD *method;
        switch (http->ssl.version)
        {
        case HTTP_TLS_1_0:
            method = TLSv1_client_method();
            break;
        case HTTP_TLS_1_1:
            method = TLSv1_1_client_method();
            break;
        case HTTP_TLS_1_2:
            method = TLSv1_2_client_method();
            break;
        case HTTP_TLS_1_3:
            method = TLS_client_method();
            break;
        default:
            method = TLS_client_method();
            break;
        }
        ctx = SSL_CTX_new(method);
        // SSL_CTX_set_options(ctx, SSL_CTX_set_timeout(ctx, 0));
        if (!ctx)
        {
            __set_error_msg(http, "SSL error\n");
            http->error_code = HTTP_SSL_ERROR;
            return HTTP_ERROR;
        }
        /**
         * If the user requested to use http/2, then we use
         * SSL_CTX to set the type of protocol we want to negotiate with the server
         */
        if (http->connection.version == HTTP_2)
        {
            if (SSL_CTX_set_alpn_protos(ctx, (const unsigned char *)"\x02h2", 3) != 0)
            {
                __set_error_msg(http, "OpenSSL, ALPN: failed to set protocol 'h2'");
                http->error_code = HTTP_SSL_ERROR;
                return HTTP_ERROR;
            }
        }

        ssl_tmp = SSL_new(ctx);
        if (!ssl_tmp)
        {
            __set_error_msg(http, "SSL error");
            http->error_code = HTTP_SSL_ERROR;
            SSL_CTX_free(ctx);
            return HTTP_ERROR;
        }

        if (!SSL_set_tlsext_host_name(ssl_tmp, http->connection.proxy.hostname))
        {
            SSL_CTX_free(ctx);
            SSL_free(ssl_tmp);
            return HTTP_ERROR;
        }
        // SSL_set_timeout(http->ssl.ssl, 5);
        // printf("Timeout %d\n", SSL_get_timeout(http->ssl.ssl));
        SSL_set_fd(ssl_tmp, http->proxy_socket);
        if (SSL_connect(ssl_tmp) == -1)
        {
            SSL_CTX_free(ctx);
            SSL_free(ssl_tmp);
            __set_error_msg(http, "Failed to establish SSL/TLS connection");
            http->error_code = HTTP_SSL_CONN_FAILED;
            return HTTP_ERROR;
        }
        http->ssl.proxy_ssl = ssl_tmp;

        /**
         * Connection established, check if the user requested to use http/2
         * and that Openssl has negotaited with the server
         */
        if (http->connection.version == HTTP_2)
        {
            const unsigned char *alpn = NULL;
            int alpnlen = 0;

#ifndef OPENSSL_NO_NEXTPROTONEG
            SSL_get0_next_proto_negotiated(http->ssl.proxy_ssl, &alpn, &alpnlen);
#endif

            if (alpn == NULL)
            {
                SSL_get0_alpn_selected(http->ssl.proxy_ssl, &alpn, &alpnlen);
            }

            if (alpn == NULL || alpnlen == 0 || memcmp("h2", alpn, 2) != 0)
            {
                if (http->verbose == 1)
                {
                    lfprintf(http, "** ALPN: server does'nt accept to use h2, switching to http/1.1\n");
                }
            }
            else
            {
                http->ssl.alpn_h2_negotiated = 1;
                if (http->verbose == 1)
                {
                    lfprintf(http, "** ALPN: server accepted to use h2\n");
                }
                const char *client_preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
                char read[1024];
                SSL_write(http->ssl.proxy_ssl, client_preface, strlen(client_preface));
                // SSL_write(http->ssl.ssl, "HTTP2-Settings: ENABLE_PUSH", 21);
                int n = SSL_read(http->ssl.proxy_ssl, read, 1024);
                if (n < 1)
                {
                    printf("Connection closed\n");
                    exit(0);
                }
                printf("** Server preface: %.*s\n", n, read);
            }
        }
        // Get the server certificate
        X509 *cert = SSL_get_peer_certificate(http->ssl.proxy_ssl);
        char *subject, *issuer;
        if ((subject = strdup(X509_NAME_oneline(X509_get_subject_name(cert), 0, 0))) != NULL)
        {
            http->ssl.cert_subject = strdup(subject);
            OPENSSL_free(subject);
        }

        if ((issuer = strdup(X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0))) != NULL)
        {
            http->ssl.cert_issuer = strdup(issuer);
            OPENSSL_free(issuer);
        }
        long verify = SSL_get_verify_result(http->ssl.proxy_ssl);
        if (http->verbose == 1)
        {
            lfprintf(http, "** certificate subject: %s\n", http_get_certificate_subject(http));
            lfprintf(http, "** certificate issuer: %s\n", http_get_certificate_issuer(http));

            if (verify == X509_V_OK)
                lfprintf(http, "** certificate verification successful\n");
            else
                lfprintf(http, "** certificate verification failed\n");
        }
        // unsigned char vectors[] = {
        //     11, 'h', 't', 't', 'p', '/', '2'
        // };
        // unsigned int len = sizeof(vectors);
    }
    return HTTP_OK;
}

// Following redirects
int __follow_redirect__(http_session http, char *location)
{

    http->connection.url = location;
    http->connection.hostname = "";
    http->connection.port = "";
    http->connection.path = "";
    http->connection.query = "";

    __parse_url(http, &http->connection.hostname, &http->connection.port,
                &http->connection.path, &http->connection.query);
    if (http_connect(http) != HTTP_OK)
    {
        return HTTP_ERROR;
    }
    if (http_session_start(http) != HTTP_OK)
        return HTTP_ERROR;
    http->connection.c_redirect_num += 1;
    return HTTP_OK;
}

// following redirects for proxy connection

int __follow_redirect_proxy__(http_session http, char *location)
{

    http->connection.url = location;
    http->connection.hostname = "";
    http->connection.port = "";
    http->connection.path = "";
    http->connection.query = "";

    __parse_url(http, &http->connection.hostname, &http->connection.port,
                &http->connection.path, &http->connection.query);

    if (http_proxy_session_start(http) != HTTP_OK)
        return HTTP_ERROR;
    http->connection.c_redirect_num += 1;
    return HTTP_OK;
}

// Wait for proxy server response
int __wait_proxy_response(http_session http)
{

    char response[MAXRESPONSE + 1];
    char *p = response, *q;
    char *end = response + MAXRESPONSE;
    char *body = 0;

    enum
    {
        length,
        chunked,
        connection
    };
    int encoding = 0;
    int remaining = 0;

    // time_t start;
    // struct tm *start_tm, *end_tm;
    // start = time(0);
    // start_tm = localtime(&start);
    // const int start_sec = start_tm->tm_sec;
    while (1)
    {

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(http->proxy_socket, &reads);
        struct timeval timeout;
        timeout.tv_sec = http->connection.res_timeout >= 1 ? http->connection.res_timeout : RES_TIMEOUT;
        timeout.tv_usec = 0;

        if (select(http->proxy_socket + 1, &reads, 0, 0, &timeout) < 0)
        {
            __set_error_msg(http, "a call to select() failed");
            return HTTP_ERROR;
        }

        if (FD_ISSET(http->proxy_socket, &reads))
        {
            int bytes_received = 0;
            if (http->proxy_flag == HTTPS)
            {
                bytes_received = SSL_read(http->ssl.proxy_ssl, p, end - p);
            }
            else
            {
                bytes_received = recv(http->proxy_socket, p, end - p, 0);
            }
            if (bytes_received < 1)
            {
                if (encoding == connection && body)
                {
                    sprintf(http->response.body + strlen(http->response.body),
                            "%.*s", (int)(end - body), body);
                }
                __set_error_msg(http, "connection closed by peer");
                http->error_code = HTTP_CONNECTION_RESET;
                break;
            }
            p += bytes_received;
            *p = 0;
            // printf("%s", response);
            if (!body && strstr(response, "\r\n\r\n"))
            {

                body = strdup(strstr(response, "\r\n\r\n"));
                *body = 0;
                body += 4;
                char *headers = strdup(response);
                char *k;
                if ((k = strstr(headers, "\r\n\r\n")))
                {
                    *k = 0;
                    http->response.headers = strdup(headers);
                }
                if (http->connection.redirects != HTTP_REDIRECTS_DISALLOW && http->connection.max_redirect >= 1 && http->connection.c_redirect_num <= http->connection.max_redirect)
                {

                    if (strstr(headers, "\nLocation: "))
                    {

                        char *x = strdup(http_get_header(http, "Location"));
                        if (http->verbose == 1)
                        {

                            char *o = strdup(headers);
                            fprintf(stdout, "<| ");
                            while (*o)
                            {
                                if (*o == '\n')
                                {
                                    fprintf(stdout, "\n<| ");
                                    *o++;
                                }
                                else
                                {
                                    fprintf(stdout, "%c", *o);
                                    *o++;
                                }
                            }
                            fprintf(stdout, "\n");
                            if (http->lfp != NULL)
                                fprintf(http->lfp, "%s", headers);
                            lfprintf(http, "** Following %s ...\n", x);
                        }
                        if (__follow_redirect_proxy__(http, x) != HTTP_OK)
                        {
                            return HTTP_ERROR;
                        }
                    }
                }
                if (strstr(response, "\nContent-Length: "))
                {
                    q = strdup(strstr(response, "\nContent-Length: "));
                    q = strchr(q, ' ');
                    q += 1;
                    encoding = length;
                    remaining = strtol(q, 0, 10);
                }
                else if (strstr(response, "\nTransfer-Encoding: chunked"))
                {
                    encoding = chunked;
                    remaining = 0;
                }
                else
                {
                    encoding = connection;
                }
            }
            if (body)
            {
                if (encoding == length)
                {
                    if (p - body >= remaining)
                    {
                        sprintf(http->response.body + strlen(http->response.body),
                                "%.*s", remaining, body);
                        break;
                    }
                }
                else if (encoding == chunked)
                {
                    do
                    {
                        if (remaining == 0)
                        {
                            if ((q = strstr(body, "\r\n")))
                            {
                                remaining = strtol(body, 0, 16);
                                if (!remaining)
                                    return HTTP_OK;
                                body = q + 2;
                            }
                            else
                                return HTTP_OK;
                        }
                        if (remaining && p - body >= remaining)
                        {
                            sprintf(http->response.body + strlen(http->response.body),
                                    "%.*s", remaining, body);
                            body += remaining + 2;
                            remaining = 0;
                        }
                    } while (!remaining);
                }
            } // if(body)
        } // if(FD_ISSET)
        else
        {
            int r = http->connection.res_timeout;
            __set_error_msg(http, "Response timed out after %.2fs", r >= 1 ? r : RES_TIMEOUT);
            http->error_code = HTTP_RES_TIMEOUT;
            return HTTP_ERROR;
        }
    } // while(1)

    return HTTP_OK;
}

// Sending http CONNECT request method to the server
int http_proxy_send_request(http_session http)
{

    if (!http->proxy_connected)
    {
        __set_error_msg(http, "Sockets ends not connected");
        http->error_code = HTTP_FD_NOT_CONNECTED;
        return HTTP_ERROR;
    }
    if (!http->connection.url && !http->connection.hostname && !http->connection.port)
    {
        __set_error_msg(http, "Proxy error: no target url");
        http->error_code = HTTP_PROXY_NO_URL;
        return HTTP_ERROR;
    }
    if (__http_connect(http) != HTTP_OK)
    {
        return HTTP_ERROR;
    }

    while (1)
    {
        fd_set proxy_fd;
        FD_ZERO(&proxy_fd);
        FD_SET(http->proxy_socket, &proxy_fd);

        struct timeval timeout;

        timeout.tv_sec = RES_TIMEOUT;
        timeout.tv_usec = 0;
        select(http->proxy_socket + 1, &proxy_fd, 0, 0, &timeout);
        if (FD_ISSET(http->proxy_socket, &proxy_fd))
        {
            char response[MAXRESPONSE];
            int bytes_received = 0;

            if (http->proxy_flag == HTTPS)
            {
                bytes_received = SSL_read(http->ssl.proxy_ssl, response, MAXRESPONSE);
            }
            else
            {
                bytes_received = recv(http->proxy_socket, response, MAXRESPONSE, 0);
            }
            if (bytes_received < 1)
            {
                __set_error_msg(http, "Connection closed by peer");
                http->error_code = HTTP_CONNECTION_RESET;
                return HTTP_ERROR;
            }
            http->response.headers = response;
            http->connection.proxy.http_send_request_flag = 1;
            break;
        }
        else
        {
            __set_error_msg(http, "Response timed out after %.2fs", RES_TIMEOUT);
            http->error_code = HTTP_RES_TIMEOUT;
            return HTTP_ERROR;
        }
    }

    return HTTP_OK;
}

// Starting a proxy session
int http_proxy_session_start(http_session http)
{

    if (!http->proxy_connected)
    {
        __set_error_msg(http, "Sockets ends not connected");
        http->error_code = HTTP_FD_NOT_CONNECTED;
        return HTTP_ERROR;
    }

    if (__send_request_headers(http, 1) != HTTP_OK)
    {
        return HTTP_ERROR;
    }

    enum http_requests r = http->connection.method;

    if (r == HTTP_POST || r == HTTP_PUT || r == HTTP_PATCH)
    {
        if (__send_request_body(http, 1) != HTTP_OK)
        {
            return HTTP_ERROR;
        }
    }

    // if (strlen(http->response.headers) >= 1)
    //     memset(http->response.headers, 0, sizeof(http->response.headers));

    // if (strlen(http->response.body) >= 1)
    //     memset(http->response.body, 0, sizeof(http->response.body));
    if (__wait_proxy_response(http) != HTTP_OK)
    {
        return HTTP_ERROR;
    }

    return HTTP_OK;
}

// Wait for server response
int __wait_response(http_session http)
{

    char response[MAXRESPONSE + 1];
    char *p = response, *q;
    char *end = response + MAXRESPONSE;
    char *body = 0;

    enum
    {
        length,
        chunked,
        connection
    };
    int encoding = 0;
    int remaining = 0;

    while (1)
    {
        fd_set reads;

        FD_ZERO(&reads);
        FD_SET(http->socket, &reads);
        struct timeval timeout;
        timeout.tv_sec = http->connection.res_timeout >= 1 ? http->connection.res_timeout : RES_TIMEOUT;
        timeout.tv_usec = 0;

        if (select(http->socket + 1, &reads, 0, 0, &timeout) < 0)
        {
            __set_error_msg(http, "a call to select() failed");
            return HTTP_ERROR;
        }

        if (FD_ISSET(http->socket, &reads))
        {
            int bytes_received;
            if (http->flag == HTTPS)
            {
                bytes_received = SSL_read(http->ssl.ssl, p, end - p);
            }
            else
            {
                bytes_received = recv(http->socket, p, end - p, 0);
            }
            if (bytes_received < 1)
            {
                if (encoding == connection && body)
                {
                    sprintf(http->response.body + strlen(http->response.body),
                            "%.*s", (int)(end - body), body);
                }
                __set_error_msg(http, "Connection closed by peer");
                http->error_code = HTTP_CONNECTION_RESET;
                break;
            }
            printf("%s", p);
            p += bytes_received;
            *p = 0;
            if (!body && strstr(response, "\r\n\r\n"))
            {

                body = strdup(strstr(response, "\r\n\r\n"));
                *body = 0;
                body += 4;
                char *headers = strdup(response);
                char *k;
                if ((k = strstr(headers, "\r\n\r\n")))
                {
                    *k = 0;
                    http->response.headers = strdup(headers);
                }
                if (http->connection.redirects != HTTP_REDIRECTS_DISALLOW)
                {

                    if (strstr(headers, "\nLocation: "))
                    {

                        http_disconnect(http);

                        char *x = strdup(http_get_header(http, "Location"));
                        if (http->verbose == 1)
                        {

                            char *o = strdup(headers);
                            fprintf(stdout, "<| ");
                            while (*o)
                            {
                                if (*o == '\n')
                                {
                                    fprintf(stdout, "\n<| ");
                                    *o++;
                                }
                                else
                                {
                                    fprintf(stdout, "%c", *o);
                                    *o++;
                                }
                            }
                            fprintf(stdout, "\n");
                            if (http->lfp != NULL)
                                fprintf(http->lfp, "%s", headers);
                            lfprintf(http, "** Following %s ...\n", x);
                        }
                        if (__follow_redirect__(http, x) != HTTP_OK)
                        {
                            return HTTP_ERROR;
                        }
                    }
                }
                if (strstr(response, "\nContent-Length: "))
                {
                    q = strdup(strstr(response, "\nContent-Length: "));
                    q = strchr(q, ' ');
                    q += 1;
                    encoding = length;
                    remaining = strtol(q, 0, 10);
                    // printf("Remaining: %d\n", remaining);
                }
                else if (strstr(response, "\nTransfer-Encoding: chunked"))
                {
                    encoding = chunked;
                    remaining = 0;
                }
                else
                {
                    encoding = connection;
                }
            }
            if (body)
            {
                if (encoding == length)
                {
                    // printf("p - body %d\n", (p - body));
                    if (p - body <= remaining)
                    {
                        sprintf(http->response.body + strlen(http->response.body),
                                "%.*s", remaining, body);
                        break;
                    }
                }
                else if (encoding == chunked)
                {
                    do
                    {
                        if (remaining == 0)
                        {
                            if ((q = strstr(body, "\r\n")))
                            {
                                remaining = strtol(body, 0, 16);
                                if (!remaining)
                                    return HTTP_OK;
                                body = q + 2;
                            }
                            else
                                return HTTP_OK;
                        }
                        if (remaining && p - body >= remaining)
                        {
                            sprintf(http->response.body + strlen(http->response.body),
                                    "%.*s", remaining, body);
                            body += remaining + 2;
                            remaining = 0;
                        }
                    } while (!remaining);
                }
            } // if(body)
        } // if(FD_ISSET)
        else
        {
            int r = http->connection.res_timeout;
            __set_error_msg(http, "Response timed out after %.2fs", r >= 1 ? r : RES_TIMEOUT);
            http->error_code = HTTP_RES_TIMEOUT;
            return HTTP_ERROR;
        }
    } // while(1)

    return HTTP_OK;
}

/**
 * Function for starting a http request session
 * @param http_session -> the http_session structure
 */
int http_session_start(http_session http)
{

    // Make sure the socket is connected to the server
    if (!http->connected)
    {
        __set_error_msg(http, "Sockets ends not connected");
        http->error_code = HTTP_FD_NOT_CONNECTED;
        return HTTP_ERROR;
    }

    // send the request headers
    if (__send_request_headers(http, 0) != HTTP_OK)
    {
        return HTTP_ERROR;
    }

    /**
     * Check to see if the request method is POST, PUT or PATCH
     * we need to check if the request body is available,
     * we need to send them
     */
    enum http_requests r = http->connection.method;
    if (r == HTTP_POST || r == HTTP_PUT || r == HTTP_PATCH)
    {
        if (__send_request_body(http, 0) != HTTP_OK)
        {
            return HTTP_ERROR;
        }
    }
    // if (strlen(http->response.headers) >= 1)
    //     memset(http->response.headers, 0, sizeof(http->response.headers));

    // if (strlen(http->response.body) >= 1)
    //     memset(http->response.headers, 0, sizeof(http->response.body));
    // wait for response
    if (__wait_response(http) != HTTP_OK)
    {
        return HTTP_ERROR;
    }
    return HTTP_OK;
}

/**
 * This helper function allows you to perform 2 tasks automatically
 * Without having to call http_connect(), and http_session_star() this
 * function automatically calls it for you to shorten your code
 * @param http_session -> the http_session structure
 */
int http_perform_req(http_session http)
{

    if (http_connect(http) != HTTP_OK)
    {
        return HTTP_ERROR;
    }

    if (http_session_start(http) != HTTP_OK)
    {
        return HTTP_ERROR;
    }

    return HTTP_OK;
};

/**
 * This helper function allows you to perform 2 tasks automatically
 * Without having to call http_proxy_connect(), and http_proxy_send_request() this
 * function automatically calls it for you to shorten your code
 * @param http_session -> the http_session structure
 */
int http_proxy_perform_req(http_session http)
{

    // perform a proxy connection
    if (http_proxy_connect(http) != HTTP_OK)
    {
        return HTTP_ERROR;
    }

    if (http_proxy_send_request(http) != HTTP_OK)
        return HTTP_ERROR;

    return HTTP_OK;
};

/**
 * This functions helps you write response(headers and body) to a file describtor
 * @param http_session -> The current http_session
 * @param fp -> A file describtor of where to print out the response
 */
void http_write_res_fp(http_session http, FILE *fp)
{

    if (http_get_headers(http) && http_get_body(http))
        fprintf(fp, "%s%s", http_get_headers(http), http_get_body(http));
}

/**
 * Prints out the response headers
 * @param http_session -> The current http_session
 * @param fp -> A file describtor of where to print out the response
 */
void http_write_res_headers_fp(http_session http, FILE *fp)
{

    if (http_get_headers(http))
        fprintf(fp, "%s", http_get_headers(http));
}

/**
 * Prints out the response body
 * @param http_session -> The current http_session
 * @param fp -> A file describtor of where to print out the response
 */
void http_write_res_body_fp(http_session http, FILE *fp)
{
    if (http_get_body(http))
        fprintf(fp, "%s", http_get_body(http));
}

/**
 * Prints out the response body with some stylish
 * @param http_session -> The current http_session
 * @param fp -> A file describtor of where to print out the response
 */

void http_write_res_body_fp_friendly(http_session http, FILE *fp)
{

    // copy the response body to tmp
    char *tmp = strdup(http_get_body(http));

    // if no response body, return the function
    if (!tmp)
        return;

    fprintf(fp, "|> ");

    while (*tmp)
    {
        if (*tmp == '\n')
        {
            fprintf(fp, "\n|> ");
            *tmp++;
        }
        else
        {
            fprintf(fp, "%c", *tmp);
            *tmp++;
        }
    }
    fprintf(fp, "\n");
}

/**
 * Prints out the response headers with some stylish
 * @param http_session -> The current http_session
 * @param fp -> A file describtor of where to print out the response
 */
void http_write_res_headers_fp_friendly(http_session http, FILE *fp)
{

    char *tmp = strdup(http_get_headers(http));

    if (!tmp)
        return;

    fprintf(fp, "|> ");

    while (*tmp)
    {
        if (*tmp == '\n')
        {
            fprintf(fp, "\n|> ");
            *tmp++;
        }
        else
        {
            fprintf(fp, "%c", *tmp);
            *tmp++;
        }
    }
    fprintf(fp, "\n");
}

/**
 * Prints out the response with some stylish
 * @param http_session -> The current http_session
 * @param fp -> A file describtor of where to print out the response
 */
void http_write_res_fp_friendly(http_session http, FILE *fp)
{

    char *tmp = strdup(http_get_headers(http));
    if (!tmp)
        return;
    fprintf(fp, "|> ");
    while (*tmp)
    {
        if (*tmp == '\n')
        {
            fprintf(fp, "\n|> ");
            *tmp++;
        }
        else
        {
            fprintf(fp, "%c", *tmp);
            *tmp++;
        }
    }
    fprintf(fp, "\n");

    char *q = strdup(http_get_body(http));
    if (!q)
        return;
    fprintf(fp, "|> ");
    while (*q)
    {
        if (*q == '\n')
        {
            fprintf(fp, "\n|> ");
            *q++;
        }
        else
        {
            fprintf(fp, "%c", *q);
            *q++;
        }
    }
    fprintf(fp, "\n");
}

/**
 *  COMING SOON
 */
int http_userauth_basic(http_session http, const char *username, const char *password)
{
}