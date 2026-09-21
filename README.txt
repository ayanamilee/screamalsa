ScreamALSA - Virtual Audio Driver and Helper Scripts

This repository contains a Linux kernel module `snd-screamalsa.c` and helper scripts to build and install it. The driver exposes a virtual ALSA sound card that transmits PCM/DSD audio over the network using an extended Scream protocol, including support for DSD format streaming.

Key features
- Virtual ALSA audio device backed by a network transport
- Extended Scream protocol with 6-byte header, stereo-only PCM 16/24/32-bit, and DSD support
- One module, two wire dialects via header_str (extended 6-byte or original 5-byte)
- Live ip_addr_str / port / header_str (next packet; no rmmod to switch)
- Multi-distro build and install helpers for common Linux distributions
- Module 2.0.7: original header is S32_LE (+ DSD) only, matching igor63r/apscream
- Module 2.0.6: live ip_addr_str/port applied on the next packet (no rmmod)
- Module 2.0.5: header_str=extended|original live switch (one .ko, 6-byte or 5-byte wire)
- Module 2.0.4: PipeWire-friendly PCM (PAUSE, keep playback kthread on socket reuse)

Driver 2.0.7 (header_str + live endpoint; original dialect = igor63r/apscream)
- One .ko. Do not ship a second "legacy" module. Switch dialect with header_str:
    header_str=extended  (default)  6-byte fork header, PCM 16/24/32 + DSD
    header_str=original  (or legacy) 5-byte igor63r header, PCM S32_LE + DSD
  Pair original with scream -L / scream2diretta --legacy / apscream.
  Pair extended with this tree's Unix receiver (no -L) or s2d without --legacy.
- Live sysfs (0644), same as ip_addr_str. Next packet uses the new dialect
  or destination; do not rmmod to switch header or Receiver IP.
    echo original > /sys/module/snd_screamalsa/parameters/header_str
    echo 172.20.0.2 > /sys/module/snd_screamalsa/parameters/ip_addr_str
  Persist in /etc/modprobe.d/screamalsa.conf:
    options snd-screamalsa ip_addr_str=... port=4011 protocol_str=udp header_str=extended
  scream.conf HEADER=extended|original is applied by scream_config.sh.
- 2.0.5 added header_str. 2.0.6 made ip_addr_str/port take effect per packet
  (UDP send used the sockaddr from the first open(); Apply IP then switch
  back left packets going to the old address).
- 2.0.7: original 5-byte header has no wire_layout byte. igor63r advertised
  only S32_LE (+ DSD); apscream and scream -L treat PCM as 32-bit. Advertising
  S16/S24 in original mode made MPD (auto_format no) open 16/24-bit, which
  both apscream and s2d --legacy played as noise. Original dialect now:
    * ALSA hw: S32_LE + DSD_U32_BE only (16/24 stay on extended)
    * header byte[1] = 32 for PCM, 1 for DSD
    * DSD still uses convert_data() byte shuffle so -L can deinterleave
  After switching header_str, reopen the PCM (stop/start the player) so
  ALSA renegotiates S32.
- Rebuild on the machine that loads the module (only when replacing the .ko):
    cd /home/audiolinux/screamalsa && sudo make && sudo make install
    sudo rmmod snd_screamalsa
    sudo modprobe snd-screamalsa
  Check: modinfo snd-screamalsa | grep ^version   → 2.0.7
          cat /sys/module/snd_screamalsa/parameters/header_str

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
- header_str=extended (default): 6-byte header. byte[5] (wire_layout) only for
  24-bit PCM (0=packed S24_3LE 3B, 1=S24_LE 4B container). Rate encoding uses
  byte[0] + bits in byte[4] (DSD up to DSD512). DSD is standard ALSA DSD_U32_BE
  frame order (no convert_data shuffle).
- header_str=original: 5-byte igor63r header. PCM is S32_LE only (byte[1]=32).
  DSD (byte[1]=1) uses convert_data() interleave. Rate is the original 8-bit
  code in byte[0] ( >=128 → 44100*(v-128), else 48000*v ). byte[4] is 0 while
  playing, 0x80 at end-of-track. No byte[5].
- Receivers should ignore wire_layout for non-24 and DSD (byte[1]==1).
- ALSA receiver supports full PCM 16/24/32 + DSD in extended mode; use -L for
  original. Pulse receiver has 24-bit wire_layout fix (S24_32LE vs S24LE).

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

Windows Apple Music sender (AMScream)
- windows/amscream/ is AMExclusive plus a Scream exclusive backend. Turning on
  Exclusive mode in Apple Music sends Scream (6-byte extended or 5-byte original)
  to the receiver in am-exclusive.ini; the Windows default device is not hogged.
  Build on Windows x64: see windows/amscream/README.md

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
