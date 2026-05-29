#include "umrk_input.h"

#include <algorithm>
#include <array>
#include <cstdio>

#include "config.h"

namespace {

#if defined(PLATFORM_MLP1)
constexpr int kMlp1BtnB = 0;
constexpr int kMlp1BtnA = 1;
constexpr int kMlp1BtnX = 2;
constexpr int kMlp1BtnY = 3;
constexpr int kMlp1BtnL1 = 4;
constexpr int kMlp1BtnR1 = 5;
constexpr int kMlp1BtnL2 = 6;
constexpr int kMlp1BtnR2 = 7;
constexpr int kMlp1BtnSelect = 8;
constexpr int kMlp1BtnStart = 9;
constexpr int kMlp1BtnMenu = 10;
constexpr int kMlp1BtnStick = 11;
#endif

constexpr int kAxisDeadzone = 16384;

std::array<bool, 32> g_held {};
uint8_t g_hat_state = 0;
int g_axis_x = 0;
int g_axis_y = 0;
bool g_l2_axis_held = false;
bool g_r2_axis_held = false;
#ifdef USE_SDL2
SDL_GameController *g_controller = nullptr;
#endif

bool isValidButton(UmrkButton button)
{
    return button != UmrkButton::None &&
        static_cast<int>(button) >= 0 &&
        static_cast<size_t>(button) < g_held.size();
}

void setHeld(UmrkButton button, bool held)
{
    if (!isValidButton(button)) return;
    g_held[static_cast<size_t>(button)] = held;
}

bool isHeld(UmrkButton button)
{
    if (!isValidButton(button)) return false;
    return g_held[static_cast<size_t>(button)];
}

UmrkButton keyboardButton(SDL_Keycode sym)
{
    switch (sym) {
        case SDLK_UP: return UmrkButton::Up;
        case SDLK_DOWN: return UmrkButton::Down;
        case SDLK_LEFT: return UmrkButton::Left;
        case SDLK_RIGHT: return UmrkButton::Right;
        case SDLK_a: return UmrkButton::A;
        case SDLK_b: return UmrkButton::B;
        case SDLK_x: return UmrkButton::X;
        case SDLK_y: return UmrkButton::Y;
        case SDLK_l: return UmrkButton::L1;
        case SDLK_SEMICOLON: return UmrkButton::L2;
        case SDLK_r: return UmrkButton::R1;
        case SDLK_t: return UmrkButton::R2;
        case SDLK_RETURN: return UmrkButton::Start;
        case SDLK_SPACE: return UmrkButton::Select;
        case SDLK_h: return UmrkButton::Menu;
#if !defined(PLATFORM_MLP1)
        case SDLK_q: return UmrkButton::Quit;
#endif
        default: return UmrkButton::None;
    }
}

UmrkButton joyButton(int button)
{
#if defined(PLATFORM_MLP1)
    switch (button) {
        case kMlp1BtnA: return UmrkButton::A;
        case kMlp1BtnB: return UmrkButton::B;
        case kMlp1BtnX: return UmrkButton::X;
        case kMlp1BtnY: return UmrkButton::Y;
        case kMlp1BtnL1: return UmrkButton::L1;
        case kMlp1BtnR1: return UmrkButton::R1;
        case kMlp1BtnL2: return UmrkButton::L2;
        case kMlp1BtnR2: return UmrkButton::R2;
        case kMlp1BtnSelect: return UmrkButton::Select;
        case kMlp1BtnStart: return UmrkButton::Start;
        case kMlp1BtnMenu: return UmrkButton::Menu;
        case kMlp1BtnStick: return UmrkButton::None;
        default:
            std::fprintf(stderr, "Thing-File input: unmapped MLP1 joystick button=%d\n", button);
            return UmrkButton::None;
    }
#else
    switch (button) {
        case 0: return UmrkButton::B;
        case 1: return UmrkButton::A;
        case 2: return UmrkButton::Y;
        case 3: return UmrkButton::X;
        case 4: return UmrkButton::L1;
        case 5: return UmrkButton::R1;
        case 6: return UmrkButton::Select;
        case 7: return UmrkButton::Start;
        case 8: return UmrkButton::Menu;
        case 10: return UmrkButton::L2;
        case 11: return UmrkButton::R2;
        default: return UmrkButton::None;
    }
#endif
}

#ifdef USE_SDL2
UmrkButton controllerButton(int button)
{
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return UmrkButton::A;
        case SDL_CONTROLLER_BUTTON_B: return UmrkButton::B;
        case SDL_CONTROLLER_BUTTON_X: return UmrkButton::X;
        case SDL_CONTROLLER_BUTTON_Y: return UmrkButton::Y;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return UmrkButton::L1;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return UmrkButton::R1;
        case SDL_CONTROLLER_BUTTON_BACK: return UmrkButton::Select;
        case SDL_CONTROLLER_BUTTON_START: return UmrkButton::Start;
        case SDL_CONTROLLER_BUTTON_GUIDE: return UmrkButton::Menu;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return UmrkButton::Up;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return UmrkButton::Down;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return UmrkButton::Left;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return UmrkButton::Right;
        default: return UmrkButton::None;
    }
}
#endif

