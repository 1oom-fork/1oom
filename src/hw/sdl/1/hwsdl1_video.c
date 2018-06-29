#include "config.h"

#include <stdio.h>
#include <string.h>

#include "SDL.h"
#ifdef HAVE_SDL1GL
#include "SDL_opengl.h"
#endif

#include "hw.h"
#include "hw_video.h"
#include "hwsdl_video.h"
#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "vgabuf.h"
#include "vgapal.h"

/* -------------------------------------------------------------------------- */

static struct sdl_video_s {
    SDL_Surface *screen;
#ifdef HAVE_SDL1GL
    SDL_Surface *hwrenderbuf;
    /* precalculated 32bit palette */
    uint32_t pal32[256];
#endif
    /* "best" video mode reported by SDL */
    struct {
        int w, h, bpp;
    } bestmode;
} video = { 0 };

/* -------------------------------------------------------------------------- */

static void video_render_8bpp(const uint8_t *buf)
{
    SDL_Surface *target = video.screen;
    if (SDL_LockSurface(target) < 0) {
        return;
    }
    int pitch = target->pitch;
    Uint8 *p = (Uint8 *)target->pixels;
    const uint8_t *q = buf;
    int w = target->w;
    int h = target->h;
    for (int y = 0; y < h; ++y) {
        memcpy(p, q, w);
        p += pitch;
        q += w;
    }
    SDL_UnlockSurface(target);
}

static void video_update_8bpp(void)
{
    SDL_UpdateRect(video.screen, 0, 0, video.screen->w, video.screen->h);
}

static void video_setpal_8bpp(const uint8_t *pal, int first, int num)
{
    SDL_Color color[256];
    for (int i = first; i < (first + num); ++i) {
        color[i].r = vgapal_6bit_to_8bit(*pal++);
        color[i].g = vgapal_6bit_to_8bit(*pal++);
        color[i].b = vgapal_6bit_to_8bit(*pal++);
    }
    SDL_SetColors(video.screen, &color[first], first, num);
    video_update_8bpp();
}

#ifdef HAVE_SDL1GL

static void video_render_gl_32bpp(const uint8_t *buf)
{
    SDL_Surface *target = video.hwrenderbuf;
    if (SDL_LockSurface(target) < 0) {
        return;
    }
    int pitch_skip = ((target->w * sizeof(Uint32)) - target->pitch) / sizeof(Uint32);

    Uint32 *p = (Uint32 *)target->pixels;
    const uint8_t *q = buf;
    int w = target->w;
    int h = target->h;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            *p++ = video.pal32[*q++];
        }
        p += pitch_skip;
    }
    SDL_UnlockSurface(target);
}

static void video_update_gl_32bpp(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    {
        int tbl[] = { GL_NEAREST, GL_LINEAR };
        int filter = tbl[hw_opt_gl_filter];

        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, filter);
    }
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, video.hwrenderbuf->w, video.hwrenderbuf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, video.hwrenderbuf->pixels);

    glBegin(GL_QUADS);

    /* Lower Right Of Texture */
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, 1.0f);

    /* Upper Right Of Texture */
    glTexCoord2f(0.0f, (float)(video.hwrenderbuf->h));
    glVertex2f(-1.0f, -1.0f);

    /* Upper Left Of Texture */
    glTexCoord2f((float)(video.hwrenderbuf->w), (float)(video.hwrenderbuf->h));
    glVertex2f(1.0f, -1.0f);

    /* Lower Left Of Texture */
    glTexCoord2f((float)(video.hwrenderbuf->w), 0.0f);
    glVertex2f(1.0f, 1.0f);

    glEnd();

    SDL_GL_SwapBuffers();
}

static void video_setpal_gl_32bpp(const uint8_t *pal, int f, int num)
{
    for (int i = 0; i < num; ++i) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        video.pal32[f + i] = (vgapal_6bit_to_8bit(pal[i * 3 + 0]) << 24)
                           | (vgapal_6bit_to_8bit(pal[i * 3 + 1]) << 16)
                           | (vgapal_6bit_to_8bit(pal[i * 3 + 2]) << 8)
                           ;
#else
        video.pal32[f + i] = (vgapal_6bit_to_8bit(pal[i * 3 + 0]) << 0)
                           | (vgapal_6bit_to_8bit(pal[i * 3 + 1]) << 8)
                           | (vgapal_6bit_to_8bit(pal[i * 3 + 2]) << 16)
                           ;
#endif
    }
    hw_video_refresh();
}
#endif /* HAVE_SDL1GL */

