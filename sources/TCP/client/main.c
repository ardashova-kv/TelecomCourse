#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <errno.h>

int writeWithLength(int socket, const char* command, const char* content) {
    char lenString[6];
    int len = strlen(command) + strlen(content);
    bzero(lenString, 6);
    sprintf(lenString, "%04d\n", len);
    int n1 = send(socket, lenString, 5, 0);
    int n2 = send(socket, command, strlen(command), 0);
    int n3 = send(socket, content, strlen(content), 0);
    if (n1 < 0 || n2 < 0 || n3 < 0) {
        perror("ERROR writing to socket"); \
        return 1;
    } else {
        return 0;
    }
}

int readn(int s, char* vptr, int n) {
    int nleft;
    int nread;
    char* ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nread = recv(s, ptr, nleft, 0)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return (-1);
        }
        else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);         /* return >= 0 */
}

/*
 *
 */
int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int i;

    char* content = malloc(10000);
    char command[6];

    char* recv_buffer = malloc(10000);

    char recv_len_buf[5];
    int recv_msg_len;

    const int portno = 5000;
    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }
    printf("Please enter the server address: \n");
    char addr[100];
    bzero(addr, 100);
    fgets(addr, 100, stdin);
    addr[strlen(addr)-1] = '\0';
    server = gethostbyname(addr);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd,(const struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        exit(1);
    }

    while (1) {
        /* Now ask for a message from the user, this message
         * will be read by server
         */
        printf("Please enter the command: \n");
        bzero(command, 6);
        fgets(command, 6, stdin);

        if (strlen(command) == 0) {
            perror("Empty message, try again");
            continue;
        }
        printf("Please enter the query (finish with an empty line): \n");
        bzero(content, 10000);
        char* ptr = content;
        while(1) {
            fgets(ptr, 10000, stdin);
            if (ptr[0] == '\n') {
                break;
            }
            ptr += strlen(ptr);
        };
        // Remove EOL
        for (i = strlen(content) - 1; i >= 0; i--) {
            if (content[i] == '\n') {
                content[i] = '\0';
            } else {
                break;
            }
        }
        /* Send message to the server */
        writeWithLength(sockfd, command, content);
        /* Now read server response */
        int n = readn(sockfd, recv_len_buf, 5);
        if (n <= 0) {
            perror("ERROR getting message length");
            break;
        }
        sscanf(recv_len_buf, "%d\n", &recv_msg_len);
        printf("Receiving message with length %d\n", recv_msg_len);

        bzero(recv_buffer, 10000);
        n = readn(sockfd, recv_buffer, recv_msg_len);
        if (n < 0) {
            perror("ERROR reading message");
            break;
        }

        printf("Received: %s\n", recv_buffer);
    }
    return 0;
}
