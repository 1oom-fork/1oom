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
#define SCALE 5
/* Buffers 0-3 are for drawing; 4 is a copy of last frame, 5 updated pixels */
static uint8_t moobuf[6][MOO_HEIGHT][MOO_WIDTH] __attribute__((aligned(16)));
static uint32_t palette_scaled[256] = { 0 };    /* 8-bit ARGB */
static unsigned char palette_raw[3*256];        /* With 6-bit RGB components */
static uint64_t palette_b0g0r[256];
static int bufi = 0;
static int minpal = 767, maxpal = 0;            /* Palette change boundaries */
static uint32_t *screenbuf = NULL;              /* Pointer to graphics memory*/
static long *icondata = NULL;                   /* Icon, if set */
const uint64_t interp_patterns[4] = {           /* When to interpolate */
    0xffffdffffef70a03ULL,
    0xffebf3f0f080c084ULL,
    0x8787c10000000000ULL,
    0x0f03062731130301ULL };

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
    return (int64_t)tv.tv_usec + 1000000LL * (int64_t)tv.tv_sec;
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

/* A B0G0R is a 64-bit RGB value in BGR order with 16-bit RGB components 
   and 4 dont care (zero) bits inbetween */
#define B0G0R_MASK    0x0ffff0ffff0ffff0ULL
#define B0G0R_REDPREC 0x0ffe00ffe00ffe00ULL
#define B0G0R_INT     0x0ff000ff000ff000ULL
#define B0G0R_SIGN    0x0800008000080000ULL
#define B0G0R_ONE     0x0000100001000010ULL
#define B0G0R_MAX     0x1000010000100000ULL
#define B0G0R_MAGIC   0x0100000010000001ULL

static inline __attribute__((const)) uint64_t rgb_to_b0g0r(uint32_t rgb) {
    register uint64_t b0g0r = (rgb * B0G0R_MAGIC >> 4u) & B0G0R_INT;
    return b0g0r;
}