SDL_Event keyEventFor(UmrkButton button)
{
    SDL_Event result {};
    result.type = SDL_KEYDOWN;
    result.key.type = SDL_KEYDOWN;
    result.key.state = SDL_PRESSED;
    result.key.keysym.sym = UmrkInput::actionKeycode(button);
    return result;
}

bool emitButton(UmrkButton button, bool pressed, SDL_Event *out_key_event)
{
    if (!isValidButton(button)) return false;

    const bool was_held = isHeld(button);
    setHeld(button, pressed);

    if (!pressed || was_held || !out_key_event) return false;
    *out_key_event = keyEventFor(button);
    return out_key_event->key.keysym.sym != 0;
}

bool emitAxisDirection(int &current, int next, UmrkButton negative,
                       UmrkButton positive, SDL_Event *out_key_event)
{
    if (current == next) return false;
    if (current < 0) setHeld(negative, false);
    if (current > 0) setHeld(positive, false);
    current = next;
    if (next < 0) return emitButton(negative, true, out_key_event);
    if (next > 0) return emitButton(positive, true, out_key_event);
    return false;
}

bool emitTriggerAxis(bool &held, bool next, UmrkButton button, SDL_Event *out_key_event)
{
    if (held == next) return false;
    held = next;
    return emitButton(button, next, out_key_event);
}

int axisDirection(Sint16 value)
{
    if (value < -kAxisDeadzone) return -1;
    if (value > kAxisDeadzone) return 1;
    return 0;
}

bool handleHat(uint8_t next_hat, SDL_Event *out_key_event)
{
    const uint8_t previous = g_hat_state;
    g_hat_state = next_hat;

    if (!(next_hat & SDL_HAT_UP) && (previous & SDL_HAT_UP)) setHeld(UmrkButton::Up, false);
    if (!(next_hat & SDL_HAT_DOWN) && (previous & SDL_HAT_DOWN)) setHeld(UmrkButton::Down, false);
    if (!(next_hat & SDL_HAT_LEFT) && (previous & SDL_HAT_LEFT)) setHeld(UmrkButton::Left, false);
    if (!(next_hat & SDL_HAT_RIGHT) && (previous & SDL_HAT_RIGHT)) setHeld(UmrkButton::Right, false);

    if ((next_hat & SDL_HAT_UP) && !(previous & SDL_HAT_UP))
        return emitButton(UmrkButton::Up, true, out_key_event);
    if ((next_hat & SDL_HAT_DOWN) && !(previous & SDL_HAT_DOWN))
        return emitButton(UmrkButton::Down, true, out_key_event);
    if ((next_hat & SDL_HAT_LEFT) && !(previous & SDL_HAT_LEFT))
        return emitButton(UmrkButton::Left, true, out_key_event);
    if ((next_hat & SDL_HAT_RIGHT) && !(previous & SDL_HAT_RIGHT))
        return emitButton(UmrkButton::Right, true, out_key_event);
    return false;
}

} // namespace

