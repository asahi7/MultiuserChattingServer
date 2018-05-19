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
#include <chrono>
#include <thread>
#include <set>
#include <mutex>
#include <unordered_map>

#define PORT 5000

std::unordered_map<int, std::unordered_map<std::string, int> > chats;
std::mutex mtx;

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

void send_message(int sock, std::string msg) {
    const char *msg_c = msg.c_str();
    // std::cout << msg_c << std::endl; // TODO remove in future
    send(sock, msg_c, strlen(msg_c), 0);
}

void send_room_msg_exc(int room_no, std::string msg, std::set<std::string> except) {
    mtx.lock();
    auto it = chats[room_no].begin();
    for(; it != chats[room_no].end(); it++) {
        std::string user_n = (*it).first;
        if(except.find(user_n) == except.end()) {
            send_message((*it).second, msg);
        }
    }
    mtx.unlock();
}

std::set<std::string> send_room_msg_to(int room_no, std::string msg, std::set<std::string> rcps) {
    mtx.lock();
    auto map = chats[room_no];
    std::set<std::string> unfound;
    for(std::string rcp : rcps) {
        if(map.find(rcp) != map.end()) {
            send_message(map[rcp], msg);
        } else {
            unfound.insert(rcp);
        }
    }
    mtx.unlock();
    return unfound;
}


std::set<std::string> get_recepients(char *ch) {
    std::set<std::string> res;
    std::string rec;
    for(int i = 0; i < strlen(ch); i++) {
        if(ch[i] == ':') {
            res.insert(rec);
            break;
        }
        if(ch[i] != ' ' && ch[i] != ',') {
            rec += ch[i];
        } else if(ch[i] == ',') {
            res.insert(rec);
            rec = "";
        }
    }
    return res;
}

std::string get_msg(char* ch) {
    bool message_start = false;
    std::string res = "";
    for(int i = 0; i < strlen(ch); i++) {
        if(message_start) {
            res += ch[i];
        }

        if(ch[i] == ':' && ! message_start) {
            message_start = true;
            i++;
            if(ch[i] == ' ') {
                continue;
            }
        }
    }
    return res;
}

void respond(int sock) { // individual client's thread
    char buffer[4096], bufcpy[4096];
    int room_no;
    std::string username;

    while(1) {

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        bzero(buffer, sizeof(buffer));
        recv(sock, buffer, sizeof(buffer), 0);
        if(strlen(buffer) == 0) {
            continue;
        }

        strcpy(bufcpy, buffer);

        std::cout << "Client sent: " << buffer << std::endl;

        char * token = strtok(buffer, " ");

        if(strncmp(token, "/new", 4) == 0) { // TODO allow this only once for a client
            token = strtok(NULL, " "); // TODO may be NULL, check
            room_no = atoi(token);
            token = strtok(NULL, " "); // TODO may be NULL, check
            username = token;

            mtx.lock();
            chats[room_no].insert(make_pair(username, sock)); // inserting new member to chat
            mtx.unlock();

            send_room_msg_exc(room_no, username + " joined room " + std::to_string(room_no), {username});

            send_message(sock, "Hello " + username + "! This is room #" + std::to_string(room_no));
        } else if(strncmp(token, "/join", 5) == 0) {
            token = strtok(NULL, " "); // TODO may be NULL, check
            int new_room_no = atoi(token);
            mtx.lock();
            chats[room_no].erase(chats[room_no].find(username));
            chats[new_room_no].insert(make_pair(username, sock));
            room_no = new_room_no;
            mtx.unlock();
        } else if(strncmp(token, "/quit”", 5) == 0) {
            mtx.lock();
            chats[room_no].erase(chats[room_no].find(username));
            mtx.unlock();
            send_message(sock, "Good bye " + username);
            send_room_msg_exc(room_no, username + " is disconnected from room #" + std::to_string(room_no), {username});
            break;
        } else if(strncmp(token, "/list”", 5) == 0) {
            mtx.lock();
            int i = 1;
            std::string msg = "This is list of users in room " + std::to_string(room_no);
            for(auto user = chats[room_no].begin(); user != chats[room_no].end(); user++) { // TODO should a user also be displayed here?
                msg += "\n" + std::to_string(i) + ". " + (*user).first;
                i++;
            }
            mtx.unlock();
            send_message(sock, msg);
        }
        else { // send message command
            if(strncmp(bufcpy, "All : ", 6) == 0) {
                std::string msg_str(bufcpy);
                std::string msg = msg_str.substr(6, std::string::npos);
                send_room_msg_exc(room_no, username + " : " + msg, {username});
            } else {
                auto rcps = get_recepients(bufcpy);
                std::string msg = get_msg(bufcpy);
                auto unfound = send_room_msg_to(room_no, username + " : " + msg, rcps);
                std::string unf_msg = "";
                bool first = 1;
                for(std::string name : unfound) {
                    if(! first) {
                        unf_msg += ", ";
                    }
                    unf_msg += name;
                    first = 0;
                }
                if(unfound.size() != 0) {
                    send_message(sock, "There is no such user : " + unf_msg);
                }
            }
        }
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

}
