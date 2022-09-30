//
// Created by pedro on 30/09/22.
//
#ifndef GREENUP_USERSTABLE_H
#define GREENUP_USERSTABLE_H

using namespace std;

#include <iostream>
#include <cstring>
#include <map>
#include <list>
#include <iomanip>
#include <mutex>

typedef struct _participant {
  string hostname;  // Participant hostname
  string MAC;       // Participant MAC address
  string IP;        // Participant IP address
  string status;    // Participant status
  int pid;        // Participant PID
} Participant;

class ParticipantsTable {
  map<string, Participant> table;

  mutex tableMutex;
public:
  ParticipantsTable() = default;

  void addParticipant(Participant p);

  void deleteParticipant(const string &IPaddress);

  void printTable();

  void sleepParticipant(const string &IPaddress);

  void wakeParticipant(const string &IPAddress);

  list<string> getAllParticipantsIP();

  string getParticipantMac(const string &hostname);

  const char *getParticipantStatus(const string &IPAddress);

  bool participantExists(const string &IPaddress);
};

#endif //GREENUP_USERSTABLE_H