namespace UmrkInput {

void init()
{
    std::fill(g_held.begin(), g_held.end(), false);
    g_hat_state = 0;
    g_axis_x = 0;
    g_axis_y = 0;
    g_l2_axis_held = false;
    g_r2_axis_held = false;

#ifdef USE_SDL2
#if !defined(PLATFORM_MLP1)
    if (SDL_NumJoysticks() > 0 && SDL_IsGameController(0)) {
        g_controller = SDL_GameControllerOpen(0);
        if (g_controller) {
            std::printf("Opened GameController 0: %s\n", SDL_GameControllerName(g_controller));
        }
    }
#endif
#endif
}

void shutdown()
{
#ifdef USE_SDL2
    if (g_controller) {
        SDL_GameControllerClose(g_controller);
        g_controller = nullptr;
    }
#endif
}

SDLC_Keycode actionKeycode(UmrkButton button)
{
    const auto &c = config();
    switch (button) {
        case UmrkButton::Up: return c.key_up;
        case UmrkButton::Down: return c.key_down;
        case UmrkButton::Left: return c.key_left;
        case UmrkButton::Right: return c.key_right;
        case UmrkButton::A: return c.key_open;
        case UmrkButton::B: return c.key_parent;
        case UmrkButton::X: return c.key_operation;
        case UmrkButton::Y: return c.key_system;
        case UmrkButton::L1:
        case UmrkButton::L2: return c.key_pageup;
        case UmrkButton::R1:
        case UmrkButton::R2: return c.key_pagedown;
        case UmrkButton::Start: return c.key_transfer;
        case UmrkButton::Select: return c.key_select;
        case UmrkButton::Menu: return c.key_system;
        case UmrkButton::Quit: return SDLK_q;
        case UmrkButton::None: return 0;
    }
    return 0;
}

bool actionHeld(SDLC_Keycode keycode)
{
    for (size_t i = 0; i < g_held.size(); ++i) {
        if (!g_held[i]) continue;
        if (actionKeycode(static_cast<UmrkButton>(i)) == keycode) return true;
    }
    return false;
}

bool handleEvent(const SDL_Event &event, SDL_Event *out_key_event)
{
    switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.repeat) return false;
            return emitButton(keyboardButton(event.key.keysym.sym), true, out_key_event);
        case SDL_KEYUP:
            return emitButton(keyboardButton(event.key.keysym.sym), false, out_key_event);
        case SDL_JOYBUTTONDOWN:
#ifdef USE_SDL2
            if (g_controller) return false;
#endif
            return emitButton(joyButton(event.jbutton.button), true, out_key_event);
        case SDL_JOYBUTTONUP:
#ifdef USE_SDL2
            if (g_controller) return false;
#endif
            return emitButton(joyButton(event.jbutton.button), false, out_key_event);
        case SDL_JOYHATMOTION:
#ifdef USE_SDL2
            if (g_controller) return false;
#endif
            return handleHat(event.jhat.value, out_key_event);
        case SDL_JOYAXISMOTION:
            if (event.jaxis.axis == 0)
                return emitAxisDirection(g_axis_x, axisDirection(event.jaxis.value),
                                         UmrkButton::Left, UmrkButton::Right, out_key_event);
            if (event.jaxis.axis == 1)
                return emitAxisDirection(g_axis_y, axisDirection(event.jaxis.value),
                                         UmrkButton::Up, UmrkButton::Down, out_key_event);
            if (event.jaxis.axis == 2)
                return emitTriggerAxis(g_l2_axis_held, event.jaxis.value > 0,
                                       UmrkButton::L2, out_key_event);
            if (event.jaxis.axis == 5)
                return emitTriggerAxis(g_r2_axis_held, event.jaxis.value > 0,
                                       UmrkButton::R2, out_key_event);
            return false;
#ifdef USE_SDL2
        case SDL_CONTROLLERBUTTONDOWN:
            return emitButton(controllerButton(event.cbutton.button), true, out_key_event);
        case SDL_CONTROLLERBUTTONUP:
            return emitButton(controllerButton(event.cbutton.button), false, out_key_event);
        case SDL_CONTROLLERAXISMOTION:
            if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
                return emitAxisDirection(g_axis_x, axisDirection(event.caxis.value),
                                         UmrkButton::Left, UmrkButton::Right, out_key_event);
            if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
                return emitAxisDirection(g_axis_y, axisDirection(event.caxis.value),
                                         UmrkButton::Up, UmrkButton::Down, out_key_event);
            if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
                return emitTriggerAxis(g_l2_axis_held, event.caxis.value > kAxisDeadzone,
                                       UmrkButton::L2, out_key_event);
            if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                return emitTriggerAxis(g_r2_axis_held, event.caxis.value > kAxisDeadzone,
                                       UmrkButton::R2, out_key_event);
            return false;
#endif
        default:
            return false;
    }
}

} // namespace UmrkInput
