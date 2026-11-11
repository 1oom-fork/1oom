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

/* -------------------------------------------------------------------------- */

static mookey_t xlat_keysym(KeySym ks)
{
    switch (ks) {
    case XK_BackSpace:      return MOO_KEY_BACKSPACE;
    case XK_Tab:            return MOO_KEY_TAB;
    case XK_Clear:          return MOO_KEY_CLEAR;
    case XK_Return:         return MOO_KEY_RETURN;
    case XK_Pause:          return MOO_KEY_PAUSE;
    case XK_Escape:         return MOO_KEY_ESCAPE;
    case XK_space:          return MOO_KEY_SPACE;
    case XK_exclam:         return MOO_KEY_EXCLAIM;
    case XK_quotedbl:       return MOO_KEY_QUOTEDBL;
    case XK_numbersign:     return MOO_KEY_HASH;
    case XK_dollar:         return MOO_KEY_DOLLAR;
    case XK_ampersand:      return MOO_KEY_AMPERSAND;
    case XK_apostrophe:     return MOO_KEY_QUOTE;
    case XK_parenleft:      return MOO_KEY_LEFTPAREN;
    case XK_parenright:     return MOO_KEY_RIGHTPAREN;
    case XK_asterisk:       return MOO_KEY_ASTERISK;
    case XK_plus:           return MOO_KEY_PLUS;
    case XK_comma:          return MOO_KEY_COMMA;
    case XK_minus:          return MOO_KEY_MINUS;
    case XK_period:         return MOO_KEY_PERIOD;
    case XK_slash:          return MOO_KEY_SLASH;
    case XK_0:              return MOO_KEY_0;
    case XK_1:              return MOO_KEY_1;
    case XK_2:              return MOO_KEY_2;
    case XK_3:              return MOO_KEY_3;
    case XK_4:              return MOO_KEY_4;
    case XK_5:              return MOO_KEY_5;
    case XK_6:              return MOO_KEY_6;
    case XK_7:              return MOO_KEY_7;
    case XK_8:              return MOO_KEY_8;
    case XK_9:              return MOO_KEY_9;
    case XK_colon:          return MOO_KEY_COLON;
    case XK_semicolon:      return MOO_KEY_SEMICOLON;
    case XK_less:           return MOO_KEY_LESS;
    case XK_equal:          return MOO_KEY_EQUALS;
    case XK_greater:        return MOO_KEY_GREATER;
    case XK_question:       return MOO_KEY_QUESTION;
    case XK_at:             return MOO_KEY_AT;
    case XK_bracketleft:    return MOO_KEY_LEFTBRACKET;
    case XK_backslash:      return MOO_KEY_BACKSLASH;
    case XK_bracketright:   return MOO_KEY_RIGHTBRACKET;
    case XK_asciicircum:    return MOO_KEY_CARET;
    case XK_underscore:     return MOO_KEY_UNDERSCORE;
    case XK_grave:          return MOO_KEY_BACKQUOTE;
    case XK_a: case XK_A:   return MOO_KEY_a;
    case XK_b: case XK_B:   return MOO_KEY_b;
    case XK_c: case XK_C:   return MOO_KEY_c;
    case XK_d: case XK_D:   return MOO_KEY_d;
    case XK_e: case XK_E:   return MOO_KEY_e;
    case XK_f: case XK_F:   return MOO_KEY_f;
    case XK_g: case XK_G:   return MOO_KEY_g;
    case XK_h: case XK_H:   return MOO_KEY_h;
    case XK_i: case XK_I:   return MOO_KEY_i;
    case XK_j: case XK_J:   return MOO_KEY_j;
    case XK_k: case XK_K:   return MOO_KEY_k;
    case XK_l: case XK_L:   return MOO_KEY_l;
    case XK_m: case XK_M:   return MOO_KEY_m;
    case XK_n: case XK_N:   return MOO_KEY_n;
    case XK_o: case XK_O:   return MOO_KEY_o;
    case XK_p: case XK_P:   return MOO_KEY_p;
    case XK_q: case XK_Q:   return MOO_KEY_q;
    case XK_r: case XK_R:   return MOO_KEY_r;
    case XK_s: case XK_S:   return MOO_KEY_s;
    case XK_t: case XK_T:   return MOO_KEY_t;
    case XK_u: case XK_U:   return MOO_KEY_u;
    case XK_v: case XK_V:   return MOO_KEY_v;
    case XK_w: case XK_W:   return MOO_KEY_w;
    case XK_x: case XK_X:   return MOO_KEY_x;
    case XK_y: case XK_Y:   return MOO_KEY_y;
    case XK_z: case XK_Z:   return MOO_KEY_z;
    case XK_Delete:         return MOO_KEY_DELETE;
    case XK_KP_0:           return MOO_KEY_KP0;
    case XK_KP_1:           return MOO_KEY_KP1;
    case XK_KP_2:           return MOO_KEY_KP2;
    case XK_KP_3:           return MOO_KEY_KP3;
    case XK_KP_4:           return MOO_KEY_KP4;
    case XK_KP_5:           return MOO_KEY_KP5;
    case XK_KP_6:           return MOO_KEY_KP6;
    case XK_KP_7:           return MOO_KEY_KP7;
    case XK_KP_8:           return MOO_KEY_KP8;
    case XK_KP_9:           return MOO_KEY_KP9;
    case XK_KP_Decimal:     return MOO_KEY_KP_PERIOD;
    case XK_KP_Divide:      return MOO_KEY_KP_DIVIDE;
    case XK_KP_Multiply:    return MOO_KEY_KP_MULTIPLY;
    case XK_KP_Subtract:    return MOO_KEY_KP_MINUS;
    case XK_KP_Add:         return MOO_KEY_KP_PLUS;
    case XK_KP_Enter:       return MOO_KEY_RETURN;
    case XK_KP_Equal:       return MOO_KEY_KP_EQUALS;
    case XK_Up:             return MOO_KEY_UP;
    case XK_Down:           return MOO_KEY_DOWN;
    case XK_Right:          return MOO_KEY_RIGHT;
    case XK_Left:           return MOO_KEY_LEFT;
    case XK_Insert:         return MOO_KEY_INSERT;
    case XK_Home:           return MOO_KEY_HOME;
    case XK_End:            return MOO_KEY_END;
    case XK_Page_Up:        return MOO_KEY_PAGEUP;
    case XK_Page_Down:      return MOO_KEY_PAGEDOWN;
    case XK_F1:             return MOO_KEY_F1;
    case XK_F2:             return MOO_KEY_F2;
    case XK_F3:             return MOO_KEY_F3;
    case XK_F4:             return MOO_KEY_F4;
    case XK_F5:             return MOO_KEY_F5;
    case XK_F6:             return MOO_KEY_F6;
    case XK_F7:             return MOO_KEY_F7;
    case XK_F8:             return MOO_KEY_F8;
    case XK_F9:             return MOO_KEY_F9;
    case XK_F10:            return MOO_KEY_F10;
    case XK_F11:            return MOO_KEY_F11;
    case XK_F12:            return MOO_KEY_F12;
    case XK_Num_Lock:       return MOO_KEY_NUMLOCK;
    case XK_Caps_Lock:      return MOO_KEY_CAPSLOCK;
    case XK_Scroll_Lock:    return MOO_KEY_SCROLLOCK;
    case XK_Shift_R:        return MOO_KEY_RSHIFT;
    case XK_Shift_L:        return MOO_KEY_LSHIFT;
    case XK_Control_R:      return MOO_KEY_RCTRL;
    case XK_Control_L:      return MOO_KEY_LCTRL;
    case XK_Alt_R:          return MOO_KEY_RALT;
    case XK_Alt_L:          return MOO_KEY_LALT;
    case XK_Meta_R:         return MOO_KEY_RMETA;
    case XK_Meta_L:         return MOO_KEY_LMETA;
    case XK_Super_L:        return MOO_KEY_LSUPER;
    case XK_Super_R:        return MOO_KEY_RSUPER;
    case XK_Mode_switch:    return MOO_KEY_MODE;
    case XK_Menu:           return MOO_KEY_MENU;
    case XK_Help:           return MOO_KEY_HELP;
    case XK_Print:          return MOO_KEY_PRINT;
    case XK_Sys_Req:        return MOO_KEY_SYSREQ;
    case XK_Undo:           return MOO_KEY_UNDO;
    default:
        return MOO_KEY_UNKNOWN;
    }
}

/* -------------------------------------------------------------------------- */

int hw_event_handle(void)
{
    XEvent e;
    int button;
    static unsigned char buttonstate = 0;

    while (XPending(dpy) > 0) {
        XNextEvent(dpy, &e);
        switch (e.type) {
        case KeyPress:
            {
                uint32_t mod = 0;
                if (e.xkey.state & ShiftMask)   mod |= MOO_MOD_SHIFT;
                if (e.xkey.state & Mod1Mask)    mod |= MOO_MOD_ALT;
                if (e.xkey.state & ControlMask) mod |= MOO_MOD_CTRL;
                if (e.xkey.state & Mod4Mask)    mod |= MOO_MOD_META;
                {
                    char ascii[8];
                    KeySym keysym = NoSymbol;
                    int n = XLookupString(&e.xkey, ascii, sizeof(ascii), &keysym, NULL);
                    char c = 0;
                    if ((n == 1) && (ascii[0] != 0) && ((ascii[0] & 0x80) == 0)) {
                        c = ascii[0];
                    }
                    mookey_t key = xlat_keysym(keysym);
                    if (!kbd_handle_hotkey(key, mod)) {
                        kbd_add_keypress(key, mod, c);
                    }
                }
            }
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
