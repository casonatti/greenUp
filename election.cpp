//
// Created by pedro on 30/09/22.
//

#include "election.h"

#define ELECTION_MESSAGE "election"
#define ELECTION_ANSWER "lose"
#define PORT_ELECTION_SERVICE_BROADCAST 10000
#define PORT_ELECTION_SERVICE_LISTENER 10001

void Election::monitorElection() {
    bool alreadyJoined = false;
    bool result;
    int sockfd, ret_value, len;
    struct sockaddr_in addr{}, from{};
    socklen_t from_len = sizeof(struct sockaddr_in);
    Packet *pack = (Packet *) malloc(sizeof(Packet));
    sockfd = Communication::createSocket();
    addr = Communication::bindSocket(sockfd, PORT_ELECTION_SERVICE_LISTENER);

    while (true) {
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), MSG_WAITALL,
                             (struct sockaddr *) &from, &from_len);

        if (strcmp(pack->payload, ELECTION_MESSAGE) == 0) {
            strcpy(pack->payload, ELECTION_ANSWER);
            pack->seqn++;
            ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), MSG_CONFIRM,
                               (struct sockaddr *) &from, sizeof from_len);

            result = startElection();
            if(result){
                sendCoordinator();
            }
        }
    }

}

void Election::startElectionThread() {
    thread th1(monitorElection);
    th1.join();
}

bool Election::startElection() {
//    struct sockaddr_in manager_addr{}, p_address{};
//    Packet electionPacket = {
//            .type = TYPE_ELECTION,
//            .length = sizeof(ELECTION_MESSAGE),
//            .seqn  = 0
//    };
//    strcpy(electionPacket.payload, ELECTION_MESSAGE);
//
//    struct sockaddr_in send_addr;
//    send_addr.sin_family = AF_INET;
//    send_addr.sin_addr.s_addr = INADDR_ANY;
//
//    list<string> listIP = pTable.getAllParticipantsIP();
//    list<string>::iterator it;
//
//    for (it = listIP.begin(); it != listIP.end(); ++it) {
//        inet_aton(it->c_str(), (in_addr *) &p_address.sin_addr.s_addr);
//        ret_value = sendto(sockfd, electionPacket, (1024 + sizeof(electionPacket)), 0,
//                           (struct sockaddr *) &p_address, sizeof p_address);
//        if (ret_value < 0) {
//            cout << "Sendto error." << endl;
//            exit(0);
//        }
//
//        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
//                             (struct sockaddr *) &p_address, &p_address_len);
//    }
    return true;
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
