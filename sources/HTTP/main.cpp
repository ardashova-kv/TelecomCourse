#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include <netdb.h>

#include <map>
#include <sstream>
#include <thread>
#include <iostream>
#include "format.h"
#include <regex>

using namespace std;

const string baseDir = "/home/ks/spbstu/www.spbstu.ru/"; // etcwd(nullptr, 256);

// curl --data "a=b&c=d" -v http://localhost:8080/fix.txt

vector<string> split(const string& str, const string& delimiter) {
    std::regex rgx(delimiter);
    std::sregex_token_iterator
            first{begin(str), end(str), rgx, -1},
            last;

    return{first, last};
}

bool startsWith(string full, string prefix) {
    return full.compare(0, prefix.size(), prefix) == 0;
}

pair<string, bool> getPath(string url) {
    string fullPath = baseDir + "/" + url;
    if (fullPath[fullPath.size()-1] == '/') {
        fullPath += "index.html";
    }

    char* rawPath = realpath(fullPath.c_str(), NULL);

    if (rawPath == nullptr) {
        return make_pair("", false);
    }

    string abspath = rawPath;
    if (startsWith(abspath, baseDir) == false) {
        return make_pair("", false);
    }
    return make_pair(abspath, true);
}

pair<vector<char>, bool> readFile(string path) {
    FILE* pFile = fopen(path.c_str(), "rb");

    if (pFile == NULL) {
        perror("File error");
        return make_pair(vector<char>(), false);
    }

    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    long lSize = ftell(pFile);
    rewind (pFile);

    // allocate memory to contain the whole file:
    vector<char> buffer((unsigned long) lSize);

    // copy the file into the buffer:
    size_t result = fread(buffer.data(), 1, lSize, pFile);
    if (result != lSize) {
        perror("Reading error");
        return make_pair(vector<char>(), false);
    }

    /* the whole file is now loaded in the memory buffer. */

    // terminate
    fclose(pFile);
    return make_pair(buffer, true);
}

struct request {
    string data;
    string payload;

    vector<string> lines;
    string method;
    string url;
//    int contentLength;

    void parse() {
        lines = split(data, "\r\n");
        vector<string> firstLineParts = split(lines[0], " ");
        assert(firstLineParts.size() == 3);

        method = firstLineParts[0];
        url = firstLineParts[1];

//        for (string line: lines) {
//            std::regex re("Content-Length: (.*)");
//            std::smatch match;
//            std::regex_search(line, match, re);
//            if (match.size() > 1) {
//                contentLength = stoi(match.str(1));
//            }
//        }
    }
};

struct response {
    int code = 200;
    string message = "OK";

    vector<char> data;
    string head;
    map<string, string> headers;

    bool onlyHeader = false;

    void makeHead() {
        headers["Content-Length"] = to_string(data.size());
        headers["Connection"] = "close";

        head += fmt::format("HTTP/1.1 {} {}\r\n", code, message);
        for (pair<string, string> header: headers) {
            head += fmt::format("{}: {}\r\n", header.first, header.second);
        }
        head += "\r\n";
    }

    void writeResponse(const int socket) {
        makeHead();
        write(socket, head.data(), head.size());
        if (onlyHeader == false) {
            write(socket, data.data(), data.size());
        }
        close(socket);
    }
};

void worker(int socket) {
    printf("Worker for socket %d is up\n", socket);

    request req;
    array<char, 1024> buffer;

    string requestEnd = "\r\n\r\n";

    while (true) {
        ssize_t n = read(socket, buffer.data(), buffer.size());
        auto bufEnd = next(buffer.begin(), n);
        req.data.insert(req.data.end(), buffer.begin(), bufEnd);
        unsigned long pos = req.data.find(requestEnd);
        if (pos != std::string::npos) {
            req.parse();
            req.payload = req.data.substr(pos + requestEnd.length());
            break;
        }
    }

    response resp;

    fmt::printf("%s %s\n", req.method, req.url);

    if (req.method == "GET" || req.method == "HEAD") {
        pair<string, bool> path = getPath(req.url);
        if (path.second == false) {
            resp.code = 404;
            resp.message = "Not Found";
            resp.writeResponse(socket);
            return;
        }

        pair<vector<char>, bool> readRes = readFile(path.first);
        if (readRes.second == false) {
            resp.code = 404;
            resp.message = "Forbidden";
            resp.writeResponse(socket);
            return;
        }

        resp.data = readRes.first;
        resp.onlyHeader = (req.method == "HEAD");
        resp.writeResponse(socket);
    } else if (req.method == "POST") {
        vector<string> params = split(req.payload, "&");

        string data = "You posted these data: \n";
        for (string param: params) {
            vector<string> splitted = split(param, "=");
            data += fmt::format("Name: {}, value: {}\n", splitted[0], splitted[1]);
        }

        vector<char> tmp(data.begin(), data.end());
        resp.data = tmp;
        resp.writeResponse(socket);
    }
}

int main(int argc, char** argv) {
    uint16_t portno;
    int accept_socket;

    struct sockaddr_in serv_addr;

    accept_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (accept_socket < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = 8080;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    int optval = 1;
    setsockopt(accept_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    /* Now bind the host address using bind() call.*/
    if (bind(accept_socket, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Now start listening for the clients, here process will
    * go in sleep mode and will wait for the incoming connection
    */
    listen(accept_socket, 5);

    vector<thread> worker_threads;

    while (true) {
        printf("Waiting\n");
        /* Accept actual connection from the client */
        int newsockfd = accept(accept_socket, NULL, NULL);
        if (newsockfd <= 0) {
            perror("ERROR on accept");
            break;
        }
        fmt::printf("Connection, socket: %d, thread %ld\n", newsockfd, worker_threads.size());

        thread new_thread(worker, newsockfd);
        worker_threads.push_back(move(new_thread));
    }

    return 0;
}