/* -------------------------------------------------------------------------- */

#ifdef HAVE_SDL1GL
static void set_viewport(unsigned int src_w, unsigned int src_h, unsigned int dest_w, unsigned int dest_h)
{
    int dest_x = 0, dest_y = 0;

    if (hw_opt_aspect != 0) {
        double aspect = ((double)(hw_opt_aspect)) / 1000000.;
        if (dest_w * src_h < src_w * aspect * dest_h) {
            dest_y = dest_h;
            dest_h = (unsigned int)(dest_w * src_h / (src_w * aspect));
            dest_y = (dest_y - dest_h) / 2;
        } else {
            dest_x = dest_w;
            dest_w = (unsigned int)(dest_h * src_w * aspect / src_h);
            dest_x = (dest_x - dest_w) / 2;
        }
    }
    glViewport(dest_x, dest_y, dest_w, dest_h);
}
#endif /* HAVE_SDL1GL */

/* -------------------------------------------------------------------------- */

static int video_sw_set(int w, int h)
{
    int flags;
    flags = SDL_SWSURFACE | SDL_DOUBLEBUF;
    if (hw_opt_fullscreen) {
        flags |= SDL_FULLSCREEN;
    }
    log_message("SDL_SetVideoMode(%i, %i, %i, 0x%x)\n", w, h, 8, flags);
    video.screen = SDL_SetVideoMode(w, h, 8, flags);
    if (!video.screen) {
        log_error("SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------------- */

int hw_video_resize(int w, int h)
{
#ifdef HAVE_SDL1GL
    unsigned int actual_w, actual_h;
    int flags;

    log_message("SDL: resize %ix%i (%s)\n", w, h, hw_opt_fullscreen ? "full" : "window");

    if (hw_opt_force_sw) {
        return 0;
    }

    if ((w < 0) || (h < 0)) {
        w = video.hwrenderbuf->w;
        h = video.hwrenderbuf->h;
    }

    flags = SDL_OPENGL | SDL_RESIZABLE;

    if (hw_opt_fullscreen) {
        flags |= SDL_FULLSCREEN;
        if (hw_opt_screen_fsw && hw_opt_screen_fsh) {
            actual_w = hw_opt_screen_fsw;
            actual_h = hw_opt_screen_fsh;
        } else {
            actual_w = video.bestmode.w;
            actual_h = video.bestmode.h;
        }
    } else {
        hw_opt_screen_winw = actual_w = w;
        hw_opt_screen_winh = actual_h = h;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);

    log_message("SDL_SetVideoMode(%i, %i, %i, 0x%x)\n", actual_w, actual_h, 32, flags);
    video.screen = SDL_SetVideoMode(actual_w, actual_h, 32, flags);
    if (!video.screen) {
        log_error("SDL_SetVideoMode failed!\n");
        log_error("Resize %s failed. Run with -%s or set width/height with -%sw W -%sh H.\n",
                hw_opt_fullscreen ? "fullscreen" : "window",
                hw_opt_fullscreen ? "window" : "fs",
                hw_opt_fullscreen ? "fs" : "win",
                hw_opt_fullscreen ? "fs" : "win"
                );
        goto fail;
    }
    set_viewport(video.hwrenderbuf->w, video.hwrenderbuf->h, actual_w, actual_h);
    if (hw_opt_fullscreen) {
        hw_mouse_grab();
    }
    return 0;
    fail:
    return -1;
#else
    return 0;
#endif
}

bool hw_video_toggle_fullscreen(void)
{
    hw_opt_fullscreen = !hw_opt_fullscreen;
    if (i_hw_video.setmode(hw_opt_screen_winw, hw_opt_screen_winh)) {
        hw_opt_fullscreen = !hw_opt_fullscreen; /* restore the setting for the config file */
        return false;
    }
#ifdef HAVE_SDL1GL
    if (hw_opt_force_sw)
#endif /* HAVE_SDL1GL */
    {
        hw_video_refresh_palette();
    }
    return true;
}

int hw_video_init(int w, int h)
{
    {
        const SDL_VideoInfo *p = SDL_GetVideoInfo();
        video.bestmode.w = p->current_w;
        video.bestmode.h = p->current_h;
        video.bestmode.bpp = p->vfmt->BitsPerPixel;
        log_message("SDL_GetVideoInfo -> %ix%i %ibpp\n", video.bestmode.w, video.bestmode.h, video.bestmode.bpp);
    }

#ifdef HAVE_SDL1GL
    if (hw_opt_force_sw)
#endif
    {
        i_hw_video.setmode = video_sw_set;
        i_hw_video.render = video_render_8bpp;
        i_hw_video.update = video_update_8bpp;
        i_hw_video.setpal = video_setpal_8bpp;
    }
#ifdef HAVE_SDL1GL
    else {
        bool find_resolution;
        const Uint32
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000, gmask = 0x00ff0000, bmask = 0x0000ff00, amask = 0x000000ff;
#else
        rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000, amask = 0xff000000;
#endif
        log_message("SDL_CreateRGBSurface(...)\n");
        video.hwrenderbuf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask);
        if (!video.hwrenderbuf) {
            log_error("SDL_CreateRGBSurface failed!\n");
            log_error("Run with -nogl to disable OpenGL.\n");
            return -1;
        }
        if ((video.hwrenderbuf->pitch % sizeof(Uint32)) != 0) {
            log_warning("SDL renderbuf pitch mod %i == %i\n", sizeof(Uint32), video.hwrenderbuf->pitch);
        }
        i_hw_video.setmode = hw_video_resize;
        i_hw_video.render = video_render_gl_32bpp;
        i_hw_video.update = video_update_gl_32bpp;
        i_hw_video.setpal = video_setpal_gl_32bpp;
        find_resolution = true;
        if ((hw_opt_screen_winw != 0) && (hw_opt_screen_winh != 0)) {
            if ((hw_opt_screen_winw < w) || (hw_opt_screen_winh < h)) {
                log_warning("ignoring too small configured resolution %ix%i < %ix%i\n", hw_opt_screen_winw, hw_opt_screen_winh, w, h);
            } else {
                w = hw_opt_screen_winw;
                h = hw_opt_screen_winh;
                find_resolution = false;
            }
        }
        if (find_resolution) {
            int bufw = w;
            int bufh = h;
            int scale = (video.bestmode.w - 50/*window borders*/) / bufw + 1;
            if (scale > 1) {
                do {
                    --scale;
                    h = bufh * scale;
                    if (hw_opt_aspect != 0) {
                        h = (int)(((double)(h) * 1000000.) / ((double)(hw_opt_aspect)));
                    }
                } while ((scale > 1) && ((h + 50/*space for window borders, taskbar etc*/) > video.bestmode.h));
                w = bufw * scale;
            }
        }
    }
#endif

    if (i_hw_video.setmode(w, h)) {
        return -1;
    }
    return 0;
}

void hw_video_shutdown(void)
{
    if (video.screen) {
        SDL_FreeSurface(video.screen);
        video.screen = NULL;
    }
#ifdef HAVE_SDL1GL
    if (video.hwrenderbuf) {
        SDL_FreeSurface(video.hwrenderbuf);
        video.hwrenderbuf = NULL;
    }
#endif
}

void hw_video_input_grab(bool grab)
{
    SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

void hw_video_position_cursor(int mx, int my)
{
    /* Not implemented */
}

int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h)
{
    SDL_Color color[256];
    SDL_Surface *icon;
    Uint8 *mask = 0;
    Uint8 *p;
    uint8_t maxb = 0;
    uint32_t mi = 0;
    icon = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
    if (!icon) {
        log_error("Icon: SDL_CreateRGBSurface failed!\n");
        return -1;
    }
    mask = lib_malloc((w * h + 7) / 8);
    p = (Uint8 *)icon->pixels;
    mi = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t b;
            b = *data++;
            p[x] = b;
            if (b) {
                mask[mi >> 3] |= (1 << (7 - (mi & 7)));
                if (b > maxb) {
                    maxb = b;
                }
            }
            ++mi;
        }
        p += icon->pitch;
    }
    for (int i = 0; i < 256; ++i) {
        color[i].r = vgapal_6bit_to_8bit(*pal++);
        color[i].g = vgapal_6bit_to_8bit(*pal++);
        color[i].b = vgapal_6bit_to_8bit(*pal++);
    }
    SDL_SetColors(icon, color, 0, maxb + 1);
    SDL_WM_SetIcon(icon, mask);
    SDL_FreeSurface(icon);
    icon = NULL;
    lib_free(mask);
    mask = NULL;
    return 0;
}

#include "hw_video.c"
