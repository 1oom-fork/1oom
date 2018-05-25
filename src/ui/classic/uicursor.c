#include "config.h"

#include <string.h>

#include "uicursor.h"
#include "comp.h"
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

static struct cursor_bg_s cursor_bg0;
static struct cursor_bg_s cursor_bg1;

static bool cursor_i0_bg_stored = false;

/* -------------------------------------------------------------------------- */

ui_cursor_area_t ui_cursor_area_all_i0 = { 0, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 };
ui_cursor_area_t ui_cursor_area_all_i1 = { 1, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 };

ui_cursor_area_t ui_cursor_area_tbl[] = {
    /*0*/ { 1, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 },
    /*1*/ { 1, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 },
    /*2*/ { 8, 0, 3, 2, 218, 174 },
    /*3*/ { 1, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 },
    /*4*/ { 7, 4, 3, 2, 218, 174 },
    /*5*/ { 1, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 },
    /*6*/ { 7, 4, 3, 2, 218, 174 },
    /*7*/ { 5, 0, 0, 0, 0, 0 },
    /*8*/ { 9, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 },
    /*9*/ { 10, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 },
    /*a*/ { 11, 0, 0, 0, UI_VGA_W - 1, UI_VGA_H - 1 }
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
    p += my * UI_SCREEN_W + mx;
    w = CURSOR_W;
    if ((mx + w) > UI_SCREEN_W) {
        w = UI_SCREEN_W - mx;
    }
    h = CURSOR_H;
    if ((my + h) > UI_SCREEN_H) {
        h = UI_SCREEN_H - my;
    }
    for (int y = 0; y < h; ++y) {
        memcpy(q, p, w);
        p += UI_SCREEN_W;
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
    p += my * UI_SCREEN_W + mx;
    w = CURSOR_W;
    if ((mx + w) > UI_SCREEN_W) {
        w = UI_SCREEN_W - mx;
    }
    h = CURSOR_H;
    if ((my + h) > UI_SCREEN_H) {
        h = UI_SCREEN_H - my;
    }
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t b = q[x * CURSOR_W];
            if (b) {
                p[x] = b;
            }
        }
        p += UI_SCREEN_W;
        ++q;
    }
}

static void ui_cursor_erase(uint8_t *p, struct cursor_bg_s *bg)
{
    int w, h;
    int mx = bg->x;
    int my = bg->y;
    uint8_t *q = bg->data;
    p += my * UI_SCREEN_W + mx;
    w = CURSOR_W;
    if ((mx + w) > UI_SCREEN_W) {
        w = UI_SCREEN_W - mx;
    }
    h = CURSOR_H;
    if ((my + h) > UI_SCREEN_H) {
        h = UI_SCREEN_H - my;
    }
    for (int y = 0; y < h; ++y) {
        memcpy(p, q, w);
        p += UI_SCREEN_W;
        q += CURSOR_W;
    }
}

static void ui_cursor_init_do(int scale, ui_cursor_area_t *area)
{
    area->x0 *= scale;
    area->y0 *= scale;
    if (area->x1 == (UI_VGA_W - 1)) {
        area->x1 = UI_SCREEN_W - 1;
    } else {
        area->x1 *= scale;
    }
    if (area->y1 == (UI_VGA_H - 1)) {
        area->y1 = UI_SCREEN_H - 1;
    } else {
        area->y1 *= scale;
    }
}

/* -------------------------------------------------------------------------- */

void ui_cursor_init(int scale)
{
    ui_cursor_init_do(scale, &ui_cursor_area_all_i0);
    ui_cursor_init_do(scale, &ui_cursor_area_all_i1);
    for (int i = 0; i < TBLLEN(ui_cursor_area_tbl); ++i) {
        ui_cursor_init_do(scale, &ui_cursor_area_tbl[i]);
    }
}

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
        while ((num >= 0) && ((mx < area->x0) || (mx > area->x1) || (my < area->y0) || (my > area->y1))) {
            --num;
            --area;
        }
    }

    ui_cursor_mouseoff = area->mouseoff;
    ui_cursor_gfx_i = area->cursor_i;
}

void ui_cursor_store_bg1(int mx, int my)
{
    if ((ui_cursor_gfx_i == 0) && (ui_cursor_gfx_i_old == 0)) {
        if (cursor_i0_bg_stored) {
            return;
        }
        cursor_i0_bg_stored = true;
    }
    ui_cursor_store_bg(mx, my, hw_video_get_buf(), &cursor_bg1);
}

void ui_cursor_store_bg0(int mx, int my)
{
    if (ui_cursor_gfx_i == 0) {
        if (cursor_i0_bg_stored) {
            return;
        }
        cursor_i0_bg_stored = true;
    }
    ui_cursor_store_bg(mx, my, hw_video_get_buf_front(), &cursor_bg0);
}

void ui_cursor_draw1(int mx, int my)
{
    if (ui_cursor_gfx_i != 0) {
        ui_cursor_draw(mx, my, hw_video_get_buf());
    }
}

void ui_cursor_draw0(int mx, int my)
{
    if (ui_cursor_gfx_i != 0) {
        ui_cursor_draw(mx, my, hw_video_get_buf_front());
    }
}

void ui_cursor_erase0(void)
{
    if (ui_cursor_gfx_i_old != 0) {
        ui_cursor_erase(hw_video_get_buf_front(), &cursor_bg0);
    }
}

void ui_cursor_erase1(void)
{
    if (ui_cursor_gfx_i != 0) {
        ui_cursor_erase(hw_video_get_buf(), &cursor_bg1);
    }
}

void ui_cursor_copy_bg1_to_bg0(void)
{
    memcpy(&cursor_bg0, &cursor_bg1, sizeof(cursor_bg0));
}

void ui_cursor_refresh(int mx, int my)
{
    if (ui_cursor_gfx_i == 0) {
        return;
    }
    ui_cursor_store_bg0(mx, my);
    ui_cursor_draw0(mx, my);
    hw_video_redraw_front();
    ui_cursor_erase0();
}
