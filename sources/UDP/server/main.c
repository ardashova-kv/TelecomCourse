#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <netinet/in.h>

#define max_conn 100
#define max_users 10

struct user
{
    char* username;
    char* password;
    time_t last_seen;
};

struct user users[max_users];
int user_num = 0;

struct post
{
    char* author;
    time_t date;
    char* content;
};

struct thread
{
    char* topic;
    struct post* posts;
    int post_num;
    int post_cap;
};

struct forum_t
{
    struct thread* threads;
    int thread_num;
    int thread_cap;
} forum;

struct request
{
    char* command_text;
    enum { POST, REPLY, SHOWALL, SHOW, RECENT, LOGIN, REGISTER } command;
    char* topic;
    char* content;
    int id;
    char* login;
    char* password;
};

struct client
{
    struct sockaddr_in address;
    int num;
    int usr_num;
};

int clients_num = 0;
struct client clients[max_conn];

int sockaddr_cmp(struct sockaddr_in *x, struct sockaddr_in *y)
{
#define CMP(a, b) if (a != b) return 0;

    struct sockaddr_in *xin = (void*)x, *yin = (void*)y;
    CMP(ntohl(xin->sin_addr.s_addr), ntohl(yin->sin_addr.s_addr));
    CMP(ntohs(xin->sin_port), ntohs(yin->sin_port));

    return 1;
}

char* alloc_string(char* src)
{
    char* new_str = malloc(sizeof(char)*(strlen(src)+1));
    strcpy(new_str, src);
    return new_str;
}

struct request parseRequest(char* request)
{
    struct request req;
    char* context = NULL;
    char* delim = "\n";
    bzero(&req, sizeof(req));
    req.command_text = alloc_string(strtok_r(request, delim, &context));
    if (strcmp(req.command_text, "POST") == 0)
    {
        req.command = POST;
        req.topic = alloc_string(strtok_r(NULL, delim, &context));
        req.content = alloc_string(strtok_r(NULL, "", &context));
    }
    else if (strcmp(req.command_text, "SHOWALL") == 0)
    {
        req.command = SHOWALL;
    }
    else if (strcmp(req.command_text, "SHOW") == 0)
    {
        req.command = SHOW;
        char* id_text = alloc_string(strtok_r(NULL, delim, &context));
        req.id = atoi(id_text);
    }
    else if (strcmp(req.command_text, "REPLY") == 0)
    {
        req.command = REPLY;
        char* id_text = alloc_string(strtok_r(NULL, delim, &context));
        req.id = atoi(id_text);
        req.content = alloc_string(strtok_r(NULL, "", &context));
    }
    else if (strcmp(req.command_text, "LOGIN") == 0)
    {
        req.command = LOGIN;
        req.login = alloc_string(strtok_r(NULL, delim, &context));
        req.password = alloc_string(strtok_r(NULL, delim, &context));
    }
    else if (strcmp(req.command_text, "REGISTER") == 0)
    {
        req.command = REGISTER;
        req.login = alloc_string(strtok_r(NULL, delim, &context));
        req.password = alloc_string(strtok_r(NULL, delim, &context));
    }
    else if (strcmp(req.command_text, "RECENT") == 0)
    {
        req.command = RECENT;
    }
    return req;
}

int showTopic(char* buffer, struct thread t, int i, int buf_len) {
    buf_len += sprintf(buffer + buf_len, "-- Thread %d: \n", i);
    buf_len += sprintf(buffer + buf_len, "-- Topic: %s \n", t.topic);
    buf_len += sprintf(buffer + buf_len, "-- User: %s \n", t.posts[0].author);
    char date_txt[30];
    strcpy(date_txt, ctime(&(t.posts[0].date)));
    date_txt[strlen(date_txt) - 1] = '\0';

    buf_len += sprintf(buffer + buf_len, "-- Post at %s:\n %s \n", date_txt, t.posts[0].content);
    for (int p = 1; p < forum.threads[i].post_num; p++)
    {
        buf_len += sprintf(buffer + buf_len, "-- Reply %d from %s:\n %s\n", p, t.posts[p].author, t.posts[p].content);
    }
    return buf_len;
}

void processRequest(struct request req, char* buffer, int usr)
{
    if (req.command == POST) {
        struct post p;
        p.date = time(NULL);
        p.author = users[usr].username;
        p.content = req.content;

        struct thread th;
        th.topic = req.topic;
        th.post_num = 1;
        th.post_cap = 10;
        th.posts = malloc(sizeof(struct post) * 10);
        th.posts[0] = p;

        if (forum.thread_num + 1 >= forum.thread_cap)
        {
            forum.threads = realloc(forum.threads, sizeof(struct thread)*forum.thread_cap * 2);
        }
        forum.threads[forum.thread_num] = th;
        sprintf(buffer, "Topic %d added\n", forum.thread_num);
        forum.thread_num++;
    }

    else if (req.command == SHOWALL) {
        bzero(buffer, 9999);
        int buf_len = 0;
        for (int i = 0; i < forum.thread_num; i++)
        {
            buf_len = showTopic(buffer, forum.threads[i], i, buf_len);
        }
    }

    else if (req.command == SHOW) {
        bzero(buffer, 9999);
        int i = req.id;
        showTopic(buffer, forum.threads[i], i, 0);
    }

    else if (req.command == REPLY) {
        struct post p;
        p.date = time(NULL);
        p.author = users[usr].username;
        p.content = req.content;

        struct thread* th = &forum.threads[req.id];
        if (th->post_num + 1 >= th->post_cap)
        {
            th->posts = realloc(th->posts, sizeof(struct post) * th->post_cap * 2);
        }
        th->posts[th->post_num] = p;
        sprintf(buffer, "Reply num %d posted\n", th->post_num);
        th->post_num++;
    }

    else if (req.command == RECENT) {
        bzero(buffer, 9999);
        int buf_len = 0;
        for (int th = 0; th < forum.thread_num; th++)
        {
            for (int n = 0; n < forum.threads[th].post_num; n++)
            {
                struct post* p = &(forum.threads[th].posts[n]);
                if (p->date > users[usr].last_seen)
                {
                    buf_len += sprintf(buffer + buf_len,
                        "-- Post from topic \"%s\" user %s:\n %s \n",
                        forum.threads[th].topic, p->author, p->content);
                }
            }
        }
        users[usr].last_seen = time(NULL);
    }

    else {
        strcpy(buffer, "Unknown command\n");
    }
}

