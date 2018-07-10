#include "config.h"

#include "comp.h"
#include "mouse.h"
#include "vgabuf.h"

#define MOUSE_SCREEN_W  VGABUF_W
#define MOUSE_SCREEN_H  VGABUF_H

/* ------------------------------------------------------------------------- */

static bool mouse_have_click_hw = false;
static bool mouse_have_click_sw = false;

static int moo_mouse_x = 0;
static int moo_mouse_y = 0;
static int moo_mouse_click_x = 0;
static int moo_mouse_click_y = 0;

/* ------------------------------------------------------------------------- */

int mouse_buttons = 0;
int mouse_stored_x = 0;
int mouse_stored_y = 0;
int mouse_click_buttons = 0;

/* ------------------------------------------------------------------------- */

static inline void mouse_clamp_xy(int *x, int *y)
{
    SETRANGE(*x, 0, MOUSE_SCREEN_W - 1);
    SETRANGE(*y, 0, MOUSE_SCREEN_H - 1);
}

/* ------------------------------------------------------------------------- */

bool mouse_onscreen_xy(int mx, int my)
{
    return (mx >= 0) && (mx < MOUSE_SCREEN_W) && (my >= 0) && (my < MOUSE_SCREEN_H);
}

bool mouse_offscreen_xy(int mx, int my)
{
    return (mx < 0) || (mx >= MOUSE_SCREEN_W) || (my < 0) || (my >= MOUSE_SCREEN_H);
}

void mouse_set_xy_from_hw(int mx, int my)
{
    mouse_clamp_xy(&mx, &my);
    moo_mouse_x = mx;
    moo_mouse_y = my;
}

void mouse_set_buttons_from_hw(int buttons)
{
    buttons &= (MOUSE_BUTTON_MASK_LEFT | MOUSE_BUTTON_MASK_RIGHT);
    mouse_buttons = buttons;
    if (buttons) {
        mouse_click_buttons = buttons;
        moo_mouse_click_x = moo_mouse_x;
        moo_mouse_click_y = moo_mouse_y;
        mouse_have_click_hw = true;
        mouse_have_click_sw = true;
    }
}

void mouse_set_xy(int mx, int my)
{
    moo_mouse_x = mx;
    moo_mouse_y = my;
}

void mouse_set_click_xy(int mx, int my)
{
    mouse_have_click_sw = true;
    moo_mouse_click_x = mx;
    moo_mouse_click_y = my;
}

int mouse_get_x(void)
{
    return moo_mouse_x;
}

int mouse_get_y(void)
{
    return moo_mouse_y;
}

int mouse_get_click_x(void)
{
    return moo_mouse_click_x;
}

int mouse_get_click_y(void)
{
    return moo_mouse_click_y;
}

bool mouse_getclear_click_hw(void)
{
    bool r = mouse_have_click_hw;
    mouse_have_click_hw = false;
    return r;
}

bool mouse_getclear_click_sw(void)
{
    bool r = mouse_have_click_sw;
    mouse_have_click_sw = false;
    return r;
}
