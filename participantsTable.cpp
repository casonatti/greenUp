#include <iostream>
#include <cstring>
#include <map>
using namespace std;

struct participant {
    string hostname;       // Participant hostname
    string MAC;            // Participant MAC address
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
        void sleepTable();
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
    cout << "--------------------------------------------------------------------------\n";
    cout << "|Hostname\t|MAC Address\t\t|IP Address\t\t|Status\t|\n";
    for(auto &ent : table){
        cout << "|" << ent.second.hostname << "\t";
        cout << "|" << ent.second.MAC << "\t";
        cout << "|" << ent.second.IP << "\t\t";
        cout << "|" << ent.second.status << "\t|\n";
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

void participantsTable::sleepTable(){
    cout << table.size();
    for(auto &ent : table){
        ent.second.status = "asleep";
        cout << ent.second.hostname;
        cout << ent.second.IP;
    }
}