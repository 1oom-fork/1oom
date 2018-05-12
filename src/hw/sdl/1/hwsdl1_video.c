#include "config.h"

#include <stdio.h>
#include <string.h>

#include "SDL.h"
#ifdef HAVE_SDL1GL
#include "SDL_opengl.h"
#endif

#include "hw.h"
#include "hwsdl_video.h"
#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "lib.h"
#include "log.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* double buffering + 2 aux buffers */
#define NUM_VIDEOBUF    4

static struct sdl_video_s {
    SDL_Surface *screen;
#ifdef HAVE_SDL1GL
    SDL_Surface *hwrenderbuf;
#endif
    void (*render)(int bufi);
    void (*update)(void);
    void (*setpal)(uint8_t *pal, int first, int num);

    /* buffers used by UI */
    uint8_t *buf[NUM_VIDEOBUF];
    int bufw;
    int bufh;
    int bufi;

    /* palette as set by UI, 6bpp */
    uint8_t pal[256 * 3];
#ifdef HAVE_SDL1GL
    /* precalculated 32bit palette */
    uint32_t pal32[256];
#endif
    /* "best" video mode reported by SDL */
    struct {
        int w, h, bpp;
    } bestmode;
} video = { 0 };

/* -------------------------------------------------------------------------- */

static void video_render_8bpp(int bufi)
{
    int pitch = video.screen->pitch;
    Uint8 *p = (Uint8 *)video.screen->pixels;
    uint8_t *q = video.buf[bufi];
    for (int y = 0; y < video.bufh; ++y) {
        memcpy(p, q, video.bufw);
        p += pitch;
        q += video.bufw;
    }
}

static void video_update_8bpp(void)
{
    SDL_UpdateRect(video.screen, 0, 0, video.screen->w, video.screen->h);
}

static void video_setpal_8bpp(uint8_t *pal, int first, int num)
{
    SDL_Color color[256];
    memcpy(&video.pal[first * 3], pal, num * 3);
    for (int i = first; i < (first + num); ++i) {
        color[i].r = *pal++ << 2;
        color[i].g = *pal++ << 2;
        color[i].b = *pal++ << 2;
    }
    SDL_SetColors(video.screen, &color[first], first, num);
    video_update_8bpp();
}

#ifdef HAVE_SDL1GL

static void video_render_gl_32bpp(int bufi)
{
    int pitch_skip = ((video.bufw * sizeof(Uint32)) - video.hwrenderbuf->pitch) / sizeof(Uint32);

    Uint32 *p = (Uint32 *)video.hwrenderbuf->pixels;
    uint8_t *q = video.buf[bufi];
    for (int y = 0; y < video.bufh; ++y) {
        for (int x = 0; x < video.bufw; ++x) {
            *p++ = video.pal32[*q++];
        }
        p += pitch_skip;
    }
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
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, video.bufw, video.bufh, 0, GL_RGBA, GL_UNSIGNED_BYTE, video.hwrenderbuf->pixels);

    glBegin(GL_QUADS);

    /* Lower Right Of Texture */
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, 1.0f);

    /* Upper Right Of Texture */
    glTexCoord2f(0.0f, (float)(video.bufh));
    glVertex2f(-1.0f, -1.0f);

    /* Upper Left Of Texture */
    glTexCoord2f((float)(video.bufw), (float)(video.bufh));
    glVertex2f(1.0f, -1.0f);

    /* Lower Left Of Texture */
    glTexCoord2f((float)(video.bufw), 0.0f);
    glVertex2f(1.0f, 1.0f);

    glEnd();

    SDL_GL_SwapBuffers();
}

static void video_setpal_gl_32bpp(uint8_t *pal, int f, int num)
{
    memcpy(&video.pal[f * 3], pal, num * 3);
    for (int i = 0; i < num; ++i) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        video.pal32[f + i] = ((pal[i * 3 + 0] << 2) << 24)
                           | ((pal[i * 3 + 1] << 2) << 16)
                           | ((pal[i * 3 + 2] << 2) << 8)
                           ;
#else
        video.pal32[f + i] = ((pal[i * 3 + 0] << 2) << 0)
                           | ((pal[i * 3 + 1] << 2) << 8)
                           | ((pal[i * 3 + 2] << 2) << 16)
                           ;
#endif
    }
    hw_video_refresh(1);
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

