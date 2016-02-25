// UDPClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <assert.h>

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)  
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

int main(int argc, char** argv) {
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char command[20];

	char* recv_buffer = malloc(sizeof(char) * 10000);
	char* send_buffer = malloc(sizeof(char) * 10000);
	char* send_content = malloc(sizeof(char) * 10000);

	int recv_num = 0;
	int send_num = 0;

	// Initialize Winsock
	WSADATA wsaData;
	n = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (n != 0) {
		printf("WSAStartup failed with error: %d\n", n);
		return 1;
	}

	const int portno = 5000;
	/* Create a socket point */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("ERROR opening socket");
		exit(1);
	}
	printf("Please enter the server address: \n");
	char addr[100];
	bzero(addr, 100);
	fgets(addr, 100, stdin);
	addr[strlen(addr) - 1] = '\0';
	server = gethostbyname(addr);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
		(char *)&serv_addr.sin_addr.s_addr,
		server->h_length);
	serv_addr.sin_port = htons(portno);

	/* Now connect to the server */
	if (connect(sockfd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR connecting");
		exit(1);
	}

	while (1) {
		/* Now ask for a message from the user, this message
		* will be read by server
		*/
		printf("Please enter the command: \n");
		bzero(command, 20);
		fgets(command, 20, stdin);

		if (strlen(command) == 0) {
			perror("Empty message, try again");
			continue;
		}
		printf("Please enter the query (finish with an empty line): \n");
		bzero(send_content, 10000);
		char* ptr = send_content;
		while (1) {
			fgets(ptr, 10000, stdin);
			if (ptr[0] == '\n') {
				break;
			}
			ptr += strlen(ptr);
		};
		// Remove EOL
		for (int i = strlen(send_content) - 1; i >= 0; i--) {
			if (send_content[i] == '\n') {
				send_content[i] = '\0';
			}
			else {
				break;
			}
		}
		/* Send message to the server */
		bzero(send_buffer, 10000);
		strcat(send_buffer, command);
		strcat(send_buffer, send_content);
		int n = send(sockfd, send_buffer, strlen(send_buffer), 0);
		/* Now read server response */
		bzero(recv_buffer, 10000);
		n = recv(sockfd, recv_buffer, 10000, 0);
		if (n == INVALID_SOCKET) {
			perror("ERROR reading message");
			break;
		}

		printf("Received: %s\n", recv_buffer);
	}
	return 0;
}