#include "config.h"

#include "comp.h"
#include "mouse.h"

/* ------------------------------------------------------------------------- */

static bool mouse_hmm4 = false;
static bool mouse_hmm5 = false;

/* ------------------------------------------------------------------------- */

int mouse_x = 0;
int mouse_y = 0;
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
    mouse_x = mx;
    mouse_y = my;
}

void mouse_set_buttons_from_hw(int buttons)
{
    buttons &= (MOUSE_BUTTON_MASK_LEFT | MOUSE_BUTTON_MASK_RIGHT);
    mouse_buttons = buttons;
    if (buttons) {
        mouse_click_buttons = buttons;
        mouse_click_x = mouse_x;
        mouse_click_y = mouse_y;
        mouse_hmm4 = true;
        mouse_hmm5 = true;
    }
}

void mouse_set_xy(int mx, int my)
{
    mouse_x = mx;
    mouse_y = my;
}

void mouse_set_click_xy(int mx, int my)
{
    mouse_hmm5 = true;
    mouse_click_x = mx;
    mouse_click_y = my;
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
