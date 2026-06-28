#ifndef NETWORK_H
#define NETWORK_H

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "scream.h"

#define DEFAULT_MULTICAST_GROUP "239.255.77.77"
#define DEFAULT_PORT 4010

/* ScreamALSA extended protocol header size.
 * See the driver source (snd-screamalsa.c) for the full 6-byte header layout.
 * byte[0]+byte[4] extended for high sample rates (DSD512/DSD1024 support).
 * byte[5] is wire_layout only for 24-bit PCM.
 */
#define HEADER_SIZE 6
#define MAX_SO_PACKETSIZE (1152 + HEADER_SIZE)

typedef struct rctx_network {
  int sockfd;
  struct sockaddr_in servaddr;
  struct ip_mreq imreq;
  unsigned char buf[MAX_SO_PACKETSIZE];
} rctx_network_t;

int init_network(enum receiver_type receiver_mode, in_addr_t interface, int port, char* multicast_group, int legacy);
void rcv_network(receiver_data_t* receiver_data);

#endif
