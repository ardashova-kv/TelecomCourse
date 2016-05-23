#ifndef _SOCKET__H
#define _SOCKET__H

#include <string>
#include "error.h"

//  Для соединения с сервером

class Socket {

private:

    int socketfd;           // Идентификатор сокета

    std::string buffer;     // Буфер для приема данных

    void close();           // Закрыть сокет

public:

    ~Socket();  // Деструктор

    bool write(std::string request); // Отправка команды (запроса серверу)

    std::string readLine();          // Чтение строки принимаемых данных от сервера

    bool open(std::string const& address, int port);    // Установление соединения с сервером


};

#endif
