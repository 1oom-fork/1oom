bool hw_video_in_gfx = false;

static int hw_video_lock(void)
{
    return 0;
}

static void hw_video_unlock(void)
{
}

#include "hw_video.c"
