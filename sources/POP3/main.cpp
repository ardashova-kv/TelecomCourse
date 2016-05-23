#include <iostream>
#include <stdlib.h>
#include "pop3.h"
#include "format.h"

using namespace std;

// Запрашивает пользовательский ввод
string askUser(string tmp)
{
    string line;
    getline(cin, line);
    return line;
}

// Для получения команд пользователя
int getMessageNum() {
    string msgNum = askUser("Choose message num: ");
    return stoi(msgNum);
}

int main(int argc, char **argv)
{
    string login;
    string server;
    string password;

    login = "test@positiveux.ru";  // askUser("Login:");
    password = "123456qwerty";     // askUser("Password:");
    server = "pop3.timeweb.ru";    // askUser("Server:");

    // Объект класса Socket
    Socket socket;
    // Установить соединение с сервером (указан станд. порт для POP3)
    socket.open(server, 110);

    // Объект класса POP3
    POP3 pop3(&socket);
    // Аутентификация
    pop3.authenticate(login, password);

    // Вывод количества сообщений в почтовом ящике и их размера
    fmt::printf("Messages available: (num, size) \n");
    pop3.printStats();

    int num;
    vector<int> list;

    // Реализация пользовательского интерфейса
    while (true) {
        std::cout << "Available commands: " << endl <<
                "1) List messages" << endl <<
                "2) Show message" << endl <<
                "3) Delete message" << endl <<
                "4) Reset delete markers" << endl <<
                "5) Read headers" << endl <<
                "6) Quit" << endl;

        string command = askUser("Enter command: ");

        switch (stoi(command)) {
            case 1:
                pop3.printMessageList(); // Вывести список сообщений с их размерами
                break;
            case 2:
                std::cout << "Enter message number \n" << endl;
                num = getMessageNum();   // Запрос у пользователя номера сообщения
                pop3.printMessage(num);  // Показать указанное сообщение
                break;
            case 3:
                std::cout << "Enter message number \n" << endl;
                num = getMessageNum();
                pop3.deleteMessage(num); // Поставить метку на удаление для выбранного сообщения
                break;
            case 4:
                pop3.reset();            // Убрать метки на удаление
                break;
            case 5:
                list = pop3.getMessageList();   // Получить список сообщений
                // Вывести заголовки сообщений
                for (int msg: list) {
                    fmt::printf("Message number %d\n\n", msg);
                    pop3.printHeaders(msg);
                }
                break;
            case 6:
                pop3.close();   // Закрыть соединение
                exit(0);
            default:
                cout << "Invalid command" << endl;
                continue;
        }
    }

    return EXIT_SUCCESS;
}

