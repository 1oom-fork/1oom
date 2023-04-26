/*
   X11 user interface for 1oom, written by Tapani Utriainen.
   This file is Open Source and licensed under GPLv2.
*/
#include "config.h"
#include "cfg.h"
#include "hw.h"
#include "kbd.h"
#include "main.h"
#include "mouse.h"
#include "palette.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XShm.h>

/* -- X11 variables --------------------------------------------------------- */

static Display *dpy = NULL;            /* X server connection */
static GC gc;
static Visual *visual;
static Window mainwin;
static int sizex, sizey;               /* Size of window */
static XImage *shared_image = NULL;
static XShmSegmentInfo shminfo;
static bool hw_key_press_repeat_active = true;

/* -- Variables for 1OOM graphics ------------------------------------------- */

#define MOO_WIDTH 320
#define MOO_HEIGHT 200
/* Buffers 0-3 are for drawing; 4 is a copy of last frame, 5 updated pixels */
static uint8_t moobuf[6][MOO_HEIGHT][MOO_WIDTH] __attribute__((aligned(16)));
static uint32_t palette_scaled[256] = { 0 };    /* 8-bit ARGB */
static int bufi = 0, scale = 4;                 /* TODO: set by cmd line arg */
static int minpal = 767, maxpal = 0;            /* Palette change boundaries */
static uint32_t *screenbuf = NULL;              /* Pointer to graphics memory*/
static long *icondata = NULL;                   /* Icon, if set */

/* -- Variables required by HW API ------------------------------------------ */

const char *idstr_hw = "x11";
const struct cmdline_options_s hw_cmdline_options[] = { { NULL, 0, NULL, NULL, NULL, NULL } };
const struct cmdline_options_s hw_cmdline_options_extra[] = { { NULL, 0, NULL, NULL, NULL, NULL } };
const struct uiopt_s hw_uiopts[] = { UIOPT_ITEM_END };
const struct uiopt_s hw_uiopts_extra[] = { UIOPT_ITEM_END };
const struct cfg_items_s hw_cfg_items[] = { CFG_ITEM_END };
const struct cfg_items_s hw_cfg_items_extra[] = { CFG_ITEM_END };

/* -- HW API Audio functions, dummies since X11 does not do audio ----------- */

void hw_audio_shutdown(void) { }
int  hw_audio_init(void) { return 0; }
int  hw_audio_music_init(int ix, const uint8_t *data, uint32_t len) { return 0; }
void hw_audio_music_fadeout(void) { }
void hw_audio_music_play(int ix) { }
void hw_audio_music_release(int ix) { }
void hw_audio_music_stop(void) { }
bool hw_audio_music_volume(int volume) { return true; }
int  hw_audio_sfx_batch_end(void) { return 0; }
int  hw_audio_sfx_batch_start(int sfx_index_max) { return 0; }
int  hw_audio_sfx_init(int ix, const uint8_t *data, uint32_t len) { return 0; }
void hw_audio_sfx_play(int ix) { }
void hw_audio_sfx_release(int ix) { }
void hw_audio_sfx_stop(void) { }
bool hw_audio_sfx_volume(int volume) { return true; }

/* -- Misc functions mandated by HW API ------------------------------------- */

void hw_log_message(const char *msg) { fputs(msg, stdout); }
void hw_log_warning(const char *msg) { fputs(msg, stderr); }
void hw_log_error(const char *msg) { fputs(msg, stderr); }
void hw_textinput_start(void) { }
void hw_textinput_stop(void) { }
int  hw_early_init(void) { return 0; }

int64_t hw_get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_usec + 1000000 * (int64_t)tv.tv_sec;
}

/* -- Real code starts here ------------------------------------------------- */

