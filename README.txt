ScreamALSA - Virtual Audio Driver and Helper Scripts

This repository contains a Linux kernel module `snd-screamalsa.c` and helper scripts to build and install it. The driver exposes a virtual ALSA sound card that transmits PCM/DSD audio over the network using an extended Scream protocol, including support for DSD format streaming.

Key features
- Virtual ALSA audio device backed by a network transport
- Extended Scream protocol with 6-byte header, stereo-only PCM 16/24/32-bit, and DSD support
- Multi-distro build and install helpers for common Linux distributions

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
- Additional receivers for other platforms are available in the archive:
  https://albumplayer.ru/asioscream4.zip

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
