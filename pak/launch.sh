#!/bin/sh
set -eu

APP_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
BIN="$APP_DIR/bin/thing-file"
PAK_SDCARD_ROOT=$(CDPATH= cd -- "$APP_DIR/../.." && pwd)
MLP1_DEFAULT_SDCARD_PATH=/mnt/sdcard

if [ -n "${UMRK_ENV_FILE:-}" ] && [ -f "$UMRK_ENV_FILE" ]; then
    . "$UMRK_ENV_FILE"
elif [ -n "${SDCARD_PATH:-}" ] && [ -f "$SDCARD_PATH/umrk-launcher/env.sh" ]; then
    . "$SDCARD_PATH/umrk-launcher/env.sh"
elif [ -f "$PAK_SDCARD_ROOT/umrk-launcher/env.sh" ]; then
    . "$PAK_SDCARD_ROOT/umrk-launcher/env.sh"
fi

if [ -z "${PLATFORM:-}" ]; then
    case "$PAK_SDCARD_ROOT" in
        "$MLP1_DEFAULT_SDCARD_PATH") PLATFORM=mlp1 ;;
        *) PLATFORM=mac ;;
    esac
fi
SDCARD_PATH=${SDCARD_PATH:-${JAWAKA_SDCARD_ROOT:-$PAK_SDCARD_ROOT}}
case "$PLATFORM" in
    tg5040|tg5050|my355) DEFAULT_SYSTEM_PATH=$SDCARD_PATH/.system/$PLATFORM ;;
    *) DEFAULT_SYSTEM_PATH=$SDCARD_PATH/UMRK/$PLATFORM ;;
esac
SYSTEM_PATH=${SYSTEM_PATH:-$DEFAULT_SYSTEM_PATH}
UMRK_PLATFORM_PATH=${UMRK_PLATFORM_PATH:-$SYSTEM_PATH}
export PLATFORM SDCARD_PATH SYSTEM_PATH UMRK_PLATFORM_PATH

DEFAULT_LEFT=$SDCARD_PATH
DEFAULT_RIGHT=$UMRK_PLATFORM_PATH
DEFAULT_FS=/
if [ "${PLATFORM:-}" = "mlp1" ]; then
    DEFAULT_FS=/dev/mmcblk1p1
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
