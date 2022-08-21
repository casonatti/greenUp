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

class participantsTable{
    public:
        participantsTable(){
            table;
        }
        std::map<string, participant> table;
        
        void updateTable(participant p);
        void deleteParticipant(string IPaddress);
        void printTable();
        bool isAwake(string IPaddress);
        void sleepParticipant(string IPaddress);
        std::list<string> getAllParticipantsIP();
};

void participantsTable::updateTable(participant p){
    p.status = "awake";
    table.insert_or_assign(p.IP, p);
    return;
}

void participantsTable::deleteParticipant(string IPaddress){
    table.erase(IPaddress);
    return;
}


void participantsTable::printTable(){
    cout << std::left;
    cout << "--------------------------------------------------------------------------\n";
    cout << "|Hostname \t|MAC Address      |IP Address     |Status|\n";
    for(auto &ent : table){
        cout << "|" << setw(15) << ent.second.hostname;
        cout << "|" << ent.second.MAC;
        cout << "|" << setw(15) << ent.second.IP;
        cout << "|" << setw(6) << ent.second.status << "|\n";
    }
    cout << "----------------------------------------------------------\n";
    return;
}

bool participantsTable::isAwake(string IPaddress){
    if(table.count(IPaddress)){
        participant p = table.at(IPaddress);
        return p.status == "awake";
    }
    return false;
}

void participantsTable::sleepParticipant(string IPAddress){
    table.at(IPAddress).status = "asleep";
    return;
}

std::list<string> participantsTable::getAllParticipantsIP(){
    std::list<string> listP = {};
    std::list<string>::iterator it = listP.begin();
    for(auto &ent : table){
        listP.insert(it, ent.second.IP);
    }
    return listP;
}