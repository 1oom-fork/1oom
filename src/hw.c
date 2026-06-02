#include "config.h"

#include "hw.h"
#include "hw/hw_internal.h"

#include "types.h"

/* -------------------------------------------------------------------------- */

struct i_hw_video_s i_hw_video = { 0 };

/* -------------------------------------------------------------------------- */

int hw_video_set_scale(int scale)
{
    if (!i_hw_video.setscale) {
        return 0;
    }
    return i_hw_video.setscale(scale);
}

void hw_video_refresh_palette(void)
{
    i_hw_video.setpal(vgapal, 0, 256);
}

void hw_video_redraw_front(void)
{
    i_hw_video.render();
    i_hw_video.update();
}
