#include "../../lib/libhttp.hpp"
#include <iostream>
#include <cstring>

using nsh::HTTPException;
using nsh::HTTPSession;
int main()
{

    std::string cookie_file{"cookies.txt"};
    int tls_version{HTTP_TLS_1_3};
    int verbose{HTTP_VERBOSITY_ENABLE};
    try
    {

        HTTPSession httpSession{"http://localhost/exploit.php?cmd=ls"};
        // Set http options for this session
        httpSession.setOption(HTTP_OPTIONS_LOAD_COOKIES_FILE, cookie_file.c_str());
        httpSession.setOption(HTTP_OPTIONS_TLS_VERSION, tls_version);
        httpSession.setOption(HTTP_OPTIONS_VERBOSITY, verbose);

        // std::cout << httpSession.getQuery() << std::endl;
        std::cout << "[*] Connecting " << httpSession.getHostname()
                  << ":" << httpSession.getPort() << ".....\n";

        httpSession.connect();
        std::cout << "[*] Connected" << std::endl;

        // Perform http HEAD request method
        httpSession.httpHead();
        httpSession.writeResponseFriendly(stdout);
    }
    catch (HTTPException err)
    {
        std::cout << "[*] Error: " << err.getError() << std::endl;
        return -1;
    }

    return 0;
}