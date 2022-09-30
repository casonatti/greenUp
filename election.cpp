//
// Created by pedro on 30/09/22.
//

#include "election.h"

void Election::monitorElection() {
    int sock_fd;
    bool manager_alive = false;
    ssize_t ret_value;

    while (!g_has_manager);

    while (true) {
        manager_alive = isManagerAlive();

        if (!manager_alive) {
            cout << "Election!" << endl << endl;

        }

        sleep(4);
    }
}

void Election::startElectionThread() {
    thread th1(monitorElection);
    th1.join();
}

//static void sendElectionMessage(ParticipantsTable table, string message){
//    Packet electionPacket = {
//            .type = TYPE_ELECTION,
//            .length = sizeof (message),
//            .seqn  = 0
//    };
//    strcpy(electionPacket.payload, message.c_str());
//
//    struct sockaddr_in send_addr;
//    send_addr.sin_family = AF_INET;
//    send_addr.sin_addr.s_addr = INADDR_ANY;
//
//    list<string> listIP = table.getAllParticipantsIP();
//    list<string>::iterator it;
//
//    for (it = listIP.begin(); it != listIP.end(); ++it) {
//        inet_aton(it->c_str(), (in_addr *) &p_address.sin_addr.s_addr);
//        ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
//                           (struct sockaddr *) &p_address, sizeof p_address);
//        if (ret_value < 0) {
//            cout << "Sendto error." << endl;
//            exit(0);
//        }
//
//        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
//                             (struct sockaddr *) &p_address, &p_address_len);
//}
