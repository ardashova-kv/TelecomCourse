#include <iostream>
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

int main(int argc, char **argv)
{
    /* Get password. */
    string login;
    string server;
    string password;

    login = askUser("Login:");
    password = askUser("Password:");
    server = askUser("Server:");

    Socket socket;
    socket.open(server, 110);

    POP3 pop3(&socket);
    pop3.authenticate(login, password);

    fmt::printf("Messages available: (num, size) \n");
    pop3.printMessageList();
    while (true) {
        string msgNum = askUser("Choose message num: ");
        if (msgNum.empty()) break;
        int num = stoi(msgNum);
        pop3.printMessage(num);
    }

    return EXIT_SUCCESS;
}

