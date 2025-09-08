#include "libhttp.hpp"
#include <iostream>
#include <string.h>

int main() {

    int tls_version { HTTP_TLS_1_3 };
    int verbose { HTTP_VERBOSITY_ENABLE };
    FILE *fp;
    std::string url {  };

    fp = fopen("shot.jpg", "w");

    try {
        nsh::HTTPSession http { "http://192.168.43.106:8080/" };
        nsh::HTTPSession http2 {};
        http.setOption(HTTP_OPTIONS_VERBOSITY, verbose);

        http.performRequest();
        std::cout << "Status code: " << http.getStatusCode() << std::endl;
        http.writeResponseFriendly(stdout);
        http.writeResponseBody(stdout);

    } catch (nsh::HTTPException ex) {

        std::cout << "[*] An Error Occured: " << ex.getError() << std::endl;
    }
}