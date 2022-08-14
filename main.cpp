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

static void *thr_participant_discovery_service(__attribute__((unused)) void *arg) {
    int sockfd, true_flag = true;
    ssize_t ret_value;
    socklen_t manager_len = sizeof(struct sockaddr_in);
    struct sockaddr_in manager_addr{};
    auto *pack = (struct packet *) malloc(sizeof(struct packet));

    // creates participant's discovery socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
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

    // configure manager's discovery broadcast address
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = (in_port_t) htons(PORT_DISCOVERY_SERVICE_BROADCAST);
    manager_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the participant's discovery socket to the listening port
    ret_value = bind(sockfd, (struct sockaddr *) &manager_addr, manager_len);
    if (ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }

    while (true) {
        cout << "[D] To aguardando msg..." << endl; // TODO: debug (apagar depois)
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                             (struct sockaddr *) &manager_addr, &manager_len);
        if (ret_value < 0) {
            cout << "Recvfrom error.";
            exit(0);
        }

        cout << "[D] Packet from: " << inet_ntoa(manager_addr.sin_addr) << endl;
        cout << "[D] Received dgram type: " << pack->type << endl;
        cout << "[D] Received dgram seqn: " << pack->seqn << endl;
        cout << "[D] Received dgram payload: " << pack->payload << endl;
        cout << "[D] Received dgram length: " << pack->length << endl << endl;

        strcpy(pack->payload, "Ha!");
        pack->type = 1; // TODO: modificar
        pack->length = strlen(pack->payload);

        ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                           (struct sockaddr *) &manager_addr, sizeof manager_addr);
        if (ret_value < 0) {
            cout << "Sendto error.";
            exit(0);
        }
    }
}

static void *thr_manager_discovery_service(__attribute__((unused)) void *arg) {
    int sockfd, seqn = 1, true_flag = true;
    ssize_t ret_value;
    socklen_t participant_len = sizeof(struct sockaddr_in);
    struct sockaddr_in manager_addr{}, broadcast_addr{}, participant_addr{};
    auto *pack = (struct packet *) malloc(sizeof(struct packet));

    // creates manager's discovery socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
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

    // configure manager's discovery listening address
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = (in_port_t) htons(PORT_DISCOVERY_SERVICE_LISTENER);
    manager_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // configure manager's discovery broadcast address
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = (in_port_t) htons(PORT_DISCOVERY_SERVICE_BROADCAST);
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // bind the manager's discovery socket to the listening port
    ret_value = bind(sockfd, (struct sockaddr *) &manager_addr, sizeof(manager_addr));
    if (ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }

    // ------------------------------------------ DISCOVERY SUBSERVICE -------------------------------------------------

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
            pack->seqn = seqn;
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
            cout << "[D] Enviei (x" << seqn++ << ")" << " [" << pack->payload << "]" << endl;

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

            cout << "[D] Packet from: " << inet_ntoa(participant_addr.sin_addr) << endl;
            cout << "[D] Received dgram type: " << pack->type << endl;
            cout << "[D] Received dgram g_seqn: " << pack->seqn << endl;
            cout << "[D] Received dgram payload: " << pack->payload << endl;
            cout << "[D] Received dgram length: " << pack->length << endl << endl;

            // TODO: logica para o banco de clients
        }
    }
}

static void *thr_participant_monitoring_service(__attribute__((unused)) void *arg) {
    int sockfd, true_flag = true;
    ssize_t ret_value;
    socklen_t manager_len = sizeof(struct sockaddr_in);
    struct sockaddr_in manager_addr{};
    auto *pack = (struct packet *) malloc(sizeof(struct packet));

    // creates participant's monitoring socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
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

    // configure manager's monitoring broadcast address
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = (in_port_t) htons(PORT_MONITORING_SERVICE_BROADCAST);
    manager_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the participant's monitoring socket to the listening port
    ret_value = bind(sockfd, (struct sockaddr *) &manager_addr, manager_len);
    if (ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }

    while (true) {
        cout << "[M] To aguardando msg..." << endl; // TODO: debug (apagar depois)
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                             (struct sockaddr *) &manager_addr, &manager_len);
        if (ret_value < 0) {
            cout << "Recvfrom error.";
            exit(0);
        }

        cout << "[M] Packet from: " << inet_ntoa(manager_addr.sin_addr) << endl;
        cout << "[M] Received dgram type: " << pack->type << endl;
        cout << "[M] Received dgram seqn: " << pack->seqn << endl;
        cout << "[M] Received dgram payload: " << pack->payload << endl;
        cout << "[M] Received dgram length: " << pack->length << endl << endl;

        strcpy(pack->payload, "Awake!");
        pack->type = 1; // TODO: modificar
        pack->length = strlen(pack->payload);

        ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                           (struct sockaddr *) &manager_addr, sizeof manager_addr);
        if (ret_value < 0) {
            cout << "Sendto error.";
            exit(0);
        }
    }
}

