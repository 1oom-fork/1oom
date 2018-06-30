#include "config.h"

#include "types.h"
#include <allegro.h>

#include "hw.h"
#include "hwalleg_mouse.h"
#include "mouse.h"

/* -------------------------------------------------------------------------- */

bool hw_mouse_enabled = true;

/* -------------------------------------------------------------------------- */

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
