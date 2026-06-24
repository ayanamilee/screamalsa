#ifndef SCREAM_H
#define SCREAM_H

#include <stdint.h>

enum receiver_type {
  Unicast, Multicast, SharedMem, Pcap
};

enum output_type {
  Raw, Alsa, Pulseaudio, Jack, Sndio
};

/* wire_layout byte (header byte[5]). Meaningful only for 24-bit PCM;
 * receivers must ignore it for all other sample sizes.
 */
enum scream_wire_layout {
  SCREAM_WIRE_PACKED = 0,   /* S24_3LE: 3 bytes per sample on the wire */
  SCREAM_WIRE_S24_LE = 1,   /* S24_LE: 24-bit samples in 32-bit LE containers */
};

#define SCREAM_SUPPORTED_CHANNELS 2

typedef struct receiver_format {
  uint32_t sample_rate;        /* Decoded rate value (from extended encoding). For DSD this is the halved value; receiver doubles when sample_size==1. */
  unsigned char sample_size;   /* 1=DSD, 16/24/32=PCM */
  unsigned char channels;
  uint16_t channel_map;
  unsigned char wire_layout;   /* byte[5]: see enum scream_wire_layout */
} receiver_format_t;

/* Return the number of bytes occupied by one sample on the wire.
 * For PCM this follows wire_layout when sample_size == 24.
 * For DSD (sample_size == 1) it is always 4 bytes (DSD_U32_BE).
 */
static inline unsigned int scream_bytes_per_sample(const receiver_format_t *rf)
{
  switch (rf->sample_size) {
  case 1:  return 4;   /* DSD_U32_BE; wire_layout ignored */
  case 16: return 2;
  case 24: return (rf->wire_layout == SCREAM_WIRE_S24_LE) ? 4 : 3;
  case 32: return 4;
  default: return 0;
  }
}

/* Decode rate from wire bytes (supports extended high-rate encoding for DSD512+).
 * b0 = byte[0], b4 = byte[4]
 */
static inline uint32_t scream_decode_rate(unsigned char b0, unsigned char b4)
{
  uint16_t mult = (uint16_t)b0 | ((uint16_t)(b4 & 0x0F) << 8);
  unsigned int base = (b4 & 0x10) ? 44100U : 48000U;
  return (uint32_t)base * mult;
}

static inline int scream_is_end_of_track(unsigned char b4)
{
  return (b4 & 0x80) != 0;
}

typedef struct receiver_data {
  receiver_format_t format;
  unsigned int audio_size;
  unsigned char* audio;
} receiver_data_t;

extern int verbosity;

#endif
