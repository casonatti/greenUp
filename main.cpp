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
#include <mutex>

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

    exit(signum);
}

//participant parsePayload(string payLoad){
//    std::string s;
//    std::string delimiter = ", ";
//    participant p;
//    int i = 0;
//
//    s = payLoad;
//    size_t pos = 0;
//    std::string token;
//    while ((pos = s.find(delimiter)) != std::string::npos) {
//        token = s.substr(0, pos);
//        std::cout << token << std::endl;
//        switch (i)
//        {
//        case 0:
//            p.hostname = token;
//            break;
//        case 1:
//            p.MAC = token;
//            break;
//        }
//        s.erase(0, pos + delimiter.length());
//        i++;
//    }
//    p.IP = s;
//    return p;
//}


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
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                             (struct sockaddr *) &manager_addr, &manager_len);
        if (ret_value < 0) {
            cout << "Recvfrom error.";
            exit(0);
        }
        pthread_mutex_lock(&mtx);
        cout << "[D] Recebi (x" << pack->seqn << ") [" << inet_ntoa(manager_addr.sin_addr) << "]" << endl;
        pthread_mutex_unlock(&mtx);

        string s_payload = my_hostname + ", " + my_mac_addr + ", " + my_ip_addr;
        strcpy(pack->payload, s_payload.data());
//        cout << pack->payload;
        pack->type = TYPE_DISCOVERY;
        pack->length = strlen(pack->payload);

        ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                           (struct sockaddr *) &manager_addr, sizeof manager_addr);
        if (ret_value < 0) {
            cout << "Sendto error.";
            exit(0);
        }
    }
}

static void *thr_manager_discovery_broadcaster(__attribute__((unused)) void *arg) {
    int sockfd, seqn = 1, true_flag = true;
    ssize_t ret_value;
    socklen_t participant_len = sizeof(struct sockaddr_in);
    struct sockaddr_in manager_addr{}, broadcast_addr{};
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

    // ----------------------------------------- DISCOVERY BROADCASTER -------------------------------------------------

    // loop responsible for broadcasting discovery packets
    while (true) {
        pack->type = TYPE_DISCOVERY;
        pack->seqn = seqn++;
        strcpy(pack->payload, SLEEP_SERVICE_DISCOVERY);
        pack->length = strlen(pack->payload);

        ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                           (struct sockaddr *) &broadcast_addr,sizeof broadcast_addr);
        if (ret_value < 0) {
            cout << "Sendto error." << endl;
            exit(0);
        }

//        table.sleepTable();
//        cout << "printando tabela sleep \n";
//        table.printTable();

        // wake on lan test
        string message = "\xFF\xFF\xFF\xFF\xFF\xFF";
        string mac_addr = "\xAB\xCD\xEF\x01\x23\x45";
        for (int i = 16; i > 0; i--) {
            message += mac_addr;
        }
        ret_value = sendto(sockfd, message.c_str(), message.length(), 0,
                           (struct sockaddr *) &broadcast_addr,sizeof broadcast_addr);
        if (ret_value < 0) {
            cout << "Sendto error." << endl;
            exit(0);
        }

        sleep(5);
    }
}

static void *thr_manager_discovery_listener(__attribute__((unused)) void *arg) {
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

    // ------------------------------------------- DISCOVERY LISTENER --------------------------------------------------

    // loop responsible for receiving answers to broadcasted discovery packets
    while (true) {
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                             (struct sockaddr *) &participant_addr, &participant_len);
        if (ret_value < 0) {
            cout << "Recvfrom error.";
            exit(0);
        }
        pthread_mutex_lock(&mtx);
        cout << "[D] Recebi (x" << pack->seqn << ") [" << inet_ntoa(participant_addr.sin_addr) << "] Banana: ";
        cout << ++banana << endl;
        pthread_mutex_unlock(&mtx);

        // TODO: logica para o banco de clients
//        participant p = parsePayload(pack->payload);
//        table.updateTable(p);
//        table.printTable();
    }
}

static void *thr_manager_discovery_service(__attribute__((unused)) void *arg) {

    // ------------------------------------------- DISCOVERY THREADS ---------------------------------------------------

    ssize_t ret_value;
    pthread_t thr_broadcaster, thr_listener;
    pthread_attr_t attr_broadcaster, attr_listener;

    ret_value = pthread_attr_init(&attr_broadcaster);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }
    ret_value = pthread_attr_init(&attr_listener);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }

    pthread_create(&thr_broadcaster, &attr_broadcaster, &thr_manager_discovery_broadcaster,
                   nullptr);
    pthread_create(&thr_listener, &attr_listener, &thr_manager_discovery_listener,
                   nullptr);

    pthread_join(thr_broadcaster, nullptr);
    pthread_join(thr_listener, nullptr);

    exit(0);
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
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                             (struct sockaddr *) &manager_addr, &manager_len);
        if (ret_value < 0) {
            cout << "Recvfrom error.";
            exit(0);
        }

        pthread_mutex_lock(&mtx);
        cout << "[M] Recebi (x" << pack->seqn << ") [" << inet_ntoa(manager_addr.sin_addr) << "]" << endl;
        pthread_mutex_unlock(&mtx);

        string s_payload = my_hostname + ", " + my_mac_addr + ", " + my_ip_addr;
        strcpy(pack->payload, s_payload.data());
        pack->type = TYPE_MONITORING;
        pack->length = strlen(pack->payload);
        ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                           (struct sockaddr *) &manager_addr, sizeof manager_addr);
        if (ret_value < 0) {
            cout << "Sendto error.";
            exit(0);
        }
    }
}

