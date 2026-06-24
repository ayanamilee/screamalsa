#!/bin/sh
# Run the Unix receiver with ScreamALSA driver defaults (UDP unicast, port 4011).
set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SCREAM_BIN="$SCRIPT_DIR/build/scream"

if [ ! -x "$SCREAM_BIN" ]; then
    echo "Receiver not built. Run:" >&2
    echo "  cd $SCRIPT_DIR && mkdir -p build && cd build && cmake .. && make" >&2
    exit 1
fi

exec "$SCREAM_BIN" -u -p 4011 -o alsa "$@"