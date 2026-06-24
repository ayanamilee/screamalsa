#include "network.h"
#include "stdio.h"
#include <unistd.h>

static rctx_network_t rctx_network;

int init_network(enum receiver_type receiver_mode, in_addr_t interface, int port, char* multicast_group)
{
  rctx_network.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (rctx_network.sockfd < 0) {
    perror("Failed to craete socket");
    return 1;
  }

  memset((void *)&(rctx_network.servaddr), 0, sizeof(rctx_network.servaddr));
  rctx_network.servaddr.sin_family = AF_INET;
  rctx_network.servaddr.sin_addr.s_addr = (receiver_mode == Unicast) ? interface : htonl(INADDR_ANY);
  rctx_network.servaddr.sin_port = htons(port);

  if (bind(rctx_network.sockfd, (struct sockaddr *)&rctx_network.servaddr, sizeof(rctx_network.servaddr)) != 0) {
    perror("Failed to bind to interface");
    return 1;
  }

  if (receiver_mode == Multicast) {
    rctx_network.imreq.imr_multiaddr.s_addr = inet_addr(multicast_group ? multicast_group : DEFAULT_MULTICAST_GROUP);
    rctx_network.imreq.imr_interface.s_addr = interface;

    if (setsockopt(rctx_network.sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                  (const void *)&rctx_network.imreq, sizeof(struct ip_mreq)) != 0) {
      perror("Failed to join multicast group");
      return 1;
    };
  }

  return 0;
}

void rcv_network(receiver_data_t* receiver_data)
{
  ssize_t n = 0;

  while (n < HEADER_SIZE) {
    n = recvfrom(rctx_network.sockfd, rctx_network.buf, MAX_SO_PACKETSIZE, 0, NULL, 0);
    if (n < 0) {
      if (verbosity) perror("recvfrom");
      usleep(1000);
      n = 0;
      continue;
    }
  }
  unsigned char b0 = rctx_network.buf[0];
  unsigned char b1 = rctx_network.buf[1];
  unsigned char b2 = rctx_network.buf[2];
  unsigned char b3 = rctx_network.buf[3];
  unsigned char b4 = rctx_network.buf[4];
  unsigned char b5 = rctx_network.buf[5];

  receiver_data->format.sample_rate = scream_decode_rate(b0, b4);
  receiver_data->format.sample_size = b1;
  receiver_data->format.channels = b2;
  /* channel_map from low byte only (driver puts chmask here; b4 now carries rate+flag) */
  receiver_data->format.channel_map = b3;
  receiver_data->format.wire_layout = b5;
  receiver_data->audio_size = (n > HEADER_SIZE) ? (n - HEADER_SIZE) : 0;
  receiver_data->audio = &rctx_network.buf[HEADER_SIZE];
}

