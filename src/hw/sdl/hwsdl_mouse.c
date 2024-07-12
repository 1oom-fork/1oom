#include "config.h"

#include "hw.h"
#include "comp.h"
#include "hwsdl_mouse.h"
#include "hwsdl_video.h"
#include "hwsdl_opt.h"
#include "mouse.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

static int hw_mouse_w;
static int hw_mouse_h;
static int hw_mouse_dx_acc = 0;
static int hw_mouse_dy_acc = 0;
static int hw_mouse_sx = 1;
static int hw_mouse_sy = 1;

/* -------------------------------------------------------------------------- */

bool hw_mouse_enabled = false;

/* -------------------------------------------------------------------------- */

void hw_mouse_grab(void)
{
    if (!hw_mouse_enabled) {
        hw_mouse_enabled = true;
        SDL_ShowCursor(SDL_DISABLE);
        hw_video_input_grab(true);
    }
}

void hw_mouse_ungrab(void)
{
    if (hw_mouse_enabled) {
        hw_mouse_enabled = false;
        SDL_ShowCursor(SDL_ENABLE);
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

void hw_mouse_set_limits(int w, int h)
{
    hw_mouse_w = w;
    hw_mouse_h = h;
}

void hw_mouse_set_scale(int w, int h)
{
    hw_mouse_sx = w / hw_mouse_w;
    if (hw_opt_mousediv > 1) {
        hw_mouse_sx /= hw_opt_mousediv;
    }
    SETMAX(hw_mouse_sx, 1);
    hw_mouse_sy = h / hw_mouse_h;
    if (hw_opt_mousediv > 1) {
        hw_mouse_sy /= hw_opt_mousediv;
    }
    SETMAX(hw_mouse_sy, 1);
}

void hw_mouse_move(int dx, int dy)
{
    int x, y;
    if (hw_mouse_sx > 1) {
        hw_mouse_dx_acc += dx;
        dx = hw_mouse_dx_acc / hw_mouse_sx;
        hw_mouse_dx_acc = hw_mouse_dx_acc % hw_mouse_sx;
    }
    if (hw_mouse_sy > 1) {
        hw_mouse_dy_acc += dy;
        dy = hw_mouse_dy_acc / hw_mouse_sy;
        hw_mouse_dy_acc = hw_mouse_dy_acc % hw_mouse_sy;
    }
    if ((dx == 0) && (dy == 0)) {
        return;
    }
    x = moouse_x + dx;
    if (x < 0) { x = 0; }
    if (x >= hw_mouse_w) { x = hw_mouse_w - 1; }
    y = moouse_y + dy;
    if (y < 0) { y = 0; }
    if (y >= hw_mouse_h) { y = hw_mouse_h - 1; }
    mouse_set_xy_from_hw(x, y);
}

void hw_mouse_button(int i, int pressed)
{
    if (hw_mouse_enabled) {
        int b = mouse_buttons;
        if (i == (int)SDL_BUTTON_LEFT) {
            if (pressed) {
                b |= MOUSE_BUTTON_MASK_LEFT;
            } else {
                b &= ~MOUSE_BUTTON_MASK_LEFT;
            }
        } else if (i == (int)SDL_BUTTON_RIGHT) {
            if (pressed) {
                b |= MOUSE_BUTTON_MASK_RIGHT;
            } else {
                b &= ~MOUSE_BUTTON_MASK_RIGHT;
            }
        }
        mouse_set_buttons_from_hw(b);
    }

    if (pressed) {
        if (hw_mouse_enabled) {
            if (i == (int)SDL_BUTTON_MIDDLE) {
                hw_mouse_ungrab();
            }
        } else {
            hw_mouse_grab();
        }
    }
}

void hw_mouse_scroll(int scroll)
{
    mouse_set_scroll_from_hw(scroll);
}
