ScreamALSA - Virtual Audio Driver and Helper Scripts

This repository contains a Linux kernel module `snd-screamalsa.c` and helper scripts to build and install it. The driver exposes a virtual ALSA sound card that transmits PCM/DSD audio over the network using an extended Scream protocol, including support for DSD format streaming.

Key features
- Virtual ALSA audio device backed by a network transport
- Extended Scream protocol with 6-byte header, stereo-only PCM 16/24/32-bit, and DSD support
- Multi-distro build and install helpers for common Linux distributions
- Module 2.0.4: PipeWire-friendly PCM (PAUSE, keep playback kthread on socket reuse)

Driver 2.0.4 (PipeWire clients, e.g. Spotify Soloist)
- Symptom: first track OK; next track silent (receiver may still see a PCM stream of zeros,
  or idle if the sender stops). Direct ALSA clients (MPD, CamillaDSP, shairport-sync) were fine.
  Seeking in the Spotify app restored sound until the following track change.
- Cause: spa-alsa vs this virtual card. A short write gap at track boundaries could
  TRIGGER_STOP / XRUN; injecting silence while RUNNING ran hw_ptr past appl_ptr so the
  next track was never heard. Do not send fake silence on underrun.
- Module changes (snd-screamalsa.c, MODULE_VERSION 2.0.4):
  * SNDRV_PCM_INFO_PAUSE plus TRIGGER_PAUSE_PUSH / PAUSE_RELEASE (and SUSPEND/RESUME)
  * open() reuses the UDP/TCP socket but restarts the scream_tx kthread if close() stopped it
  * stop_threshold = buffer_size (allow XRUN; PipeWire recovers with prepare+start, same path as seek)
  * only packetize real ALSA data; no silence fill
- Rebuild on the machine that loads the module (kernel headers must match uname -r):
    cd /home/audiolinux/screamalsa && sudo make && sudo make install
    sudo rmmod snd_screamalsa
    sudo insmod /lib/modules/$(uname -r)/extra/snd-screamalsa.ko \
        ip_addr_str=<receiver> protocol_str=udp port=4011
  Check: modinfo snd-screamalsa | grep ^version   → 2.0.4
- Isolated PipeWire sink used with this driver should keep the device open and use a
  ~1.5 s buffer (see streamtx spotify-soloist/install.sh 10-spotify-sink.conf):
    session.suspend-timeout-seconds = 0
    node.pause-on-idle = false
    api.alsa.headroom = 2048
    api.alsa.period-size = 2048
    api.alsa.period-num = 32

Protocol notes
- Header byte[5] (wire_layout) only for 24-bit PCM (0=packed S24_3LE 3B, 1=S24_LE 4B container).
- Rate encoding extended using byte[0] + bits in byte[4] to support DSD rates up to DSD512. (DSD1024+ limited by fixed payload and scheduling.)
- Receivers should ignore wire_layout for non-24 and DSD (byte[1]==1).
- ALSA receiver supports full PCM 16/24/32 + DSD; Pulse receiver has 24-bit wire_layout fix (S24_32LE vs S24LE).

Receivers
- Unix/Linux receiver (ALSA, PulseAudio, JACK, etc.) is included in this repository under Receivers/unix/.
  Build: cd Receivers/unix && mkdir -p build && cd build && cmake .. && make
  Run with ScreamALSA defaults: ./Receivers/unix/screamalsa.sh -v
  (UDP unicast, port 4011, ALSA output)
  Legacy mode (original 5-byte Scream header): ./scream -u -p 4011 -o alsa -d plughw:1,0 -L -v
  Use legacy mode for the original screamalsa driver, ap2renderer, or any sender that uses
  the original 5-byte Scream protocol. Legacy mode also deinterleaves DSD frames to match
  standard ALSA DSD_U32_BE order.
- Additional receivers for other platforms are available in the archive:
  https://albumplayer.ru/asioscream4.zip

