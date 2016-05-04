#ifndef _SOCKET__H
#define _SOCKET__H

#include <string>
#include "error.h"

class Socket {
    int socketfd;
public:
    ~Socket();

    bool write(std::string request);

    std::string readLine();

    bool open(std::string const& address, int port);

private:

    void close();

    std::string buffer;
};

#endif
