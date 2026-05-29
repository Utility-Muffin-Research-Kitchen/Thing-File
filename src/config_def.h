#ifndef CONFIG_DEF_H_
#define CONFIG_DEF_H_

// Defines for config defaults, so that they can be overriden via the build
// system.

#ifndef RES_DIR
#define RES_DIR "res/"
#endif

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 320
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 240
#endif

#ifndef SCREEN_BPP
#define SCREEN_BPP 16
#endif

#ifndef PPU_Y
#define PPU_Y 1
#endif
#ifndef PPU_X
#define PPU_X 1
#endif

#ifndef AUTOSCALE
#define AUTOSCALE 1
#endif
#ifndef AUTOSCALE_DPI
#define AUTOSCALE_DPI 1
#endif

#ifndef PATH_DEFAULT
#define PATH_DEFAULT getenv("PWD")
#endif

#ifndef PATH_DEFAULT_RIGHT
#define PATH_DEFAULT_RIGHT getenv("HOME")
#endif

#ifndef FILE_SYSTEM
#define FILE_SYSTEM "/dev/sda4"
#endif

#ifndef CMDR_KEY_UP
#define CMDR_KEY_UP SDLK_UP
#endif
#ifndef CMDR_KEY_RIGHT
#define CMDR_KEY_RIGHT SDLK_RIGHT
#endif
#ifndef CMDR_KEY_DOWN
#define CMDR_KEY_DOWN SDLK_DOWN
#endif
#ifndef CMDR_KEY_LEFT
#define CMDR_KEY_LEFT SDLK_LEFT
#endif
#ifndef CMDR_KEY_SYSTEM
#define CMDR_KEY_SYSTEM SDLK_y
#endif
#ifndef CMDR_KEY_PAGEUP
#define CMDR_KEY_PAGEUP SDLK_l
#endif
#ifndef CMDR_KEY_PAGEDOWN
#define CMDR_KEY_PAGEDOWN SDLK_r
#endif
#ifndef CMDR_KEY_OPEN
#define CMDR_KEY_OPEN SDLK_a
#endif
#ifndef CMDR_KEY_PARENT
#define CMDR_KEY_PARENT SDLK_b
#endif
#ifndef CMDR_KEY_OPERATION
#define CMDR_KEY_OPERATION SDLK_x
#endif
#ifndef CMDR_KEY_SELECT
#define CMDR_KEY_SELECT SDLK_SPACE
#endif
#ifndef CMDR_KEY_TRANSFER
#define CMDR_KEY_TRANSFER SDLK_RETURN
#endif
#ifndef CMDR_KEY_MENU
#define CMDR_KEY_MENU SDLK_h
#endif

#endif // CONFIG_DEF_H_
