#include "participantsTable.h"

void ParticipantsTable::addParticipant(Participant p) {
    p.status = "awake";
    tableMutex.lock();
    p.pid = id++;
    table.insert({p.IP, p});
    tableMutex.unlock();
}

void ParticipantsTable::deleteParticipant(const string &IPaddress) {
    tableMutex.lock();
    table.erase(IPaddress);
    tableMutex.unlock();
}

void ParticipantsTable::printTable() {
    tableMutex.lock();
    cout << std::left;
    cout << "--------------------------------------------------------------------------\n";
    cout << "|Hostname \t|MAC Address      |IP Address     |Status|PID|\n";
    for (auto &ent: table) {
        cout << "|" << setw(15) << ent.second.hostname;
        cout << "|" << ent.second.MAC;
        cout << "|" << setw(15) << ent.second.IP;
        cout << "|" << setw(6) << ent.second.status;
        cout << "|" << setw(3) << ent.second.pid << "|\n";
    }
    cout << "--------------------------------------------------------------------------\n";
    tableMutex.unlock();
}

void ParticipantsTable::sleepParticipant(const string &IPAddress) {
    tableMutex.lock();
    table.at(IPAddress).status = "asleep";
    tableMutex.unlock();
}

void ParticipantsTable::wakeParticipant(const string &IPAddress) {
    tableMutex.lock();
    table.at(IPAddress).status = "awake";
    tableMutex.unlock();
}

list <string> ParticipantsTable::getAllParticipantsIP() {
    tableMutex.lock();
    std::list<string> listP = {};
    auto it = listP.begin();
    for (auto &ent: table) {
        listP.insert(it, ent.second.IP);
    }
    tableMutex.unlock();
    return listP;
}

string ParticipantsTable::getParticipantMac(const string &hostname) {
    std::string macaddr = {};
    tableMutex.lock();
    for (auto &ent: table) {
        if (ent.second.hostname == hostname) {
            tableMutex.unlock();
            return ent.second.MAC;
        }
    }
    tableMutex.unlock();
    return "";
}

const char *ParticipantsTable::getParticipantStatus(const string &IPAddress) {
    tableMutex.lock();
    const char *status = table.at(IPAddress).status.c_str();
    tableMutex.unlock();
    return status;
}

bool ParticipantsTable::participantExists(const string &IPaddress) {
    tableMutex.lock();
    for (auto &ent: table) {
        if (ent.second.IP == IPaddress) {
            tableMutex.unlock();
            return true;
        }
    }
    tableMutex.unlock();
    return false;
}