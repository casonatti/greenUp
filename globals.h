//
// Created by pedro on 05/10/22.
//

#ifndef GREENUP_GLOBALS_H
#define GREENUP_GLOBALS_H
#include "participantsTable.h"

extern string g_my_hostname, g_my_mac_addr, g_my_ip_addr, g_manager_hostname, g_manager_MAC, g_manager_ip;
extern ParticipantsTable pTable;
extern bool is_manager, g_has_manager;
extern struct sockaddr_in g_serv_addr;

#endif //GREENUP_GLOBALS_H
