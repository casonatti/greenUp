#include <iostream>
#include <cstring>
#include <map>
#include <list>
#include <iomanip>

using namespace std;

struct participant {
    string hostname;       // Participant hostname
    string MAC;           // Participant MAC address
    string IP;             // Participant IP address
    string status;         // Participant Status
};

class participantsTable {
public:
    participantsTable() {
        table;
    }

    std::map<string, participant> table;

    void addParticipant(participant p);

    void deleteParticipant(const string &IPaddress);

    void printTable();

    void sleepParticipant(const string &IPaddress);

    void wakeParticipant(const string &IPAddress);

    std::list<string> getAllParticipantsIP();
};

void participantsTable::addParticipant(participant p) {
    p.status = "awake";
    table.insert_or_assign(p.IP, p);
}

void participantsTable::deleteParticipant(const string &IPaddress) {
    table.erase(IPaddress);
}

void participantsTable::printTable() {
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
