#ifndef _POP3SESSION__H
#define _POP3SESSION__H

#include <list>
#include <string>

#include "error.h"
#include "socket.h"

class POP3 {
    Socket *socket;

public:
    POP3(Socket *socket);

    ~POP3();

    bool authenticate(std::string const &username, std::string const &password);

    bool printMessageList();

    bool printMessage(int messageId);

private:
    struct ServerResponse;

    void sendCommand(std::string const &command);

    void getResponse(ServerResponse *response);

    void getMultilineData(ServerResponse *response);

    bool readWelcome();

    void close();
};

struct POP3::ServerResponse {
    bool status;
    std::string statusMessage;
    std::list<std::string> data;
};

#endif
