#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SDLMAIN
#include "SDL_main.h"
#endif

#include "SDL.h"
#include "hw.h"
#include "hwsdl_audio.h"
#include "hwsdl_mouse.h"
#include "hwsdl2_window.h"
#include "hwsdl2_video_buffers.h"
#include "log.h"
#include "main.h"
#include "options.h"
#include "types.h"

const char *idstr_hw = "sdl2";
int main(int argc, char **argv) { return main_1oom(argc, argv); }
void hw_log_message(const char *msg) { fputs(msg, stdout); }
void hw_log_warning(const char *msg) { fputs(msg, stderr); }
void hw_log_error(const char *msg) { fputs(msg, stderr); }
int64_t hw_get_time_us(void) { return SDL_GetTicks() * 1000; }
int hw_early_init(void) { return 0; }

int hw_init(void)
{
    int flags = SDL_INIT_VIDEO | (opt_audio_enabled ? SDL_INIT_AUDIO : 0);
    if (SDL_Init(flags) < 0) {
        log_error("SDL_Init(0x%x) failed: %s\n", flags, SDL_GetError());
        return 11;
    }
    if (hw_audio_init()) {
        return 12;
    }
    return 0;
}

void hw_shutdown(void)
{
    hw_audio_shutdown();
    hwsdl_video_shutdown();
    SDL_Quit();
}

int hw_video_init(int w, int h)
{
    hw_mouse_set_moo_range(w, h);
    if (!hwsdl_video_buffers_alloc(w, h)) {
        return -1;
    }
    if (hwsdl_win_init(w, h) < 0) {
        return -1;
    }
    return 0;
}

