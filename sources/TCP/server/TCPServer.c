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
};

struct post
{
	char* author;
	long date;
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
	enum {POST, CREATE, SHOWALL, RECENT} command;
	char* topic;
	char* content;
	char* id;
};

int worker_count = 0;
SOCKET worker_socket[max_conn];

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
	}
	else if (strcmp(req.command_text, "SHOWALL") == 0)
	{
		req.command = SHOWALL;
		// req.id = strtok(NULL, delim);
	}
	req.content = strtok_s(NULL, "", &context);
	return req;
}

void processRequest(struct request req, char* buffer)
{
	if (req.command == POST) {
		struct post p;
		p.date = time(NULL);
		p.author = "abc";
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
			buf_len += sprintf(buffer + buf_len, "-- Thread %d: \n", i);
			buf_len += sprintf(buffer + buf_len, "-- Topic: %s \n", forum.threads[i].topic);
			buf_len += sprintf(buffer + buf_len, "-- Post: %s \n", forum.threads[i].posts[0].content);
			for (int p = 0; p < forum.threads[i].post_num; p++)
			{
				buf_len += sprintf(buffer + buf_len, "%s\n", forum.threads[i].posts[p].content);
			}
		}
	}

	else {
		strcpy(buffer, "Unknown command\n");
	}
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
	while (pch != NULL)
	{
		printf("Request is:\n %s \n", pch);
		struct request req = parseRequest(pch);
		processRequest(req, resp_buffer);
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
	SOCKET socketfd = *(SOCKET*)arg;
	printf("Worker for %d is up\n", socketfd);
	char* buffer = malloc(sizeof(char)*9999);
	char* content_start;
	char recv_len_buf[5];
	int recv_msg_len;
	int n;

	while (1) {
		n = readn(socketfd, recv_len_buf, 5);
		if (n < 0) {
			perror("ERROR getting message length");
			break;
		}
		recv_len_buf[4] = '\0';
		recv_msg_len = atoi(recv_len_buf);
		printf("Receiving message with length %d\n", recv_msg_len);

		bzero(buffer, 9999);
		n = readn(socketfd, buffer, recv_msg_len);
		if (n < 0) {
			perror("ERROR reading message");
			break;
		}

		printf("Received %s\n", buffer);
		struct request req = parseRequest(buffer);

		WaitForSingleObject(
			mutex,    // handle to mutex
			INFINITE);  // no time-out interval

		processRequest(req, buffer);

		ReleaseMutex(mutex);

		if (writeWithLength(socketfd, buffer)) { break; }
	}
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

		worker_socket[worker_count] = newsockfd;

		SOCKET* sock = &(worker_socket[worker_count]);

		worker_thread[worker_count] = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			worker,       // thread function name
			(void*)sock,          // argument to thread function 
			0,                      // use default creation flags 
			NULL);   // returns the thread identifier 

		worker_count++;
	}

	printf("Closed accept socket\n");
	for (i = 0; i < worker_count; i++) {
		shutdown(worker_socket[i], 2);
		closesocket(worker_socket[i]);
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
			shutdown(worker_socket[client], 2);
			closesocket(worker_socket[client]);
		}
	}

	printf("Exit...\n");
	shutdown(sockfd, 2);
	closesocket(sockfd);
	WaitForSingleObject(accept_thread, INFINITE);

	printf("Done\n");

	return 0;
}