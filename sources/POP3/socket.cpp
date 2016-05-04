#include "socket.h"

#include <iostream>
#include <sstream>

#include <sys/socket.h>
#include <netdb.h>

#include <unistd.h>
#include <string.h>

using namespace std;

bool Socket::open(std::string const &address, int port) {
    struct addrinfo hints;
    struct addrinfo *result, *record;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    int rc = getaddrinfo(address.data(), to_string(port).data(), &hints, &result);
    if (rc != 0) {
        return false;
    }

    for (record = result; record != NULL; record = record->ai_next) {
        socketfd = socket(record->ai_family, record->ai_socktype, record->ai_protocol);
        if (socketfd == -1) {
            continue;
        }

        if (connect(socketfd, record->ai_addr, record->ai_addrlen) != -1) {
            break;
        }

        ::close(socketfd);
    }

    if (record == NULL) {
        return false;
    }

    freeaddrinfo(result);
    return true;
}

Socket::~Socket() {
    if (socketfd > 0) {
        close();
    }
}

void Socket::close() {
    shutdown(socketfd, SHUT_RDWR);
    ::close(socketfd);
}

ssize_t Socket::read(char *buffer, size_t size) {
    ssize_t bytesRead = ::read(socketfd, buffer, size);
    if (bytesRead < 0) {
        printf("Recieving error: Unable to resolve data from remote host");
        return 0;
    }

    return bytesRead;
}

bool Socket::write(std::string request) {
    if (::write(socketfd, request.c_str(), request.length()) < 0) {
        printf("Sending error: Unable to send data to remote host");
        return false;
    }
    return true;
}

bool Socket::readCharacter(char *buffer) {
    return read(buffer, 1) > 0;
}

size_t Socket::readLine(std::string *line) {
    char buffer[2];
    *line = "";
    size_t bytesRead = 0;

    if (readCharacter(&(buffer[1]))) {
        do {
            *line += buffer[0];
            buffer[0] = buffer[1];
            bytesRead++;
        }
        while (readCharacter(&(buffer[1])) &&
               !(buffer[0] == '\r' && buffer[1] == '\n'));

        line->erase(0, 1);
    }

    return bytesRead;
}

