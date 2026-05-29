#ifndef UMRK_INPUT_H_
#define UMRK_INPUT_H_

#include <SDL.h>

#include "sdl_backports.h"

enum class UmrkButton {
    None = 0,
    Up,
    Down,
    Left,
    Right,
    A,
    B,
    X,
    Y,
    L1,
    L2,
    R1,
    R2,
    Start,
    Select,
    Menu,
    Quit,
};

namespace UmrkInput {

void init();
void shutdown();
bool handleEvent(const SDL_Event &event, SDL_Event *out_key_event);
bool actionHeld(SDLC_Keycode keycode);
SDLC_Keycode actionKeycode(UmrkButton button);

} // namespace UmrkInput

#endif // UMRK_INPUT_H_
