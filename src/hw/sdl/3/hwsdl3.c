#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "SDL3/SDL.h"
#include "SDL3/SDL_keycode.h"

#include "hw.h"
#include "hw_internal.h"
#include "hwsdl_audio.h"
#include "hwsdl_opt.h"
#include "hwsdl_video.h"
#include "hwsdl3_video.h"
#include "kbd.h"
#include "log.h"
#include "main.h"
#include "mouse.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const char *idstr_hw = "sdl3";

/* -------------------------------------------------------------------------- */

static mookey_t key_xlat_key[0x80];
static mookey_t key_xlat_scan[SDL_SCANCODE_COUNT];

#define SDLK_TBLI_FROM_SCAN(i) ((i) & (~SDLK_SCANCODE_MASK))

static void build_key_xlat(void)
{
    memset(key_xlat_key, 0, sizeof(key_xlat_key));
    memset(key_xlat_scan, 0, sizeof(key_xlat_scan));
    key_xlat_key[SDLK_BACKSPACE] = MOO_KEY_BACKSPACE;
    key_xlat_key[SDLK_TAB] = MOO_KEY_TAB;
    key_xlat_key[SDLK_RETURN] = MOO_KEY_RETURN;
    key_xlat_key[SDLK_ESCAPE] = MOO_KEY_ESCAPE;
    key_xlat_key[SDLK_SPACE] = MOO_KEY_SPACE;
    key_xlat_key[SDLK_EXCLAIM] = MOO_KEY_EXCLAIM;
    key_xlat_key[SDLK_DBLAPOSTROPHE] = MOO_KEY_QUOTEDBL;
    key_xlat_key[SDLK_HASH] = MOO_KEY_HASH;
    key_xlat_key[SDLK_DOLLAR] = MOO_KEY_DOLLAR;
    key_xlat_key[SDLK_AMPERSAND] = MOO_KEY_AMPERSAND;
    key_xlat_key[SDLK_APOSTROPHE] = MOO_KEY_QUOTE;
    key_xlat_key[SDLK_LEFTPAREN] = MOO_KEY_LEFTPAREN;
    key_xlat_key[SDLK_RIGHTPAREN] = MOO_KEY_RIGHTPAREN;
    key_xlat_key[SDLK_ASTERISK] = MOO_KEY_ASTERISK;
    key_xlat_key[SDLK_PLUS] = MOO_KEY_PLUS;
    key_xlat_key[SDLK_COMMA] = MOO_KEY_COMMA;
    key_xlat_key[SDLK_MINUS] = MOO_KEY_MINUS;
    key_xlat_key[SDLK_PERIOD] = MOO_KEY_PERIOD;
    key_xlat_key[SDLK_SLASH] = MOO_KEY_SLASH;
    key_xlat_key[SDLK_0] = MOO_KEY_0;
    key_xlat_key[SDLK_1] = MOO_KEY_1;
    key_xlat_key[SDLK_2] = MOO_KEY_2;
    key_xlat_key[SDLK_3] = MOO_KEY_3;
    key_xlat_key[SDLK_4] = MOO_KEY_4;
    key_xlat_key[SDLK_5] = MOO_KEY_5;
    key_xlat_key[SDLK_6] = MOO_KEY_6;
    key_xlat_key[SDLK_7] = MOO_KEY_7;
    key_xlat_key[SDLK_8] = MOO_KEY_8;
    key_xlat_key[SDLK_9] = MOO_KEY_9;
    key_xlat_key[SDLK_COLON] = MOO_KEY_COLON;
    key_xlat_key[SDLK_SEMICOLON] = MOO_KEY_SEMICOLON;
    key_xlat_key[SDLK_LESS] = MOO_KEY_LESS;
    key_xlat_key[SDLK_EQUALS] = MOO_KEY_EQUALS;
    key_xlat_key[SDLK_GREATER] = MOO_KEY_GREATER;
    key_xlat_key[SDLK_QUESTION] = MOO_KEY_QUESTION;
    key_xlat_key[SDLK_AT] = MOO_KEY_AT;
    key_xlat_key[SDLK_LEFTBRACKET] = MOO_KEY_LEFTBRACKET;
    key_xlat_key[SDLK_BACKSLASH] = MOO_KEY_BACKSLASH;
    key_xlat_key[SDLK_RIGHTBRACKET] = MOO_KEY_RIGHTBRACKET;
    key_xlat_key[SDLK_CARET] = MOO_KEY_CARET;
    key_xlat_key[SDLK_UNDERSCORE] = MOO_KEY_UNDERSCORE;
    key_xlat_key[SDLK_GRAVE] = MOO_KEY_BACKQUOTE;
    key_xlat_key[SDLK_A] = MOO_KEY_a;
    key_xlat_key[SDLK_B] = MOO_KEY_b;
    key_xlat_key[SDLK_C] = MOO_KEY_c;
    key_xlat_key[SDLK_D] = MOO_KEY_d;
    key_xlat_key[SDLK_E] = MOO_KEY_e;
    key_xlat_key[SDLK_F] = MOO_KEY_f;
    key_xlat_key[SDLK_G] = MOO_KEY_g;
    key_xlat_key[SDLK_H] = MOO_KEY_h;
    key_xlat_key[SDLK_I] = MOO_KEY_i;
    key_xlat_key[SDLK_J] = MOO_KEY_j;
    key_xlat_key[SDLK_K] = MOO_KEY_k;
    key_xlat_key[SDLK_L] = MOO_KEY_l;
    key_xlat_key[SDLK_M] = MOO_KEY_m;
    key_xlat_key[SDLK_N] = MOO_KEY_n;
    key_xlat_key[SDLK_O] = MOO_KEY_o;
    key_xlat_key[SDLK_P] = MOO_KEY_p;
    key_xlat_key[SDLK_Q] = MOO_KEY_q;
    key_xlat_key[SDLK_R] = MOO_KEY_r;
    key_xlat_key[SDLK_S] = MOO_KEY_s;
    key_xlat_key[SDLK_T] = MOO_KEY_t;
    key_xlat_key[SDLK_U] = MOO_KEY_u;
    key_xlat_key[SDLK_V] = MOO_KEY_v;
    key_xlat_key[SDLK_W] = MOO_KEY_w;
    key_xlat_key[SDLK_X] = MOO_KEY_x;
    key_xlat_key[SDLK_Y] = MOO_KEY_y;
    key_xlat_key[SDLK_Z] = MOO_KEY_z;

    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_CAPSLOCK)] = MOO_KEY_CAPSLOCK;

    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F1)] = MOO_KEY_F1;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F2)] = MOO_KEY_F2;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F3)] = MOO_KEY_F3;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F4)] = MOO_KEY_F4;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F5)] = MOO_KEY_F5;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F6)] = MOO_KEY_F6;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F7)] = MOO_KEY_F7;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F8)] = MOO_KEY_F8;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F9)] = MOO_KEY_F9;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F10)] = MOO_KEY_F10;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F11)] = MOO_KEY_F11;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F12)] = MOO_KEY_F12;

    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_SCROLLLOCK)] = MOO_KEY_SCROLLOCK;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_PAUSE)] = MOO_KEY_PAUSE;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_INSERT)] = MOO_KEY_INSERT;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_HOME)] = MOO_KEY_HOME;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_PAGEUP)] = MOO_KEY_PAGEUP;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_DELETE)] = MOO_KEY_DELETE;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_END)] = MOO_KEY_END;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_PAGEDOWN)] = MOO_KEY_PAGEDOWN;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_RIGHT)] = MOO_KEY_RIGHT;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_LEFT)] = MOO_KEY_LEFT;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_DOWN)] = MOO_KEY_DOWN;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_UP)] = MOO_KEY_UP;

    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_CLEAR)] = MOO_KEY_CLEAR;

    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F13)] = MOO_KEY_F13;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F14)] = MOO_KEY_F14;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_F15)] = MOO_KEY_F15;

    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_0)] = MOO_KEY_KP0;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_1)] = MOO_KEY_KP1;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_2)] = MOO_KEY_KP2;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_3)] = MOO_KEY_KP3;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_4)] = MOO_KEY_KP4;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_5)] = MOO_KEY_KP5;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_6)] = MOO_KEY_KP6;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_7)] = MOO_KEY_KP7;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_8)] = MOO_KEY_KP8;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_9)] = MOO_KEY_KP9;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_PERIOD)] = MOO_KEY_KP_PERIOD;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_DIVIDE)] = MOO_KEY_KP_DIVIDE;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_MULTIPLY)] = MOO_KEY_KP_MULTIPLY;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_MINUS)] = MOO_KEY_KP_MINUS;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_PLUS)] = MOO_KEY_KP_PLUS;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_ENTER)] = MOO_KEY_RETURN;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_KP_EQUALS)] = MOO_KEY_KP_EQUALS;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_NUMLOCKCLEAR)] = MOO_KEY_NUMLOCK;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_RSHIFT)] = MOO_KEY_RSHIFT;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_LSHIFT)] = MOO_KEY_LSHIFT;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_RCTRL)] = MOO_KEY_RCTRL;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_LCTRL)] = MOO_KEY_LCTRL;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_RALT)] = MOO_KEY_RALT;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_LALT)] = MOO_KEY_LALT;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_LGUI)] = MOO_KEY_LSUPER;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_RGUI)] = MOO_KEY_RSUPER;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_MODE)] = MOO_KEY_MODE;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_HELP)] = MOO_KEY_HELP;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_PRINTSCREEN)] = MOO_KEY_PRINT;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_SYSREQ)] = MOO_KEY_SYSREQ;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_MENU)] = MOO_KEY_MENU;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_POWER)] = MOO_KEY_POWER;
    key_xlat_scan[SDLK_TBLI_FROM_SCAN(SDLK_UNDO)] = MOO_KEY_UNDO;
}

