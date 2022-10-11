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
  static struct sockaddr_in monitorAddr, startAddr,broadcastAddr, coordinatorAddr;
public:
  static int monitorSockfd, startSockfd, coordinatorSockfd;
  static int result;
  static bool alreadyJoined;

  static void monitorElection();

  static void startElectionThread();

  static void startElection();

  static void sendCoordinator();

  static bool isManagerAlive();
};


#endif //GREENUP_ELECTION_H