static void hide_cursor(void) {	/* Hide the X11 mouse cursor when in 1oom win */
    Cursor no_ptr;
    XColor black, dummy;
    static char no_data[8] = { 0 };
    unsigned long colormap = DefaultColormap(dpy, DefaultScreen(dpy));
    unsigned long bm_no = XCreateBitmapFromData(dpy, mainwin, no_data, 8, 8);

    XAllocNamedColor(dpy, colormap, "black", &black, &dummy);
    no_ptr = XCreatePixmapCursor(dpy, bm_no, bm_no, &black, &black, 0, 0);
    XDefineCursor(dpy, mainwin, no_ptr);
    XFreeCursor(dpy, no_ptr);
}

/* Invalidate all pixels of certain colours */
static void invalidate_palrange(uint8_t min, uint8_t max) {
    for (int y = 0; y < MOO_HEIGHT; y++) {
        for (int x = 0; x < MOO_WIDTH; x++) {
            if (moobuf[4][y][x] >= min && moobuf[4][y][x] <= max)
                moobuf[5][y][x] = 2;
        }
    }
}

/* Convert 6-bit RGB palette to 8-bit */
static void convert_palette(const unsigned char *const palette) {
    for (int i = 0; i < 256; i++) {
        unsigned char b = (palette[i*3 + 0] << 2) + (palette[i*3 + 0] >> 4);
        unsigned char g = (palette[i*3 + 1] << 2) + (palette[i*3 + 1] >> 4);
        unsigned char r = (palette[i*3 + 2] << 2) + (palette[i*3 + 2] >> 4);
        palette_scaled[i] = r + (g << 8) + (b << 16);
    }
}

/* Main paint method, where the graphics actually happen */
static void hw_video_paint(int toshow) {
    int xcur, ycur;

    if (maxpal >= minpal) invalidate_palrange(minpal/3, maxpal/3);
    convert_palette(ui_palette);
    minpal = 768; maxpal = 0;

    /* Buf 5 tracks which pixels need update, buf 4 is a copy of last frame */
    if (toshow != 4) { /* Optimization: the loops do nothing when visual==4*/
        for (int y=0; y<MOO_HEIGHT; y++) {
            for (int x=0; x<MOO_WIDTH; x++) {
                /* Set buf[5] nonzero when buf[4] != buf[toshow] */
                moobuf[5][y][x] |= moobuf[4][y][x] ^ moobuf[toshow][y][x];
            }
        }
    }
    /* Could be taken out when scale is hard coded */
    ycur = sizey/scale < MOO_HEIGHT ? sizey/scale : MOO_HEIGHT;
    xcur = sizex/scale < MOO_WIDTH ? sizex/scale : MOO_WIDTH;
    for (int y=0; y<ycur; y++) {
        for (int x=0; x<xcur; x++) {
            if (moobuf[5][y][x]) {
                unsigned char c = moobuf[toshow][y][x];
                for (int j=0; j<scale; j++) {
                    for (int i=0; i<scale; i++) {
                        screenbuf[(scale*y + j)*sizex + scale*x + i] = palette_scaled[c];
                    }
                }
                moobuf[5][y][x] = 0;
                moobuf[4][y][x] = c;
            }
        }
    }
    XShmPutImage(dpy, mainwin, gc, shared_image, 0, 0, 0, 0, sizex, sizey, false);
    XFlush(dpy);
}

/* -- HW graphics API functions -------------------------------------------- */

void hw_video_set_palette(const uint8_t *pal, const int first, const int num) {
    ui_palette_set(pal, first, num);
    if (first*3 < minpal) minpal = first*3;
    if ((first+num)*3 > maxpal) maxpal = (first+num)*3;
}

void hw_video_set_palette_color(int i, uint8_t r, uint8_t g, uint8_t b)
{
    ui_palette_set_color(i, r, g, b);
    if ((i * 3) < minpal) minpal = i * 3;
    if ((i * 3 + 2) > maxpal) maxpal = i * 3 + 2;
}

void hw_video_refresh_palette(void) { hw_video_paint(4); }
void hw_video_redraw_front(void) { hw_video_paint(bufi ^ 1); }

uint8_t *hw_video_draw_buf(void) {
    hw_video_paint(bufi);
    return &moobuf[bufi ^= 1][0][0];
}

