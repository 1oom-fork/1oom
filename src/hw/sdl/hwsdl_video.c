static int hw_video_lock(void)
{
    if (SDL_MUSTLOCK(video.screen)) {
        if (SDL_LockSurface(video.screen) < 0) {
            return -1;
        }
    }
    return 0;
}

static void hw_video_unlock(void)
{
    if (SDL_MUSTLOCK(video.screen)) {
        SDL_UnlockSurface(video.screen);
    }
}

#include "hw_video.c"
