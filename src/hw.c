#include "config.h"

#include "hw.h"
#include "hw/hw_internal.h"

#include "types.h"

/* -------------------------------------------------------------------------- */

struct i_hw_video_s i_hw_video = { 0 };

/* -------------------------------------------------------------------------- */

void hw_video_refresh_palette(void)
{
    i_hw_video.setpal(vgapal, 0, 256);
}

void hw_video_redraw_front(void)
{
    i_hw_video.render();
    i_hw_video.update();
}

void hw_video_position_cursor(int mx, int my)
{
    if (opt_mouse_warp_enabled && i_hw_video.position_cursor) {
        i_hw_video.position_cursor(mx, my);
    }
}
