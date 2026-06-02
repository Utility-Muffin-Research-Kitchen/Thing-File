# Thing-File

Thing-File is UMRK's Jawaka-launchable fork of
[LoveRetro/NextCommander](https://github.com/LoveRetro/NextCommander), itself
an OpenDingux-oriented DinguxCommander fork.

It remains a two-pane commander-style file manager with the upstream operation
set: browse, view, edit text, view images, copy, move, symlink, rename, delete,
create directories, inspect disk usage, and execute files.

## UMRK changes

- app identity renamed to `Thing-File`
- binary renamed to `thing-file`
- native macOS preview build
- Miniloong Pocket 1 build through the UMRK MLP1 toolchain image
- local UMRK input adapter mirroring Catastrophe's virtual button model
- Jawaka `Apps/Thing-File.pak` package generation and ADB staging

## Build

```sh
make native
make package-native
make install-jawaka-app
make mlp1
make package-mlp1
make adb-stage-pak-mlp1
```

## Run Desktop Preview

```sh
make run-native
```

Desktop controls follow Catastrophe's development mapping:

- arrows: move
- `A`: open/input
- `B`: parent/cancel
- `X`: selected-item operations
- `Y` or `H`: system menu
- `L` / `R`: page up/down
- `;` / `T`: page up/down trigger aliases
- `Space`: select highlighted item
- `Enter`: open source path in destination panel
- `Q`: quit from the main panel

On MLP1, raw Loong Gamepad button indices are normalized before they reach the
upstream Commander windows.

## Jawaka Package

`make package-native` or `make package-mlp1` creates:

```text
build/package/Thing-File.pak/
  bin/thing-file
  launch.sh
  pak.json
  res/
```

`launch.sh` writes a temporary config with the active left/right panel defaults:

- left panel: `SDCARD_PATH`
- right panel: `UMRK_PLATFORM_PATH` (or `SYSTEM_PATH`), except on MLP1 where it
  defaults to `/media` when present so secondary SD mounts like
  `/media/sdcard1` are immediately visible

It sources `$SDCARD_PATH/umrk-launcher/env.sh` when present. Direct launches
can still override the panels with `UMRK_THING_FILE_LEFT` and
`UMRK_THING_FILE_RIGHT`.

## Upstream And License Notes

Upstream history and credits are intentionally preserved. The upstream
repository does not currently publish GitHub license metadata or a `LICENSE`
file, so this fork does not add a new blanket license for the imported source.

See `README.txt` for the original DinguxCommander readme and credits.
