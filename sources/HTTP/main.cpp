#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include <netdb.h>

#include <map>
#include <sstream>
#include <thread>
#include <iostream>
#include "format.h"
#include <regex>

using namespace std;

// Папка на сервере с сайтом
const string baseDir = "/home/ks/spbstu/www.spbstu.ru/";

// Функция превращает строку с разделителями в массив строк
vector<string> split(const string& str, const string& delimiter) {
    std::regex rgx(delimiter);
    std::sregex_token_iterator
            first{begin(str), end(str), rgx, -1},
            last;
    return{first, last};
}

//  Функция сравнения
bool startsWith(string full, string prefix) {
    return full.compare(0, prefix.size(), prefix) == 0;
}

// Получение пути к запрашиваемому файлу на сервере
pair<string, bool> getPath(string url) {

    // Полный путь к файлу
    string fullPath = baseDir + "/" + url;
    // Если заканчивается на "/" - будет показан сайт
    if (fullPath[fullPath.size()-1] == '/') {
        fullPath += "index.html";
    }
    // Корректность пути запроса (для безопасности данных сервера, станд. функция)
    char* rawPath = realpath(fullPath.c_str(), NULL);
    // Запрет
    if (rawPath == nullptr) {
        return make_pair("", false);
    }
    string abspath = rawPath;
    // Если файл не в специальной папке на сервере - запрет
    if (startsWith(abspath, baseDir) == false) {
        return make_pair("", false);
    }
    return make_pair(abspath, true);
}

// Считывание файла  (данные, состояние)
pair<vector<char>, bool> readFile(string path) {

    FILE* pFile = fopen(path.c_str(), "rb");

    if (pFile == NULL) {
        perror("File error");
        return make_pair(vector<char>(), false);
    }

    // Получение размера файла
    fseek (pFile , 0 , SEEK_END);
    long lSize = ftell(pFile);
    rewind (pFile);

    // Распределение памяти для размещения всего файла
    vector<char> buffer((unsigned long) lSize);

    // Копирование файла в буфер
    size_t result = fread(buffer.data(), 1, lSize, pFile);
    if (result != lSize) {
        perror("Reading error");
        return make_pair(vector<char>(), false);
    }

    fclose(pFile);
    return make_pair(buffer, true);
}

// Структура запроса клиента
struct request {

    string data;            // Данные
    string payload;         // Выражение типа "a=b&c=d"
    vector<string> lines;   // Вектор строк
    string method;          // Метод
    string url;             // Адрес

    // Разбить запрос на составные части
    void parse() {
        // Деление по строкам
        lines = split(data, "\r\n");
        // Деление первой строки по пробелам
        vector<string> firstLineParts = split(lines[0], " ");
        // Должны быть получены 3 части (метод, url, протокол)
        assert(firstLineParts.size() == 3);
        // Заполнение полей структуры
        method = firstLineParts[0];
        url = firstLineParts[1];
    }
};

// Структура ответа сервера
struct response {

    int code = 200;             // Нормальный код ответа
    string message = "OK";      // Сообщение ОК
    vector<char> data;          // Данные
    string head;                // Заголовок
    map<string, string> headers;// Карта заголовков
    bool onlyHeader = false;    // Для метода HEAD

    // Формирование заголовка (служебные данные)
    void makeHead() {

        headers["Content-Length"] = to_string(data.size()); // Размер данных
        headers["Connection"] = "close";                    // Состояние соединения
        head += fmt::format("HTTP/1.1 {} {}\r\n", code, message); // Код ответа и ОК
        for (pair<string, string> header: headers) {
            head += fmt::format("{}: {}\r\n", header.first, header.second);
        }
        head += "\r\n";
    }

    // Отправка ответа сервера
    void writeResponse(const int socket) {
        // Формирование заголовка
        makeHead();
        // Отправка ответа
        write(socket, head.data(), head.size());
        // Для GET - отправляются данные
        if (onlyHeader == false) {
            write(socket, data.data(), data.size());
        }
        // Закрыть соединение
        close(socket);
    }
};