/* -------------------------------------------------------------------------- */

static bool hw_kbd_check_hotkey(SDL_Keycode key, SDL_Keymod smod, char c)
{
    if ((smod & SDL_KMOD_CTRL) && !(smod & SDL_KMOD_ALT)) {
        if (key == SDLK_ESCAPE) {
            log_message("SDL: got Ctrl-ESC, quitting now\n");
            exit(EXIT_SUCCESS);
        } else if (key == SDLK_F10) {
            if (hw_mouse_enabled) {
                hw_mouse_ungrab();
            } else {
                hw_mouse_grab();
            }
            return true;
        } else if (key == SDLK_RIGHTBRACKET) {
            if (smod & SDL_KMOD_SHIFT) {
                hw_audio_music_volume(opt_music_volume + 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume + 4);
            }
            return true;
        } else if (key == SDLK_LEFTBRACKET) {
            if (smod & SDL_KMOD_SHIFT) {
                hw_audio_music_volume(opt_music_volume - 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume - 4);
            }
            return true;
        }
    } else if ((smod & SDL_KMOD_ALT) && !(smod & SDL_KMOD_CTRL)) {
        if (key == SDLK_RETURN) {
            if (hw_video_toggle_fullscreen() < 0) {
                log_message("SDL: fs toggle failure, quitting now\n");
                exit(EXIT_FAILURE);
            }
            return true;
        }
    }
    return false;
}

