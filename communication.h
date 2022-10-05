//
// Created by pedro on 04/10/22.
//

#ifndef GREENUP_COMMUNICATION_H
#define GREENUP_COMMUNICATION_H

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
using namespace std;

class Communication {
public:
  static int createSocket();
  static struct sockaddr_in bindSocket(int sockfd, int port);
  static struct sockaddr_in createBroadcastAddress(int port);

};

#endif //GREENUP_COMMUNICATION_H
