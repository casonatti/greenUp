//
// Created by pedro on 30/09/22.
//

#include "election.h"

#define ELECTION_MESSAGE "election"
#define ELECTION_ANSWER "lose"
#define ELECTION_COORDINATOR "lose"
#define PORT_ELECTION_SERVICE_BROADCAST 10000
#define PORT_ELECTION_SERVICE_LISTENER 10001

void Election::monitorElection() {
    bool alreadyJoined = false;
    int ret_value, len;
    struct sockaddr_in from{};
    socklen_t from_len = sizeof(struct sockaddr_in);
    Packet *pack = (Packet *) malloc(sizeof(Packet));

    while (true) {
        cout << "Election monitor created\n";
        ret_value = recvfrom(listenerSockfd, pack, sizeof(*pack), MSG_WAITALL,
                             (struct sockaddr *) &from, &from_len);
        cout << "Received packet: " << pack->payload << endl;

        if (strcmp(pack->payload, ELECTION_MESSAGE) == 0) {
            alreadyJoined = true;
            strcpy(pack->payload, ELECTION_ANSWER);
            pack->seqn++;
            ret_value = sendto(listenerSockfd, pack, (1024 + sizeof(*pack)), MSG_CONFIRM,
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
                setsockopt(listenerSockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
                ret_value = recvfrom(listenerSockfd, pack, sizeof(*pack), MSG_WAITALL,
                                     (struct sockaddr *) &from, &from_len);

            }


        }
    }

}

void Election::startElectionThread() {
    listenerSockfd = Communication::createSocket();
    listenerAddr = Communication::bindSocket(listenerSockfd, PORT_ELECTION_SERVICE_LISTENER);

    broadcastSockfd = Communication::createSocket();
    broadcastAddr = Communication::bindSocket(broadcastSockfd, PORT_ELECTION_SERVICE_BROADCAST);
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

    strcpy(pack->payload, ELECTION_MESSAGE);
    pTable.printTable();
    myPid = pTable.getParticipantPid(g_my_hostname);
    list<string> listIP = pTable.getBiggerParticipantsIP(myPid);

    list<string>::iterator it;

    for (it = listIP.begin(); it != listIP.end(); ++it) {
        inet_aton(it->c_str(), (in_addr *) &to.sin_addr.s_addr);
        ret_value = sendto(broadcastSockfd, pack, (1024 + sizeof(pack)), MSG_CONFIRM,
                           (struct sockaddr *) &to, to_len);
        if (ret_value < 0) {
            cout << "Sendto error." << endl;
            exit(0);
        }
    }
    while(i < listIP.size()){
        sleep(1);
        ret_value = recvfrom(broadcastSockfd, pack, sizeof(*pack), MSG_DONTWAIT,
                             (struct sockaddr *) &from, &to_len);
        if(strcmp(pack->payload, ELECTION_ANSWER) == 0) {
            result = -1;
            return;
        }
        i++;
    }
    result = 1;
    return;
}

void Election::sendCoordinator(){
    return;
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
