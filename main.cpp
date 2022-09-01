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
    strcpy(g_pack->payload, SLEEP_SERVICE_EXIT);
    g_pack->length = strlen(g_pack->payload);

    sendto(g_sockfd, g_pack, (1024 + sizeof(*g_pack)), 0, (struct sockaddr *) &g_serv_addr, sizeof g_serv_addr);

    exit(signum);
}

participant parsePayload(string payLoad) {
    std::string s;
    std::string delimiter = ", ";
    participant p;
    int i = 0;

    s = payLoad;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        switch (i) {
            case 0:
                p.hostname = token;
                break;
            case 1:
                p.MAC = token;
                break;
            default:
                break;
        }
        s.erase(0, pos + delimiter.length());
        i++;
    }
    p.IP = s;
    return p;
}

static void *thr_participant_interface_service(__attribute__((unused)) void *arg) {
    char buffer[32];
    auto *pack = (struct packet *) malloc(sizeof(struct packet));

    // creates participant's discovery socket
    g_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sockfd < 0) {
        cout << "Socket creation error";
        exit(0);
    }

    while (true) {
        cin.getline(buffer, 32);
        if (strcmp(buffer, "EXIT") == 0) {
            pthread_mutex_lock(&mtx);
            cout << "Sending command EXIT..." << endl;
            pthread_mutex_unlock(&mtx);
            pack->type = TYPE_EXIT;
            strcpy(pack->payload, SLEEP_SERVICE_EXIT);
            pack->length = strlen(pack->payload);
            pack->seqn = 0;

            sendto(g_sockfd, pack, (1024 + sizeof(*pack)), 0,
                   (struct sockaddr *) &g_serv_addr, sizeof g_serv_addr);
            exit(0);
        }
    }
}

[[noreturn]] static void *thr_manager_table_updater(__attribute__((unused)) void *arg) {
    while(true) {
        if(g_table_updated) {
            pthread_mutex_lock(&mtx);
            pthread_mutex_lock(&mtable);
            system("clear");
            table.printTable();
            pthread_mutex_unlock(&mtable);
            pthread_mutex_unlock(&mtx);
            
            g_table_updated = false;
        }
    }
}

[[noreturn]] static void *thr_manager_interface_service(__attribute__((unused)) void *arg) {
    int sockfd, true_flag = true;
    ssize_t ret_value;
    struct sockaddr_in manager_addr{}, broadcast_addr{};
    pthread_t thr_table_updater;
    pthread_attr_t attr_table_updater;

    ret_value = pthread_attr_init(&attr_table_updater);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }

    pthread_create(&thr_table_updater, &attr_table_updater, &thr_manager_table_updater, nullptr);

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

    // creates participant's discovery socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        cout << "Socket creation error";
        exit(0);
    }

    while (true) {
        char buffer[32];
        cin.getline(buffer, 32);
        stringstream stream(buffer);

        string word;
        stream >> word;
        if (word != "WAKE") {
            pthread_mutex_lock(&mtx);
            cout << "Usage: WAKE <hostname>" << endl;
            pthread_mutex_unlock(&mtx);
            stream.clear();
            continue;
        }

        stream >> word;
        string macaddr = table.getParticipantMac(word);
        if (macaddr.empty()) {
            pthread_mutex_lock(&mtx);
            cout << "Hostname not found" << endl;
            pthread_mutex_unlock(&mtx);
            stream.clear();
            continue;
        }

        string message_mac;
        string message = "\xFF\xFF\xFF\xFF\xFF\xFF";
        for (int i = 0; i < macaddr.length(); i += 3) {
            message_mac += static_cast<char>(stoul(macaddr.substr(i, 2), nullptr, 16) & 0xFF);
        }

        for (int i = 16; i > 0; i--) {
            message += message_mac;
        }
        sendto(sockfd, message.c_str(), message.length(), 0,
               (struct sockaddr *) &broadcast_addr, sizeof broadcast_addr);
    }

    pthread_join(thr_table_updater, nullptr);
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
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                             (struct sockaddr *) &manager_addr, &manager_len);
        if (ret_value < 0) {
            cout << "Recvfrom error.";
            exit(0);
        }

        if (!g_has_manager) {
            g_serv_addr = manager_addr;
            pthread_mutex_lock(&mtx);
            cout << "----------------------------------------------------------\n";
            cout << "|Hostname\t|MAC Address\t\t|IP Address\n";
            cout << "|" << "m. hostname" << "\t";
            cout << "|" << "m. mac address" << "\t\t";
            cout << "|" << inet_ntoa(g_serv_addr.sin_addr) << "\n";
            cout << "----------------------------------------------------------\n";
            pthread_mutex_unlock(&mtx);
            g_has_manager = true;
        }

        string s_payload = my_hostname + ", " + my_mac_addr + ", " + my_ip_addr;
        strcpy(pack->payload, s_payload.data());
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
                           (struct sockaddr *) &broadcast_addr, sizeof broadcast_addr);
        if (ret_value < 0) {
            cout << "Sendto error." << endl;
            exit(0);
        }

        /*// wake on lan test

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
        }*/

        sleep(8);
    }
}

