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
    std::vector<int> list = getMessageList();

    // Вывести заголовки сообщений
    for (int msg: list) {
        fmt::printf("\n-- Message number %d --\n", msg);
        std::list<std::string> headers = onlyHeaders(msg);

        for (std::string& line: headers) {
            printHeader(line);
        }
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

    bool readingHeaders = true;
    for (std::string& line: response.data) {
        if (line.empty()) {
            readingHeaders = false;
        }
        if (readingHeaders) {
            printHeader(line);
        } else{
            std::cout << line << std::endl;
        }
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
std::list<std::string> POP3::onlyHeaders(int messageId) {
    ServerResponse response;
    // Запрос на вывод заголовка и 0 строк сообщения
    sendCommand("TOP " + std::to_string(messageId) + " 0");
    // Ответ
    getResponse(&response);
    if (!response.status) {
        fmt::printf("Unable to retrieve requested message: %s", response.statusMessage);
        return std::list<std::string>();
    }
    // Получение в буфер строк ответа
    getMultilineData(&response);
    return response.data;
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

//  Функция сравнения
bool POP3::startsWith(std::string full, std::string prefix) {
    return full.compare(0, prefix.size(), prefix) == 0;
}

void POP3::printHeader(std::string line) {
    std::array<std::string, 5> allowedHeaders = {"Date", "From", "Subject", "To"};
    for (std::string key: allowedHeaders) {
        if (startsWith(line, key + ":")) {
            std::cout << line << std::endl;
        }
    }
}