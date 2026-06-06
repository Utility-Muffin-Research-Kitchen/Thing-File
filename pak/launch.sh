#!/bin/sh
set -eu

APP_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
BIN="$APP_DIR/bin/thing-file"
MLP1_DEFAULT_SDCARD_PATH=/mnt/sdcard
TG5040_DEFAULT_SDCARD_PATH=/mnt/SDCARD

find_sdcard_root() {
    probe=$APP_DIR
    while [ "$probe" != "/" ] && [ -n "$probe" ]; do
        if [ -d "$probe/.system/leaf/platforms" ] ||
           [ -f "$probe/.system/leaf/launcher/env.sh" ] ||
           { [ -d "$probe/Apps" ] && [ -d "$probe/.system" ]; }; then
            printf '%s\n' "$probe"
            return 0
        fi
        probe=$(dirname "$probe")
    done

    case "$APP_DIR" in
        */Apps/*/*.pak) (CDPATH= cd -- "$APP_DIR/../../.." && pwd) ;;
        *) (CDPATH= cd -- "$APP_DIR/../.." && pwd) ;;
    esac
}

infer_platform() {
    case "$SDCARD_PATH" in
        "$MLP1_DEFAULT_SDCARD_PATH") printf '%s\n' mlp1; return 0 ;;
        "$TG5040_DEFAULT_SDCARD_PATH") printf '%s\n' tg5040; return 0 ;;
    esac

    case "$APP_DIR" in
        */Apps/*/*.pak)
            platform_dir=$(basename "$(dirname "$APP_DIR")")
            if [ "$platform_dir" != "shared" ]; then
                printf '%s\n' "$platform_dir"
                return 0
            fi
            ;;
    esac

    if [ "$(uname -s 2>/dev/null || true)" = "Darwin" ]; then
        printf '%s\n' mac
        return 0
    fi

    echo "PLATFORM is not set; launch from Jawaka or source .system/leaf/platforms/<platform>/launcher/env.sh" >&2
    exit 1
}

PAK_SDCARD_ROOT=$(find_sdcard_root)
SDCARD_PATH=${SDCARD_PATH:-${JAWAKA_SDCARD_ROOT:-$PAK_SDCARD_ROOT}}
if [ -z "${PLATFORM:-}" ]; then
    PLATFORM=$(infer_platform)
fi
DEFAULT_SYSTEM_PATH=$SDCARD_PATH/.system/leaf/platforms/$PLATFORM
PLATFORM_ENV_FILE=$DEFAULT_SYSTEM_PATH/launcher/env.sh

if [ -n "${UMRK_ENV_FILE:-}" ] && [ -f "$UMRK_ENV_FILE" ]; then
    . "$UMRK_ENV_FILE"
elif [ -f "$PLATFORM_ENV_FILE" ]; then
    . "$PLATFORM_ENV_FILE"
elif [ -n "${SDCARD_PATH:-}" ] && [ -f "$SDCARD_PATH/.system/leaf/platforms/$PLATFORM/launcher/env.sh" ]; then
    . "$SDCARD_PATH/.system/leaf/platforms/$PLATFORM/launcher/env.sh"
elif [ -n "${SDCARD_PATH:-}" ] && [ -f "$SDCARD_PATH/.system/leaf/launcher/env.sh" ]; then
    . "$SDCARD_PATH/.system/leaf/launcher/env.sh"
elif [ -f "$PAK_SDCARD_ROOT/.system/leaf/launcher/env.sh" ]; then
    . "$PAK_SDCARD_ROOT/.system/leaf/launcher/env.sh"
fi

SDCARD_PATH=${SDCARD_PATH:-${JAWAKA_SDCARD_ROOT:-$PAK_SDCARD_ROOT}}
if [ -z "${PLATFORM:-}" ]; then
    PLATFORM=$(infer_platform)
fi
DEFAULT_SYSTEM_PATH=$SDCARD_PATH/.system/leaf/platforms/$PLATFORM
SYSTEM_PATH=${SYSTEM_PATH:-$DEFAULT_SYSTEM_PATH}
UMRK_PLATFORM_PATH=${UMRK_PLATFORM_PATH:-$SYSTEM_PATH}
export PLATFORM SDCARD_PATH SYSTEM_PATH UMRK_PLATFORM_PATH

DEFAULT_LEFT=$SDCARD_PATH
DEFAULT_RIGHT=$UMRK_PLATFORM_PATH
DEFAULT_FS=/
if [ "${PLATFORM:-}" = "mlp1" ]; then
    if [ -d /media ]; then
        DEFAULT_RIGHT=/media
    fi
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
