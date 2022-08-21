#include <netinet/in.h>
#include <cstdlib> // exit() precisa desse include nos labs do inf
#include "participantsTable.cpp"

// -------------------------------------------------- CONSTANTS --------------------------------------------------------

#define BUFFER_SIZE 128
#define PORT_DISCOVERY_SERVICE_BROADCAST 8000
#define PORT_DISCOVERY_SERVICE_LISTENER 8001
#define PORT_MONITORING_SERVICE_BROADCAST 9000
#define PORT_MONITORING_SERVICE_LISTENER 9001
#define TYPE_DISCOVERY 1
#define TYPE_MONITORING 2
#define TYPE_EXIT 3

#define SLEEP_SERVICE_DISCOVERY "sleep service discovery"
#define SLEEP_STATUS_REQUEST    "sleep status request"

// ------------------------------------------------- STRUCTURES --------------------------------------------------------

struct packet {
    uint16_t type;              // DATA | CMD
    uint16_t seqn;              // sequence number
    uint16_t length;            // payload length
    char payload[BUFFER_SIZE];// packet data
};

// -------------------------------------------------- VARIABLES --------------------------------------------------------

// prefixadas com g_ por serem globais e para nao sofrerem shadowing das locais
int g_sockfd, g_seqn = 1;
struct sockaddr_in g_serv_addr{};
struct packet *g_pack = (struct packet *) malloc(sizeof(packet));
//participantsTable table;
string my_hostname, my_mac_addr, my_ip_addr;
int banana = 0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
participantsTable table;
