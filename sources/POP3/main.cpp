#include <iostream>
#include <stdlib.h>
#include "pop3.h"
#include "format.h"

using namespace std;

string askUser(string prompt)
{
    cout << prompt << endl;
    string line;
    getline(cin, line);
    return line;
}

int getMessageNum() {
    string msgNum = askUser("Choose message num: ");
    return stoi(msgNum);
}

int main(int argc, char **argv)
{
    /* Get password. */
    string login;
    string server;
    string password;

    login = "test@positiveux.ru"; // askUser("Login:");
    password = askUser("Password:");
    server = "pop3.timeweb.ru"; // askUser("Server:");

    Socket socket;
    socket.open(server, 110);

    POP3 pop3(&socket);
    pop3.authenticate(login, password);

    fmt::printf("Messages available: (num, size) \n");
    pop3.printStats();
    int num;
    vector<int> list;
    while (true) {
        std::cout << "Available commands: " << endl <<
                "1) List messages" << endl <<
                "2) Retrieve message" << endl <<
                "3) Delete message" << endl <<
                "4) Reset delete markers" << endl <<
                "5) Read headers" << endl <<
                "6) Quit" << endl;
        string command = askUser("Enter command: ");
        switch (stoi(command)) {
            case 1:
                pop3.printMessageList();
                break;
            case 2:
                num = getMessageNum();
                pop3.printMessage(num);
                break;
            case 3:
                num = getMessageNum();
                pop3.deleteMessage(num);
                break;
            case 4:
                pop3.reset();
                break;
            case 5:
                list = pop3.getMessageList();
                for (int msg: list) {
                    pop3.printHeaders(msg);
                }
                break;
            case 6:
                exit(0);
            default:
                cout << "Invalid command" << endl;
                continue;
        }
    }

    return EXIT_SUCCESS;
}

