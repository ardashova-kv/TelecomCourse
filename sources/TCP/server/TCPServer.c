#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)  
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)

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
	enum {POST, REPLY, SHOWALL, SHOW, RECENT, LOGIN, REGISTER} command;
	char* topic;
	char* content;
	int id;
	char* login;
	char* password;
};

struct client
{
	SOCKET socket;
	int num;
	struct user* usr;
};

int worker_count = 0;
struct client clients[max_conn];

HANDLE mutex;

struct request parseRequest(char* request)
{
	struct request req;
	char* pch;
	char* context = NULL;
	char* delim = "\n";
	bzero(&req, sizeof(req));
	req.command_text = strtok_s(request, delim, &context);
	if (strcmp(req.command_text, "POST") == 0)
	{
		req.command = POST;
		req.topic = strtok_s(NULL, delim, &context);
		req.content = strtok_s(NULL, "", &context);
	}
	else if (strcmp(req.command_text, "SHOWALL") == 0)
	{
		req.command = SHOWALL;
	}
	else if (strcmp(req.command_text, "SHOW") == 0)
	{
		req.command = SHOW;
		char* id_text = strtok_s(NULL, delim, &context);
		req.id = atoi(id_text);
	}
	else if (strcmp(req.command_text, "REPLY") == 0)
	{
		req.command = REPLY;
		char* id_text = strtok_s(NULL, delim, &context);
		req.id = atoi(id_text);
		req.content = strtok_s(NULL, "", &context);
	}
	else if (strcmp(req.command_text, "LOGIN") == 0)
	{
		req.command = LOGIN;
		req.login = strtok_s(NULL, delim, &context);
		req.password = strtok_s(NULL, delim, &context);
	}
	else if (strcmp(req.command_text, "REGISTER") == 0)
	{
		req.command = REGISTER;
		req.login = strtok_s(NULL, delim, &context);
		req.password = strtok_s(NULL, delim, &context);
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
	date_txt[strlen(date_txt)-1] = '\0';

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
		th.post_cap = 1;
		th.posts = malloc(sizeof(struct post));
		th.posts[0] = p;

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
		th->posts[th->post_num] = p;
		th->post_num++;

		sprintf(buffer, "Reply num %d posted\n", forum.thread_num);
	}

	else if (req.command == RECENT) {
		bzero(buffer, 9999);
		int i = req.id;
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

	FILE* db = fopen("C:\\Forum\\db.txt", "r");
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

	pch = strtok_s(buffer, "|", &context);
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
		pch = strtok_s(NULL, "|", &context);
	}
}

int readn(SOCKET s, char* vptr, int n) {
	int nleft;
	int nread;
	char* ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		nread = recv(s, ptr, nleft, 0);
		if (nread == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEINTR)
				nread = 0;      /* and call read() again */
			else
				return (-1);
		}
		else if (nread == 0)
			return -1;

		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft);         /* return >= 0 */
}

int writeWithLength(SOCKET socket, const char* content) {
	char lenString[6];
	int len = strlen(content);
	bzero(lenString, 6);
	sprintf(lenString, "%04d\n", len);
	int n1 = send(socket, lenString, 5, 0);
	int n2 = send(socket, content, len, 0);
	if (n1 == SOCKET_ERROR || n2 == SOCKET_ERROR) {
		perror("ERROR writing to socket"); \
			return 1;
	}
	else {
		return 0;
	}
}

DWORD WINAPI worker(LPVOID arg) {
	struct client cl = *(struct client*)arg;
	printf("Worker number %d for socket %d is up\n", cl.num, cl.socket);
	char* buffer = malloc(sizeof(char)*9999);
	char* content_start;
	int usr = -1;
	char recv_len_buf[5];
	int recv_msg_len;
	int n;

	while (1) {
		n = readn(cl.socket, recv_len_buf, 5);
		if (n < 0) {
			perror("ERROR getting message length");
			break;
		}
		recv_len_buf[4] = '\0';
		recv_msg_len = atoi(recv_len_buf);
		printf("Receiving message with length %d\n", recv_msg_len);

		bzero(buffer, 9999);
		n = readn(cl.socket, buffer, recv_msg_len);
		if (n < 0) {
			perror("ERROR reading message");
			break;
		}

		printf("Received %s\n", buffer);
		struct request req = parseRequest(buffer);

		WaitForSingleObject(
			mutex,    // handle to mutex
			INFINITE);  // no time-out interval
		if (usr < 0)
		{
			usr = processAuth(req, buffer);
		} else
		{
			processRequest(req, buffer, usr);
		}

		ReleaseMutex(mutex);

		if (writeWithLength(cl.socket, buffer)) { break; }
	}
	return 0;
}

DWORD WINAPI accept_loop(LPVOID arg) {
	SOCKET accept_socket = *(int*)arg;

	HANDLE worker_thread[max_conn];

	int newsockfd;
	int clilen;
	struct sockaddr_in cli_addr;

	int i;

	clilen = sizeof(cli_addr);

	while (worker_count < max_conn) {
		printf("Waiting\n");
		/* Accept actual connection from the client */
		newsockfd = accept(accept_socket, NULL, NULL);
		if (newsockfd <= 0) {
			perror("ERROR on accept");
			break;
		}
		printf("Connection, socket: %d, thread %d\n", newsockfd, worker_count);

		clients[worker_count].socket = newsockfd;
		clients[worker_count].num = worker_count;
		clients[worker_count].usr = NULL;

		struct client* arg = &(clients[worker_count]);

		worker_thread[worker_count] = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			worker,       // thread function name
			(void*)arg,          // argument to thread function 
			0,                      // use default creation flags 
			NULL);   // returns the thread identifier 

		worker_count++;
	}

	printf("Closed accept socket\n");
	for (i = 0; i < worker_count; i++) {
		shutdown(clients[i].socket, 2);
		closesocket(clients[i].socket);
	}

	WaitForMultipleObjects(
		worker_count,           // number of objects in array
		worker_thread,     // array of objects
		TRUE,       // wait for all objects
		INFINITE);       // timeout
	return 0;
}

/*
*
*/
int main(int argc, char** argv) {
	int portno, clilen;
	SOCKET sockfd;

	struct sockaddr_in serv_addr, cli_addr;
	int  n;
	char optval;

	char command;

	// Initialize Winsock
	WSADATA wsaData;
	n = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (n != 0) {
		printf("WSAStartup failed with error: %d\n", n);
		return 1;
	}

	forum.thread_num = 0;
	forum.thread_cap = 10;
	forum.threads = malloc(sizeof(struct thread) * 10);

	loadDB();

	mutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	HANDLE accept_thread;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
	// setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

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

	accept_thread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		accept_loop,       // thread function name
		(void*)&sockfd,          // argument to thread function 
		0,                      // use default creation flags 
		NULL);   // returns the thread identifier 

	while (1) {
		command = getchar();
		if (command == 'q') {
			break;
		}
		else if (command == 'd') {
			int client;
			scanf("%d", &client);
			shutdown(clients[client].socket, 2);
			closesocket(clients[client].socket);
		}
	}

	printf("Exit...\n");
	shutdown(sockfd, 2);
	closesocket(sockfd);
	WaitForSingleObject(accept_thread, INFINITE);

	printf("Done\n");

	return 0;
}