int hw_video_resize(int w, int h)
{
#ifdef HAVE_SDL1GL
    unsigned int new_w, new_h;
    unsigned int actual_w, actual_h;
    int flags;

    log_message("SDL: resize %ix%i (%s)\n", w, h, hw_opt_fullscreen ? "full" : "window");

    if (!hw_opt_use_gl) {
        return 0;
    }

    if ((w < 0) || (h < 0)) {
        w = video.bufw;
        h = video.bufh;
    }

    new_w = w;
    new_h = h;

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
        hw_opt_screen_winw = actual_w = new_w;
        hw_opt_screen_winh = actual_h = new_h;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);

    log_message("SDL_SetVideoMode(%i, %i, %i, 0x%x)\n", w, h, 32, flags);
    video.screen = SDL_SetVideoMode(actual_w, actual_h, 32, flags);
    if (!video.screen) {
        log_error("SDL_SetVideoMode failed!");
        goto fail;
    }
    set_viewport(video.bufw, video.bufh, actual_w, actual_h);
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

int hw_video_init(int w, int h)
{
    hw_mouse_set_limits(w, h);
    video.bufw = w;
    video.bufh = h;
    {
        const SDL_VideoInfo *p = SDL_GetVideoInfo();
        video.bestmode.w = p->current_w;
        video.bestmode.h = p->current_h;
        video.bestmode.bpp = p->vfmt->BitsPerPixel;
        log_message("SDL_GetVideoInfo -> %ix%i %ibpp\n", video.bestmode.w, video.bestmode.h, video.bestmode.bpp);
    }

#ifdef HAVE_SDL1GL
    if (!hw_opt_use_gl)
#endif
    {
        log_message("SDL_SetVideoMode(%i, %i, ...)\n", w, h);
        video.screen = SDL_SetVideoMode(w, h, 8, SDL_SWSURFACE | SDL_DOUBLEBUF);
        if (!video.screen) {
            log_error("SDL_SetVideoMode failed: %s\n", SDL_GetError());
            return -1;
        }
        video.render = video_render_8bpp;
        video.update = video_update_8bpp;
        video.setpal = video_setpal_8bpp;
    }
#ifdef HAVE_SDL1GL
    else {
        const Uint32
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000, gmask = 0x00ff0000, bmask = 0x0000ff00, amask = 0x000000ff;
#else
        rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000, amask = 0xff000000;
#endif
        log_message("SDL_CreateRGBSurface(...)\n");
        video.hwrenderbuf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask);
        if (!video.hwrenderbuf) {
            log_error("SDL_CreateRGBSurface failed!");
            return -1;
        }
        if ((video.hwrenderbuf->pitch % sizeof(Uint32)) != 0) {
            log_warning("SDL renderbuf pitch mod %i == %i", sizeof(Uint32), video.hwrenderbuf->pitch);
        }
        video.render = video_render_gl_32bpp;
        video.update = video_update_gl_32bpp;
        video.setpal = video_setpal_gl_32bpp;
        if ((hw_opt_screen_winw != 0) && (hw_opt_screen_winh != 0)) {
            w = hw_opt_screen_winw;
            h = hw_opt_screen_winh;
        }
        if (hw_video_resize(w, h)) {
            return -1;
        }
    }
#endif

    for (int i = 0; i < NUM_VIDEOBUF; ++i) {
        video.buf[i] = lib_malloc(w * h);
    }
    video.bufi = 0;
    memset(video.pal, 0, sizeof(video.pal));
    hw_video_refresh_palette();
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
    for (int i = 0; i < NUM_VIDEOBUF; ++i) {
        lib_free(video.buf[i]);
        video.buf[i] = NULL;
    }
}

void hw_video_input_grab(bool grab)
{
    SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

#include "hwsdl_video.c"