static void *thr_manager_discovery_listener(__attribute__((unused)) void *arg) {
    int sockfd, true_flag = true;
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

        if (!strcmp(pack->payload, SLEEP_SERVICE_EXIT)) {
            pthread_mutex_lock(&mtable);
            table.deleteParticipant(inet_ntoa(participant_addr.sin_addr));
            g_table_updated = true;
            pthread_mutex_unlock(&mtable);
        } else {
            participant p = parsePayload(pack->payload);
            if(!table.participantExists(p.IP)) {
                pthread_mutex_lock(&mtable);
                table.addParticipant(p);
                g_table_updated = true;
                pthread_mutex_unlock(&mtable);
            }
        }
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

        string s_payload = "estou acordado";
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

static void *thr_manager_monitoring_service(__attribute__((unused)) void *arg) {
    int sockfd, seqn = 1, true_flag = true;
    ssize_t ret_value;
    struct sockaddr_in manager_addr{}, p_address{};
    socklen_t p_address_len = sizeof(struct sockaddr_in);
    auto *pack = (struct packet *) malloc(sizeof(struct packet));

    // creates manager's monitoring socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        cout << "Socket creation error";
        exit(0);
    }

    // set timeout for socket
    struct timeval timeout{};
    timeout.tv_sec = 5;
    ret_value = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    if (ret_value < 0) {
        cout << "Setsockopt [SO_RCVTIMEO] error." << endl;
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

    // initial configuration for the participant address
    p_address.sin_family = AF_INET;
    p_address.sin_port = (in_port_t) htons(PORT_MONITORING_SERVICE_BROADCAST);

    // bind the manager's monitoring socket to the listening port
    ret_value = bind(sockfd, (struct sockaddr *) &manager_addr, sizeof(manager_addr));
    if (ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }

    // ---------------------------------------- MONITORING BROADCASTER -------------------------------------------------

    // loop responsible for broadcasting monitoring packets
    while (true) {
        pthread_mutex_lock(&mtable);
        list<string> listIP = table.getAllParticipantsIP();
        pthread_mutex_unlock(&mtable);
        list<string>::iterator it;
        for (it = listIP.begin(); it != listIP.end(); ++it) {
            pack->type = TYPE_MONITORING;
            pack->seqn = seqn++;
            strcpy(pack->payload, SLEEP_STATUS_REQUEST);
            pack->length = strlen(pack->payload);

            inet_aton(it->c_str(), (in_addr *) &p_address.sin_addr.s_addr);
            ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                               (struct sockaddr *) &p_address, sizeof p_address);
            if (ret_value < 0) {
                cout << "Sendto error." << endl;
                exit(0);
            }

            ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                                 (struct sockaddr *) &p_address, &p_address_len);
            
            if (ret_value < 0) {
                //Participant doesn't sent response => It's asleep
                pthread_mutex_lock(&mtable);               
                if(strcmp(table.getParticipantStatus(*it), "awake") == 0) {
                    table.sleepParticipant(*it);
                    g_table_updated = true;
                }
                pthread_mutex_unlock(&mtable);
            } else {
                //Participant sent response => It's awake
                pthread_mutex_lock(&mtable);
                if(strcmp(table.getParticipantStatus(*it), "asleep") == 0) {
                    table.wakeParticipant(*it);
                    g_table_updated = true;
                }
                pthread_mutex_unlock(&mtable);
            }
        }
        sleep(5);
    }
}