macOS virtual sound card
- macos/ contains ScreamAudio, a Core Audio HAL plug-in that appears as "Scream (Exclusive)"
  (hog / integer mode, like ALSA hw:) or "Scream (Network)" if EXCLUSIVE=0.
  It streams the same 6-byte ScreamALSA protocol over UDP/TCP. PCM 16/24/32-bit stereo is
  supported; DSD is not (Core Audio has no DSD path).
  Build:  cd macos && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
  Install: sudo ./macos/install.sh
  Config:  macos/scream.conf  then  sudo screamctl apply
  HEADER=original speaks the upstream 5-byte igor63r/screamalsa protocol;
  HEADER=extended (default) is this fork's 6-byte header.

Windows ASIO sender
- windows/asioscreamalsa/ is a GPL-2 ASIO driver that applies the same protocol
  as this fork (6-byte header, optional HEADER=original for stock asioscream 4).
  The asioscream4/ folder is the original closed-source Album Player bundle
  (no source); use ASIO ScreamALSA instead of ASIO Scream 4.
  Build on Windows: see windows/asioscreamalsa/README.md
  See macos/README.md for formats, rates, and limitations.

Command-line options (Unix receiver)
- -u                       : Use unicast instead of multicast.
- -p <port>                : UDP port (default 4010 for multicast, often 4011 for unicast).
- -i <iface>               : Bind to interface by name or IP.
- -g <group>               : Multicast group address (default 239.255.77.77).
- -o pulse|alsa|jack|sndio|raw : Audio output backend.
- -d <device>              : ALSA/sndio device name.
- -s <sink>                : PulseAudio sink name.
- -n <name>                : PulseAudio stream name / JACK client name.
- -t <latency>             : Target latency in milliseconds (default 50).
- -l <latency>             : Max latency for PulseAudio (default 200).
- -c                       : Do not auto-connect JACK ports.
- -L                       : Legacy mode: parse original 5-byte Scream header.
- -v                       : Verbose output (repeat for more detail).

Scream Scripts - Quick Guide

This repository contains helper scripts to build and install the ScreamALSA kernel module.

1) build_scream.sh
   - Purpose: Verifies build environment across many Linux distros and builds the driver.
   - What it does:
     * Detects your package manager (apt, pacman, dnf, yum, zypper, xbps, apk, emerge, nix).
     * Checks for required files and kernel headers for the running kernel.
     * Checks for build tools (gcc, make) and provides distro-specific hints if missing.
     * Runs make and logs output to build.log.
   - Usage (Linux):
     ./build_scream.sh

2) install_prebuild.sh
   - Purpose: Installs an already-built module file from the current directory.
   - What it does:
     * Requires root privileges (sudo).
     * Verifies that snd-screamalsa.ko exists in the current directory.
     * Copies it to /lib/modules/$(uname -r)/extra and runs depmod -a.
     * Creates /etc/modprobe.d/screamalsa.conf with default options.
     * Creates /etc/modules-load.d/screamalsa.conf to autoload the module.
     * Loads the module via modprobe snd-screamalsa.
   - Usage (Linux):
     sudo ./install_prebuild.sh

3) install_full.sh
   - Purpose: Performs a full installation: builds the driver, installs it, and loads the module.
   - What it does:
     * Runs the build script to compile the module for the current kernel.
     * Installs the resulting snd-screamalsa.ko into /lib/modules/$(uname -r)/extra and runs depmod -a.
     * Creates /etc/modprobe.d/screamalsa.conf with default options.
     * Creates /etc/modules-load.d/screamalsa.conf to autoload the module.
     * Loads the module via modprobe snd-screamalsa.
   - Usage (Linux):
     sudo ./install_full.sh <command>
     Example: sudo ./install_full.sh install
  - Commands:
     install    - Full installation (build + install + load)
     build      - Build module only
     load       - Load module
     unload     - Unload module
     remove     - Full removal from the system
     status     - Show status
     test       - Test the module
     help       - Show help

4)  scream_config.sh 
     - Purpose: Configures the driver parameters according to those specified in the scream.conf file.
Notes
- Always run build/install steps on the same kernel version you intend to load the module on.
- If build fails, follow the hints.
- If you cloned this repository with git clone, the shell scripts already have executable permissions, so you can run them directly.
- If you downloaded the project as a ZIP archive instead, Unix file permissions are not preserved. In that case, make the scripts executable manually, for example:
   chmod +x build_scream.sh
  
- You can share the compiled driver with other users by uploading it to the repository at the following link:
  https://albumplayer.ru/screamalsa/
