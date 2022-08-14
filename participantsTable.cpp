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
        std::map<const char*, participant> table;
        
        void updateTable(participant p);
        void deleteParticipant(const char* IPaddress);
        void printTable();
        void sleepParticipant(const char* IPaddress);
        bool isAwake(const char* IPaddress);
};

void participantsTable::updateTable(participant p){
    if(isAwake(p.IP))
        return;
    strcpy(p.status, "awake");
    table.insert_or_assign(p.IP, p);
    return;
}

void participantsTable::deleteParticipant(const char* IPaddress){
    table.erase(IPaddress);
    return;
}

void participantsTable::sleepParticipant(const char* IPaddress){
    participant p = table.at(IPaddress);
    strcpy(p.status, "awake");
    table.insert_or_assign(IPaddress, p);
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

bool participantsTable::isAwake(const char* address){
    if(table.count(address)){
        participant p = table.at(address);
        return strcmp(p.status, "awake") == 0;
    }
    return false;
}

participant parsePayload(char* payLoad){
    std::string s;
    std::string delimiter = ", ";
    participant p;
    int i = 0;

    s = payLoad;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        std::cout << token << std::endl;
        switch (i)
        {
        case 0:
            p.hostname = token.c_str();
            break;
        case 1:
            p.IP = token.c_str();
            break;
        case 2:
            p.MAC = token.c_str();
            break;    

        }
        s.erase(0, pos + delimiter.length());
        i++;
    }
    return p;
}