static inline __attribute__((const)) uint32_t b0g0r_to_rgb(uint64_t b0g0r) {
    register uint64_t temp = b0g0r & B0G0R_INT;
    register uint32_t rgb = (temp << 4u) + (temp >> 24u) + (temp >> 52u);
    return rgb;
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
static void convert_palette(const unsigned char *const restrict palette, int min, int max) {
    for (int i = min; i <= max; i++) {
        unsigned char b = (palette[i*3 + 0] << 2) + (palette[i*3 + 0] >> 4);
        unsigned char g = (palette[i*3 + 1] << 2) + (palette[i*3 + 1] >> 4);
        unsigned char r = (palette[i*3 + 2] << 2) + (palette[i*3 + 2] >> 4);
        palette_scaled[i] = r + (g << 8) + (b << 16);
        palette_b0g0r[i] = rgb_to_b0g0r(palette_scaled[i]);
    }
}

/* Returns interpolation delta b0g0r (vector) */
static inline __attribute__((const)) uint64_t linear_interp5(uint64_t start, uint64_t stop) {
    start &= B0G0R_MASK;
    stop &= B0G0R_MASK;

    /* Calculate difference vector d */
    uint64_t d = B0G0R_MAX + stop - start;

    /* Flip all components of d to positive (absolute differences) */
    uint64_t signmask = (d & B0G0R_MAX) ^ B0G0R_MAX;
    signmask = (signmask >> 12u) * 0xffe;
    d &= B0G0R_REDPREC;
    d ^= signmask;

    unsigned char reg = (d >> 38) & 3;
    unsigned char u = ((d >> 17) & 7) | ((d >> 54) & 0x38);
    /* Check which algorithm to use: nearest neighbour or linear interpolation */
    if (d & 0x0e0000c0000e0000 && (!((interp_patterns[reg] >> u) & 1))) {
        d = 0ULL;
    } else {
        /* Divide all bogor components with 5 - using reciprocal multiplication of absolute differences */
        d = ((d >> 8u) * 0x332 >> 4u) & 0x03ff003ff003ff00;
        /* Flip signs back of those that were negative */
        d ^= signmask;
        /* There probably is no need to use these tricks ... but it was fun to code :-) */
    }
    return d;
}

static inline void linear_interp5_wrapper(unsigned char c0, unsigned char c1, uint64_t *restrict table) {
    uint64_t b0 = palette_b0g0r[c0];
    uint64_t b1 = palette_b0g0r[c1];
    uint64_t delta = linear_interp5(b0, b1);

    if (delta) { /* A zero delta flags that no smoothing should be done */
        table[0] = b0;
        table[1] = b0 += delta;
        table[2] = b0 += delta;
        table[3] = b0 += delta;
        table[4] = b0 += delta;
    } else {
        table[2] = table[1] = table[0] = b0;
        table[3] = (b0+b1) >> 1;
        table[4] = b1;
    }
}

static inline void nearest_neighbour(int x, int y, int toshow) {
    unsigned char c00 = moobuf[toshow][y][x];
    for (int j=0; j<SCALE; j++) {
        uint32_t *ptr = &screenbuf[(SCALE*y + j)*sizex + SCALE*x];
        for (int i=0; i<SCALE; i++) {
            *ptr++ = palette_scaled[c00];
        }
    }
    moobuf[5][y][x] = 0;
    moobuf[4][y][x] = c00;
}

/* Main paint method, where the graphics actually happen */
static void hw_video_paint(int toshow) {
    uint64_t left_b0g0r[SCALE]; /* only scale first are used */
    uint64_t right_b0g0r[SCALE]; /* only scale first are used */

    if (maxpal >= minpal) invalidate_palrange(minpal/3, maxpal/3);
    convert_palette(palette_raw, minpal/3, maxpal/3);
    minpal = 768; maxpal = 0;

    /* Buf 5 tracks which pixels need update, buf 4 is a copy of last frame */
    if (toshow != 4) { /* Optimization: the loops do nothing when visual==4*/
        for (int y=0; y<MOO_HEIGHT; y++) {
            for (int x=0; x<MOO_WIDTH; x++) {
                /* Set buf[5] nonzero when buf[4] != buf[toshow] */
                moobuf[5][y][x] |= moobuf[4][y][x] ^ moobuf[toshow][y][x];
            }
        }

        for (int y=0; y<MOO_HEIGHT; y++) {
            for (int x=0; x<MOO_WIDTH; x+=sizeof(uint64_t)) {
                *(uint64_t *)&moobuf[5][y][x] |= *(uint64_t *)&moobuf[5][y][x+1];
            }
        }

        for (int y=0; y<MOO_HEIGHT-1; y++) {
            for (int x=0; x<MOO_WIDTH; x+=sizeof(uint64_t)) {
                *(uint64_t *)&moobuf[5][y][x] |= *(uint64_t *)&moobuf[5][y+1][x];
            }
        }
    }

    for (int y=0; y<MOO_HEIGHT-1; y++) {
        register uint64_t *left, *right;
        bool left_recalced = false; /* silly optimization */
        for (int x=0; x<MOO_WIDTH-1; x++) {
            if (moobuf[5][y][x]) {
                unsigned char c00 = moobuf[toshow][y][x];
                unsigned char c01 = moobuf[toshow][y][x+1];
                unsigned char c10 = moobuf[toshow][y+1][x];
                unsigned char c11 = moobuf[toshow][y+1][x+1];
                if (left_recalced) { /* reuse edge when possible */
                    register uint64_t *temp = left;
                    left = right;
                    right = temp;
                } else {
                    left = left_b0g0r;
                    right = right_b0g0r;
                    linear_interp5_wrapper(c00, c10, left);
                }
                linear_interp5_wrapper(c01, c11, right);
                left_recalced = true;
                /* j is row within magnified pixel */
                for (int j=0; j<SCALE; j++) {
                    uint64_t delta = linear_interp5(left[j], right[j]);
                    uint32_t *ptr = &screenbuf[(SCALE*y + j)*sizex + SCALE*x];
                    /* unroll columns within magnified pixel*/
                    if (delta) {
                        uint64_t col = left[j];
                        *ptr++ = b0g0r_to_rgb(col);
                        *ptr++ = b0g0r_to_rgb(col += delta);
                        *ptr++ = b0g0r_to_rgb(col += delta);
                        *ptr++ = b0g0r_to_rgb(col += delta);
                        *ptr = b0g0r_to_rgb(col += delta);
                    } else {
                        uint32_t a = b0g0r_to_rgb(left[j]);
                        uint32_t b = b0g0r_to_rgb(right[j]);
                        *ptr++ = a;
                        *ptr++ = a;
                        *ptr++ = a;
                        *ptr++ = ((a & 0xfefeff) + (b & 0xfefeff)) >> 1;
                        *ptr = b;
                    }
                }
                moobuf[5][y][x] = 0;
                moobuf[4][y][x] = c00;
            } else
                left_recalced = false;
        }
        if (moobuf[5][y][MOO_WIDTH-1]) nearest_neighbour(MOO_WIDTH-1, y, toshow);
    }
    for (int x=0; x<MOO_WIDTH; x++) if (moobuf[5][MOO_HEIGHT-1][x]) nearest_neighbour(x, MOO_HEIGHT-1, toshow);

    XShmPutImage(dpy, mainwin, gc, shared_image, 0, 0, 0, 0, sizex, sizey, false);
    XFlush(dpy);
}

/* -- HW graphics API functions -------------------------------------------- */

void hw_video_set_palette(const uint8_t *pal, const int first, const int num) {
    memmove(&palette_raw[first*3], pal, num*3);
    if (first*3 < minpal) minpal = first*3;
    if ((first+num)*3 > maxpal) maxpal = (first+num)*3;
}

void hw_video_set_palette_byte(int i, uint8_t b) {
    palette_raw[i] = b & 0x3f;
    if (i < minpal) minpal = i;    /* minpal/maxpal are not colour numbers! */
    if (i > maxpal) maxpal = i;
}

uint8_t hw_video_get_palette_byte(int i) { return palette_raw[i]; }
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
    convert_palette(pal, 0, 255);
    for (int i=0; i<w*h; i++) icondata[i+2] = palette_scaled[data[i]] | 0xff000000;
    return 0;
}

