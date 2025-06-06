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

uint8_t *hw_video_draw_buf(void)
{
    vgabuf_select_back();
    i_hw_video.render();
    i_hw_video.update();
    return vgabuf_get_back();
}

void hw_video_redraw_front(void)
{
    i_hw_video.render();
    i_hw_video.update();
}
