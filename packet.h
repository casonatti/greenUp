//
// Created by pedro on 30/09/22.
//

#ifndef GREENUP_PACKET_H
#define GREENUP_PACKET_H

#include <netinet/in.h>
#include <ctime>

#define BUFFER_SIZE 128

#define TYPE_DISCOVERY 1
#define TYPE_MONITORING 2
#define TYPE_EXIT 3
#define TYPE_KEEP_ALIVE 4
#define TYPE_NEWCOMMER 5
#define TYPE_ELECTION 6

#define KEEP_ALIVE "keep alive"
#define NEWCOMMER "new commer"
#define SLEEP_SERVICE_DISCOVERY "sleep service discovery"
#define SLEEP_SERVICE_EXIT      "sleep service exit"
#define SLEEP_STATUS_REQUEST    "sleep status request"

typedef struct _packet {
  uint16_t type;              // DATA | CMD
  uint16_t seqn;              // sequence number
  uint16_t length;            // payload length
  char payload[BUFFER_SIZE];// packet data
} Packet;

#endif //GREENUP_PACKET_H
