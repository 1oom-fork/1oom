#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SDLMAIN
#include "SDL_main.h"
#endif

#include "SDL.h"
#include "SDL_keysym.h"

#include "hw.h"
#include "hwsdl_audio.h"
#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "hwsdl_video.h"
#include "kbd.h"
#include "log.h"
#include "main.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const char *idstr_hw = "sdl1";

/* -------------------------------------------------------------------------- */

#include "hwsdl.c"

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
    SDL_WM_SetCaption("1oom", "1oom");
    if (hw_audio_init()) {
        return 12;
    }
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    SDL_EnableUNICODE(1);
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

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_KEYDOWN:
                {
                    uint16_t c = e.key.keysym.unicode;
                    if (((c & 0xff80) == 0) && ((c & 0x7f) != 0)) {
                        c &= 0x7f;
                    } else {
                        c = 0;
                    }
                    SDLMod smod = e.key.keysym.mod;
                    if (!(hw_kbd_check_hotkey(e.key.keysym.sym, smod, c))) {
                        uint32_t mod = 0;
                        if (smod & KMOD_SHIFT) { mod |= MOO_MOD_SHIFT; }
                        if (smod & KMOD_ALT) { mod |= MOO_MOD_ALT; }
                        if (smod & KMOD_CTRL) { mod |= MOO_MOD_CTRL; }
                        if (smod & KMOD_META) { mod |= MOO_MOD_META; }
                        kbd_add_keypress(key_xlat[e.key.keysym.sym], mod, c);
                    }
                }
                break;
            case SDL_KEYUP:
                break;
            case SDL_MOUSEMOTION:
                if (hw_mouse_enabled) {
                    hw_mouse_move((int)(e.motion.xrel), (int)(e.motion.yrel));
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                hw_mouse_button((int)(e.button.button), (e.button.state == SDL_PRESSED));
                break;
            case SDL_QUIT:
                exit(EXIT_SUCCESS);
                break;
            case SDL_VIDEORESIZE:
                hw_video_resize((unsigned int)e.resize.w, (unsigned int)e.resize.h);
                break;
            case SDL_VIDEOEXPOSE:
                hw_video_update();
                break;
            default:
                break;
        }
    }

    SDL_Delay(10);
    return 0;
}