static void *thr_manager_monitoring_service(__attribute__((unused)) void *arg) {
    int sockfd, seqn = 1, true_flag = true;
    ssize_t ret_value;
    socklen_t participant_len = sizeof(struct sockaddr_in);
    struct sockaddr_in manager_addr{}, broadcast_addr{}, participant_addr{};
    auto *pack = (struct packet *) malloc(sizeof(struct packet));

    // creates manager's monitoring socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
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

    // configure manager's monitoring listening address
    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = (in_port_t) htons(PORT_MONITORING_SERVICE_LISTENER);
    manager_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // configure manager's monitoring broadcast address
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = (in_port_t) htons(PORT_MONITORING_SERVICE_BROADCAST);
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // bind the manager's monitoring socket to the listening port
    ret_value = bind(sockfd, (struct sockaddr *) &manager_addr, sizeof(manager_addr));
    if (ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }

    // ----------------------------------------- MONITORING SUBSERVICE -------------------------------------------------

    ssize_t send_ret_value, recv_ret_value;
    pid_t c_pid = fork();
    if (c_pid == -1) {
        perror("fork");
        exit(0);
    }

    // process responsible for broadcasting monitoring packets
    if (c_pid > 0) {
        while (true) {
            // sending in broadcast
            pack->type = 1; // TODO: modificar
            pack->seqn = seqn;
            strcpy(pack->payload, SLEEP_STATUS_REQUEST);
            pack->length = strlen(pack->payload);

            send_ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                                    (struct sockaddr *) &broadcast_addr, sizeof broadcast_addr);
            if (send_ret_value < 0) {
                cout << "Sendto error." << endl;
                exit(0);
            }

            // TODO: debug...
            cout << "[M] Enviei (x" << seqn++ << ")" << " [" << pack->payload << "]" << endl;

            sleep(8);
        }
    } else { // process responsible for receiving answers to sent monitoring packets
        while (true) {
            recv_ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                                      (struct sockaddr *) &participant_addr, &participant_len);
            if (recv_ret_value < 0) {
                cout << "Recvfrom error.";
                exit(0);
            }

            cout << "[M] Packet from: " << inet_ntoa(participant_addr.sin_addr) << endl;
            cout << "[M] Received dgram type: " << pack->type << endl;
            cout << "[M] Received dgram g_seqn: " << pack->seqn << endl;
            cout << "[M] Received dgram payload: " << pack->payload << endl;
            cout << "[M] Received dgram length: " << pack->length << endl << endl;

            // TODO: logica para o banco de clients
        }
    }
}

static void *thr_participant_function(__attribute__((unused)) void *arg) {
    int i;
    ssize_t ret_value;
    char my_hostname[32], my_mac_addr[16], *my_ip_addr;
    const char *status;
    struct sockaddr_in *teste;
    struct ifaddrs *ifap, *ifa;

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

    // ---------------------------------------------- SUBSERVICES ------------------------------------------------------

    pthread_t thr_discovery, thr_monitoring;
    pthread_attr_t attr_discovery, attr_monitoring;

    ret_value = pthread_attr_init(&attr_discovery);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }
    ret_value = pthread_attr_init(&attr_monitoring);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }

    pthread_create(&thr_discovery, &attr_discovery, &thr_participant_discovery_service,
                   nullptr);
    pthread_create(&thr_monitoring, &attr_monitoring, &thr_participant_monitoring_service,
                   nullptr);

    pthread_join(thr_discovery, nullptr);
    pthread_join(thr_monitoring, nullptr);

    exit(0);
}

static void *thr_manager_function(__attribute__((unused)) void *arg) {

    // ---------------------------------------------- SUBSERVICES ------------------------------------------------------

    ssize_t ret_value;
    pthread_t thr_discovery, thr_monitoring;
    pthread_attr_t attr_discovery, attr_monitoring;

    ret_value = pthread_attr_init(&attr_discovery);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }
    ret_value = pthread_attr_init(&attr_monitoring);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }

    pthread_create(&thr_discovery, &attr_discovery, &thr_manager_discovery_service,
                   nullptr);
    pthread_create(&thr_monitoring, &attr_monitoring, &thr_manager_monitoring_service,
                   nullptr);

    pthread_join(thr_discovery, nullptr);
    pthread_join(thr_monitoring, nullptr);

    exit(0);
}

// ------------------------------------------------ MAIN CODE section --------------------------------------------------

int main(int argc, char **argv) {
    // TODO: signal for CTRL+D
    signal(SIGINT, signalHandler); // CTRL+C
    signal(SIGHUP, signalHandler); // terminal closed while process still running

// ----------------------------------------------- PARTICIPANT section -------------------------------------------------

    ssize_t ret_value;
    if (argc == 1) {
        pthread_t thr_participant;
        pthread_attr_t attr;

        ret_value = pthread_attr_init(&attr);
        if (ret_value != 0) {
            cout << "Pthread_attr_init error." << endl;
            return -1;
        }

        pthread_create(&thr_participant, &attr, &thr_participant_function, nullptr);

        pthread_join(thr_participant, nullptr);
        return 0;
    }

// ------------------------------------------------- MANAGER section ---------------------------------------------------

    // argv[1] does exist and equals to "manager".
    if (argc == 2 && strcmp(argv[1], "manager") == 0) {
        pthread_t thr_manager;
        pthread_attr_t attr_manager;

        ret_value = pthread_attr_init(&attr_manager);
        if (ret_value != 0) {
            cout << "Pthread_attr_init error." << endl;
            return -1;
        }

        pthread_create(&thr_manager, &attr_manager, &thr_manager_function, nullptr);

        pthread_join(thr_manager, nullptr);
        return 0;
    }

    cout << "Call: ./sleep_server OR ./sleep_server manager" << endl;
    return -1;
}
