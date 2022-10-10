#include "participantsTable.h"

ParticipantsTable::ParticipantsTable(string message) {
    std::string s;
    std::string columnDelimiter = ",";
    string lineDelimiter = ";";
    Participant p;
    int i = 0;

    s = message;
    size_t column = 0;
    size_t line = 0;
    string lineToken;
    string fieldToken;

    while ((line = s.find(lineDelimiter)) != std::string::npos) {
        lineToken = s.substr(0, line);
        while ((column = lineToken.find(columnDelimiter)) != std::string::npos) {
            fieldToken = lineToken.substr(0, column);
            switch (i) {
                case 0:
                    p.hostname = fieldToken;
                    break;
                case 1:
                    p.MAC = fieldToken;
                    break;
                case 2:
                    p.IP = fieldToken;
                    break;
                case 3:
                    p.status = fieldToken;
                    break;
                default:
                    break;
            }
            lineToken.erase(0, column + columnDelimiter.length());
            i++;
        }
        p.pid = stoi(lineToken);
        table.insert({p.IP, p});
        s.erase(0, line + lineDelimiter.length());
        i = 0;
    }
    printTable();
}

void ParticipantsTable::addParticipant(Participant p) {
    p.status = "awake";
    tableMutex.lock();
    p.pid = id++;
    p.isManager = false;
    table.insert({p.IP, p});
    tableMutex.unlock();
}

void ParticipantsTable::addManager(Participant m) {
    m.status = "awake";
    tableMutex.lock();
    m.pid = id++;
    m.isManager = true;
    if(table.count(m.IP))
        table.erase(m.IP);
    table.insert({m.IP, m});
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
    cout << "|Hostname \t|MAC Address      |IP Address     |Status|PID |isManager |\n";
    for (auto &ent: table) {
        cout << "|" << setw(15) << ent.second.hostname;
        cout << "|" << ent.second.MAC;
        cout << "|" << setw(15) << ent.second.IP;
        cout << "|" << setw(6) << ent.second.status;
        cout << "|" << setw(4) << ent.second.pid;
        cout << "|" << setw(10) << ent.second.isManager << "|\n";
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

list <string> ParticipantsTable::getBiggerParticipantsIP(int pid) {
    tableMutex.lock();
    std::list<string> listP = {};
    auto it = listP.begin();
    for (auto &ent: table) {
        if (ent.second.pid > pid)
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

int ParticipantsTable::getParticipantPid(const string &ip) {
    int pid;
    tableMutex.lock();
    for (auto &ent: table) {
        if (ent.second.IP == ip) {
            tableMutex.unlock();
            return ent.second.pid;
        }
    }
    tableMutex.unlock();
    return -1;
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

string ParticipantsTable::parseTostring() {
    string message = "";
    tableMutex.lock();
    for (auto &ent: table) {
        message =
                message + ent.second.hostname + "," + ent.second.MAC + "," + ent.second.IP + "," + ent.second.status +
                "," + to_string(ent.second.pid) + "," + to_string(ent.second.isManager) + ";";
    }
    tableMutex.unlock();
    message = message + to_string(id);
    return message;
}

void ParticipantsTable::updateTable(string s) {
    table.clear();
    std::string columnDelimiter = ",";
    string lineDelimiter = ";";
    Participant p;
    int i = 0;

    size_t column = 0;
    size_t line = 0;
    string lineToken;
    string fieldToken;

    while ((line = s.find(lineDelimiter)) != std::string::npos) {
        lineToken = s.substr(0, line);
        while ((column = lineToken.find(columnDelimiter)) != std::string::npos) {
            fieldToken = lineToken.substr(0, column);
            switch (i) {
                case 0:
                    p.hostname = fieldToken;
                    break;
                case 1:
                    p.MAC = fieldToken;
                    break;
                case 2:
                    p.IP = fieldToken;
                    break;
                case 3:
                    p.status = fieldToken;
                    break;
                case 4:
                    p.pid = stoi(lineToken);
                    break;
                default:
                    break;
            }
            lineToken.erase(0, column + columnDelimiter.length());
            i++;
        }
        p.isManager = stoi(lineToken);
        table.insert({p.IP, p});
        s.erase(0, line + lineDelimiter.length());
        i = 0;
    }
    id = stoi(s);
    printTable();
}