static void hw_mouse_button(int i, int pressed)
{
    if (hw_mouse_enabled) {
        int b = mouse_buttons;
        if (i == (int)SDL_BUTTON_LEFT) {
            if (pressed) {
                b |= MOUSE_BUTTON_MASK_LEFT;
            } else {
                b &= ~MOUSE_BUTTON_MASK_LEFT;
            }
        } else if (i == (int)SDL_BUTTON_RIGHT) {
            if (pressed) {
                b |= MOUSE_BUTTON_MASK_RIGHT;
            } else {
                b &= ~MOUSE_BUTTON_MASK_RIGHT;
            }
        }
        mouse_set_buttons_from_hw(b);
    }

    if (pressed) {
        if (hw_mouse_enabled) {
            if (i == (int)SDL_BUTTON_MIDDLE) {
                hw_mouse_ungrab();
            }
        } else {
            hw_mouse_grab();
        }
    }
}

uint32_t hw_get_time_us(void)
{
    return SDL_GetTicks() * 1000;
}

/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    return main_1oom(argc, argv);
}

int hw_early_init(void)
{
    return 0;
}

int hw_init(void)
{
    log_message("SDL_Init: ");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        log_message("FAILED\n");
        log_error("Failed to initialize video: %s\n", SDL_GetError());
        return 11;
    }
    if (opt_audio_enabled) {
        if (!SDL_Init(SDL_INIT_AUDIO))
        {
            log_message("FAILED\n");
            log_error("Failed to initialize audio\n");
            return 12;
        }
        log_message("OK\n");
        if (hw_audio_init()) {
            log_error("Unable to set up sound.\n");
            return 13;
        }
    }
    build_key_xlat();
    return 0;
}

