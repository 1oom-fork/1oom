#include "config.h"

#include <string.h>

#include "uicursor.h"
#include "hw.h"
#include "lbxpal.h"
#include "types.h"
#include "uidefs.h"

/* -------------------------------------------------------------------------- */

#define CURSOR_W    16
#define CURSOR_H    16

static ui_cursor_area_t *ui_cursor_area_def_ptr = 0;
static int ui_cursor_area_def_num = 1;
static uint16_t ui_cursor_gfx_i_old = 0;

struct cursor_bg_s {
    int x, y;
    uint8_t data[CURSOR_W * CURSOR_H];
};

static struct cursor_bg_s cursor_bg_front;
static struct cursor_bg_s cursor_bg_back;

static bool cursor_i0_bg_stored = false;

/* -------------------------------------------------------------------------- */

ui_cursor_area_t ui_cursor_area_all_i0 = { 0, 0, VGABUF_RECT };
ui_cursor_area_t ui_cursor_area_all_i1 = { 1, 0, VGABUF_RECT };

ui_cursor_area_t ui_cursor_area_tbl[] = {
    /*0*/ { 1, 0, 0, 0, 319, 199 },
    /*1*/ { 1, 0, 0, 0, 319, 199 },
    /*2*/ { 8, 0, 3, 2, 218, 174 },
    /*3*/ { 1, 0, 0, 0, 319, 199 },
    /*4*/ { 7, 4, 3, 2, 218, 174 },
    /*5*/ { 1, 0, 0, 0, 319, 199 },
    /*6*/ { 7, 4, 3, 2, 218, 174 },
    /*7*/ { 5, 0, 0, 0, 0, 0 },
    /*8*/ { 9, 0, 0, 0, 319, 199 },
    /*9*/ { 10, 0, 0, 0, 319, 199 },
    /*a*/ { 11, 0, 0, 0, 319, 199 }
};

uint16_t ui_cursor_mouseoff = 0;
uint16_t ui_cursor_gfx_i = 0;

/* -------------------------------------------------------------------------- */

static void ui_cursor_store_bg(int mx, int my, uint8_t *p, struct cursor_bg_s *bg)
{
    int w, h;
    uint8_t *q = bg->data;
    bg->x = mx;
    bg->y = my;
    p += VGABUF_OFFSET(mx, my);
    w = CURSOR_W;
    if ((mx + w) > VGABUF_W) {
        w = VGABUF_W - mx;
    }
    h = CURSOR_H;
    if ((my + h) > VGABUF_H) {
        h = VGABUF_H - my;
    }
    for (int y = 0; y < h; ++y) {
        memcpy(q, p, w);
        p += VGABUF_PITCH;
        q += CURSOR_W;
    }
}

static void ui_cursor_draw(int mx, int my, uint8_t *p)
{
    if (ui_cursor_gfx_i == 0) {
        return;
    }
    int w, h;
    uint8_t *q = lbxpal_cursors + ((ui_cursor_gfx_i - 1) * CURSOR_W * CURSOR_H);
    p += VGABUF_OFFSET(mx, my);
    w = CURSOR_W;
    if ((mx + w) > VGABUF_W) {
        w = VGABUF_W - mx;
    }
    h = CURSOR_H;
    if ((my + h) > VGABUF_H) {
        h = VGABUF_H - my;
    }
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t b = q[x * CURSOR_W];
            if (b) {
                p[x] = b;
            }
        }
        p += VGABUF_PITCH;
        ++q;
    }
}

static void ui_cursor_erase(uint8_t *p, struct cursor_bg_s *bg)
{
    int w, h;
    int mx = bg->x;
    int my = bg->y;
    uint8_t *q = bg->data;
    p += VGABUF_OFFSET(mx, my);
    w = CURSOR_W;
    if ((mx + w) > VGABUF_W) {
        w = VGABUF_W - mx;
    }
    h = CURSOR_H;
    if ((my + h) > VGABUF_H) {
        h = VGABUF_H - my;
    }
    for (int y = 0; y < h; ++y) {
        memcpy(p, q, w);
        p += VGABUF_PITCH;
        q += CURSOR_W;
    }
}

/* -------------------------------------------------------------------------- */

void ui_cursor_setup_area(int num, ui_cursor_area_t *area)
{
    ui_cursor_area_def_num = num;
    ui_cursor_area_def_ptr = area;
    if (--num > 0) {
        area += num;
        while (num && (area->x0 || area->y0)) {
            --num;
            --area;
        }
    }
    ui_cursor_mouseoff = area->mouseoff;
    ui_cursor_gfx_i = area->cursor_i;
}

void ui_cursor_update_gfx_i(int mx, int my)
{
    int num = ui_cursor_area_def_num;
    ui_cursor_area_t *area = ui_cursor_area_def_ptr;

    ui_cursor_gfx_i_old = ui_cursor_gfx_i;

    if (!area) {
        return;
    }

    if (--num) {
        area += num;
        while ((num >= 0) && ((mx < area->x0 * vgabuf_scale) || (mx > area->x1 * vgabuf_scale) || (my < area->y0 * vgabuf_scale) || (my > area->y1 * vgabuf_scale))) {
            --num;
            --area;
        }
    }

    ui_cursor_mouseoff = area->mouseoff;
    ui_cursor_gfx_i = area->cursor_i;
}

void ui_cursor_store_bg_back(int mx, int my)
{
    if ((ui_cursor_gfx_i == 0) && (ui_cursor_gfx_i_old == 0)) {
        if (cursor_i0_bg_stored) {
            return;
        }
        cursor_i0_bg_stored = true;
    }
    ui_cursor_store_bg(mx, my, vgabuf_get_back(), &cursor_bg_back);
}

void ui_cursor_store_bg_front(int mx, int my)
{
    if (ui_cursor_gfx_i == 0) {
        if (cursor_i0_bg_stored) {
            return;
        }
        cursor_i0_bg_stored = true;
    }
    ui_cursor_store_bg(mx, my, vgabuf_get_front(), &cursor_bg_front);
}

void ui_cursor_draw_back(int mx, int my)
{
    if (ui_cursor_gfx_i != 0) {
        ui_cursor_draw(mx, my, vgabuf_get_back());
    }
}

void ui_cursor_draw_front(int mx, int my)
{
    if (ui_cursor_gfx_i != 0) {
        ui_cursor_draw(mx, my, vgabuf_get_front());
    }
}

void ui_cursor_erase_front(void)
{
    if (ui_cursor_gfx_i_old != 0) {
        ui_cursor_erase(vgabuf_get_front(), &cursor_bg_front);
    }
}

void ui_cursor_erase_back(void)
{
    if (ui_cursor_gfx_i != 0) {
        ui_cursor_erase(vgabuf_get_back(), &cursor_bg_back);
    }
}

void ui_cursor_copy_bg_back_to_bg_front(void)
{
    memcpy(&cursor_bg_front, &cursor_bg_back, sizeof(cursor_bg_front));
}

void ui_cursor_refresh(int mx, int my)
{
    if (ui_cursor_gfx_i == 0) {
        return;
    }
    ui_cursor_update_gfx_i(mx, my);
    ui_cursor_store_bg_front(mx, my);
    ui_cursor_draw_front(mx, my);
    hw_video_redraw_front();
    ui_cursor_erase_front();
}