int processAuth(struct request req, char* buffer)
{
    if (req.command == REGISTER) {
        struct user usr;
        assert(user_num < max_users);
        usr.username = req.login;
        usr.password = req.password;
        usr.last_seen = 0;
        users[user_num] = usr;
        sprintf(buffer, "User %s created successfully\n", usr.username);
        user_num++;
        return user_num - 1;
    }
    if (req.command == LOGIN) {
        for (int i = 0; i < user_num; i++)
        {
            if (strcmp(users[i].username, req.login) == 0 &&
                strcmp(users[i].password, req.password) == 0)
            {
                sprintf(buffer, "Logged in as %s\n", users[i].username);
                return i;
            }
        }
        strcpy(buffer, "Incorrect login or password\n");
        return -1;
    }
    strcpy(buffer, "You need to login first\n");
    return -1;
}

void loadDB() {
    char* pch;
    char* resp_buffer = malloc(9999);
    char* context = NULL;

    FILE* db = fopen("/home/kseniya/db.txt", "r");
    if (db == NULL) {
        printf("File error");
        getchar();
        exit(1);
    }

    // obtain file size:
    fseek(db, 0, SEEK_END);
    int lSize = ftell(db);
    printf("Size %d \n", lSize);
    rewind(db);

    // allocate memory to contain the whole file:
    char* buffer = (char*)malloc(lSize);
    bzero(buffer, lSize);

    // copy the file into the buffer:
    int result = fread(buffer, 1, lSize, db);
    printf("Read %d bytes\n", result);

    pch = strtok_r(buffer, "|", &context);
    int usr = -1;
    while (pch != NULL)
    {
        printf("Request is:\n %s \n", pch);
        struct request req = parseRequest(pch);
        int new_usr = processAuth(req, resp_buffer);
        if (new_usr >= 0)
        {
            usr = new_usr;
            printf("Auth Response is:\n%s \n", resp_buffer);
        }
        if (usr >= 0)
        {
            processRequest(req, resp_buffer, usr);
        }
        printf("Response is:\n%s \n", resp_buffer);
        pch = strtok_r(NULL, "|", &context);
    }
}

void* worker(void* arg) {
    int socket = *(int*)arg;
    printf("Worker is up\n");
    char* buffer = malloc(sizeof(char) * 9999);
    int n;
    struct sockaddr_in addr; // адрес клиента

    while (1) {
        struct client* cl = NULL;
        bzero(buffer, 9999);
        unsigned int addrLen = sizeof(addr);
        n = recvfrom(socket, buffer, 9999, 0,
                                (struct sockaddr*) &addr, &addrLen);
        if (n < 0) {
            perror("ERROR reading message");
            break;
        }

        for (int i = 0; i < clients_num; i++) {
            if (sockaddr_cmp(&addr, &clients[i].address)) {
                cl = &clients[i];
                break;
            }
        }
        if (cl == NULL) {
            clients[clients_num].address = addr;
            clients[clients_num].num = clients_num;
            clients[clients_num].usr_num = -1;
            cl = &clients[clients_num];
            clients_num++;
        }

        printf("Received %s\n", buffer);
        struct request req = parseRequest(buffer);

        if (cl->usr_num < 0)
        {
            cl->usr_num = processAuth(req, buffer);
        }
        else
        {
            processRequest(req, buffer, cl->usr_num);
        }

        // отправка с явным указанием адреса
        int n = sendto(socket, buffer, strlen(buffer), 0,
                (struct sockaddr*) &addr, sizeof(addr));
        if (n <= 0) {
                perror("ERROR writing to socket"); \
                break;
        }
    }
    return 0;
}

/*
*
*/
int main(int argc, char** argv) {
    int portno;
    int sockfd;

    struct sockaddr_in serv_addr;

    char command;
    pthread_t worker_thread;

    forum.thread_num = 0;
    forum.thread_cap = 10;
    forum.threads = malloc(sizeof(struct thread) * 10);

    loadDB();

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }
    /* Initialize socket structure */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = 5000;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // optval = 1;
    // setsockopt(sockfd, SOL_int, SO_REUSEADDR, &optval, sizeof optval);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Now start listening for the clients, here process will
    * go in sleep mode and will wait for the incoming connection
    */
    listen(sockfd, 5);

    pthread_create(&(worker_thread),
                NULL,
                worker,
                (void*) &sockfd);

    while (1) {
        command = getchar();
        if (command == 'q') {
            break;
        }
    }

    printf("Exit...\n");
    shutdown(sockfd, 2);
    close(sockfd);
    pthread_join(worker_thread, NULL);

    printf("Done\n");

    return 0;
}
