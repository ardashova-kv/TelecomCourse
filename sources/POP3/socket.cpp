#include "socket.h"

#include <iostream>
#include <sstream>

#include <sys/socket.h>
#include <netdb.h>
#include <algorithm>

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

bool Socket::write(std::string request) {
    cout << "Wrote " << request << endl;
    if (::write(socketfd, request.c_str(), request.length()) < 0) {
        printf("Sending error: Unable to send data to remote host");
        return false;
    }
    return true;
}

string Socket::readLine() {
    const string endline = "\r\n";
    string::iterator it;
    while(true) {
        it = std::search(buffer.begin(), buffer.end(),
                              endline.begin(), endline.end());
        if (it != buffer.end()) {
            break;
        }

        const int bufSize = 1024;
        char* tmpBuf = (char*) calloc(bufSize, 1);
        ssize_t bytesRead = ::read(socketfd, tmpBuf, bufSize);
//        cout << "Received " << tmpBuf << " size " << bytesRead << endl;
        if (bytesRead < 0) {
            printf("Recieving error: Unable to resolve data from remote host");
            return 0;
        }
        buffer += tmpBuf;
        free(tmpBuf);
        it = std::search(buffer.begin(), buffer.end(),
                              endline.begin(), endline.end());
    }

    string line(buffer.begin(), it);
    buffer.erase(buffer.begin(), it+endline.size());
    return line;
}