bool hw_kbd_set_repeat(bool enabled)
{
    hw_key_press_repeat_active = enabled;
    return true;
}

/* -- X11 specific implementations ---------------------------------------- */

void hw_shutdown(void) {
    if (icondata != NULL) free(icondata);
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
        static bool skip_next_keypress = false;
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
                skip_next_keypress = true;
                break;
              }
            }
            if (e.type == KeyPress && skip_next_keypress) {
                skip_next_keypress = false;
                break;
            }
            modifiers = 0;
            ks = XkbKeycodeToKeysym(dpy, e.xkey.keycode, 0, 0);
            if ((e.type == KeyPress) && ((e.xkey.state & ControlMask) || (ks == XK_Control_L) || (ks == XK_Control_R))) modifiers = MOO_MOD_CTRL;
            if (e.xkey.state & ShiftMask) modifiers |= MOO_MOD_SHIFT;
            if (e.xkey.state & Mod1Mask) modifiers |= MOO_MOD_ALT;
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
            mouse_set_xy_from_hw(e.xmotion.x / SCALE, e.xmotion.y / SCALE);
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
            if (updminx > e.xexpose.x/SCALE) updminx = e.xexpose.x/SCALE;
            if (updminy > e.xexpose.y/SCALE) updminy = e.xexpose.y/SCALE;
            if (updmaxx < (e.xexpose.x + e.xexpose.width + SCALE-1)/SCALE) updmaxx = (e.xexpose.x + e.xexpose.width + SCALE-1)/SCALE;
            if (updmaxy < (e.xexpose.y + e.xexpose.height + SCALE-1)/SCALE) updmaxy = (e.xexpose.y + e.xexpose.height + SCALE-1)/SCALE;
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
    if (w != MOO_WIDTH || h != MOO_HEIGHT) {
        hw_log_error("The X11 backend only supports -uiscale=1\n");
        abort(); /* prevent saving config file with incompatible uiscale */
    }
    create_window(SCALE*MOO_WIDTH, SCALE*MOO_HEIGHT);
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

void hw_mouse_warp(int x, int y)
{
	(void) (x + y);
}

void *hw_video_get_menu_item(int i)
{
    return NULL;
}

int main(int argc, char **argv) {
    return main_1oom(argc, argv);
}
