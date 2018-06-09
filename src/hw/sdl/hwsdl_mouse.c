#include "config.h"

#include "hw.h"
#include "hwsdl_mouse.h"
#include "hwsdl_video.h"
#include "mouse.h"
#include "types.h"

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

void hw_mouse_move(int dx, int dy)
{
    mouse_set_xy_from_hw(moo_mouse_x + dx, moo_mouse_y + dy);
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
