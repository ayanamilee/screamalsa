#ifndef SCREAM_H
#define SCREAM_H

#include <stdint.h>

enum receiver_type {
  Unicast, Multicast, SharedMem, Pcap
};

enum output_type {
  Raw, Alsa, Pulseaudio, Jack, Sndio
};

typedef struct receiver_format {
  unsigned char sample_rate;
  unsigned char sample_size;
  unsigned char channels;
  uint16_t channel_map;
  unsigned char wire_layout;  /* byte[5]: 0=packed, 1=24-bit in 32-bit LE containers */
} receiver_format_t;

static inline unsigned int scream_bytes_per_sample(const receiver_format_t *rf)
{
  switch (rf->sample_size) {
  case 16: return 2;
  case 24: return rf->wire_layout ? 4 : 3;
  case 32: return 4;
  default: return 0;
  }
}

typedef struct receiver_data {
  receiver_format_t format;
  unsigned int audio_size;
  unsigned char* audio;
} receiver_data_t;

extern int verbosity;

#endif
