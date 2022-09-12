#ifndef PARTICIPANTSTABLE
#define PARTICIPANTSTABLE
#include <iostream>
#include <cstring>
#include <map>
#include <list>
#include <iomanip>

using namespace std;

struct participant {
    string hostname;  // Participant hostname
    string MAC;       // Participant MAC address
    string IP;        // Participant IP address
    string status;    // Participant status
};

class participantsTable {
public:
    participantsTable() = default;

    std::map<string, participant> table;

    void addParticipant(participant p);

    void deleteParticipant(const string &IPaddress);

    void printTable();

    void sleepParticipant(const string &IPaddress);

    void wakeParticipant(const string &IPAddress);

    std::list<string> getAllParticipantsIP();

    string getParticipantMac(const string &hostname);

    const char* getParticipantStatus(const string &IPAddress);

    bool participantExists(const string &IPaddress);
};

void participantsTable::addParticipant(participant p) {
    p.status = "awake";
    table.insert(std::pair<string, participant> (p.IP, p));
}

void participantsTable::deleteParticipant(const string &IPaddress) {
    table.erase(IPaddress);
}

void participantsTable::printTable() {
    //system("clear");
    cout << std::left;
    cout << "--------------------------------------------------------------------------\n";
    cout << "|Hostname \t|MAC Address      |IP Address     |Status|\n";
    for (auto &ent: table) {
        cout << "|" << setw(15) << ent.second.hostname;
        cout << "|" << ent.second.MAC;
        cout << "|" << setw(15) << ent.second.IP;
        cout << "|" << setw(6) << ent.second.status << "|\n";
    }
    cout << "--------------------------------------------------------------------------\n";
}

void participantsTable::sleepParticipant(const string &IPAddress) {
    table.at(IPAddress).status = "asleep";
}

void participantsTable::wakeParticipant(const string &IPAddress) {
    table.at(IPAddress).status = "awake";
}

std::list<string> participantsTable::getAllParticipantsIP() {
    std::list<string> listP = {};
    auto it = listP.begin();
    for (auto &ent: table) {
        listP.insert(it, ent.second.IP);
    }
    return listP;
}

string participantsTable::getParticipantMac(const string& hostname) {
    std::string macaddr = {};
    for (auto &ent: table) {
        if (ent.second.hostname == hostname) {
            return ent.second.MAC;
        }
    }
    return "";
}

const char* participantsTable::getParticipantStatus(const string& IPAddress) {
    return table.at(IPAddress).status.c_str();
}

bool participantsTable::participantExists(const string &IPaddress) {
    for (auto &ent: table) {
        if (ent.second.IP == IPaddress) {
            return true;
        }
    }
    return false;
}

#endif