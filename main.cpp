#include <csignal>
#include <iostream>
#include <ifaddrs.h>
#include <cstring>
#include <pthread.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib> // exit() precisa desse include nos labs do inf

using namespace std;

// ------------------------------------------------ GLOBAL VAR section -------------------------------------------------
#include "globals.cpp"
// ---------------------------------------------------------------------------------------------------------------------

void signalHandler(int signum) {
    g_pack->type = TYPE_EXIT;
    g_pack->seqn = g_seqn;
    strcpy(g_pack->payload, "EXIT");
    g_pack->length = strlen(g_pack->payload);

    sendto(g_sockfd, g_pack, (1024 + sizeof(*g_pack)), 0, (struct sockaddr *) &g_serv_addr, sizeof g_serv_addr);

    //recv(sock, buffer, BUFFER_SIZE, 0);
    //cout << "received message from server: \"" << buffer << "\"" << endl << endl;

    //cout << "sending message to server: \"" << exit_message << "\"" << endl;
    //send(sock, exit_message, strlen(exit_message), 0);

    //recv(sock, buffer, BUFFER_SIZE, 0);
    //cout << "received message from server: \"" << buffer << "\"" << endl << endl;

    //close(sock);

    exit(signum);
}

static void *thr_participant_function(void *arg) {
    int i;
    int true_flag = true;
    char my_hostname[32];
    char my_mac_addr[16];
    char *my_ip_addr;
    const char *status;
    struct sockaddr_in from{}, *teste;
    unsigned int serv_addr_len = sizeof(from);
    struct hostent *server, *participant;
    struct ifaddrs *ifap, *ifa;
    auto *pack = (struct packet *) malloc(sizeof(packet));

    cout << "========= Configurando o Participante =========" << endl;

    cout << "Getting my hostname..." << endl;
    gethostname(my_hostname, sizeof(my_hostname));
    cout << "My hostname = " << my_hostname << endl << endl;

    cout << "Getting my MAC address..." << endl;
    FILE *file = fopen("/sys/class/net/enp0s3/address",
                       "r");  // TODO: colocar o nome da interface de rede (ou o nome certo do diretorio)
    i = 0;
    while (fscanf(file, "%c", &my_mac_addr[i]) == 1)
        i++;
    cout << "My MAC address = " << my_mac_addr << endl << endl;
    fclose(file);

    cout << "Getting my IP address..." << endl;
    //participant = gethostbyname(my_hostname);
    //my_ip_addr = inet_ntoa(*((struct in_addr*) participant->h_addr_list[0]));
    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            // TODO: colocar o nome da interface de rede
            if (strcmp(ifa->ifa_name, "enp0s3") == 0) {
                teste = (struct sockaddr_in *) ifa->ifa_addr;
                my_ip_addr = inet_ntoa(teste->sin_addr);
            }
        }
    }
    cout << "My IP address = " << my_ip_addr << endl << endl;

    status = "awaken";
    cout << "Status = " << status << endl;

    cout << "===============================================" << endl << endl;

    server = gethostbyname("localhost");  // TODO: modificar

    // creates the socket
    if ((g_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        cout << "Socket creation error." << endl;
        exit(0);
    }

    // set socket broadcast option to true
    g_ret_value = setsockopt(g_sockfd, SOL_SOCKET, SO_BROADCAST, &true_flag,
                             sizeof(true_flag));
    if (g_ret_value < 0) {
        cout << "Setsockopt [SO_BROADCAST] error." << endl;
        exit(0);
    }

    // set socket reuseaddr option to true
    g_ret_value = setsockopt(g_sockfd, SOL_SOCKET, SO_REUSEADDR, &true_flag,
                             sizeof(true_flag));
    if (g_ret_value < 0) {
        cout << "Setsockopt [SO_REUSEADDR] error." << endl;
        exit(0);
    }

    // participant receiving address configuration
    g_recv_addr.sin_family = AF_INET;
    g_recv_addr.sin_port = (in_port_t) htons(PORT_PARTICIPANT_LISTENING);
    g_recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // important for broadcast listening

    // manager address configuration
    g_serv_addr.sin_family = AF_INET;
    g_serv_addr.sin_port = (in_port_t) htons(PORT_MANAGER_LISTENING);
//    g_serv_addr.sin_addr = *((struct in_addr *) server->h_addr);
    g_serv_addr.sin_addr.s_addr = g_manager_addr;

    // bind the participant's listening port
    g_ret_value = bind(g_sockfd, (struct sockaddr *) &g_recv_addr, sizeof(g_recv_addr));
    if (g_ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }

    while (true) {
        // wait for manager's message
        cout << "To aguardando msg..." << endl; //TODO debug (apagar depois)
        g_ret_value = (int) recvfrom(g_sockfd, pack, sizeof(*pack), 0, (struct sockaddr *) &from,
                                     &serv_addr_len);
        if (g_ret_value < 0) {
            cout << "Recvfrom error." << endl; //TODO tratar esse erro!
            exit(0);
        }

        // TODO: debug (apagar depois)
        cout << "Pack->payload: " << pack->payload << endl;
        cout << "Packet from: " << inet_ntoa(from.sin_addr) << endl;

        // compare manager's message and work on it based on the right option
        if (strcmp(pack->payload, SLEEP_SERVICE_DISCOVERY) == 0) {
            // TODO: criar thread Discovery Subservice (?)
            strcpy(pack->payload, "Ha!");
        }

        if (strcmp(pack->payload, SLEEP_STATUS_REQUEST) == 0) {
            // TODO: criar thread Monitoring Subservice (?)
            strcpy(pack->payload, status);
        }

        pack->type = 1; //TODO modificar
        pack->seqn = g_seqn;
        pack->length = strlen(pack->payload);
        sendto(g_sockfd, pack, (1024 + sizeof(*pack)), 0, (struct sockaddr *) &g_serv_addr,
               sizeof(g_serv_addr));
        g_seqn++;
    }
}

