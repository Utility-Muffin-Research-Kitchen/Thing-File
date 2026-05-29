#!/bin/sh
set -eu

APP_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
BIN="$APP_DIR/bin/thing-file"

if [ -d /mnt/sdcard ]; then
    DEFAULT_LEFT=/mnt/sdcard
    DEFAULT_RIGHT=/mnt/sdcard/UMRK
    DEFAULT_FS=/dev/mmcblk1p1
else
    DEFAULT_LEFT=${JAWAKA_SDCARD_ROOT:-$(CDPATH= cd -- "$APP_DIR/../.." && pwd)}
    DEFAULT_RIGHT="$DEFAULT_LEFT/UMRK/mac"
    DEFAULT_FS=/
fi

LEFT=${UMRK_THING_FILE_LEFT:-$DEFAULT_LEFT}
RIGHT=${UMRK_THING_FILE_RIGHT:-$DEFAULT_RIGHT}
FILESYSTEM=${UMRK_THING_FILE_FILESYSTEM:-$DEFAULT_FS}
CFG=${TMPDIR:-/tmp}/thing-file-$$.cfg

cleanup() {
    rm -f "$CFG"
}
trap cleanup EXIT INT TERM

cat >"$CFG" <<EOF
path_default = $LEFT
path_default_right = $RIGHT
path_default_right_fallback = $LEFT
file_system = $FILESYSTEM
res_dir = $APP_DIR/res
EOF

if [ -z "${SDL_VIDEODRIVER:-}" ] && [ -d /var/run ]; then
    export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/var/run}
    export SDL_VIDEODRIVER=wayland
fi

export SDL_NOMOUSE=1
exec "$BIN" --config-prelude "$CFG" --res-dir "$APP_DIR/res"
