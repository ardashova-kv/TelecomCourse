#ifndef _POP3SESSION__H
#define _POP3SESSION__H

#include <list>
#include <string>
#include <vector>

#include "error.h"
#include "socket.h"

// Для работы с протоколом (команды протокола)

class POP3 {

private:

    Socket *socket;         // Содержит объект класса Socket

    struct ServerResponse;  // Структура для хранения ответа сервера


    void sendCommand(std::string const &command);   // Отправка команды серверу

    void getResponse(ServerResponse *response);     // Получение ответа от сервера

    void getMultilineData(ServerResponse *response);    // Получение в буфер ответа от сервера в несколько строк (для LIST, RETR)

    bool reset(int messageId);      // Убрать метки на удаление

public:

    POP3(Socket *socket);       // Конструктор

    ~POP3();                    // Деструктор

    bool authenticate(std::string const &username, std::string const &password);    // Аутентификация

    bool printMessageList();            // Вывод списка сообщений и их размера

    bool printMessage(int messageId);   // Показать выбранное сообщение

    bool printStats();                  // Запрос кол-ва сообщений в почтовом ящике и их суммарного размера

    bool deleteMessage(int messageId);  // Поставить метку на удаление для выбранного сообщения

    bool reset();                       // Убрать метки на удаление

    std::vector<int> getMessageList();  // Получить список номеров сообщений (для вывода заголовков)

    bool printHeaders(int messageId);   // Вывести заголовки сообщений

    void close();   // Закрыть соединение

    bool readWelcome();

};

// Структура для хранения ответа сервера
struct POP3::ServerResponse {
    bool status;                    // Статус ответа
    std::string statusMessage;      // Сообщение о статусе
    std::list<std::string> data;    // Ответ сервера (данные)
};

#endif