void hw_shutdown(void)
{
    hw_audio_shutdown();
    hw_video_shutdown();
    log_message("SDL_Quit\n");
    SDL_Quit();
}

int hw_event_handle(void)
{
    SDL_Event e;

    SDL_PumpEvents();

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_KEY_DOWN:
                {
                    SDL_Keycode sym;
                    SDL_Keymod smod;
                    char c;
                    sym = e.key.key;
                    smod = e.key.mod;
                    c = 0;
                    if (!(hw_kbd_check_hotkey(sym, smod, c))) {
                        mookey_t key;
                        uint32_t mod = 0;
                        if (smod & SDL_KMOD_SHIFT) { mod |= MOO_MOD_SHIFT; }
                        if (smod & SDL_KMOD_ALT) { mod |= MOO_MOD_ALT; }
                        if (smod & SDL_KMOD_CTRL) { mod |= MOO_MOD_CTRL; }
                        if (sym & SDLK_SCANCODE_MASK) {
                            key = key_xlat_scan[SDLK_TBLI_FROM_SCAN(sym)];
                            c = 0;
                        } else {
                            key = key_xlat_key[sym];
                            c = (char)sym; /* TODO SDL 2 */
                            if ((c >= 'a') && (c <= 'z')) {
                                bool mod_shift = ((smod & SDL_KMOD_SHIFT) != 0);
                                bool mod_caps = ((smod & SDL_KMOD_CAPS) != 0);
                                if (mod_shift != mod_caps) {
                                    c -= 0x20;
                                }
                            }
                        }
                        if (key < MOO_KEY_NUMLOCK) {
                            kbd_add_keypress(key, mod, c);
                        }
                    }
                }
                break;
            case SDL_EVENT_KEY_UP:
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (!hw_opt_relmouse && hw_mouse_enabled) {
                    SDL_ConvertEventToRenderCoordinates(SDL_GetRenderer(SDL_GetWindowFromEvent(&e)), &e);
                    int x, y;
                    x = e.motion.x;
                    y = e.motion.y;
                    if (hw_opt_aspect_ratio_correct) {
                        y = (y * 5 + 5) / 6;
                    }
                    mouse_set_xy_from_hw(x, y);
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                hw_mouse_button((int)(e.button.button), e.button.down);
                break;
            case SDL_EVENT_QUIT:
                exit(EXIT_SUCCESS);
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                if (e.window.windowID == hw_video_get_window_id()) {
                    hw_video_resize(0, 0);
                }
                break;
            case SDL_EVENT_WINDOW_EXPOSED:
                if (e.window.windowID == hw_video_get_window_id()) {
                    i_hw_video.update();
                }
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (e.window.windowID == hw_video_get_window_id()) {
                    hw_mouse_ungrab();
                }
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                if (e.window.windowID == hw_video_get_window_id()) {
                    if (!hw_opt_relmouse && hw_opt_nograbmouse) {
                        hw_mouse_grab();
                    }
                }
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                if (e.window.windowID == hw_video_get_window_id()) {
                    hw_video_set_visible(false);
                }
                break;
            case SDL_EVENT_WINDOW_MAXIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
                if (e.window.windowID == hw_video_get_window_id()) {
                    hw_video_set_visible(true);
                }
                break;
            default:
                break;
        }
    }

    if (hw_opt_relmouse) {
        float x, y;
        SDL_GetRelativeMouseState(&x, &y);
        if ((x != 0) || (y != 0)) {
            if (hw_mouse_enabled) {
                mouse_set_xy_from_hw(moo_mouse_x + x, moo_mouse_y + y);
            }
        }
    }

    return 0;
}
