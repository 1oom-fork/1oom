#include "config.h"

#include "mouse.h"

/* ------------------------------------------------------------------------- */

static bool mouse_hmm4 = false;
static bool mouse_hmm5 = false;

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

void mouse_set_xy_from_hw(int mx, int my)
{
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
        mouse_hmm4 = true;
        mouse_hmm5 = true;
    }
}

void mouse_set_xy(int mx, int my)
{
    moo_mouse_x = mx;
    moo_mouse_y = my;
}

void mouse_set_click_xy(int mx, int my)
{
    mouse_hmm5 = true;
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

bool mouse_getclear_hmm4(void)
{
    bool r = mouse_hmm4;
    mouse_hmm4 = false;
    return r;
}

bool mouse_getclear_hmm5(void)
{
    bool r = mouse_hmm5;
    mouse_hmm5 = false;
    return r;
}
