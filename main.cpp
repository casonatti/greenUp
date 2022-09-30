#include <csignal>
#include <iostream>
#include <ifaddrs.h>
#include <cstring>
#include <pthread.h>
#include <cstdio>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib> // exit() precisa desse include nos labs do inf
#include <mutex>
#include "participantsTable.cpp"

using namespace std;

// ------------------------------------------------ GLOBAL VAR section -------------------------------------------------

#include "globals.cpp"

// ---------------------------------------------------------------------------------------------------------------------

bool isManagerAlive(int socket_fd, sockaddr_in manager_addr);

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
            case 2:
                p.pid = stoi(token);
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
            pack->seqn = 0; //TODO: melhorar isso

            sendto(g_sockfd, pack, (1024 + sizeof(*pack)), 0,
                   (struct sockaddr *) &g_serv_addr, sizeof g_serv_addr);
            exit(0);
        }
    }
}

static void *thr_participant_keep_alive_monitoring(__attribute__((unused)) void *arg) {
    int sock_fd;
    bool manager_alive = false;
    ssize_t ret_value;
    struct sockaddr_in manager_addr{};

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = (in_port_t) htons(PORT_KEEP_ALIVE_LISTENER);
    manager_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    while(!g_has_manager);

    while(true) {
        manager_alive = isManagerAlive(sock_fd, manager_addr);

        if(!manager_alive) {
            cout << "Election!" << endl << endl;
        }
        sleep(4);
    }
}

static void *thr_manager_keep_alive(__attribute__((unused)) void *arg) {
    int sock_fd, true_flag = 1;
    ssize_t ret_value;
    struct sockaddr_in manager_addr{}, from{};
    socklen_t from_len = sizeof(struct sockaddr_in);
    auto *pack = (struct packet *) malloc(sizeof(struct packet));

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    manager_addr.sin_family = AF_INET;
    manager_addr.sin_port = (in_port_t) htons(PORT_KEEP_ALIVE_LISTENER);
    manager_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sock_fd, (struct sockaddr *) &manager_addr, sizeof(manager_addr));

    while(true) {
        recvfrom(sock_fd, pack, sizeof(*pack), 0,
                    (struct sockaddr *) &from, &from_len);

        if(!strcmp(pack->payload, KEEP_ALIVE)) {
            pack->type = TYPE_KEEP_ALIVE;
            strcpy(pack->payload, "ACK");
            pack->length = strlen(pack->payload);

            sendto(sock_fd, pack, (1024 + sizeof(*pack)), 0,
                    (struct sockaddr *) &from, sizeof from);
        }
    }
}

static void *thr_manager_table_updater(__attribute__((unused)) void *arg) {
  pthread_mutex_lock(&mtable);
  system("clear");
  table.printTable();
  pthread_mutex_unlock(&mtable);
  
    while(true) {
        if(g_table_updated) {
            pthread_mutex_lock(&mtx);
            pthread_mutex_lock(&mtable);
            system("clear");
            table.printTable();
            g_table_updated = false;
            pthread_mutex_unlock(&mtable);
            pthread_mutex_unlock(&mtx);
        }
    }
}

