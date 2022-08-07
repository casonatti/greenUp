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
#define PORT_MANAGER_LISTENING 8000
#define PORT_PARTICIPANT_LISTENING 4000

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

//---------------------------------------------------------- GLOBAL VAR section ----------------------------------------------------------
int teste = 1;

void signalHandler(int signum) {
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

static void * thr_participant_function(void* arg) {
    int sockfd, ret_value;
    int true_flag = true;
    char buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in recv_addr, serv_addr, from;
    struct hostent *server;
    
    server = gethostbyname("localhost"); //TODO modificar

    //creates the socket
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            cout << "Socket creation error." << endl;
            exit(0);
    }

    //Set socket broadcast option to true
    ret_value = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &true_flag, sizeof true_flag);
    if(ret_value < 0) {
        cout << "Setsockopt [SO_BROADCAST] error." << endl;
        exit(0);
    }

    //Set socket reuseaddr option to true
    ret_value = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &true_flag, sizeof true_flag);
    if(ret_value < 0) {
        cout << "Setsockopt [SO_REUSEADDR] error." << endl;
        exit(0);
    }

    //participant receiving address configuration
    unsigned int serv_addr_len;
    
    memset(&recv_addr, 0, sizeof recv_addr);
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = (in_port_t) htons(PORT_PARTICIPANT_LISTENING);
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //important for broadcast listening

    //manager address configuration
    memset(&serv_addr, 0, sizeof recv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = (in_port_t) htons(PORT_MANAGER_LISTENING);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);

    // bind the participant's listening port
    ret_value = bind(sockfd, (struct sockaddr*) &recv_addr, sizeof(recv_addr));
    if(ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }    
    
    while(true) {
        //wait for manager's message
        cout << "\nTo aguardando msg..." << endl; //TODO debug (apagar depois)
        memset(buffer, '\0', BUFFER_SIZE);
        ret_value = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&from, &serv_addr_len);
        if(ret_value < 0) {
            cout << "Recvfrom error." << endl; //TODO tratar esse erro!
            exit(0);
        }

        //TODO debug (apagar depois)
        cout << "Buffer: " << buffer << endl;

        //compare manager's message and work on it based on the right option
        if(strcmp(buffer, SLEEP_SERVICE_DISCOVERY) == 0) {
            //TODO criar thread Discovery Subservice (?)
            strcpy(buffer, "resposta participante!");
            cout << "Enviando mensagem: " << buffer << endl;
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*) &serv_addr, sizeof serv_addr);
        }

        if(strcmp(buffer, SLEEP_STATUS_REQUEST) == 0) {
            cout << "Entrei aqui (SLEEP_STATUS_REQUEST)!" << endl;
            //TODO criar thread Monitorin Subservice (?)
        }
    }

    close(sockfd);
    return 0;
}


//---------------------------------------------------------- MAIN CODE section ----------------------------------------------------------
int main(int argc, char** argv) {
    int ret_value;
    //TODO signal for CTRL+D
    signal(SIGINT, signalHandler); //CTRL+C
    signal(SIGHUP, signalHandler); //terminal closed while process still running


//---------------------------------------------------------- PARTICIPANT section ----------------------------------------------------------
    if(argc == 1) {
        pthread_t thr_participant;
        pthread_attr_t attr;

        ret_value = pthread_attr_init(&attr);
        if(ret_value != 0) {
            cout << "Pthread_attr_init error." << endl;
            exit(0);
        }

        pthread_create(&thr_participant, &attr, &thr_participant_function, NULL);

        pthread_join(thr_participant, 0);   
    }

//------------------------------------------------------------ MANAGER section ------------------------------------------------------------
    //argv[1] does exist.
    if(argc == 2) {
        int n_machines = 0; //number of machines connected
        int sockfd;
        int true_flag = true;
        char buffer[BUFFER_SIZE] = {0};
        socklen_t participant_len;
        struct sockaddr_in manager_addr, broadcast_addr, participant_addr;
        managerDB manDb[MAX_MACHINES]; //structure hold by manager

        if(strcmp(argv[1], "manager") != 0) { //argv[1] != "manager"
            cout << "argv NOT OK" << endl;
            return -1;
        }

        //creates manager socket
        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            cout << "Socket creation error";
            exit(0);
        }

        //set socket options broadcast and reuseaddr to true
        ret_value = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &true_flag, sizeof true_flag);
        if(ret_value < 0) {
            cout << "Setsockopt [SO_BROADCAST] error." << endl;
            exit(0);
        }

        ret_value = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &true_flag, sizeof true_flag);
        if(ret_value < 0) {
            cout << "Setsockopt [SO_REUSEADDR] error." << endl;
            exit(0);
        }

        //configure manager's sending and broadcast addresses
        memset(&manager_addr, 0, sizeof manager_addr);
        manager_addr.sin_family = AF_INET;
        manager_addr.sin_port = (in_port_t) htons(PORT_MANAGER_LISTENING);
        manager_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        memset(&broadcast_addr, 0, sizeof broadcast_addr);
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = (in_port_t) htons(PORT_PARTICIPANT_LISTENING);
        inet_aton("127.255.255.255", &broadcast_addr.sin_addr);

        ret_value = bind(sockfd, (struct sockaddr *) &manager_addr, sizeof manager_addr);
        if(ret_value < 0) {
            cout << "Bind socket error." << endl;
            exit(0);
        }

        //TODO debug (apagar depois)
        int i = 0;

        //TODO teste...
        participant_len = sizeof(struct sockaddr_in);

        while(true) {
            memset(buffer, '\0', BUFFER_SIZE);
            //inet_aton("127.255.255.255", &broadcast_addr.sin_addr);
            //sending in broadcast
            ret_value = sendto(sockfd, SLEEP_SERVICE_DISCOVERY, strlen(SLEEP_SERVICE_DISCOVERY), 0, (struct sockaddr *) &broadcast_addr, sizeof broadcast_addr);
            if(ret_value < 0) {
                cout << "Sendto error." << endl;
                exit(0);
            }

            i++;
            cout << "Enviei (x" << i << ")" << endl;

            memset(buffer, '\0', BUFFER_SIZE);
            // ret_value = recv(sockfd, buffer, sizeof(buffer)-1, 0);
            ret_value = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &participant_addr, &participant_len);
            if(ret_value < 0) {
                cout << "Recvfrom error.";
                exit(0);
            }

            cout << "Received dgram: " << buffer << endl;

            sleep(3);
        }

        close(sockfd);
    }

    return 0;
}