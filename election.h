//
// Created by pedro on 30/09/22.
//

#ifndef GREENUP_ELECTION_H
#define GREENUP_ELECTION_H

#include <string>
#include <string.h>
#include <list>
#include "packet.h"
#include "participantsTable.h"
#include <arpa/inet.h>
#include <thread>
#include "globals.cpp"

using namespace std;

class Election {

public:
  static void monitorElection();

  static void startElectionThread();
  //static void sendElectionMessage(ParticipantsTable table, string message);
};

#endif //GREENUP_ELECTION_H
