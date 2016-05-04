#ifndef _SOCKET__H
#define _SOCKET__H

#include <string>

#include "error.h"

class Socket {
    int socketfd;
public:
    ~Socket();

    ssize_t read(char *buffer, size_t size);

    bool write(std::string request);

    size_t readLine(std::string *line);

    bool open(std::string const& address, int port);

private:

    void close();

    bool readCharacter(char *buffer);

    bool has_suffix(const std::string &str, const std::string &suffix);
};

#endif
