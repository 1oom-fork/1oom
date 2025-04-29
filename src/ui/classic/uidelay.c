#include "config.h"

#include <string.h>

#include "uidelay.h"
#include "hw.h"
#include "mouse.h"
#include "os.h"
#include "types.h"
#include "uicursor.h"

/* -------------------------------------------------------------------------- */

#define DELAY_EVENT_HANDLE_LIMIT    12500
#define DELAY_MOUSE_UPDATE_LIMIT    20000

static uint32_t delay_start;

static uint16_t delay_hmm1 = 1;

/* -------------------------------------------------------------------------- */

void ui_delay_prepare(void)
{
    delay_start = os_get_time_us();
}

bool ui_delay_ticks_or_click(int ticks)
{
    return ui_delay_us_or_click(MOO_TICKS_TO_US(ticks));
}

bool ui_delay_us_or_click(uint32_t delay)
{
    bool pressed = false;
    int mx = mouse_x, my = mouse_y;
    uint32_t mouse_time = os_get_time_us();
    hw_event_handle();
    if (delay_hmm1 == 0) {
        return false;
    }
    while (1) {
        uint32_t now, diff;
        now = os_get_time_us();
        if (now < delay_start) {
            return false;
        }
        diff = now - delay_start;
        if (diff >= delay) {
            return false;
        }
        if (diff < DELAY_EVENT_HANDLE_LIMIT) {
            continue;
        }
        hw_event_handle();
        if (!pressed) {
            if (mouse_buttons) {
                pressed = true;
            }
        } else {
            if (!mouse_buttons) {
                return true;
            }
        }
        if (((mx != mouse_x) || (my != mouse_y)) && ((now - mouse_time) > DELAY_MOUSE_UPDATE_LIMIT)) {
            mouse_time = now;
            mx = mouse_x;
            my = mouse_y;
            ui_cursor_refresh(mx, my);
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
