# ifndef _LIBHTTP_H
# define _LIBHTTP_H

# ifdef _WIN32
    # ifndef _WIN32_WINNT
        # define _WIN32_WINNT 0x0600
    #endif /* _WIN32_WINNT */
    #include <winsock2.h>
    #include <ws2tcpip.h>
# else /* _WIN32 */
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
# endif /* _WIN32 */

#include <stdio.h>
#include "libhttp_version.h"

# ifdef __cplusplus
    extern "C" {
# endif


/* HTTP authentication shemes available in libhttp*/

enum http_authentitcation_scheme {
    HTTP_AUTH_BASIC = 1,
    HTTP_AUTH_DIGEST,

};
/* The HTTP request methods*/
enum http_requests {
    HTTP_GET = 1,
    HTTP_POST,
    HTTP_PUT,
    HTTP_PATCH,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_DELETE,
    HTTP_TRACE,
};

/* Options */
enum http_options {
    HTTP_OPTIONS_URL = 1,               // URL 
    HTTP_OPTIONS_HOSTNAME,
    HTTP_OPTIONS_PORT,
    HTTP_OPTIONS_PATH,
    HTTP_OPTIONS_QUERY,
    HTTP_OPTIONS_VERBOSITY,         // Weather to enable to disable verbosity, type of(http_verbose)
    HTTP_OPTIONS_REQUEST_METHOD,    // HTTP request method, type of (enum http_requets)
    HTTP_OPTIONS_HEADERS,           // Request headers
    HTTP_OPTIONS_HEADERS_INCLUDE,   // Include a single or multiple headers to the default headers
    HTTP_OPTIONS_USER_AGENT,        // Change User-Agent header field
    HTTP_OPTIONS_CONNECTION_HEADER, // Change the Connection header field default(close) default for proxy connections: keep-alive
    HTTP_OPTIONS_PROXY_URL,         // A proxy URL to connect to. (Set the HTTP_OPTIONS_REQUEST_METHOD to CONNECT)
    HTTP_OPTIONS_PROXY_HOSTNAME,    // Set Proxy hostname
    HTTP_OPTIONS_PROXY_PORT,        // Proxy port
    HTTP_OPTIONS_HTTP_VERSION,      // Value must be of type (enum http_version)
    HTTP_OPTIONS_TLS_VERSION,       // Value must be of type (enum tls_version)  
    HTTP_OPTIONS_POST_BODY,         // POST request body
    HTTP_OPTIONS_POST_BODY_FILE,    // POST request body(from a file)
    HTTP_OPTIONS_PUT_BODY,          // PUT request body
    HTTP_OPTIONS_PUT_BODY_FILE,     // PUT request body(from a file)
    HTTP_OPTIONS_PATCH_BODY,        // PATCH request body
    HTTP_OPTIONS_PATCH_BODY_FILE,   // PATCH request body(from a file)
    HTTP_OPTIONS_LOAD_COOKIES,      // Use the provided cookies for this session
    HTTP_OPTIONS_LOAD_COOKIES_FILE, // Use the cookies found in the file
    HTTP_OPTIONS_REDIRECTS,         // Weather to follow redirections or not.default is HTTP_REDIRECTS_ALLOW
    /**
     * Set the Conten-Type header field
     * For POST requests, default is: x-www-form-urlencoded.
     * For PUT and PATCH requests, default is: text/html
    */
    HTTP_OPTIONS_CONTENT_TYPE_HEADER,      
    HTTP_OPTIONS_RESPONSE_TIMEOUT,
    HTTP_OPTIONS_LOGGING_FP,         // A file describtor to log all session
    HTTP_OPTIONS_MAX_REDIRECT        // Maximum redirects to follow  
};

/* HTTP proxy options */
// enum http_proxy_options {
    
// };


/* HTTP version */
enum http_version {
    HTTP_1_0 = 1,           // HTTP/1.0
    HTTP_1_1,               // HTTP/1.1 (default)
    HTTP_2                  // HTTP/2
};

/* SSL/TLS version */
enum http_tls_version {
    HTTP_TLS_1_0 = 1,            // TLSv1.0
    HTTP_TLS_1_1,                // TLSv1.1
    HTTP_TLS_1_2,                // TLSv1.2
    HTTP_TLS_1_3                 // TLSv1.3
};

/* HTTP redirections */
enum http_redirects {
    HTTP_REDIRECTS_ALLOW = 1,
    HTTP_REDIRECTS_DISALLOW,
};

/* Verbositiy */
enum http_verbosity {
    HTTP_VERBOSITY_ENABLE = 1,        // enable verbosity
    HTTP_VERBOSITY_DISABLE = -1       // disable verbosity( this is default)
};

# ifdef _WIN32
    typedef SOCKET HTTPSOCKET;
# else
    typedef int HTTPSOCKET;
# endif


typedef struct http_session_struct *http_session;

/**
  * @brief Allocate a new http_session strucutre
 * @returns a new http_session structure
 */
http_session http_new(void);
void http_free(http_session http);
void http_disconnect(http_session http);
void http_proxy_disconnect(http_session http);
void http_options_copy(http_session dest, http_session src);
void http_write_res_fp(http_session http, FILE *fp);
void http_write_res_body_fp(http_session http, FILE *fp);
void http_write_res_headers_fp(http_session http, FILE *fp);
void http_write_res_fp_friendly(http_session http, FILE *fp);
void http_write_res_headers_fp_friendly(http_session http, FILE *fp);
void http_write_res_body_fp_friendly(http_session http, FILE *fp);
void http_options_clear(http_session http);
void LIBHTTP_free(void *target);
int  http_userauth_basic(http_session http, const char *username, const char *password); // COMING SOON
int  http_options_set(http_session http,
            enum http_options option, const void *value);
int  http_options_get(http_session http, 
            enum http_options option, char **value);
int  http_options_get_request_method(http_session http);
int  http_options_get_http_version(http_session http);
int  http_options_get_tls_version(http_session http);
int  http_get_status_code(http_session http);
int  http_get_error_code(http_session http);
int  http_perform_req(http_session http);
int  http_proxy_perform_req(http_session http);
const char *http_get_headers(http_session http);
const char *http_get_body(http_session http);
const char *http_get_header(http_session http,
                            const char *header_name);
const char *libhttp_get_version(void);
char *http_url_encode(const char *str, size_t str_len);
char *http_base64_encode(unsigned char *str, size_t str_len);
size_t htttp_bs64encode_size(size_t str_len);
int http_is_valid_base64(unsigned char *base64_str, size_t base64_str_len);
HTTPSOCKET http_get_fd(http_session http);
HTTPSOCKET http_get_proxy_fd(http_session http);
int http_connect(http_session http);
int http_session_start(http_session http);
int http_proxy_connect(http_session http);
int http_proxy_send_request(http_session http);
int http_proxy_session_start(http_session http);
const char *http_get_error(http_session http);
const char *http_get_certificate_subject(http_session http);   
const char *http_get_certificate_issuer(http_session http);

# define HTTP_OK 0      // Success
# define HTTP_ERROR -1  //Error

// HTTP status codes
enum http_status_code {
    HTTP_STATUS_CONTINUE = 100,
    HTTP_STATUS_SWITCHING_PROTOCOL,
    HTTP_STATUS_PROCESSING,
    HTTP_STATUS_EARLY_HINTS,
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED,
    HTTP_STATUS_ACCEPTED,
    HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION,
    HTTP_STATUS_NO_CONTENT,
    HTTP_STATUS_RESET_CONTENT,
    HTTP_STATUS_PARTIAL_CONTENT,
    HTTP_STATUS_MULTI_STATUS,
    HTTP_STATUS_ALREADY_REPORTED,
    HTTP_STATUS_IM_USED = 226,
    HTTP_STATUS_MULTIPLE_CHOICES = 300,
    HTTP_STATUS_MOVE_PERMANENTLY,
    HTTP_STATUS_FOUND,
    HTTP_STATUS_SEE_OTHER,
    HTTP_STATUS_NOT_MODIFIED,
    HTTP_STATUS_USE_PROXY,
    HTTP_STATUS_SWITCH_PROXY,
    HTTP_STATUS_TEMPORARY_REDIRECT,
    HTTP_STATUS_PERMANENT_REDIRECT,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED,
    HTTP_STATUS_PAYMENT_REQUIRED,
    HTTP_STATUS_FORBIDDEN,
    HTTP_STATUS_NOT_FOUND,
    HTTP_STATUS_METHOD_NOT_ALLOWED,
    HTTP_STATUS_NOT_ACCEPTABLE,
    HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED,
    HTTP_STATUS_REQUEST_TIMEOUT,
    HTTP_STATUS_CONFLICT,
    HTTP_STATUS_GONE,
    HTTP_STATUS_LENGTH_REQUIRED,
    HTTP_STATUS_PRECONDITION_FAILED,
    HTTP_STATUS_PAYLOAD_TOO_LARGE,
    HTTP_STATUS_URI_TOO_LONG,
    HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
    HTTP_STATUS_RANGE_NOT_SATISFIABLE,
    HTTP_STATUS_EXPECTATION_FAILED,
    HTTP_STATUS_IM_A_TEAPOT,
    HTTP_STATUS_MISDIRECTED_REQUEST = 421,
    HTTP_STATUS_UNPROCESSABLE_ENTITY,
    HTTP_STATUS_LOCKED,
    HTTP_STATUS_FAILED_DEPENDENCY,
    HTTP_STATUS_TOO_EARLY,
    HTTP_STATUS_UPGRADE_REQUIRED ,
    HTTP_STATUS_PRECONDITION_REQUIRED = 428,
    HTTP_STATUS_TOO_MANY_REQUESTS,
    HTTP_STATUS_REQUEST_HEADER_TOO_LARGE = 431,
    HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASON = 451,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED,
    HTTP_STATUS_BAD_GATEWAY,
    HTTP_STATUS_SERVICE_UNAVAILABLE,
    HTTP_STATUS_GATEWAY_TIMEOUT,
    HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,
    HTTP_STATUS_VARIANT_ALSO_NEGOTIATES,
    HTTP_STATUS_INSUFFICIENT_STORAGE,
    HTTP_STATE_LOOP_DETECTED,
    HTTP_STATUS_NOT_EXTENDED = 510,
    HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED
};

/* Error codes */
# define HTTP_SUCCESS            0x01   /* Success, no error*/
# define HTTP_INVALID_URL        0x02   /* The URL provided is invalid*/
# define HTTP_CONNECTION_RESET   0x03   /* Connection closed by the server */
# define HTTP_NO_URL             0x04   /* No connection URL provided */
# define HTTP_SSL_ERROR          0x05   /* SSL error occured */
# define HTTP_SSL_CONN_FAILED    0x06   /* SSL connection failed */
# define HTTP_RES_TIMEOUT        0x07   /* Response timed out */
# define HTTP_FD_NOT_CONNECTED   0x08   /* socket ends not conncted */
# define HTTP_PROXY_NO_URL       0x09   /* No proxy URL provided */
# define HTTP_CERT_VP_FAILED     0x10   /* Failed to verify server certificate */

# ifdef __cplusplus
    }
# endif
# endif     /* _LIBHTTP_H */