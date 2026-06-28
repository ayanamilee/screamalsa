#include "network.h"
#include "stdio.h"
#include <unistd.h>

static rctx_network_t rctx_network;
static int legacy_mode = 0;

int init_network(enum receiver_type receiver_mode, in_addr_t interface, int port, char* multicast_group, int legacy)
{
  legacy_mode = legacy;
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

/* Original Scream / ap2renderer sends DSD with left/right bytes interleaved
 * per 8-byte frame. ALSA DSD_U32_BE expects each channel's 4 bytes contiguous.
 * This reverses the original driver-side convert_data() transform.
 */
static void dsd_legacy_deinterleave(char *src, size_t bytes)
{
  size_t frames = bytes / 8;
  while (frames--) {
    char tmp[8];
    /* incoming: [L0][R0][L1][R1][L2][R2][L3][R3] */
    tmp[0] = src[0];
    tmp[1] = src[2];
    tmp[2] = src[4];
    tmp[3] = src[6];
    tmp[4] = src[1];
    tmp[5] = src[3];
    tmp[6] = src[5];
    tmp[7] = src[7];
    memcpy(src, tmp, 8);
    src += 8;
  }
}

void rcv_network(receiver_data_t* receiver_data)
{
  ssize_t n = 0;
  const size_t hdr_size = legacy_mode ? 5 : HEADER_SIZE;

  while (n < (ssize_t)hdr_size) {
    n = recvfrom(rctx_network.sockfd, rctx_network.buf, MAX_SO_PACKETSIZE, 0, NULL, 0);
    if (n < 0) {
      if (verbosity) perror("recvfrom");
      usleep(1000);
      n = 0;
      continue;
    }
  }

  if (legacy_mode) {
    /* Original 5-byte Scream header:
     * byte[0]: rate code
     * byte[1]: sample size
     * byte[2]: channels
     * byte[3]: channel map low byte
     * byte[4]: channel map high byte
     */
    unsigned char b0 = rctx_network.buf[0];
    unsigned char b1 = rctx_network.buf[1];
    unsigned char b2 = rctx_network.buf[2];
    unsigned char b3 = rctx_network.buf[3];
    unsigned char b4 = rctx_network.buf[4];

    receiver_data->format.sample_rate = scream_decode_rate_legacy(b0);
    receiver_data->format.sample_size = b1;
    receiver_data->format.channels = b2;
    receiver_data->format.channel_map = b3 | ((uint16_t)b4 << 8);
    receiver_data->format.wire_layout = 0; /* legacy has no wire_layout byte */
    receiver_data->audio_size = (n > 5) ? (n - 5) : 0;
    receiver_data->audio = &rctx_network.buf[5];

    /* Original Scream/ap2renderer DSD is byte-interleaved per frame.
     * Convert to standard ALSA DSD_U32_BE frame order on the fly.
     */
    if (receiver_data->format.sample_size == 1 && receiver_data->audio_size >= 8) {
      dsd_legacy_deinterleave((char*)receiver_data->audio, receiver_data->audio_size);
    }
    return;
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

