#include "config.h"
#include "SDL.h"

#ifdef HAVE_SDL1MIXER
#define HAVE_SDLMIXER
#include "SDL_mixer.h"
#include "SDL_rwops.h"
#define USE_SFX_INIT_THREAD
#define HWSDLX_CreateThread(_func_) SDL_CreateThread(_func_, 0)
#endif /* HAVE_SDL1MIXER */
#include "hwsdl_audio.c"

#include "hwsdl_mouse.c"
#include "hwsdl_aspect.c"

void hw_mouse_warp(int x, int y) {
    SDL_Event junk[100];
    int wx, wy;
    moo_to_window(x, y, NULL, &wx, &wy);
    SDL_WarpMouse(wx, wy);
    SDL_PumpEvents();
    while (SDL_PeepEvents(junk, 100, SDL_GETEVENT, SDL_MOUSEMOTIONMASK)==100) {
        /* eat generated mouse events. only randomly generated on some OS + SDL1 library combo */
    }
}

