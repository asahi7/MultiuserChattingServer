#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <string>
#include "threadpool/ThreadPool.h"

#define PORT 5000

void respond(int sock);

int main(int argc, char *argv[]) {
    int sockfd, portno = PORT;
    socklen_t clilen;
    struct sockaddr_in serv_addr;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Make port reusable */
    int tr = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Bind socket */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listen on socket */
    if (listen(sockfd, 1000) < 0) {
        perror("ERROR on listening");
        exit(-1);
    }

    printf("Server is running on port %d\n", portno);

    clilen = sizeof(serv_addr);

    ThreadPool pool(4);

    while (1) {
        /* Acccepting connections */
        int newsockfd = accept(sockfd, (struct sockaddr *) &serv_addr, (socklen_t *) &clilen);
        if (newsockfd < 0) {
            printf("ERROR on accepting");
            exit(-1);
        }

        pool.enqueue([](int newsock) {
                         respond(newsock);
                     }, newsockfd
        );
    }

    return 0;
}

void respond(int sock) {
    char buffer[4096];
    while(1) {

        bzero(buffer, sizeof(buffer));
        recv(sock, buffer, sizeof(buffer), 0);

        std::cout << "Client sent: " << buffer << std::endl;

        char * token = strtok(buffer, " ");

        if(strncmp(token, "/new", 4) == 0) {
            std::cout << "Create new user?" << std::endl; // TODO create new user
            token = strtok(NULL, " "); // TODO may be NULL, check
            std::string room_no(token);
            token = strtok(NULL, " "); // TODO may be NULL, check
            std::string username(token);
            std::string msg = "Hello " + username;
            const char *msg_c = msg.c_str();
            std::cout << msg_c << std::endl;
            send(sock, msg_c, sizeof(msg_c), 0);
            break;
        } else {
            std::cout << "Unknown command" << std::endl; // TODO send to client
        }
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

}