static void *thr_manager_monitoring_broadcaster(__attribute__((unused)) void *arg) {
    int sockfd, seqn = 1, true_flag = true;
    ssize_t ret_value;
    struct sockaddr_in manager_addr{}, broadcast_addr{};
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

    // ---------------------------------------- MONITORING BROADCASTER -------------------------------------------------

    // loop responsible for broadcasting monitoring packets
    while (true) {
        pack->type = TYPE_MONITORING;
        pack->seqn = seqn++;
        strcpy(pack->payload, SLEEP_STATUS_REQUEST);
        pack->length = strlen(pack->payload);

        ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                           (struct sockaddr *) &broadcast_addr, sizeof broadcast_addr);
        if (ret_value < 0) {
            cout << "Sendto error." << endl;
            exit(0);
        }
//            table.sleepTable();
//            cout << "printando tabela sleep \n";
//            table.printTable();

        sleep(5);
    }
}

static void *thr_manager_monitoring_listener(__attribute__((unused)) void *arg) {
    int sockfd, true_flag = true;
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

    // ------------------------------------------ MONITORING LISTENER --------------------------------------------------

    // loop responsible for receiving answers to broadcasted monitoring packets
    while (true) {
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                                  (struct sockaddr *) &participant_addr, &participant_len);
        if (ret_value < 0) {
            cout << "Recvfrom error.";
            exit(0);
        }

        pthread_mutex_lock(&mtx);
        cout << "[M] Recebi (x" << pack->seqn << ") [" << inet_ntoa(participant_addr.sin_addr) << "] Banana: ";
        cout << ++banana << endl;
        pthread_mutex_unlock(&mtx);

        // TODO: logica para o banco de clients
//            participant p = parsePayload(pack->payload);
//            table.updateTable(p);
//            table.printTable();
    }
}

static void *thr_manager_monitoring_service(__attribute__((unused)) void *arg) {

    // ------------------------------------------ MONITORING THREADS ---------------------------------------------------

    ssize_t ret_value;
    pthread_t thr_broadcaster, thr_listener;
    pthread_attr_t attr_broadcaster, attr_listener;

    ret_value = pthread_attr_init(&attr_broadcaster);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }
    ret_value = pthread_attr_init(&attr_listener);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }

    pthread_create(&thr_broadcaster, &attr_broadcaster, &thr_manager_monitoring_broadcaster,
                   nullptr);
    pthread_create(&thr_listener, &attr_listener, &thr_manager_monitoring_listener,
                   nullptr);

    pthread_join(thr_broadcaster, nullptr);
    pthread_join(thr_listener, nullptr);

    exit(0);
}

static void participant_function() {
    int i;
    ssize_t ret_value;
    const char *status;
    struct sockaddr_in *teste;
    struct ifaddrs *ifap, *ifa;

    cout << "========= Configurando o Participante =========" << endl;

    cout << "Getting my hostname..." << endl;
    char c_my_hostname[32];
    gethostname(c_my_hostname, 32);
    my_hostname = c_my_hostname;
    cout << "My hostname = " << my_hostname << endl << endl;

    cout << "Getting my MAC address..." << endl;
    FILE *file = fopen("/sys/class/net/wlo1/address",
                       "r");  // TODO: colocar o nome da interface de rede (ou o nome certo do diretorio)
    i = 0;
    char c_my_mac_addr[17];
    while (fscanf(file, "%c", &c_my_mac_addr[i]) == 1){
        my_mac_addr += c_my_mac_addr[i];
        i++;
    }
    cout << "My MAC address = " << my_mac_addr << endl << endl;
    fclose(file);

    cout << "Getting my IP address..." << endl;
    //participant = gethostbyname(my_hostname);
    //my_ip_addr = inet_ntoa(*((struct in_addr*) participant->h_addr_list[0]));
    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            // TODO: colocar o nome da interface de rede
            if (strcmp(ifa->ifa_name, "wlo1") == 0) {
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
}

static void manager_function() {

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
}

// ------------------------------------------------ MAIN CODE section --------------------------------------------------

int main(int argc, char **argv) {
    // TODO: signal for CTRL+D
    signal(SIGINT, signalHandler); // CTRL+C
    signal(SIGHUP, signalHandler); // terminal closed while process still running

// ----------------------------------------------- PARTICIPANT section -------------------------------------------------

    ssize_t ret_value;
    if (argc == 1) {
        participant_function();
    }

// ------------------------------------------------- MANAGER section ---------------------------------------------------

    // argv[1] does exist and equals to "manager".
    if (argc == 2 && strcmp(argv[1], "manager") == 0) {
        manager_function();
    }

    cout << "Call: ./sleep_server OR ./sleep_server manager" << endl;
    return -1;
}
