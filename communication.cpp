//
// Created by pedro on 04/10/22.
//

#include "communication.h"

int Communication::createSocket() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cout << "Socket creation error";
        exit(0);
    }
    return sockfd;
}

struct sockaddr_in Communication::bindSocket(int sockfd, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET; // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *) &addr,
             sizeof(addr)) < 0) {
        cout << "Bind socket error";
        exit(0);
    }
    return addr;
}