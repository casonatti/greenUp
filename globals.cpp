#include <netinet/in.h>
#include <cstdlib> // exit() precisa desse include nos labs do inf
#include "participantsTable.h"

// -------------------------------------------------- CONSTANTS --------------------------------------------------------
#define I_AM_MANAGER 0
#define I_AM_PARTICIPANT 1
#define PORT_KEEP_ALIVE 5556
#define PORT_KEEP_ALIVE_LISTENER 5555
#define PORT_DISCOVERY_SERVICE_BROADCAST 8000
#define PORT_DISCOVERY_SERVICE_LISTENER 8001
#define PORT_MONITORING_SERVICE_BROADCAST 9000
#define PORT_MONITORING_SERVICE_LISTENER 9001


// -------------------------------------------------- VARIABLES --------------------------------------------------------

// prefixadas com g_ por serem globais e para nao sofrerem shadowing das locais
int g_sockfd, g_seqn = 1;
bool is_manager = false;
bool g_has_manager = false;
bool g_table_updated = false;
struct sockaddr_in g_serv_addr{};
string g_my_hostname, g_my_mac_addr, g_my_ip_addr, g_manager_hostname, g_manager_MAC, g_manager_ip;
ParticipantsTable pTable;
pthread_t thr_discovery, thr_monitoring, thr_interface, thr_keep_alive;
pthread_attr_t attr_discovery, attr_monitoring, attr_interface, attr_keep_alive;

Packet *g_pack = (Packet *) malloc(sizeof(Packet));
pid_t g_my_pid;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
