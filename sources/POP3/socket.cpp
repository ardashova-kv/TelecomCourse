#include "socket.h"

#include <iostream>
#include <sstream>

#include <sys/socket.h> // Для работы с сокетами
#include <netdb.h>      // Для getaddrinfo
#include <algorithm>

#include <unistd.h>
#include <string.h>

using namespace std;

// Здесь описаны функции класса Socket

// Установление соединения с сервером
bool Socket::open(std::string const &address, int port) {

    // Структуры параметров соединений
    struct addrinfo hints;
    struct addrinfo *result, *record;

    // Заполнение нулями структуры hints
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = PF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP, потоковый сокет
    hints.ai_flags = 0;                 // Доп. опции
    hints.ai_protocol = 0;              // Протокол не указан

    // Определение ip-адресов по имени хоста
    int rc = getaddrinfo(address.data(), to_string(port).data(), &hints, &result);
    if (rc != 0) {
        return false;
    }

    // Перебор ip-адресов c целью установить соединение
    for (record = result; record != NULL; record = record->ai_next) {
        // Создание сокета
        socketfd = socket(record->ai_family, record->ai_socktype, record->ai_protocol);
        // Если сокет на создан, то перебор адресов дальше
        if (socketfd == -1) {
            continue;
        }
        // Установление соединения
        if (connect(socketfd, record->ai_addr, record->ai_addrlen) != -1) {
            break;
        }
        // Закрыть сокет при неудаче
        ::close(socketfd);
    }

    // Все ip-адреса просмотрены
    if (record == NULL) {
        return false;
    }

    // Очистить структуру с ip-адресами
    freeaddrinfo(result);

    return true;
}

// Деструктор класса
Socket::~Socket() {
    if (socketfd > 0) {
        close();
    }
}

// Закрытие сокета (вызывается деструктором)
void Socket::close() {
    // Отключение соединения
    shutdown(socketfd, SHUT_RDWR);
    // Закрытие сокета
    ::close(socketfd);
}

// Отправка команды (запроса серверу)
bool Socket::write(std::string request) {
    if (::write(socketfd, request.c_str(), request.length()) < 0) {
        printf("Sending error: Unable to send data to remote host");
        return false;
    }
    return true;
}

// Чтение строки ответа сервера
string Socket::readLine() {

    const string endline = "\r\n";  // Перенос строки
    string::iterator it;


    while(true) {
        // Поиск переноса строки
        it = std::search(buffer.begin(), buffer.end(),
                              endline.begin(), endline.end());
        if (it != buffer.end()) {
            break;
        }

        // Создание буфера на 1024 символа по 1 байту
        const int bufSize = 1024;
        char* tmpBuf = (char*) calloc(bufSize, 1);

        // Считывание порции информации
        ssize_t bytesRead = ::read(socketfd, tmpBuf, bufSize);

        if (bytesRead < 0) {
            printf("Recieving error: Unable to resolve data from remote host");
            return 0;
        }

        buffer += tmpBuf;

        free(tmpBuf);
    }

    // Найденная строка сохраняется
    string line(buffer.begin(), it);
    // ... и удаляется из буфера (чтобы избежать повторное чтение)
    buffer.erase(buffer.begin(), it+endline.size());

    return line;
}

