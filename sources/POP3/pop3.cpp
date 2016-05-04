#include "pop3.h"

#include <iostream>
#include <sstream>

#include "format.h"

POP3::POP3(Socket *socket) {
    this->socket = socket;
    readWelcome();
}

POP3::~POP3() {
    close();
}

void POP3::sendCommand(const std::string &command) {
    socket->write(command + "\r\n");
}

void POP3::getResponse(ServerResponse *response) {
    std::string buffer;
    socket->readLine(&buffer);

    response->status = (buffer[0] == '+');
    response->statusMessage = buffer;
    response->data.clear();
}

void POP3::getMultilineData(ServerResponse *response) {
    std::string buffer;
    size_t bytesRead;

    while (true) {
        buffer.clear();

        bytesRead = socket->readLine(&buffer);

        if (buffer == "." || bytesRead == 0) {
            break;
        }
        response->data.push_back(buffer);
    }
}

bool POP3::readWelcome() {
    ServerResponse welcomeMessage;

    getResponse(&welcomeMessage);

    if (!welcomeMessage.status) {
        fmt::printf("Conection refused: %s", welcomeMessage.statusMessage);
        return false;
    }
    return true;
}

void POP3::close() {
    if (socket != NULL) {
        sendCommand("QUIT");
    }
}

bool POP3::authenticate(std::string const &username, std::string const &password) {
    ServerResponse response;

    sendCommand("USER " + username);
    getResponse(&response);

    if (!response.status) {
        fmt::printf("Authentication failed: %s", response.statusMessage);
        return false;
    }

    sendCommand("PASS " + password);
    getResponse(&response);

    if (!response.status) {
        fmt::printf("Authentication failed: %s", response.statusMessage);
        return false;
    }
    return true;
}

bool POP3::printMessageList() {
    ServerResponse response;

    sendCommand("LIST");

    getResponse(&response);
    if (!response.status) {
        fmt::printf("Unable to retrieve message list: %s", response.statusMessage);
        return false;
    }

    getMultilineData(&response);

    if (response.data.size() == 0) {
        std::cout << "No messages available on the server." << std::endl;
    }

    for (std::string& line: response.data) {
        std::cout << line << std::endl;
    }
    return true;
}

bool POP3::printMessage(int messageId) {
    ServerResponse response;

    std::stringstream command;
    command << "RETR " << messageId;

    sendCommand(command.str());

    getResponse(&response);
    if (!response.status) {
        fmt::printf("Unable to retrieve requested message: %s", response.statusMessage);
        return false;
    }

    getMultilineData(&response);

    for (std::string& line: response.data) {
        std::cout << line << std::endl;
    }
    return true;
}
