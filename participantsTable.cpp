#include <iostream>
#include <cstring>
#include "globals.cpp"
#include <map>
using namespace std;

class participantsTable{
    public:
        participantsTable(){
            table;
        }
        std::map<sockaddr_in*, participant> table;
        
        void updateParticipant(sockaddr_in* address, participant p);
        void deleteParticipant(sockaddr_in* address);
        void printTable();
        void sleepParticipant(sockaddr_in* address);
};

void participantsTable::updateParticipant(sockaddr_in* address, participant p){
    table.insert_or_assign(address, p);
    return;
}

void participantsTable::deleteParticipant(sockaddr_in* address){
    table.erase(address);
    return;
}

void participantsTable::sleepParticipant(sockaddr_in* address){
    participant p = table.at(address);
    strcpy(p.status, "awake");
    table.insert_or_assign(address, p);
}

void participantsTable::printTable(){
    cout << "---------------------------------------------------------\n";
    cout << "|Hostname\t|MAC Address\t|IP Address\t|Status\t|\n";
    for(auto const &ent : table){
        cout << "|" << ent.second.hostname << "\t";
        cout << "|" << ent.second.MAC << "\t";
        cout << "|" << ent.second.IP << "\t";
        cout << "|" << ent.second.status << "\t|\n";
    }
    cout << "---------------------------------------------------------\n";
    return;
}