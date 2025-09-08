# ifndef LIBHTTP_HPP_
# define LIBHTTP_HPP_H

#include "libhttp.h"
#include <string>
#include <cstdlib>
#include <cstdio>

namespace nsh {
/**
 * Some people doesn't like the CPP exceptions.
 * This macro will help if the user does't want to use
 * C++ exception.
 * If defined, then C++ exceptions will be disabled. 
*/

# ifndef HTTP_NO_CPP_EXCEPTIONS
/**
 * @brief This is a class (HTTPException) for handling errors in HTTP sessions
 * This errors could be caused by some of the libhttp functions
*/
class HTTPException {
public:
    HTTPException(http_session session) {
        errorMessage = http_get_error(session);     // Retrieves the error message from the C-function
        errorCode = http_get_error_code(session);   // Retrieves the error code     --   -   -
    }
    /**
     * @returns an error code
    */
    int getCode() {
        return errorCode;
    }
    /**
     * @returns an error message
    */
    std::string getError() {
        return errorMessage;
    }
    private:
        int errorCode;
        std::string errorMessage;
};
/**
 *  macro for throwing exceptions if there is an error
 *  the 'http_throwable' indicates that the function used with it
 *  will probably thrown an error 
*/
# define http_throw(e) if((e) == HTTP_ERROR) throw HTTPException(getHttpSession())
# define http_throwable void
# define return_throwable return

# else
/**
 * No exception,, returns HTTP_ERROR
 * */
# define http_throw(e) if((e) == HTTP_ERROR) return HTTP_ERROR
# define http_throwable int
# define return_throwable return HTTP_OK

# endif // HTTP_NO_CPP_EXCEPTIONS

/**
 * @brief nsh_http::HTTPSession class holds information about the http connection
 *   
*/
class HTTPSession {

public:
    HTTPSession() {
        httpSession = http_new();
    }
    /**
     * @brief Initializes HTTPSession with a URL
     * @param url HTTP connection URL
     * @throws HTTPException
    */
    HTTPSession(const char *url) {
        httpSession = http_new();
        setOption(HTTP_OPTIONS_URL, url);
    }
    ~HTTPSession() {
        http_free(httpSession);
    }
    /**
     * @brief sets an option
     * @param option The option to set
     * @param value cstring contaning the option value
     * @throws HTTPException on error(s)
    */
    http_throwable setOption(enum http_options option, const char *value) {
        http_throw(http_options_set(httpSession, option, value));
        return_throwable;
    }
    /**
     * @brief sets an option
     * @param option The option to set
     * @param value long integer contaning the option value
     * @throws HTTPException on error(s)
    */
    http_throwable setOption(enum http_options option, long int value) {
        http_throw(http_options_set(httpSession, option, &value));
        return_throwable;
    }
    /**
     * @brief sets an option
     * @param option The option to set
     * @param value void pointer contaning the option value
     * @throws HTTPException on error(s)
    */
    http_throwable setOption(enum http_options option, void *value) {
        http_throw(http_options_set(httpSession, option, value));
        return_throwable;
    }
    /**
     * @brief Connects to a HTTP server
     * @throws HTTPException on error(s)
    */
    http_throwable connect() {
        http_throw(http_connect(httpSession));
        return_throwable;
    }
    /**
     * @brief Perform a HTTP GET request
     * @throws HTTPException on error(s)
    */
    http_throwable httpGet() {
        int option = http_requests::HTTP_GET;
        http_options_set(httpSession, HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_session_start(httpSession));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP POST request
     * @throws HTTPException on error(s)
    */
    http_throwable httpPost() {
        int option = http_requests::HTTP_POST;
        http_options_set(httpSession, HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_session_start(httpSession));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP PUT request
     * @throws HTTPException on error(s)
    */
    http_throwable httpPut() {
        int option = http_requests::HTTP_PUT;
        http_options_set(httpSession, HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_session_start(httpSession));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP PATCH request
     * @throws HTTPException on error(s)
    */
    http_throwable httpPatch() {
        int option = http_requests::HTTP_PATCH;
        http_options_set(httpSession, HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_session_start(httpSession));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP HEAD request
     * @throws HTTPException on error(s)
    */
    http_throwable httpHead() {
        int option = http_requests::HTTP_HEAD;
        http_options_set(httpSession, HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_session_start(httpSession));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP TRACE request
     * @throws HTTPException on error(s)
    */
    http_throwable httpTrace() {
        int option = http_requests::HTTP_TRACE;
        http_options_set(httpSession, HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_session_start(httpSession));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP OPTIONS request
     * @throws HTTPException on error(s)
    */
    http_throwable httpOptions() {
        int option = http_requests::HTTP_OPTIONS;
        http_options_set(httpSession, HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_session_start(httpSession));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP DELETE request
     * @throws HTTPException on error(s)
    */
    http_throwable httpDelete() {
        int option = http_requests::HTTP_DELETE;
        http_options_set(httpSession, HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_session_start(httpSession));
        return_throwable;
    }
    /**
     * @brief This helper function can be used to shorten your code
     * it automatically calls some functions for you.
     * like the http_connect and http_session_start from the libhttp C library
     * functions
     * @throws HTTPException
    */
    http_throwable performRequest() {
        http_throw(http_perform_req(httpSession));
        return_throwable;
    }
    /**
     * @brief Diconnects and closes all connection sockets.
    */
    void disconnect() {
        http_disconnect(httpSession);
    }

    /**
     * @brief Retrives the hostame
    */
    std::string getHostname() {
        char *hostname;
        http_options_get(httpSession, HTTP_OPTIONS_HOSTNAME, &hostname);
        return hostname;
    }
    /**
     * @brief Retrives the HTTP port
     * @returns an integer value of the port number
    */
    int getPort() {
        char *port;
        http_options_get(httpSession, HTTP_OPTIONS_PORT, &port);
        return atoi(port);
    }
    /**
     * @returns the path from the URL
    */
    std::string getPath() {
        char *path;
        http_options_get(httpSession, HTTP_OPTIONS_PATH, &path);
        return path;
    }
    /**
     * @returns query string from the URL
    */
    char * getQuery() {
        char *query;
        http_options_get(httpSession, HTTP_OPTIONS_QUERY, &query);
        // std::string c {query};
        return query;
    }
    /**
     * @brief Retrieves a status code
     * @returns the HTTP response status code
    */
    int getStatusCode() {
        int code;
        code = http_get_status_code(httpSession);
        return code;
    }
    /**
     * @brief Gets a response header
     * @return HTTP response header
    */
    std::string getHeaders() {
        return http_get_headers(httpSession);
    }

    /**
     * @brief Gets a specific response header field values
     * @param header_name The header name to get its value
     * @return HTTP response header field value
     * @example getHeader("Set-Cookie") to get the field values of the header Set-Cookie
    */
    std::string getHeader(const char *header_name) {
        return http_get_header(httpSession, header_name);
    }
    /**
     * @brief Gets a response body
     * @return HTTP response body
    */
    std::string getBody() {
        return http_get_body(httpSession);
    }
    /**
     * @brief Retrives the HTTPS server's certificate subject name
    */
    std::string getCertificateSubjectName() {
        return http_get_certificate_subject(httpSession);
    }
    /**
     * @brief Retrives the HTTPS server's certificate issuer name
    */
    std::string getCertificateIssuerName() {
        return http_get_certificate_issuer(httpSession);
    }
    /**
     * @brief Retrives the connection socket fd
     * @return HTTP connection socket fd
    */
    HTTPSOCKET getFd() {
        return http_get_fd(httpSession);
    }
    /**
     * @brief This function clears all options for this session
    */
    void clearOptions() {
        http_options_clear(httpSession);
    }
    /**
     * @brief Copies all options in this class to dest
     * @param dest is the destination of the copy operation
    */
    void copyOptions(HTTPSession dest) {

        http_options_copy(dest.getHttpSession(), httpSession);
    }
    /**
     * @return the libhttp version 
    */
    std::string getVersion() {

        return libhttp_get_version();
    }
    /**
     * @brief write response to a file describtor
    */
    void writeResponse(FILE *fp) {
        http_write_res_fp(httpSession, fp);
    }
    /**
     * @brief write response body to a file describtor
    */
    void writeResponseBody(FILE *fp) {
        http_write_res_body_fp(httpSession, fp);
    }
    /**
     * @brief write response headers to a file describtor
    */
    void writeResponseHeaders(FILE *fp) {
        http_write_res_headers_fp(httpSession, fp);
    }

    /**
     * @brief write response to a file describtor with some styling
    */
    void writeResponseFriendly(FILE *fp) {
        http_write_res_fp_friendly(httpSession, fp);
    }
    /**
     * @brief write response body to a file describtor with some styling
    */
    void writeResponseBodyFriendly(FILE *fp) {
        http_write_res_body_fp_friendly(httpSession, fp);
    }
    /**
     * @brief write response headers to a file describtor some styling
    */
    void writeResponseHeadersFriendly(FILE *fp) {
        http_write_res_headers_fp_friendly(httpSession, fp);
    }
    /**
     * @brief Encodes a text to a base64 encoding
     * @param str is the string to encode
     * @param str_len is the length of str
     * @return an encoded base64 string
    */
    std::string base64Encode(char* str, int str_len) {
        return http_base64_encode((unsigned char*)str, str_len);
    }

    /**
     * @brief Encodes a text to a URL encoding
     * @param str is the string to encode
     * @param str_len is the length of str
     * @return an encoded URL string
    */
    std::string urlEncode(char* str, int str_len) {
        return http_url_encode(str, str_len);
    }
    /**
     * @return an error code
    */
    int getErrorCode() {
        return http_get_error_code(httpSession);
    }
    /**
     * @brief Retrives an error message
     * @return error message
    */
    std::string getError() {
        return http_get_error(httpSession);
    }

    http_session getHttpSession() {
        return httpSession;
    }
private:
    http_session httpSession;
};

/**
 * @brief http::HTTPSession class holds information about the http proxy connection
 *  it inherits the http::HTTPSession class 
*/
class HTTPProxySession : public HTTPSession {

public:    
    HTTPProxySession() {
        HTTPSession();
    }
    /**
     * @brief Initializes HTTPProxySession with a URL
     * @param proxy_url HTTP proxy connection URL
     * @throws HTTPException
    */
    HTTPProxySession(const char *proxy_url) {
        HTTPSession();
        setOption(HTTP_OPTIONS_PROXY_URL, proxy_url);
    }
    // ~HTTPProxySession() {
    //     http::HTTPSession::~HTTPSession();
    // }
        /**
     * @brief Connects to a HTTP server
     * @throws HTTPException on error(s)
    */
    http_throwable connect() {
        http_throw(http_proxy_connect(getHttpSession()));
        return_throwable;
    }
     /**
     * @brief Sends HTTP CONNECT request to proxy server(Must be called after calling connect())
     * @throws HTTPException on error(s)
    */
    http_throwable proxySendRequest() {
        http_throw(http_proxy_send_request(getHttpSession()));
        return_throwable;
    }
    /**
     * @brief Perform a HTTP GET request method
     * @throws HTTPException on error(s)
    */
    http_throwable httpGet() {
        int option = HTTP_GET;
        http_options_set(getHttpSession(), HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_proxy_session_start(getHttpSession()));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP POST request method
     * @throws HTTPException on error(s)
    */
    http_throwable httpPost() {
        int option = HTTP_POST;
        http_options_set(getHttpSession(), HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_proxy_session_start(getHttpSession()));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP PUT request method
     * @throws HTTPException on error(s)
    */
    http_throwable httpPut() {
        int option = HTTP_PUT;
        http_options_set(getHttpSession(), HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_proxy_session_start(getHttpSession()));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP PATCH request method
     * @throws HTTPException on error(s)
    */
    http_throwable httpPatch() {
        int option = HTTP_PATCH;
        http_options_set(getHttpSession(), HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_proxy_session_start(getHttpSession()));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP HEAD request method
     * @throws HTTPException on error(s)
    */
    http_throwable httpHead() {
        int option = HTTP_HEAD;
        http_options_set(getHttpSession(), HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_proxy_session_start(getHttpSession()));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP TRACE request method
     * @throws HTTPException on error(s)
    */
    http_throwable httpTrace() {
        int option = HTTP_TRACE;
        http_options_set(getHttpSession(), HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_proxy_session_start(getHttpSession()));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP OPTIONS request method
     * @throws HTTPException on error(s)
    */
    http_throwable httpOptions() {
        int option = HTTP_PATCH;
        http_options_set(getHttpSession(), HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_proxy_session_start(getHttpSession()));
        return_throwable;
    }
     /**
     * @brief Perform a HTTP DELETE request method
     * @throws HTTPException on error(s)
    */
    http_throwable httpDelete() {
        int option = HTTP_PATCH;
        http_options_set(getHttpSession(), HTTP_OPTIONS_REQUEST_METHOD, &option);
        http_throw(http_proxy_session_start(getHttpSession()));
        return_throwable;
    }
    /**
     * @brief This helper function can be used to shorten your code
     * it automatically calls some functions for you
     * @throws HTTPException
    */
    http_throwable performRequest() {
        http_throw(http_proxy_perform_req(getHttpSession()));
        return_throwable;
    }
    /**
     * @brief Diconnect and close all connection sockets.
    */
    void disconnect() {
        http_proxy_disconnect(getHttpSession());
    }
    /**
     * @brief Retrives the connection socket fd
     * @return HTTP proxy connection socket fd
    */
    HTTPSOCKET getFd() {
        return http_get_proxy_fd(getHttpSession());
    }
};
} // namespace http

# endif /* LIBHTTP_HPP_ */