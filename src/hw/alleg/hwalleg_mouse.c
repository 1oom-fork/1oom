#include "config.h"

#include "types.h"
#include <allegro.h>

#include "hw.h"
#include "hwalleg_mouse.h"
#include "hwalleg_opt.h"
#include "hwalleg_video.h"
#include "mouse.h"

/* -------------------------------------------------------------------------- */

bool hw_mouse_enabled = true;

/* -------------------------------------------------------------------------- */

void hw_mouse_grab(void)
{
    if (!hw_mouse_enabled) {
        hw_mouse_enabled = true;
        hw_video_input_grab(true);
    }
}

void hw_mouse_ungrab(void)
{
    if (hw_mouse_enabled) {
        hw_mouse_enabled = false;
        hw_video_input_grab(false);
    }
}

void hw_mouse_toggle_grab(void)
{
    if (hw_mouse_enabled) {
        hw_mouse_ungrab();
    } else {
        hw_mouse_grab();
    }
}

void hw_mouse_move(int dx, int dy)
{
    mouse_set_xy_from_hw(moouse_x + dx, moouse_y + dy);
}

void hw_mouse_set_xy(int mx, int my)
{
    if (hw_opt_relmouse) {
        mouse_set_xy_from_hw(mx, my);
    } else {
        position_mouse(mx, my);
    }
}

void hw_mouse_buttons(int state)
{
    if (hw_mouse_enabled) {
        int b = mouse_buttons;
        if (state & 1) {
            b |= MOUSE_BUTTON_MASK_LEFT;
        } else {
            b &= ~MOUSE_BUTTON_MASK_LEFT;
        }
        if (state & 2) {
            b |= MOUSE_BUTTON_MASK_RIGHT;
        } else {
            b &= ~MOUSE_BUTTON_MASK_RIGHT;
        }
        mouse_set_buttons_from_hw(b);
    }
}

void hw_mouse_scroll(int scroll)
{
    mouse_set_scroll_from_hw(scroll);
}