// Описание функционирования нити
void worker(int socket) {

    printf("Worker for socket %d is up\n", socket);

    // Обработка запроса от клиента

    request req;                // Структура запроса
    array<char, 1024> buffer;   // Буфер (символы, их количество)

    string requestEnd = "\r\n\r\n"; // Комбинация в конце запроса

    // Чтение запроса
    while (true) {
        // Количество считанных данных
        ssize_t n = read(socket, buffer.data(), buffer.size());
        // Следующая позиция для продолжения считывания
        auto bufEnd = next(buffer.begin(), n);
        // Запись в буфер считанных данных
        req.data.insert(req.data.end(), buffer.begin(), bufEnd);
        // Позиция конца запроса
        unsigned long pos = req.data.find(requestEnd);
        // Если достигли конца - разбить запрос на составные части
        if (pos != std::string::npos) {
            req.parse();
            // Заполнение поля структуры - payload (выражение)
            req.payload = req.data.substr(pos + requestEnd.length());
            break;
        }
    }

    // Обработка ответа сервера

    response resp;  // Структура ответа

    fmt::printf("%s %s\n", req.method, req.url);

    if (req.method == "GET" || req.method == "HEAD") {

        // Получение пути к запрашиваемому файлу
        pair<string, bool> path = getPath(req.url);

        // Если путь некорректен
        if (path.second == false) {
            resp.code = 404;    // Не найдено
            resp.message = "Not Found";
            // Ответ
            resp.writeResponse(socket);
            return;
        }

        // Считывание файла в буфер
        pair<vector<char>, bool> readRes = readFile(path.first);
        if (readRes.second == false) {
            resp.code = 403;    // Доступ запрещен
            resp.message = "Forbidden";
            // Ответ
            resp.writeResponse(socket);
            return;
        }

        // Заполнение структуры ответа
        resp.data = readRes.first;
        resp.onlyHeader = (req.method == "HEAD");
        // Ответ
        resp.writeResponse(socket);


    } else if (req.method == "POST") {

        // Разбить строку вида "a=b&c=d" по "&"
        vector<string> params = split(req.payload, "&");

        string data = "You posted these data: \n";

        for (string param: params) {
            // Разбить строку по "="
            vector<string> splitted = split(param, "=");
            data += fmt::format("Name: {}, value: {}\n", splitted[0], splitted[1]);
        }

        // Данные занесены в структуру ответа
        vector<char> tmp(data.begin(), data.end());
        resp.data = tmp;
        // Ответ
        resp.writeResponse(socket);
    }
}

int main(int argc, char** argv) {

    uint16_t portno;    // 16-битный номер порта
    int accept_socket;  // Дескриптор соединения

    struct sockaddr_in serv_addr;   // Структура адреса

    // Создание сокета
    accept_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (accept_socket < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    // Инициализация структуры сокета
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = 8080;                          // Номер порта
    serv_addr.sin_family = AF_INET;         // Семейство протоколов
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Все адреса локального хоста
    serv_addr.sin_port = htons(portno);     // Номер порта (конвертирован в сетевой порядок байтов)

    int optval = 1;

    // Опции сокета
    setsockopt(accept_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    // Сокет ассоциирован с портом
    if (bind(accept_socket, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    // Настройка прослушивания сокета
    // Размер очереди - 5 клиентов
    listen(accept_socket, 5);

    // Вектор нитей
    vector<thread> worker_threads;

    // Ожидание клиентов
    while (true) {
        printf("Waiting\n");
        // Идентификатор нового соединения с клиентов
        int newsockfd = accept(accept_socket, NULL, NULL);
        if (newsockfd <= 0) {
            perror("ERROR on accept");
            break;
        }
        fmt::printf("Connection, socket: %d, thread %ld\n", newsockfd, worker_threads.size());

        // Создание новой нити
        thread new_thread(worker, newsockfd);
        // Добавление новой нити в вектор нитей
        worker_threads.push_back(move(new_thread));
    }

    return 0;
}
