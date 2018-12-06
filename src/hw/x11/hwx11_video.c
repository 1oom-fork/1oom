#include "config.h"

#include <stdlib.h>

#include <sys/shm.h>
#include <sys/time.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include "hw_internal.h"

#include "hwx11_video.h"

/* -------------------------------------------------------------------------- */

Display *dpy = NULL;

static GC gc;
static Visual *visual;
static Window mainwin;
static XImage *shared_image = NULL;
static XShmSegmentInfo shminfo;

static uint32_t palette_scaled[256] = { 0 };

/* -------------------------------------------------------------------------- */

static void video_render(void)
{
    uint32_t *p = (uint32_t *)shared_image->data;
    const uint8_t *q = vgabuf_get_front();
    int w = shared_image->width;
    int h = shared_image->height;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            p[x] = palette_scaled[q[x]];
        }
        p += shared_image->bytes_per_line / sizeof(uint32_t);
        q += w;
    }
    XShmPutImage(dpy, mainwin, gc, shared_image, 0, 0, 0, 0, w, h, false);
    XFlush(dpy);
}

static void video_update(void)
{
    /* FIXME */
}

static void video_setpal(const uint8_t *pal, int first, int num)
{
    for (int i = first; i < (first + num); ++i) {
        uint32_t b = vgapal_6bit_to_8bit(pal[i*3 + 0]);
        uint32_t g = vgapal_6bit_to_8bit(pal[i*3 + 1]);
        uint32_t r = vgapal_6bit_to_8bit(pal[i*3 + 2]);
        palette_scaled[i] = r + (g << 8) + (b << 16);
    }
    i_hw_video.render();
}

static void hide_cursor(void)
{	/* Hide the X11 mouse cursor when in 1oom win */
    Cursor no_ptr;
    XColor black, dummy;
    static char no_data[8] = { 0 };
    unsigned long colormap = DefaultColormap(dpy, DefaultScreen(dpy));
    unsigned long bm_no = XCreateBitmapFromData(dpy, mainwin, no_data, 8, 8);

    XAllocNamedColor(dpy, colormap, "black", &black, &dummy);
    no_ptr = XCreatePixmapCursor(dpy, bm_no, bm_no, &black, &black, 0, 0);
    XDefineCursor(dpy, mainwin, no_ptr);
    XFreeCursor(dpy, no_ptr);
    XFreePixmap(dpy, bm_no);
}

/* -------------------------------------------------------------------------- */

int create_window(int width, int height)
{
    XSetWindowAttributes attribs = { 0 };
    XSizeHints sizehint;

    visual = DefaultVisual(dpy, DefaultScreen(dpy));
    attribs.backing_store = NotUseful;
    attribs.override_redirect = True;
    mainwin = XCreateWindow(dpy, DefaultRootWindow(dpy), -1, -1, width, height,
                            1, CopyFromParent, InputOutput, visual, 0, &attribs);
    gc = XCreateGC(dpy, mainwin, 0, NULL);
    XSelectInput(dpy, mainwin, ButtonPressMask | ButtonReleaseMask | ExposureMask |
                                   KeyPressMask | KeyReleaseMask | ButtonMotionMask | PointerMotionMask );
    sizehint.min_width = sizehint.max_width = width;
    sizehint.min_height = sizehint.max_height = height;
    sizehint.flags = PMinSize | PMaxSize;
    XSetWMNormalHints(dpy, mainwin, &sizehint);
    XSetWindowBorderWidth(dpy, mainwin, 0);
    XMapWindow(dpy, mainwin);
    XStoreName(dpy, mainwin, "1oom");
    XFlush(dpy);
    shared_image = XShmCreateImage(dpy, visual, 24, ZPixmap, NULL, &shminfo, width, height);
    shminfo.shmid = shmget(IPC_PRIVATE, shared_image->bytes_per_line * shared_image->height, IPC_CREAT|0777);
    shminfo.shmaddr = shared_image->data = shmat(shminfo.shmid, 0, 0);
    shminfo.readOnly = false;
    XShmAttach(dpy, &shminfo);
    hide_cursor();
    return 0;
}

int hw_video_init(int w, int h)
{
    i_hw_video.render = video_render;
    i_hw_video.update = video_update;
    i_hw_video.setpal = video_setpal;
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL || !XShmQueryExtension(dpy) || DefaultDepth(dpy, DefaultScreen(dpy)) != 24) {
        log_error("Cannot connect to XServer, or it does not supprt XSHM, or is not 24/32 bit colour\n");
        return 11;
    }
    create_window(w, h);
    return 0;
}

void hw_video_shutdown(void)
{
    if (gc != NULL) {
        XShmDetach(dpy, &shminfo);
        XDestroyImage(shared_image);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
        XDestroyWindow(dpy, mainwin);
        XFreeGC(dpy, gc);
        gc = NULL;
    }
    if (dpy != NULL) {
        XCloseDisplay(dpy);
        dpy = NULL;
    }
}

int hw_video_toggle_fullscreen(void)
{
    return 0;
}

int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h) {
    return 0;
}

void hw_mouse_toggle_grab(void)
{
}