static void *thr_manager_interface_service(__attribute__((unused)) void *arg) {
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

static void *thr_manager_newcommer_service(__attribute__((unused)) void *arg) {
    int sockfd, true_flag = true;
    ssize_t ret_value;
    socklen_t newcommer_len = sizeof(struct sockaddr_in);
    struct sockaddr_in broadcast_addr{}, newcommer_addr;
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
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = (in_port_t) htons(PORT_DISCOVERY_SERVICE_BROADCAST);
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the participant's discovery socket to the listening port
    ret_value = bind(sockfd, (struct sockaddr *) &broadcast_addr, sizeof broadcast_addr);
    if (ret_value < 0) {
        cout << "Bind socket error." << endl;
        exit(0);
    }

    while(true) {
        ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                             (struct sockaddr *) &newcommer_addr, &newcommer_len);
        if (ret_value < 0) {
            cout << "Recvfrom error.";
            exit(0);
        }

        if(!strcmp(pack->payload, NEWCOMMER)) {
            // send the manager info to the newcommer participant
            string s_payload = g_my_hostname + ", " + g_my_mac_addr + ", " + to_string(g_my_pid) + ", " + g_my_ip_addr;
            pack->type = TYPE_NEWCOMMER;
            strcpy(pack->payload, s_payload.data());
            
            ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                           (struct sockaddr *) &newcommer_addr, sizeof newcommer_addr);
            if (ret_value < 0) {
                cout << "Sendto error.";
                exit(0);
            }

            // send the list of all participants to the newcommer participant (for bully algorithm)
            list<string> all_participants = table.getAllParticipantsIP();

            // pack->payload = all_participants;
        }
    }
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
            cout << "====================== MANAGER INFO ======================\n";
            cout << "----------------------------------------------------------\n";
            cout << "|Hostname\t|MAC Address\t\t|IP Address\n";
            cout << "|" << g_manager_hostname << "\t";
            cout << "|" << g_manager_MAC << "\t";
            cout << "|" << inet_ntoa(g_serv_addr.sin_addr) << "\n";
            cout << "----------------------------------------------------------\n";
            pthread_mutex_unlock(&mtx);
            g_has_manager = true;
        }

        string s_payload = g_my_hostname + ", " + g_my_mac_addr + ", " + to_string(g_my_pid) + ", " + g_my_ip_addr;
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
    ssize_t ret_value;

    // ---------------------------------------------- SUBSERVICES ------------------------------------------------------

    pthread_t thr_discovery, thr_monitoring, thr_interface, thr_keep_alive;
    pthread_attr_t attr_discovery, attr_monitoring, attr_interface, attr_keep_alive;

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
    ret_value = pthread_attr_init(&attr_keep_alive);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }
    
    pthread_create(&thr_interface, &attr_interface, &thr_participant_interface_service,
                   nullptr);
    pthread_create(&thr_discovery, &attr_discovery, &thr_participant_discovery_service,
                   nullptr);
    pthread_create(&thr_monitoring, &attr_monitoring, &thr_participant_monitoring_service,
                   nullptr);
    pthread_create(&thr_keep_alive, &attr_keep_alive, &thr_participant_keep_alive_monitoring,
                   nullptr);

    pthread_join(thr_interface, nullptr);
    pthread_join(thr_discovery, nullptr);
    pthread_join(thr_monitoring, nullptr);
    pthread_join(thr_monitoring, nullptr);
}

static void manager_function() {

    // ---------------------------------------------- SUBSERVICES ------------------------------------------------------

    ssize_t ret_value;
    pthread_t thr_discovery, thr_monitoring, thr_interface, thr_manager_newcommer, thr_keep_alive;
    pthread_attr_t attr_discovery, attr_monitoring, attr_interface, attr_newcommer, attr_keep_alive;

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
    ret_value = pthread_attr_init(&attr_newcommer);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }
    ret_value = pthread_attr_init(&attr_keep_alive);
    if (ret_value != 0) {
        cout << "Pthread_attr_init error." << endl;
        exit(0);
    }

    pthread_create(&thr_interface, &attr_interface, &thr_manager_interface_service, nullptr);
    pthread_create(&thr_manager_newcommer, &attr_newcommer, &thr_manager_newcommer_service, nullptr);
    pthread_create(&thr_discovery, &attr_discovery, &thr_manager_discovery_service, nullptr);
    pthread_create(&thr_keep_alive, &attr_keep_alive, &thr_manager_keep_alive, nullptr);
    pthread_create(&thr_monitoring, &attr_monitoring, &thr_manager_monitoring_service, nullptr);

    pthread_join(thr_interface, nullptr);
    pthread_join(thr_manager_newcommer, nullptr);
    pthread_join(thr_discovery, nullptr);
    pthread_join(thr_keep_alive, nullptr);
    pthread_join(thr_monitoring, nullptr);
}

