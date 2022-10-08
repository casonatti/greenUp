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
#include <cstdlib>
#include <unistd.h>

typedef struct _participant {
  string hostname;  // Participant hostname
  string MAC;       // Participant MAC address
  string IP;        // Participant IP address
  string status;    // Participant status
  int pid;        // Participant PID
  bool isManager;
} Participant;

class ParticipantsTable {
  map<string, Participant> table;
  int id = 0;
  mutex tableMutex;
public:
  ParticipantsTable (string message);
  ParticipantsTable() = default;

  void addParticipant(Participant p);
S
  void addManager(Participant m);

  void deleteParticipant(const string &IPaddress);

  void printTable();

  void sleepParticipant(const string &IPaddress);

  void wakeParticipant(const string &IPAddress);

  list <string> getAllParticipantsIP();

  list <string> getBiggerParticipantsIP(int pid);

  string getParticipantMac(const string &hostname);

  int getParticipantPid(const string &hostname);

  const char *getParticipantStatus(const string &IPAddress);

  bool participantExists(const string &IPaddress);

  string parseTostring();

  void updateTable(string s);

};

#endif //GREENUP_USERSTABLE_H
