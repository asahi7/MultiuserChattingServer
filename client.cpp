#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <sys/socket.h>
#include <string.h>

using namespace std;

bool is_digit(const char *str) {
    for(int i = 0; i < strlen(str); i++) {
        if(! (str[i] >= '0' && str[i] <= '9')) {
            return false;
        }
    }
    return true;
}

int convert_str_to_int(const char *str) {
    int res = 0;
    for(int i = 0; i < strlen(str); i++) {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cout << "Incorrect number of arguments: [host:port] [room_no] [client_name]" << endl;
        exit(1);
    }

    int room_no;
    string host = "";
    int port;
    string client_name = argv[3];

    if (!is_digit(argv[2])) {
        cout << "Incorrect room_no" << endl;
        exit(1);
    }

    room_no = convert_str_to_int(argv[2]);

    string port_str = "";
    for (int i = 0, port_started = 0; i < strlen(argv[1]); i++) {
        if (argv[1][i] == ':') {
            port_started = 1;
            continue;
        }
        if (!port_started) {
            host += argv[1][i];
        } else {
            port_str += argv[1][i];
        }
    }

    if (!is_digit(port_str.c_str())) {
        cout << "Incorrect port" << endl;
        exit(1);
    }

    port = convert_str_to_int(port_str.c_str());

    cout << "User has entered the following parameters: " << host << ":" << port << " " << room_no << " " << client_name
         << endl;

    int sockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    clilen = sizeof(serv_addr);

    if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0)
    {
        printf("Address is not correct");
        exit(-1);
    }

    /* Acccepting connections */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, clilen) < 0) {
        printf("ERROR on connecting");
        exit(-1);
    }

    char buffer[4096];
    string msg;
    const char* msg_c;
    bzero(buffer, sizeof(buffer));

    msg = "/new " + to_string(room_no) + " " + client_name;
    msg_c = msg.c_str();

    send(sockfd, msg_c, strlen(msg_c), 0);

    recv(sockfd, buffer, sizeof(buffer), 0);

    cout << buffer << endl;

    close(sockfd);
}