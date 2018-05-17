#include "config.h"

#include "comp.h"
#include "hw.h"
#include "mouse.h"
#include "options.h"

/* ------------------------------------------------------------------------- */

static bool mouse_have_click_hw = false;
static bool mouse_have_click_sw = false;

/* ------------------------------------------------------------------------- */

int moo_mouse_x = 0;
int moo_mouse_y = 0;
int moo_mouse_w;
int moo_mouse_h;
int mouse_buttons = 0;
int mouse_stored_x = 0;
int mouse_stored_y = 0;
int mouse_click_x = 0;
int mouse_click_y = 0;
int mouse_click_buttons = 0;

/* ------------------------------------------------------------------------- */

void mouse_set_limits(int w, int h)
{
    moo_mouse_w = w;
    moo_mouse_h = h;
}

void mouse_set_xy_from_hw(int mx, int my)
{
    SETRANGE(mx, 0, moo_mouse_w - 1);
    SETRANGE(my, 0, moo_mouse_h - 1);
    moo_mouse_x = mx;
    moo_mouse_y = my;
}

void mouse_set_buttons_from_hw(int buttons)
{
    buttons &= (MOUSE_BUTTON_MASK_LEFT | MOUSE_BUTTON_MASK_RIGHT);
    mouse_buttons = buttons;
    if (buttons) {
        mouse_click_buttons = buttons;
        mouse_click_x = moo_mouse_x;
        mouse_click_y = moo_mouse_y;
        mouse_have_click_hw = true;
        mouse_have_click_sw = true;
    }
}

void mouse_set_xy(int mx, int my)
{
    moo_mouse_x = mx;
    moo_mouse_y = my;
    if (opt_mouse_warp_enabled) {
        hw_video_position_cursor(mx, my);
    }
}

void mouse_set_click_xy(int mx, int my)
{
    mouse_have_click_sw = true;
    mouse_click_x = mx;
    mouse_click_y = my;
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
