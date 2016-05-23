#include "pop3.h"

#include <iostream>
#include <sstream>

#include "format.h"

// Конструктор
POP3::POP3(Socket *socket) {
    this->socket = socket;
    readWelcome();
}

// Деструктор
POP3::~POP3() {
    close();
}

// Аутентификация
bool POP3::authenticate(std::string const &username, std::string const &password) {

    ServerResponse response;

    // Передача серверу имени пользователя (почтовый ящик)
    sendCommand("USER " + username);

    // Получение ответа от сервера
    getResponse(&response);

    if (!response.status) {
        fmt::printf("Authentication failed: %s", response.statusMessage);
        return false;
    }

    // Передача серверу пароля
    sendCommand("PASS " + password);

    // Получение ответа от сервера
    getResponse(&response);

    if (!response.status) {
        fmt::printf("Authentication failed: %s", response.statusMessage);
        return false;
    }

    return true;
}

// Отправка команды серверу
void POP3::sendCommand(const std::string &command) {
    socket->write(command + "\r\n");
}

// Получение ответа от сервера
void POP3::getResponse(ServerResponse *response) {
    // Получение строки ответа от сервера
    std::string buffer = socket->readLine();

    response->status = (buffer[0] == '+');
    response->statusMessage = buffer;
    response->data.clear();
}

// Получение количества сообщений в почтовом ящике и их суммарного размера
bool POP3::printStats() {

    ServerResponse response;
    // Запрос количества сообщений и их суммарного размера
    sendCommand("STAT");
    // Получение ответа от сервера
    getResponse(&response);
    std::cout << response.statusMessage << std::endl;

    return true;
}

// Вывод списка сообщений и их размера
bool POP3::printMessageList() {

    ServerResponse response;
    // Запрос
    sendCommand("LIST");
    // Ответ
    getResponse(&response);

    if (!response.status) {
        fmt::printf("Unable to retrieve message list: %s\n", response.statusMessage);
        return false;
    }

    // Получение в буфер ответа от сервера в несколько строк (для LIST, RETR)
    getMultilineData(&response);

    if (response.data.size() == 0) {
        std::cout << "No messages available on the server." << std::endl;
    }

    // Вывод полученных строк
    for (std::string& line: response.data) {
        std::cout << line << std::endl;
    }
    return true;
}

// Получение в буфер ответа от сервера в несколько строк (для LIST, RETR)
void POP3::getMultilineData(ServerResponse *response) {
    std::string buffer;

    while (true) {
        buffer = socket->readLine();

        if (buffer == ".") {
            break;
        }
        response->data.push_back(buffer);
    }
}

// Показ выбранного сообщения
bool POP3::printMessage(int messageId) {

    ServerResponse response;
    // Запрос
    sendCommand("RETR " + std::to_string(messageId));
    // Ответ
    getResponse(&response);
    if (!response.status) {
        fmt::printf("Unable to retrieve requested message: %s", response.statusMessage);
        return false;
    }
    // Получение в буфер ответа от сервера в несколько строк
    getMultilineData(&response);

    for (std::string& line: response.data) {
        std::cout << line << std::endl;
    }
    return true;
}

// Поставить метку на удаление для выбранного сообщения
bool POP3::deleteMessage(int messageId) {
    ServerResponse response;
    // Запрос
    sendCommand("DELE " + std::to_string(messageId));
    // Ответ
    getResponse(&response);
    if (!response.status) {
        fmt::printf("Unable to retrieve requested message: %s", response.statusMessage);
        return false;
    }
    return true;
}

// Убрать метки на удаление
bool POP3::reset() {
    ServerResponse response;
    // Запрос
    sendCommand("RSET");
    // Ответ
    getResponse(&response);
    if (!response.status) {
        fmt::printf("Unable to retrieve requested message: %s", response.statusMessage);
        return false;
    }
    return true;
}

// Получить список номеров сообщений (для вывода заголовков)
std::vector<int> POP3::getMessageList() {
    ServerResponse response;
    std::vector<int> list;

    sendCommand("LIST");

    getResponse(&response);
    if (!response.status) {
        fmt::printf("Unable to retrieve message list: %s\n", response.statusMessage);
        return list;
    }

    getMultilineData(&response);

    // Изъятие из строк типа "№ size" номеров сообщений
    for (std::string& line: response.data) {
        unsigned long pos = line.find(' ');
        std::string src = line.substr(0, pos);
        list.push_back(stoi(src));
    }
    return list;
}

// Вывод заголовка выбранного сообщения
bool POP3::printHeaders(int messageId) {
    ServerResponse response;
    // Запрос на вывод заголовка и 0 строк сообщения
    sendCommand("TOP " + std::to_string(messageId) + " 0");
    // Ответ
    getResponse(&response);
    if (!response.status) {
        fmt::printf("Unable to retrieve requested message: %s", response.statusMessage);
        return false;
    }
    // Получение в буфер строк ответа
    getMultilineData(&response);

    for (std::string& line: response.data) {
        std::cout << line << std::endl;
    }
    return true;
}

// Закрыть соединение
void POP3::close() {
    if (socket != NULL) {
        sendCommand("QUIT");
    }
}

bool POP3::readWelcome(){
    ServerResponse welcomeMessage;

    getResponse(&welcomeMessage);

    if(!welcomeMessage.status){
        fmt::printf("Connection refused: %s", welcomeMessage.statusMessage);
    }

    return true;
}