/* Functions for copying buffers back and forth */
void copybuf(int dst, int src) {
    memcpy(&moobuf[dst][0][0], &moobuf[src][0][0], sizeof(moobuf[0]));
}

void hw_video_copy_buf(void)             { copybuf(bufi, bufi ^ 1); }
void hw_video_copy_back_to_page2(void)   { copybuf(2, bufi); }
void hw_video_copy_back_from_page2(void) { copybuf(bufi, 2); }
void hw_video_copy_back_to_page3(void)   { copybuf(3, bufi); }
void hw_video_copy_back_from_page3(void) { copybuf(bufi, 3); }

void hw_video_copy_buf_out(uint8_t *buf) {
    memcpy(buf, &moobuf[bufi][0][0], sizeof(moobuf[0]));
}

uint8_t *hw_video_get_buf(void) { return &moobuf[bufi][0][0]; }
uint8_t *hw_video_get_buf_front(void) { return &moobuf[bufi^1][0][0]; }

/* ABI for setting an icon */
int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h) {
    if (icondata != NULL) free(icondata);
    icondata = (long *)malloc((2 + w*h) * sizeof(long));
    icondata[0] = w;
    icondata[1] = h;
    hw_video_set_palette(pal, 0, 256);
    convert_palette(pal);
    for (int i=0; i<w*h; i++) icondata[i+2] = palette_scaled[data[i]] | 0xff000000;
    return 0;
}

bool hw_kbd_set_repeat(bool enabled)
{
    hw_key_press_repeat_active = enabled;
    return true;
}

void hw_mouse_set_xy(int mx, int my)
{
    mouse_set_xy_from_hw(mx, my);
}

/* -- X11 specific implementations ---------------------------------------- */

void hw_shutdown(void) {
    if (icondata != NULL) {
        free(icondata);
        icondata = NULL;
    }
    if (gc != NULL) {
        XShmDetach(dpy, &shminfo);
        XDestroyImage(shared_image);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
        XDestroyWindow(dpy, mainwin);
        XFreeGC(dpy, gc);
    }
}

/* Obtain a pointer to window ARGB canvas */
static void create_screen_buffer(void) {
    shared_image = XShmCreateImage(dpy, visual, 24, ZPixmap, NULL, &shminfo, sizex, sizey);
    shminfo.shmid = shmget(IPC_PRIVATE, shared_image->bytes_per_line * shared_image->height, IPC_CREAT|0777);
    shminfo.shmaddr = shared_image->data = shmat(shminfo.shmid, 0, 0);
    shminfo.readOnly = false;
    XShmAttach(dpy, &shminfo);
    screenbuf = (uint32_t *)shared_image->data;
}