void initialize() {
    int i;
    const char *status;
    struct sockaddr_in *teste;
    struct ifaddrs *ifap, *ifa;

    pthread_mutex_lock(&mtx);
    cout << "============ Getting host info ============" << endl;

    cout << "Getting my pid..." << endl;
    g_my_pid = getpid();
    cout << "My PID = " << g_my_pid << endl << endl;

    cout << "Getting my hostname..." << endl;
    pthread_mutex_unlock(&mtx);
    char c_my_hostname[32];
    gethostname(c_my_hostname, 32);
    g_my_hostname = c_my_hostname;
    pthread_mutex_lock(&mtx);
    cout << "My hostname = " << g_my_hostname << endl << endl;

    cout << "Getting my MAC address..." << endl;
    pthread_mutex_unlock(&mtx);
    FILE *file = fopen("/sys/class/net/eth0/address", "r");
    i = 0;
    char c_my_mac_addr[16];
    while (fscanf(file, "%c", &c_my_mac_addr[i]) == 1) {
        if (c_my_mac_addr[i] != '\n')
            g_my_mac_addr += c_my_mac_addr[i];
        i++;
    }
    fclose(file);
    pthread_mutex_lock(&mtx);
    cout << "My MAC address = " << g_my_mac_addr << endl << endl;

    cout << "Getting my IP address..." << endl;
    pthread_mutex_unlock(&mtx);
    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            // TODO: colocar o nome da interface de rede
            if (strcmp(ifa->ifa_name, "eth0") == 0) {
                teste = (struct sockaddr_in *) ifa->ifa_addr;
                g_my_ip_addr = inet_ntoa(teste->sin_addr);
            }
        }
    }
    pthread_mutex_lock(&mtx);
    cout << "My IP address = " << g_my_ip_addr << endl;

    cout << "===========================================" << endl << endl;
    pthread_mutex_unlock(&mtx);
}

bool isManagerAlive(int socket_fd, sockaddr_in manager_addr) {
    int ret_value;
    struct sockaddr_in from{};
    socklen_t from_len = sizeof(struct sockaddr_in);
    auto *pack = (struct packet *) malloc(sizeof(struct packet));

    // set timeout for socket
    struct timeval timeout{};
    timeout.tv_sec = 2;
    ret_value = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    if (ret_value < 0) {
        cout << "Setsockopt [SO_RCVTIMEO] error." << endl;
        exit(0);
    }

    pack->type = TYPE_KEEP_ALIVE;
    strcpy(pack->payload, KEEP_ALIVE);
    pack->length = strlen(pack->payload);

    ret_value = sendto(socket_fd, pack, (1024 + sizeof(*pack)), 0,
                        (struct sockaddr *) &manager_addr, sizeof manager_addr);

    ret_value = recvfrom(socket_fd, pack, sizeof(*pack), 0,
                            (struct sockaddr *) &from, &from_len);

    free(pack);
    if(ret_value < 0) {
        cout << "Time expired [isManagerAlive()]" << endl;
        return false;
    } else {
        return true;
    }
}

bool newcommer() {
    int sockfd, true_flag = true;
    ssize_t ret_value;
    struct sockaddr_in broadcast_addr{}, m_address{};
    socklen_t m_address_len = sizeof(struct sockaddr_in);
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

    // set timeout for socket
    struct timeval timeout{};
    timeout.tv_sec = 3;
    ret_value = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    
    if (ret_value < 0) {
        cout << "Setsockopt [SO_RCVTIMEO] error." << endl;
        exit(0);
    }

    // configure manager's discovery broadcast address
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = (in_port_t) htons(PORT_DISCOVERY_SERVICE_BROADCAST);
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    
    pack->type = TYPE_NEWCOMMER;
    strcpy(pack->payload, NEWCOMMER);
    pack->length = strlen(pack->payload);

    ret_value = sendto(sockfd, pack, (1024 + sizeof(*pack)), 0,
                        (struct sockaddr *) &broadcast_addr, sizeof broadcast_addr);
    if (ret_value < 0) {
        cout << "Sendto error." << endl;
        exit(0);
    }

    ret_value = recvfrom(sockfd, pack, sizeof(*pack), 0,
                                 (struct sockaddr *) &m_address, &m_address_len);
            
    if (ret_value < 0) {
        //Manager doesn't sent response => There's no manager
        cout << "TIMEOUT EXPIRED!" << endl << endl;
        return false;
    } else {
        //Manager sent response => There's a manager
        participant m = parsePayload(pack->payload);
        g_manager_hostname = m.hostname;
        g_manager_MAC = m.MAC;
        g_manager_ip = m.IP;
        return true;
    }
}

// ------------------------------------------------ MAIN CODE section --------------------------------------------------

int main(int argc, char **argv) {
    signal(SIGINT, signalHandler); // CTRL+C
    signal(SIGHUP, signalHandler); // terminal closed while process still running
    
    bool manager_alive = false;

    initialize();

    manager_alive = newcommer();

    if(manager_alive) {
        cout << "Initializing Participant..." << endl << endl;
        sleep(2);
        participant_function();
    } else {
        cout << "Initializing Manager..." << endl << endl;
        sleep(2);
        manager_function();
    }

    cout << "Call: ./sleep_server" << endl;
    return -1;
}
