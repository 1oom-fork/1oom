#include "config.h"

#include <stdio.h>
#include <string.h>

#include "types.h"
#include <allegro.h>

#include "hw.h"
#include "hw_video.h"
#include "hwalleg_mouse.h"
#include "hwalleg_opt.h"
#include "lib.h"
#include "log.h"
#include "vgabuf.h"
#include "vgapal.h"

/* -------------------------------------------------------------------------- */

static struct alleg_video_s {
    BITMAP *bm;
    RGB color[256];
} video = { 0 };

/* -------------------------------------------------------------------------- */

static void video_render_8bpp(const uint8_t *buf)
{
    BITMAP *bm = video.bm;
    uint8_t *p;
    const uint8_t *q = buf;
    int w = bm->w;
    int h = bm->h;
    for (int y = 0; y < h; ++y) {
        p = bm->line[y];
        memcpy(p, q, w);
        q += w;
    }
}

static void video_update_8bpp(void)
{
    BITMAP *bm = video.bm;
    blit(bm, screen, 0, 0, 0, 0, bm->w, bm->h);
}

static void video_setpal_8bpp(const uint8_t *pal, int first, int num)
{
    for (int i = first; i < (first + num); ++i) {
        video.color[i].r = *pal++;
        video.color[i].g = *pal++;
        video.color[i].b = *pal++;
    }
    set_palette_range(video.color, first, first + num - 1, 1);
}

/* -------------------------------------------------------------------------- */

static int video_sw_set(int w, int h)
{
    if (set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0) != 0) {
        log_error("set_gfx_mode(..., %i, %i, 0, 0) failed!\n", w, h);
        return -1;
    }
#ifdef IS_MSDOS
    log_direct_enabled = false;
#endif
    return 0;
}

/* -------------------------------------------------------------------------- */

int hw_video_init(int w, int h)
{
    i_hw_video.setmode = video_sw_set;
    i_hw_video.render = video_render_8bpp;
    i_hw_video.update = video_update_8bpp;
    i_hw_video.setpal = video_setpal_8bpp;
    set_color_depth(8);
    video.bm = create_bitmap(w, h);
    if (i_hw_video.setmode(w, h)) {
        return -1;
    }
    return 0;
}

void hw_video_shutdown(void)
{
    if (video.bm) {
        destroy_bitmap(video.bm);
        video.bm = NULL;
    }
}

void hw_video_position_cursor(int mx, int my)
{
    position_mouse(mx, my);
}

#include "hw_video.c"
