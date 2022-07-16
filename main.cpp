#include <csignal>
#include <iostream>
#include <string.h>
using namespace std;

#define BUFFER_SIZE 128
#define MAX_MACHINES 10

typedef struct __managerDB {
    const char* hostname;
    const char* MAC;
    const char* IP;
    const char* status;
} managerDB;

typedef struct __packet {
    uint16_t type;          //DATA | CMD
    uint16_t seqn;          //sequence number
    uint16_t length;        //payload length
    uint16_t timestamp;     //packet timestamp
    const char* payload;    //packet data
} packet;

void signalHandler(int signum) { //CTRL+C handler
    const char* exit_command = "EXIT";
    const char* exit_message = "sleep service exit";
    char buffer[BUFFER_SIZE] = { 0 };

    memset(buffer, ' ', strlen(buffer));
        
    cout << "sending command to server: \"" << exit_command << "\"" << endl;
    //send(sock, exit_command, strlen(exit_command), 0);
        
    //recv(sock, buffer, BUFFER_SIZE, 0);
    cout << "received message from server: \"" << buffer << "\"" << endl << endl;

    memset(buffer, ' ', strlen(buffer));
        
    cout << "sending message to server: \"" << exit_message << "\"" << endl;
    //send(sock, exit_message, strlen(exit_message), 0);
        
    //recv(sock, buffer, BUFFER_SIZE, 0);
    cout << "received message from server: \"" << buffer << "\"" << endl << endl;

    //close(sock);

    exit(signum);
}

int main(int argc, char** argv) {
    int n_machines = 0; //number of machines connected
    signal(SIGINT, signalHandler); //CTRL+C
    signal(SIGHUP, signalHandler); //terminal closed while process still running

//---------------------------------------------------------- PARTICIPANT section ----------------------------------------------------------
    if(argc == 1) {
        // while(true) {

        // }
    }

//------------------------------------------------------------ MANAGER section ------------------------------------------------------------
    //argv[1] does exist.
    if(argc == 2) { 
        if(strcmp(argv[1], "manager") != 0) { //argv[1] != "manager"
            cout << "argv NOT OK" << endl;
            return -1;
        }

        // while(true) {

        // }
    }

    return 0;
}