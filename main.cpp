#include <csignal>
#include <iostream>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

#define BUFFER_SIZE 32
#define MAX_MACHINES 10
#define PORT_SERVER 8000
#define PORT_PARTICIPANT 4000

#define SLEEP_SERVICE_DISCOVERY "sleep service discovery"
#define SLEEP_STATUS_REQUEST    "sleep status request"

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
    int sockfd, ret_value;
    int so_broadcast = true;
    unsigned int length;
    char buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in serv_addr;
    managerDB manDb[MAX_MACHINES]; //structure hold by manager
    signal(SIGINT, signalHandler); //CTRL+C
    signal(SIGHUP, signalHandler); //terminal closed while process still running

//---------------------------------------------------------- PARTICIPANT section ----------------------------------------------------------
    if(argc == 1) {
        struct sockaddr_in from;
        struct hostent *server;
        //server = gethostbyname("localhost");
        if(server == NULL) {
            cout << "No such host!" << endl;
            exit(0);
        }

        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            cout << "Socket creation error." << endl;
            exit(0);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT_SERVER);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //bzero(&(serv_addr.sin_zero), 8);

        ret_value = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&so_broadcast, sizeof(so_broadcast));
        if(ret_value) {
            cout << "Setsockopt error.";
            exit(0);
        }

        //TODO talvez esse while possa ser uma thread separada...
        while(true) {   
            memset(buffer, '\0', BUFFER_SIZE);
            ret_value = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &from, &length);
            if(ret_value < 0) {
                cout << "Recvfrom error." << endl; //TODO tratar esse erro
            }

            if(strcmp(buffer, SLEEP_SERVICE_DISCOVERY) == 0) {
                cout << "Entrei aqui (SLEEP_SERVICE_DISCOVERY)!" << endl;
                //criar thread Discovery Subservice
            }

            if(strcmp(buffer, SLEEP_STATUS_REQUEST) == 0) {
                cout << "Entrei aqui (SLEEP_STATUS_REQUEST)!" << endl;
                //criar thread Monitorin Subservice
            }
        }

        close(sockfd);
        return 0;
    }

//------------------------------------------------------------ MANAGER section ------------------------------------------------------------
    //argv[1] does exist.
    if(argc == 2) {
        socklen_t cli_len;
        struct sockaddr_in cli_addr;

        if(strcmp(argv[1], "manager") != 0) { //argv[1] != "manager"
            cout << "argv NOT OK" << endl;
            return -1;
        }

        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            cout << "Socket creation error";
            exit(0);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT_SERVER);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //bzero(&(serv_addr.sin_zero), 8);

        ret_value = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast));
        if(ret_value) {
            cout << "Setsockopt error.";
            exit(0);
        }

        if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
            cout << "Bind error.";
            exit(0);
        }

        cli_len = sizeof(struct sockaddr_in);

        while(true) {
            //TODO enviar sleep service discovery em broadcast
            ret_value = sendto(sockfd, SLEEP_SERVICE_DISCOVERY, strlen(SLEEP_SERVICE_DISCOVERY), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
            if(ret_value < 0) {
                cout << "Sendto error. AQUI??";
                exit(0);
            }

            memset(buffer, '\0', BUFFER_SIZE);
            ret_value = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &cli_addr, &cli_len);
            if(ret_value < 0) {
                cout << "Recvfrom error.";
                exit(0);
            }

            cout << "Received dgram: " << buffer << endl;

            memset(buffer, '\0', BUFFER_SIZE);
            ret_value = sendto(sockfd, SLEEP_SERVICE_DISCOVERY, strlen(SLEEP_SERVICE_DISCOVERY), 0, (struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
            if(ret_value < 0) {
                cout << "Sendto error.";
                exit(0);
            }
        }

        close(sockfd);
    }

    return 0;
}