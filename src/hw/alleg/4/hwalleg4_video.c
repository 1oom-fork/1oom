#include "config.h"

#include <stdio.h>
#include <string.h>

#include "types.h"
#include <allegro.h>

#include "hw.h"
#include "hwalleg_video.h"
#include "hwalleg_mouse.h"
#include "hwalleg_opt.h"
#include "lib.h"
#include "log.h"
#include "palette.h"
#include "video_buf.h"

/* -------------------------------------------------------------------------- */

static struct alleg_video_s {
    BITMAP *bm;

    BITMAP *inter_bm;
    int target_x, target_y, target_w, target_h, target_scale;

    void (*render)(const uint8_t *buf);
    void (*render_target)(const uint8_t *buf, int src_x, int src_y, int src_pitch);
    void (*update)(void);
    void (*setpal)(const uint8_t *pal, int first, int num);

    RGB color[256];
} video = { 0 };

/* -------------------------------------------------------------------------- */

static void video_render_8bpp(const uint8_t *buf)
{
    BITMAP *bm = video.bm;
    int w = bm->w, h = bm->h;
    uint8_t *p;
    const uint8_t *q = buf;
    for (int y = 0; y < h; ++y) {
        p = bm->line[y];
        memcpy(p, q, w);
        q += w;
    }
}

static void video_set_render_target(int x, int y, int w, int h, int scale)
{
    video.target_x = x;
    video.target_y = y;
    video.target_w = w;
    video.target_h = h;
    video.target_scale = scale;
}

static void video_render_target_8bpp(const uint8_t *buf, int src_x, int src_y, int src_pitch)
{
    BITMAP *bm = video.inter_bm;
    uint8_t *p;
    const uint8_t *q = buf + src_x + src_y * src_pitch;
    for (int y = 0; y < video.target_h; ++y) {
        p = bm->line[y];
        memcpy(p, q, video.target_w);
        q += src_pitch;
    }
    stretch_blit(bm, video.bm, 0, 0, video.target_w, video.target_h
                 , video.target_x * video.target_scale, video.target_y * video.target_scale
                 , video.target_w * video.target_scale, video.target_h * video.target_scale);
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

int hw_video_init(int w, int h)
{
    w *= hw_opt_scale;
    h *= hw_opt_scale;
    video.render = video_render_8bpp;
    video.render_target = video_render_target_8bpp;
    video.update = video_update_8bpp;
    video.setpal = video_setpal_8bpp;
    set_color_depth(8);
    if (hw_opt_fullscreen || (set_gfx_mode(GFX_AUTODETECT_WINDOWED, w, h, 0, 0) != 0)) {
        if (set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0) != 0) {
            log_error("set_gfx_mode(..., %i, %i, 0, 0) failed!\n", w, h);
            return -1;
        }
    }
    hw_mouse_set_limits(w, h);
    hw_video_in_gfx = true;
    set_mouse_speed(hw_opt_mouse_slowdown_x, hw_opt_mouse_slowdown_y);
    video.bm = create_bitmap(w, h);
    video.inter_bm = create_bitmap(w, h);
    return 0;
}

void hw_video_shutdown(void)
{
#if 0
    /* FIXME doing this crashes the program */
    if (video.bm) {
        destroy_bitmap(video.bm);
        video.bm = NULL;
    }
    if (video.inter_bm) {
        destroy_bitmap(video.inter_bm);
        video.inter_bm = NULL;
    }
#endif
}

void hw_video_input_grab(bool grab)
{
}

#include "hwalleg_video.c"
