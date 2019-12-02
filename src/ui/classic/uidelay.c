#include "config.h"

#include <string.h>

#include "uidelay.h"
#include "hw.h"
#include "mouse.h"
#include "types.h"
#include "uicursor.h"
#include "rnd.h"

/* -------------------------------------------------------------------------- */

#define DELAY_EVENT_HANDLE_LIMIT    12500
#define DELAY_MOUSE_UPDATE_LIMIT    20000

static int64_t delay_start;

/* -------------------------------------------------------------------------- */

void ui_delay_prepare(void)
{
    delay_start = hw_get_time_us();
}

bool ui_delay_ticks_or_click(int ticks)
{
    return ui_delay_us_or_click(MOO_TICKS_TO_US(ticks));
}

bool ui_delay_us_or_click(uint32_t delay)
{
    bool pressed = false, handled = false;
    int mx = moouse_x, my = moouse_y;
    int64_t mouse_time = hw_get_time_us();
    while (1) {
        int64_t diff;
        int64_t now;
        now = hw_get_time_us();
        diff = now - delay_start;
        if ((diff < 0) || (diff >= (int64_t)delay)) {
            if (!handled) {
                hw_event_handle();
            }
            return false;
        }
        if (diff < DELAY_EVENT_HANDLE_LIMIT) {
            continue;
        }
        hw_event_handle();
        handled = true;
        if (!pressed) {
            if (mouse_buttons) {
                pressed = true;
            }
        } else {
            if (!mouse_buttons) {
                return true;
            }
        }
        if (mx != moouse_x || my != moouse_y) {
            rnd_entropy_pool ^= rnd_entropy_pool >> 23;
            rnd_entropy_pool += now;
            rnd_entropy_pool += moouse_x << 10;
            rnd_entropy_pool += moouse_y << 15;
            rnd_entropy_pool *= 1058670809;
            if(now - mouse_time > DELAY_MOUSE_UPDATE_LIMIT) {
                mouse_time = now;
                mx = moouse_x;
                my = moouse_y;
                ui_cursor_refresh(mx, my);
            }
        }
    }
}

void ui_delay_1(void)
{
    ui_delay_prepare();
    ui_delay_ticks_or_click(1);
}

void ui_delay_1e(void)
{
    ui_delay_prepare();
    ui_delay_ticks_or_click(0x1e);
}
