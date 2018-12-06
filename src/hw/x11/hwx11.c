/*
   X11 user interface for 1oom, written by Tapani Utriainen.
   This file is Open Source and licensed under GPLv2.
*/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/time.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>

#include "hw_internal.h"

#include "hwx11_video.h"
#include "main.h"
#include "options.h"

/* -------------------------------------------------------------------------- */

const char *idstr_hw = "x11";

bool hw_use_mouse = true;

/* -------------------------------------------------------------------------- */

uint32_t hw_get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_usec + 1000000 * (int64_t)tv.tv_sec;
}

int hw_early_init(void)
{
    return 0;
}

int hw_init(void)
{
    return 0;
}

void hw_shutdown(void)
{
    hw_audio_shutdown();
    hw_video_shutdown();
}

int main(int argc, char **argv)
{
    return main_1oom(argc, argv);
}

int hw_event_handle(void)
{
    XEvent e;
    int key, button, mod;
    static unsigned char buttonstate = 0;
    char ascii[4], c;

    while (XPending(dpy) > 0) {
        XNextEvent(dpy, &e);
        switch (e.type) {
        case KeyPress:
            mod = 0;
            key = e.xkey.keycode;
            if (e.xkey.state & ControlMask) mod |= MOO_MOD_CTRL;
            if (e.xkey.state & ShiftMask) mod |= MOO_MOD_SHIFT;
            if (e.xkey.state & Mod1Mask) mod |= MOO_MOD_ALT;
            XLookupString(&e.xkey, ascii, 4, NULL, NULL);
            c = ascii[0];
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            kbd_add_keypress(c, mod, ascii[0]);
            break;
        case KeyRelease:
            break;
        case MotionNotify:
            mouse_set_xy_from_hw(e.xmotion.x, e.xmotion.y); /*FIXME scale*/
            break;
        case ButtonRelease:
            button = e.xbutton.button;
            if (button == Button1) buttonstate &= ~MOUSE_BUTTON_MASK_LEFT;
            if (button == Button3) buttonstate &= ~MOUSE_BUTTON_MASK_RIGHT;
            mouse_set_buttons_from_hw(buttonstate);
            break;
       case ButtonPress:
            switch(e.xbutton.button) {
            case Button1:
                mouse_set_buttons_from_hw(MOUSE_BUTTON_MASK_LEFT);
                break;
            case Button3:
                mouse_set_buttons_from_hw(MOUSE_BUTTON_MASK_RIGHT);
                break;
            default:
                break;
            }
            break;
        case Expose:
            i_hw_video.render();
            break;
        }
    }
    return 0;
}
