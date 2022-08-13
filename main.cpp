#include <csignal>
#include <iostream>
#include <ifaddrs.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib> //exit() precisa desse include nos labs do inf

using namespace std;

#define BUFFER_SIZE 32
#define MAX_MACHINES 10
#define PORT_MANAGER_LISTENING 8000
#define PORT_PARTICIPANT_LISTENING 4000

#define SLEEP_SERVICE_DISCOVERY "sleep service discovery"
#define SLEEP_STATUS_REQUEST    "sleep status request"

struct managerDB {
    const char* hostname;
    const char* MAC;
    const char* IP;
    const char* status;
};

struct packet {
    uint16_t type;          //DATA | CMD
    uint16_t seqn;          //sequence number
    uint16_t length;        //payload length
    uint16_t timestamp;     //packet timestamp
    char payload[BUFFER_SIZE];    //packet data
};

//---------------------------------------------------------- GLOBAL VAR section ----------------------------------------------------------

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
    int sockfd, ret_value, i, seqn = 1;
    int true_flag = true;
    char buffer[BUFFER_SIZE] = {0};
    char my_hostname[32];
    char my_mac_addr[16];
    char* my_ip_addr;
    const char* status;
    struct sockaddr_in recv_addr, serv_addr, from, *teste;
    struct hostent *server, *participant;
    struct ifaddrs *ifap, *ifa;
    struct packet pack;

    cout << "========= Configurando o Participante =========" << endl;

    cout << "Getting my hostname..." << endl;
    gethostname(my_hostname, sizeof(my_hostname));
    cout << "My hostname = " << my_hostname << endl << endl;

    cout << "Getting my MAC address..." << endl;
    FILE *file = fopen("/sys/class/net/wlo1/address", "r"); //TODO mudar wlo1 -> eth0 (ou o nome certo do diretorio)
    i = 0;
    while(fscanf(file, "%c", &my_mac_addr[i]) == 1)
        i++;
    cout << "My MAC address = " << my_mac_addr << endl << endl;
    fclose(file);

    cout << "Getting my IP address..." << endl;
    //participant = gethostbyname(my_hostname);
    //my_ip_addr = inet_ntoa(*((struct in_addr*) participant->h_addr_list[0]));
    getifaddrs(&ifap);
    for(ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET) {
            if(strcmp(ifa->ifa_name, "wlo1") == 0) { //TODO trocar wlo1 -> eth0
                teste = (struct sockaddr_in *) ifa->ifa_addr;
                my_ip_addr = inet_ntoa(teste->sin_addr);
            }
        }
    }
    cout << "My IP address = " << my_ip_addr << endl << endl;

    status = "awaken";
    cout << "Status = " << status << endl;

    cout << "===============================================" << endl << endl;
    
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
    //recv_addr.sin_addr.s_addr = inet_addr("192.168.0.10");

    //manager address configuration
    memset(&serv_addr, 0, sizeof serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = (in_port_t) htons(PORT_MANAGER_LISTENING);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    //serv_addr.sin_addr.s_addr = inet_addr("192.168.0.10");

    // bind the participant's listening port
    ret_value = bind(sockfd, (struct sockaddr*) &recv_addr, sizeof(recv_addr));
    if(ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }    
    
    while(true) {
        //wait for manager's message
        cout << "To aguardando msg..." << endl; //TODO debug (apagar depois)
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
            pack.type = 1; //TODO modificar
            pack.seqn = seqn;
            //pack.payload = "Teste!";
            strcpy(pack.payload, "Teste!");
            pack.length = strlen(pack.payload);
            seqn++;

            //strcpy(buffer, "resposta participante!");
            //cout << "Enviando mensagem: " << buffer << endl; //TODO apagar depois
            sendto(sockfd, (struct __packet *)&pack, (1024 + sizeof(pack)), 0, (struct sockaddr*) &serv_addr, sizeof serv_addr);
        }

        if(strcmp(buffer, SLEEP_STATUS_REQUEST) == 0) {
            cout << "Entrei aqui (SLEEP_STATUS_REQUEST)!" << endl;
            //TODO criar thread Monitorin Subservice (?)
            strcpy(buffer, status);
            cout << "Enviando status: " << endl; //TODO apagar depois
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*) &serv_addr, sizeof serv_addr);
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
        packet * pack = (struct packet *) malloc(sizeof(struct packet));

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
        //manager_addr.sin_addr.s_addr = inet_addr("192.168.0.10");


        memset(&broadcast_addr, 0, sizeof broadcast_addr);
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = (in_port_t) htons(PORT_PARTICIPANT_LISTENING);
        inet_aton("127.255.255.255", &broadcast_addr.sin_addr);
        //inet_aton("192.168.0.255", &broadcast_addr.sin_addr); //broadcast da minha rede local

        ret_value = bind(sockfd, (struct sockaddr *) &manager_addr, sizeof manager_addr);
        if(ret_value < 0) {
            cout << "Bind socket error." << endl;
            exit(0);
        }

        //TODO debug (apagar depois)
        int n = 0;

        //TODO teste...
        participant_len = sizeof(struct sockaddr_in);

        while(true) {
            memset(buffer, '\0', BUFFER_SIZE);
            //sending in broadcast
            ret_value = sendto(sockfd, SLEEP_SERVICE_DISCOVERY, strlen(SLEEP_SERVICE_DISCOVERY), 0, (struct sockaddr *) &broadcast_addr, sizeof broadcast_addr);
            if(ret_value < 0) {
                cout << "Sendto error." << endl;
                exit(0);
            }

            n++;
            cout << "Enviei (x" << n << ")" << endl;

            memset(buffer, '\0', BUFFER_SIZE);
            // ret_value = recv(sockfd, buffer, sizeof(buffer)-1, 0);
            ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0, (struct sockaddr *) &participant_addr, &participant_len);
            if(ret_value < 0) {
                cout << "Recvfrom error.";
                exit(0);
            }

            cout << "Received dgram type: " << pack->type << endl;
            cout << "Received dgram seqn: " << pack->seqn << endl;
            cout << "Received dgram payload: " << pack->payload << endl;
            cout << "Received dgram length: " << pack->length << endl << endl;

            sleep(3);
        }

        close(sockfd);
    }

    return 0;
}
