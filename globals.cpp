#include <netinet/in.h>
#include <cstdlib> // exit() precisa desse include nos labs do inf
#include "participantsTable.cpp"

// -------------------------------------------------- CONSTANTS --------------------------------------------------------

#define BUFFER_SIZE 128
#define I_AM_MANAGER 0
#define I_AM_PARTICIPANT 1
#define PORT_DISCOVERY_SERVICE_BROADCAST 8000
#define PORT_DISCOVERY_SERVICE_LISTENER 8001
#define PORT_MONITORING_SERVICE_BROADCAST 9000
#define PORT_MONITORING_SERVICE_LISTENER 9001
#define TYPE_DISCOVERY 1
#define TYPE_MONITORING 2
#define TYPE_EXIT 3

#define SLEEP_SERVICE_DISCOVERY "sleep service discovery"
#define SLEEP_SERVICE_EXIT      "sleep service exit"
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
bool g_has_manager = false;
bool g_table_updated = false;
struct sockaddr_in g_serv_addr{};
struct packet *g_pack = (struct packet *) malloc(sizeof(packet));
string g_my_hostname, g_my_mac_addr, g_my_ip_addr;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtable = PTHREAD_MUTEX_INITIALIZER;
participantsTable table;