static void participant_function() {
    int i;
    ssize_t ret_value;
    const char *status;
    struct sockaddr_in *teste;
    struct ifaddrs *ifap, *ifa;
    pthread_mutex_lock(&mtx);
    cout << "========= Configurando o Participante =========" << endl;

    cout << "Getting my hostname..." << endl;
    pthread_mutex_unlock(&mtx);
    char c_my_hostname[32];
    gethostname(c_my_hostname, 32);
    my_hostname = c_my_hostname;
    pthread_mutex_lock(&mtx);
    cout << "My hostname = " << my_hostname << endl << endl;

    cout << "Getting my MAC address..." << endl;
    pthread_mutex_unlock(&mtx);
    FILE *file = fopen("/sys/class/net/wlo1/address", "r");
    i = 0;
    char c_my_mac_addr[16];
    while (fscanf(file, "%c", &c_my_mac_addr[i]) == 1) {
        if (c_my_mac_addr[i] != '\n')
            my_mac_addr += c_my_mac_addr[i];
        i++;
    }
    pthread_mutex_lock(&mtx);
    cout << "My MAC address = " << my_mac_addr << endl << endl;
    fclose(file);

    cout << "Getting my IP address..." << endl;
    pthread_mutex_unlock(&mtx);
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
    pthread_mutex_lock(&mtx);
    cout << "My IP address = " << my_ip_addr << endl << endl;

    cout << "===============================================" << endl << endl;
    pthread_mutex_unlock(&mtx);

    // ---------------------------------------------- SUBSERVICES ------------------------------------------------------

    pthread_t thr_discovery, thr_monitoring, thr_interface;
    pthread_attr_t attr_discovery, attr_monitoring, attr_interface;

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
    ret_value = pthread_attr_init(&attr_interface);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }

    pthread_create(&thr_discovery, &attr_discovery, &thr_participant_discovery_service,
                   nullptr);
    pthread_create(&thr_monitoring, &attr_monitoring, &thr_participant_monitoring_service,
                   nullptr);
    pthread_create(&thr_interface, &attr_interface, &thr_participant_interface_service,
                   nullptr);

    pthread_join(thr_discovery, nullptr);
    pthread_join(thr_monitoring, nullptr);
    pthread_join(thr_interface, nullptr);
}

static void manager_function() {

    // ---------------------------------------------- SUBSERVICES ------------------------------------------------------

    ssize_t ret_value;
    pthread_t thr_discovery, thr_monitoring, thr_interface;
    pthread_attr_t attr_discovery, attr_monitoring, attr_interface;

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
    ret_value = pthread_attr_init(&attr_interface);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }

    pthread_create(&thr_discovery, &attr_discovery, &thr_manager_discovery_service, nullptr);
    pthread_create(&thr_monitoring, &attr_monitoring, &thr_manager_monitoring_service, nullptr);
    pthread_create(&thr_interface, &attr_interface, &thr_manager_interface_service, nullptr);

    pthread_join(thr_discovery, nullptr);
    pthread_join(thr_monitoring, nullptr);
    pthread_join(thr_interface, nullptr);
}

// ------------------------------------------------ MAIN CODE section --------------------------------------------------

int main(int argc, char **argv) {
    // TODO: signal for CTRL+D
    signal(SIGINT, signalHandler); // CTRL+C
    signal(SIGHUP, signalHandler); // terminal closed while process still running

// ----------------------------------------------- PARTICIPANT section -------------------------------------------------

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
