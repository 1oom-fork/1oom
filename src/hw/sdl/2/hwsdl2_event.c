#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_keycode.h"

#include "types.h"
#include "hw.h"
#include "hwsdl_audio.h"
#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "hwsdl2_window.h" /* hwsdl_video_resized, hwsdl_video_update */
#include "hwsdl2_video_buffers.h" /* hwsdl_video_screenshot */
#include "hwsdl2_video_texture.h" /* hwsdl_texture_delete */
#include "kbd.h"
#include "log.h"
#include "options.h"
#include "hwsdl2_keymap.h"

static bool hw_textinput_active = false;
static bool hw_key_press_repeat_active = true;

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

/* -------------------------------------------------------------------------- */

static bool hw_kbd_check_hotkey(SDL_Keycode key, SDL_Keymod smod, char c)
{
    if ((smod & KMOD_CTRL) && (!(smod & KMOD_ALT))) {
        if (key == SDLK_ESCAPE) {
            log_message("SDL: got Ctrl-ESC, quitting now\n");
            hw_audio_shutdown_pre();
            exit(EXIT_SUCCESS);
        } else if (key == SDLK_F5) {
            hwsdl_video_screenshot();
            return true;
#ifdef FEATURE_MODEBUG
        } else if (key == SDLK_INSERT) {
            hw_opt_overlay_pal ^= 1;
            return true;
#endif
        }
    } else if ((smod & (KMOD_MODE | KMOD_ALT)) && (!(smod & KMOD_CTRL))) {
        if (key == SDLK_RETURN) {
            if (!hwsdl_video_toggle_fullscreen()) {
                log_message("SDL: fs toggle failure, quitting now\n");
                exit(EXIT_FAILURE);
            }
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------- */

static void limit_fps(void)
{
    /* prevent busy-polling the cpu to 100%
     * but if the program is slow then don't stall for no reason */
    static uint32_t prev_tick = 0;
    uint32_t now = SDL_GetTicks();
    if (now - prev_tick < 5) {
        SDL_Delay(5);
    }
    prev_tick = now;
}

int hw_event_handle(void)
{
    SDL_Event e, resize;
    bool have_expose=false;
    bool have_resize=false;

    SDL_PumpEvents();

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            uint32_t mod;
            case SDL_KEYDOWN:
                if (e.key.repeat && !hw_key_press_repeat_active) {
                    break;
                }
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
                            if (!hw_textinput_active) {
                                c = key_xlat_char[SDLK_TBLI_FROM_SCAN(sym)];
                            }
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
                hw_mouse_set_xy(e.motion.x, e.motion.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                hw_mouse_button(e.button.button, e.button.state == SDL_PRESSED);
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
                switch (e.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        if (!have_resize || resize.window.timestamp < e.window.timestamp) {
                            have_resize = true;
                            resize = e;
                        }
                        break;
                    case SDL_WINDOWEVENT_EXPOSED:
                        have_expose = true;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_RENDER_DEVICE_RESET:
            case SDL_RENDER_TARGETS_RESET:
                hwsdl_texture_delete();
                have_expose = true;
                break;
            default:
                break;
        }
    }

    if (hw_audio_check_process()) {
        exit(EXIT_FAILURE);
    }

    if (have_resize) {
        hwsdl_video_resized(resize.window.data1, resize.window.data2);
    }

    if (have_expose) {
        hwsdl_video_update();
    }

    limit_fps();
    return 0;
}

void hwsdl_generate_quit_event(void)
{
    SDL_Event e;
    e.type = SDL_QUIT;
    e.window.timestamp = SDL_GetTicks();
    SDL_PushEvent(&e);
}

bool hw_kbd_set_repeat(bool enabled)
{
    hw_key_press_repeat_active = enabled;
    return true;
}

