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

Supported PCM formats: 16-bit and 32-bit (`S16_LE` / `S32_LE`), plus DSD when the driver sends it.

See `unix/README.md` for full build and usage details.