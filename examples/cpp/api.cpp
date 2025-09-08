#include "../../lib/libhttp.hpp"
#include <iostream>
#include <cstring>

using nsh::HTTPException;
using nsh::HTTPSession;

int main()
{
  int verbose{HTTP_VERBOSITY_ENABLE};

  try
  {
    HTTPSession httpSession{"http://localhost:3001/api/test"};
    httpSession.setOption(HTTP_OPTIONS_VERBOSITY, verbose);

    httpSession.connect();
    httpSession.httpGet(); // perform GET request
    // httpSession.writeResponseFriendly(stdout); // write response to `stdout` (standard output)

    // disconnect and free memory
    httpSession.disconnect();
  }
  catch (HTTPException err)
  {
    std::cout << "Error: " << err.getError() << std::endl;
    return -1;
  }
}