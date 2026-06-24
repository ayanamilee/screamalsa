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

Supported PCM formats: 16 / 24 / 32-bit (`S16_LE`, `S24_3LE`, `S24_LE`, `S32_LE`),
plus DSD when the driver sends it. ScreamALSA uses a 6-byte protocol header;
byte `[5]` selects packed vs container layout for 24-bit streams.

See `unix/README.md` for full build and usage details.