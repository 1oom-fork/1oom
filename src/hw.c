#include "config.h"

#include "hw/hw_internal.h"

/* -------------------------------------------------------------------------- */

struct i_hw_video_s i_hw_video = { 0 };

/* -------------------------------------------------------------------------- */

void hw_video_refresh_palette(void)
{
    i_hw_video.setpal(vgapal, 0, 256);
}
