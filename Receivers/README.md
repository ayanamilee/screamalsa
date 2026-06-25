# Scream Receivers

Unix/Linux receiver for the Scream network audio protocol, bundled with ScreamALSA.

- **Source:** `unix/` — CMake-based receiver (ALSA, PulseAudio, JACK, raw, etc.)
- **Driver:** see repository root `snd-screamalsa.c` and `README.txt`

## Quick start (ScreamALSA)

Build the receiver:

```shell
cd unix
mkdir -p build && cd build
cmake ..
make
```

Run against a ScreamALSA driver (default: UDP unicast port 4011):

```shell
./screamalsa.sh -v
```

Or pass options directly:

```shell
./build/scream -u -p 4011 -o alsa -v
```

Supported PCM formats: stereo-only 16 / 24 / 32-bit (`S16_LE`, `S24_3LE`, `S24_LE`,
`S32_LE`), plus DSD (up to DSD512 with ALSA backend). DSD1024+ is not supported due to practical limits. ScreamALSA uses a 6-byte
protocol header; byte `[5]` selects packed vs container for 24-bit; rate encoding
(bytes 0+4) extended for high rates (DSD512+). Multichannel (>2) rejected by driver + ALSA rcv.

See `unix/README.md` for full build and usage details.