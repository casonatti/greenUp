//
// Created by pedro on 30/09/22.
//

#include "election.h"

#define ELECTION_MESSAGE "election"
#define ELECTION_ANSWER "lose"
#define ELECTION_COORDINATOR "lose"
#define PORT_ELECTION_SERVICE_BROADCAST 10000
#define PORT_ELECTION_SERVICE_LISTENER 10001

	int Election::sockfd = 0;
	struct sockaddr_in Election::listenerAddr;
	struct sockaddr_in Election::broadcastAddr;
	int Election::result;

void Election::monitorElection() {
    bool alreadyJoined = false;
    int ret_value, len;
    struct sockaddr_in from{};
    socklen_t from_len = sizeof(struct sockaddr_in);
    Packet *pack = (Packet *) malloc(sizeof(Packet));

    while (!is_manager) {
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), MSG_DONTWAIT,
                             (struct sockaddr *) &from, &from_len);

        if (strcmp(pack->payload, ELECTION_MESSAGE) == 0) {
            alreadyJoined = true;
            strcpy(pack->payload, ELECTION_ANSWER);
            cout << "vou enviar answer: " << pack->payload << endl;
            pack->seqn++;
            ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), MSG_CONFIRM,
                               (struct sockaddr *) &from, sizeof from_len);
            result = 0;
            thread th1(startElection);
            while(result == 0) {
            }
                cout << "result changed!" << result << endl;
            if(result == 1) {
                cout << "vou ser o novo manager!";
                sendCoordinator();
                alreadyJoined = false;
            }
            else{
                cout << "vou esperar o novo manager se pronunciar";

                struct timeval timeout{};
                timeout.tv_sec = 5;
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
                ret_value = recvfrom(sockfd, pack, sizeof(*pack), MSG_WAITALL,
                                     (struct sockaddr *) &from, &from_len);

            }


        }
    }
    char* ret; 
    ret = strdup("exit");
    pthread_exit(ret);

}

void Election::startElectionThread() {
    sockfd = Communication::createSocket();
    listenerAddr = Communication::bindSocket(sockfd, PORT_ELECTION_SERVICE_LISTENER);

    broadcastAddr = Communication::createBroadcastAddress(PORT_ELECTION_SERVICE_BROADCAST);
    thread th1(monitorElection);
    th1.join();
}

void Election::startElection() {
    cout <<  "start election\n";
    bool alreadyJoined = false;
    int ret_value, len, myPid, i = 0;
    struct sockaddr_in to{}, from{};
    socklen_t to_len = sizeof(struct sockaddr_in);
    Packet *pack = (Packet *) malloc(sizeof(Packet));

    to = Communication::createBroadcastAddress(PORT_ELECTION_SERVICE_BROADCAST);
    strcpy(pack->payload, ELECTION_MESSAGE);

    myPid = pTable.getParticipantPid(g_my_ip_addr);
    cout << "election my pid: " << myPid << endl;
    list<string> listIP = pTable.getBiggerParticipantsIP(myPid);
    cout << "election ip list: ";
    for(string item : listIP) 
        cout << item << " ";
    cout << endl;

    list<string>::iterator it;

    for (it = listIP.begin(); it != listIP.end(); ++it) {
        inet_aton(it->c_str(), (in_addr *) &to.sin_addr.s_addr);
        ret_value = sendto(sockfd, pack, (1024 + sizeof(pack)), MSG_CONFIRM,
                           (struct sockaddr *) &to, to_len);                  
        if (ret_value < 0) {
            cout << "Sendto error." << endl;
            exit(0);
        }
        cout << "election message sent to id: " << it->c_str() << "and address: " << inet_ntoa(to.sin_addr) << endl;
    }
    while(i < listIP.size()){
        sleep(1);
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), MSG_DONTWAIT,
                             (struct sockaddr *) &from, &to_len);
        if(strcmp(pack->payload, ELECTION_ANSWER) == 0) {
            result = -1;
            return;
        }
        i++;
        cout << "election answer received from id: " << inet_ntoa(from.sin_addr) << endl;
    }
    cout << "Sai do startelection " << endl;
    sleep(100);
    result = 1;
    return;
}

void Election::sendCoordinator(){

    bool alreadyJoined = false;
    int ret_value, len, myPid, i = 0;
    struct sockaddr_in to{}, from{};
    socklen_t to_len = sizeof(struct sockaddr_in);
    Packet *pack = (Packet *) malloc(sizeof(Packet));

    string load = g_my_hostname + ", " + g_my_mac_addr + ", " + g_my_ip_addr;
    strcpy(pack->payload, load.c_str());
    pack->type = TYPE_ELECTION;
    pack->seqn =  0;
    cout <<  "sending coordinator\n";
    ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                       (struct sockaddr *) &broadcastAddr, sizeof broadcastAddr);
    cout <<  "coordinator message sent :" <<  pack->payload << endl;
}

bool Election::isManagerAlive() {
//    int socket_fd, ret_value;
//    struct sockaddr_in manager_addr{}, from{};
//    socklen_t from_len = sizeof(struct sockaddr_in);
//    auto *pack = (Packet *) malloc(sizeof(Packet));
//
//    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
//
//    // set timeout for socket
//    struct timeval timeout{};
//    timeout.tv_sec = 2;
//    ret_value = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
//    if (ret_value < 0) {
//        cout << "Setsockopt [SO_RCVTIMEO] error." << endl;
//        exit(0);
//    }
//    manager_addr = g_serv_addr;
//    manager_addr.sin_port = (in_port_t) htons(PORT_KEEP_ALIVE_LISTENER);
//
//    pack->type = TYPE_KEEP_ALIVE;
//    strcpy(pack->payload, KEEP_ALIVE);
//    pack->length = strlen(pack->payload);
//
//    ret_value = sendto(socket_fd, pack, (1024 + sizeof(*pack)), 0,
//                       (struct sockaddr *) &manager_addr, sizeof manager_addr);
//
//    ret_value = recvfrom(socket_fd, pack, sizeof(*pack), 0,
//                         (struct sockaddr *) &from, &from_len);
//
//    free(pack);
//    if (ret_value < 0) {
//        cout << "Time expired [isManagerAlive()]" << endl;
//        return false;
//    } else {
//        return true;
//    }
    return true;
}