/* Event pump */
int hw_event_handle(void) {
    XEvent e;
    KeySym ks;
    int button, modifiers;
    static unsigned char buttonstate = 0;
    char ascii[4], c;
    int updminx = MOO_WIDTH, updmaxx = 0, updminy = MOO_HEIGHT, updmaxy = 0;

    while (XPending(dpy) > 0) {
        XNextEvent(dpy, &e);
        switch (e.type) {
        case KeyPress:
        case KeyRelease:
            if (!hw_key_press_repeat_active && e.type == KeyRelease && XEventsQueued(dpy, QueuedAfterReading)) {
              XEvent new;
              XPeekEvent(dpy, &new);

              if (new.type == KeyPress && new.xkey.time == e.xkey.time &&
                  new.xkey.keycode == e.xkey.keycode)
              {
                XNextEvent(dpy, &new);
                break;
              }
            }
            modifiers = 0;
            ks = XkbKeycodeToKeysym(dpy, e.xkey.keycode, 0, 0);
            if ((e.type == KeyPress) && ((e.xkey.state & ControlMask) || (ks == XK_Control_L) || (ks == XK_Control_R))) modifiers = MOO_MOD_CTRL;
            if ((e.type == KeyPress) && ((e.xkey.state & Mod1Mask) || (ks == XK_Alt_L) || (ks == XK_Alt_R))) modifiers = MOO_MOD_ALT;
            if ((e.type == KeyPress) && ((e.xkey.state & ShiftMask) || (ks == XK_Shift_L) || (ks == XK_Shift_R))) modifiers = MOO_MOD_SHIFT;
            XLookupString(&e.xkey, ascii, 4, NULL, NULL);
            c = ascii[0];
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if (e.type == KeyPress) {
                if (c > 0) kbd_add_keypress(c, modifiers, ascii[0]);
                kbd_set_pressed(c, modifiers, true);
                if (c == 27 && (modifiers & MOO_MOD_CTRL)) exit(0);
            } else
                kbd_set_pressed(c, modifiers, false);
            break;
        case MotionNotify:
            mouse_set_xy_from_hw(e.xmotion.x / scale, e.xmotion.y / scale);
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
            case Button4:
                mouse_set_scroll_from_hw(-1);
                break;
            case Button5:
                mouse_set_scroll_from_hw(1);
            default:
                break;
			}
            break;
        case Expose:
            if (updminx > e.xexpose.x/scale) updminx = e.xexpose.x/scale;
            if (updminy > e.xexpose.y/scale) updminy = e.xexpose.y/scale;
            if (updmaxx < (e.xexpose.x + e.xexpose.width + scale-1)/scale) updmaxx = (e.xexpose.x + e.xexpose.width + scale-1)/scale;
            if (updmaxy < (e.xexpose.y + e.xexpose.height + scale-1)/scale) updmaxy = (e.xexpose.y + e.xexpose.height + scale-1)/scale;
            if (e.xexpose.count == 0) { /* No more expose events queued, invalidate rectangle */
                for (int y = updminy; y < updmaxy; y++) {
                    for (int x = updminx; x < updmaxx; x++) {
                        moobuf[5][y][x] = 1;
                    }
                }
                hw_video_paint(4); /* Paint exposed regions with last frame */
            }
            break;
        }
    }
    return 0;
}

/* Create X11 main window */
int create_window(int width, int height) {
    XSetWindowAttributes attribs = { 0 };
    XSizeHints sizehint;
    sizex = width;
    sizey = height;

    visual = DefaultVisual(dpy, DefaultScreen(dpy));
    attribs.backing_store = NotUseful;
    attribs.override_redirect = True;
    mainwin = XCreateWindow(dpy, DefaultRootWindow(dpy), -1, -1, sizex, sizey,
        1, CopyFromParent, InputOutput, visual, 0, &attribs);
    gc = XCreateGC(dpy, mainwin, 0, NULL);
    XSelectInput(dpy, mainwin, ButtonPressMask | ButtonReleaseMask | ExposureMask |
        KeyPressMask | KeyReleaseMask | ButtonMotionMask | PointerMotionMask );
    if (icondata != NULL) {
        XChangeProperty(dpy, mainwin, XInternAtom(dpy, "_NET_WM_ICON", 0), XA_CARDINAL,
            32, PropModeReplace, (unsigned char *)icondata, icondata[0] * icondata[1] + 2);
    }
    sizehint.min_width = sizehint.max_width = sizex;
    sizehint.min_height = sizehint.max_height = sizey;
    sizehint.flags = PMinSize | PMaxSize;
    XSetWMNormalHints(dpy, mainwin, &sizehint);
    XSetWindowBorderWidth(dpy, mainwin, 0);
    XMapWindow(dpy, mainwin);
    XStoreName(dpy, mainwin, "1oom");
    XFlush(dpy);
    create_screen_buffer();
    hide_cursor();
    return 0;
}

int hw_video_init(int w, int h) {
    create_window(scale*MOO_WIDTH, scale*MOO_HEIGHT);
    return 0;
}

int hw_init(void) {
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL || !XShmQueryExtension(dpy) || DefaultDepth(dpy, DefaultScreen(dpy)) != 24) {
        hw_log_error("Cannot connect to XServer, or it does not supprt XSHM, or is not 24/32 bit colour\n");
        return 11;
    }
    return 0;
}

int main(int argc, char **argv) {
    return main_1oom(argc, argv);
}
