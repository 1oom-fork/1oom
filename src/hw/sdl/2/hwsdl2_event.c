#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SDLMAIN
#include "SDL_main.h"
#endif

#include "SDL.h"
#include "SDL_keycode.h"

#include "hw.h"
#include "hwsdl_audio.h"
#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "hwsdl_video.h"
#include "hwsdl2_video.h"
#include "kbd.h"
#include "log.h"
#include "main.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const char *idstr_hw = "sdl2";

/* -------------------------------------------------------------------------- */

#include "hwsdl2_keymap.h"

static bool hw_textinput_active = false;

/* -------------------------------------------------------------------------- */

#define SDL1or2Key  SDL_Keycode
#define SDL1or2Mod  SDL_Keymod

#include "hwsdl.c"

/* -------------------------------------------------------------------------- */

static void hw_event_handle_window(SDL_WindowEvent *e)
{
    switch (e->event) {
        case SDL_WINDOWEVENT_RESIZED:
            hw_video_resize(0, 0);
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            hw_video_update();
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            hw_mouse_ungrab();
            break;
        default:
            break;
    }
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
    int flags = SDL_INIT_VIDEO | (opt_audio_enabled ? SDL_INIT_AUDIO : 0);
    log_message("SDL_Init\n");
    if (SDL_Init(flags) < 0) {
        log_error("SDL_Init(0x%x) failed: %s\n", flags, SDL_GetError());
        return 11;
    }
    if (hw_audio_init()) {
        return 12;
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

void hw_textinput_start(void)
{
    SDL_StartTextInput();
    hw_textinput_active = true;
}

void hw_textinput_stop(void)
{
    SDL_StopTextInput();
    hw_textinput_active = false;
}

int hw_event_handle(void)
{
    SDL_Event e;

    SDL_PumpEvents();

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            uint32_t mod;
            case SDL_KEYDOWN:
                {
                    SDL_Keycode sym;
                    SDL_Keymod smod;
                    char c;
                    sym = e.key.keysym.sym;
                    smod = e.key.keysym.mod;
                    c = 0;
                    if (!(hw_kbd_check_hotkey(sym, smod, c))) {
                        mookey_t key;
                        if (sym & SDLK_SCANCODE_MASK) {
                            key = key_xlat_scan[SDLK_TBLI_FROM_SCAN(sym)];
                            c = 0;
                        } else {
                            key = key_xlat_key[sym];
                            c = (char)sym; /* TODO SDL 2 */
                            /* ignore ASCII range when expecting SDL_TEXTINPUT */
                            if (hw_textinput_active && ((key >= MOO_KEY_SPACE) && (key <= MOO_KEY_z))) {
                                key = MOO_KEY_LAST;
                            }
                        }
                        mod = mod_xlat(smod);
                        if (sym == SDLK_LCTRL || sym == SDLK_RCTRL) mod |= MOO_MOD_CTRL;
                        if ((key != MOO_KEY_UNKNOWN) && (key < MOO_KEY_LAST)) {
                            kbd_add_keypress(key, mod, c);
                        }
                        kbd_set_pressed(key, mod, true);
                    }
                }
                break;
            case SDL_KEYUP:
                {
                    SDL_Keycode sym;
                    SDL_Keymod smod;
                    mookey_t key;
                    sym = e.key.keysym.sym;
                    smod = e.key.keysym.mod;
                    mod = mod_xlat(smod);
                    if (sym == SDLK_LCTRL || sym == SDLK_RCTRL) mod &= ~MOO_MOD_CTRL;
                    if (sym & SDLK_SCANCODE_MASK) {
                        key = key_xlat_scan[SDLK_TBLI_FROM_SCAN(sym)];
                    } else {
                        key = key_xlat_key[sym];
                    }
                    kbd_set_pressed(key, mod, false);
                }
                break;
            case SDL_TEXTINPUT:
                if (hw_textinput_active && (e.text.text[0] != 0)) {
                    char c = e.text.text[0];
                    SDL_StopTextInput();
                    SDL_StartTextInput();
                    kbd_add_keypress(MOO_KEY_UNKNOWN, 0, c);
                }
                break;
            case SDL_MOUSEMOTION:
                if (hw_mouse_enabled) {
                    if (hw_opt_relmouse) {
                        hw_mouse_move((int)(e.motion.xrel), (int)(e.motion.yrel));
                    } else {
                        hw_mouse_set_xy(e.motion.x,e.motion.y);
                    }
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                hw_mouse_button((int)(e.button.button), (e.button.state == SDL_PRESSED));
                break;
            case SDL_MOUSEWHEEL:
                if (e.wheel.y != 0) {
                    hw_mouse_scroll((e.wheel.y > 0) ? -1 : 1);
                }
                break;
            case SDL_QUIT:
                hw_audio_shutdown_pre();
                exit(EXIT_SUCCESS);
                break;
            case SDL_WINDOWEVENT:
                if (e.window.windowID == hw_video_get_window_id()) {
                    hw_event_handle_window(&e.window);
                }
            default:
                break;
        }
    }
    if (hw_audio_check_process()) {
        exit(EXIT_FAILURE);
    }

    SDL_Delay(10);
    return 0;
}
