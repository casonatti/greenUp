#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib> // exit() precisa desse include nos labs do inf

// -------------------------------------------------- CONSTANTS --------------------------------------------------------

#define BUFFER_SIZE 32
#define MAX_MACHINES 10
#define PORT_MANAGER_LISTENING 8000
#define PORT_PARTICIPANT_LISTENING 4000
#define TYPE_EXIT 3

#define SLEEP_SERVICE_DISCOVERY "sleep service discovery"
#define SLEEP_STATUS_REQUEST    "sleep status request"

// ------------------------------------------------- STRUCTURES --------------------------------------------------------

struct managerDB {
    const char *hostname;
    const char *MAC;
    const char *IP;
    const char *status;
};

struct packet {
    uint16_t type;              // DATA | CMD
    uint16_t seqn;              // sequence number
    uint16_t length;            // payload length
    uint16_t timestamp;         // packet timestamp
    char payload[BUFFER_SIZE];  // packet data
};

// -------------------------------------------------- VARIABLES --------------------------------------------------------

// prefixadas com g_ por serem globais e para nao sofrerem shadowing das locais
int g_sockfd, g_ret_value, g_seqn = 1;
in_addr_t g_manager_addr = inet_addr("192.168.1.13"); // trocar para o endereco da VM manager
in_addr_t g_broadcast_addr = inet_addr("192.168.1.255"); // trocar para o endereco de broadcast da rede local
struct sockaddr_in g_recv_addr, g_serv_addr;
struct packet *g_pack = (struct packet *) malloc(sizeof(packet));
