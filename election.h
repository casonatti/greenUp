//
// Created by pedro on 30/09/22.
//

#ifndef GREENUP_ELECTION_H
#define GREENUP_ELECTION_H

#include <string>
#include <cstring>
#include <list>
#include "packet.h"
#include "participantsTable.h"
#include <arpa/inet.h>
#include <thread>
#include "communication.h"
#include "globals.h"

using namespace std;

class Election {
  ssize_t ret_value;
  static inline int listenerSockfd, broadcastSockfd;
  static inline struct sockaddr_in listenerAddr, broadcastAddr;
public:
  static inline int result;

  static void monitorElection();

  static void startElectionThread();

  static void startElection();

  static void sendCoordinator();

  static bool isManagerAlive();
};

#endif //GREENUP_ELECTION_H