// ------------------------------------------------ MAIN CODE section --------------------------------------------------
int main(int argc, char **argv) {
    ssize_t ret_value;
    // TODO: signal for CTRL+D
    signal(SIGINT, signalHandler); // CTRL+C
    signal(SIGHUP, signalHandler); // terminal closed while process still running


// ----------------------------------------------- PARTICIPANT section -------------------------------------------------
    if (argc == 1) {
        pthread_t thr_participant;
        pthread_attr_t attr;

        ret_value = pthread_attr_init(&attr);
        if (ret_value != 0) {
            cout << "Pthread_attr_init error." << endl;
            exit(0);
        }

        pthread_create(&thr_participant, &attr, &thr_participant_function, nullptr);

        pthread_join(thr_participant, nullptr);
    }

// ------------------------------------------------- MANAGER section ---------------------------------------------------
    // argv[1] does exist.
    if (argc == 2) {
        int n_machines = 0; // number of machines connected
        int sockfd, seqn = 1;
        int true_flag = true;
        char buffer[BUFFER_SIZE] = {0};
        socklen_t participant_len;
        struct sockaddr_in manager_addr{}, broadcast_addr{}, participant_addr{};
        managerDB manDb[MAX_MACHINES]; // structure hold by manager
        auto *pack = (struct packet *) malloc(sizeof(struct packet));

        if (strcmp(argv[1], "manager") != 0) { // argv[1] != "manager"
            cout << "argv NOT OK" << endl;
            return -1;
        }

        // creates manager socket
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            cout << "Socket creation error";
            exit(0);
        }

        // set socket options broadcast and reuseaddr to true
        ret_value = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &true_flag,
                               sizeof(true_flag));
        if (ret_value < 0) {
            cout << "Setsockopt [SO_BROADCAST] error." << endl;
            exit(0);
        }

        ret_value = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &true_flag,
                               sizeof(true_flag));
        if (ret_value < 0) {
            cout << "Setsockopt [SO_REUSEADDR] error." << endl;
            exit(0);
        }

        // configure manager's listening address
        manager_addr.sin_family = AF_INET;
        manager_addr.sin_port = (in_port_t) htons(PORT_MANAGER_LISTENING);
        manager_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        // configure manager's broadcast address
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = (in_port_t) htons(PORT_PARTICIPANT_LISTENING);
        broadcast_addr.sin_addr.s_addr = g_broadcast_addr;

        // bind the manager's socket to the listening port
        ret_value = bind(sockfd, (struct sockaddr *) &manager_addr, sizeof(manager_addr));
        if (ret_value < 0) {
            cout << "Bind socket error." << endl;
            exit(0);
        }

        // TODO: debug (apagar depois)
        int n = 0;

        participant_len = sizeof(struct sockaddr_in);

        // ---------------------------------------- DISCOVERY SUBSERVICE -----------------------------------------------
        ssize_t send_ret_value, recv_ret_value;
        pid_t c_pid = fork();
        if (c_pid == -1) {
            perror("fork");
            exit(0);
        }

        // process responsible for broadcasting discovery packets
        if (c_pid > 0) {
            while (true) {
                // sending in broadcast
                pack->type = 1; // TODO: modificar
                pack->seqn = seqn++;
                strcpy(pack->payload, SLEEP_SERVICE_DISCOVERY);
                pack->length = strlen(pack->payload);

                send_ret_value = (int) sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                                              (struct sockaddr *) &broadcast_addr,
                                              sizeof broadcast_addr);
                if (send_ret_value < 0) {
                    cout << "Sendto error." << endl;
                    exit(0);
                }

                // TODO: debug...
                cout << "Enviei (x" << n++ << ")" << " [" << pack->payload << "]" << endl;

                sleep(5);
            }
        } else { // process responsible for receiving answers to sent discovery packets
            while (true) {
                recv_ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                                          (struct sockaddr *) &participant_addr, &participant_len);
                if (recv_ret_value < 0) {
                    cout << "Recvfrom error.";
                    exit(0);
                }

                cout << "Received dgram type: " << pack->type << endl;
                cout << "Received dgram g_seqn: " << pack->seqn << endl;
                cout << "Received dgram payload: " << pack->payload << endl;
                cout << "Received dgram length: " << pack->length << endl << endl;
            }
        }

//            // ------------------------------------ MONITORING SUBSERVICE --------------------------------------------
//            // TODO: debug
//            pack->type = 1; // TODO: modificar
//            pack->seqn = seqn;
//            strcpy(pack->payload, SLEEP_STATUS_REQUEST);
//            pack->length = strlen(pack->payload);
//            seqn++;
//
//
//            ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0, (struct sockaddr *) &broadcast_addr,
//                               sizeof broadcast_addr);
//            if (ret_value < 0) {
//                cout << "Sendto error." << endl;
//                exit(0);
//            }
//
//            // TODO: debug...
//            n++;
//            cout << "Enviei (x" << n << ")" << " [" << pack->payload << "]" << endl;
//
//            // receiving packets
//            ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0, (struct sockaddr *) &participant_addr,
//                                 &participant_len);
//            if (ret_value < 0) {
//                cout << "Recvfrom error.";
//                exit(0);
//            }
//
//            cout << "Received dgram type: " << pack->type << endl;
//            cout << "Received dgram g_seqn: " << pack->seqn << endl;
//            cout << "Received dgram payload: " << pack->payload << endl;
//            cout << "Received dgram length: " << pack->length << endl << endl;
//
//            sleep(3);
    }

    return 0;
}
