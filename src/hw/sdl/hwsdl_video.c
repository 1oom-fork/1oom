#include "config.h"

#include <stdio.h>
#include <string.h>

#include "SDL/SDL.h"
#ifdef HAVE_SDL1GL
#include "SDL/SDL_opengl.h"
#endif

#include "hw_internal.h"

#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "lib.h"
#include "log.h"

/* -------------------------------------------------------------------------- */

static struct sdl_video_s {
    SDL_Surface *screen;
    SDL_Surface *source[2];
#ifdef HAVE_SDL1GL
    SDL_Surface *hwrenderbuf;
#endif
    /* "best" video mode reported by SDL */
    struct {
        int w, h, bpp;
    } bestmode;
} video = { 0 };

/* -------------------------------------------------------------------------- */

static int video_setmode_8bpp(int w, int h)
{
    int flags;
    flags = SDL_SWSURFACE | SDL_DOUBLEBUF;
    if (opt_fullscreen) {
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

static void video_render_8bpp(void)
{
    SDL_Surface *src = video.source[0];
    if (vgabuf_get_front() == video.source[1]->pixels) {
        src = video.source[1];
    }
    SDL_BlitSurface(src, NULL, video.screen, NULL);
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
    SDL_SetColors(video.source[0], &color[first], first, num);
    SDL_SetColors(video.source[1], &color[first], first, num);
    SDL_SetColors(video.screen, &color[first], first, num);
    i_hw_video.update();
}

#ifdef HAVE_SDL1GL

static void video_render_gl_32bpp(void)
{
    SDL_Surface *src = video.source[0];
    if (vgabuf_get_front() == video.source[1]->pixels) {
        src = video.source[1];
    }
    SDL_BlitSurface(src, NULL, video.hwrenderbuf, NULL);
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
    SDL_Color color[256];
    for (int i = f; i < (f + num); ++i) {
        color[i].r = vgapal_6bit_to_8bit(*pal++);
        color[i].g = vgapal_6bit_to_8bit(*pal++);
        color[i].b = vgapal_6bit_to_8bit(*pal++);
    }
    SDL_SetColors(video.source[0], &color[f], f, num);
    SDL_SetColors(video.source[1], &color[f], f, num);
    hw_video_redraw_front();
}
#endif /* HAVE_SDL1GL */

/* -------------------------------------------------------------------------- */

#ifdef HAVE_SDL1GL
static void set_viewport(unsigned int src_w, unsigned int src_h, unsigned int dest_w, unsigned int dest_h)
{
    int dest_x = 0, dest_y = 0;

    if (opt_aspect_ratio_correct) {
        if (dest_w * src_h * 6 < src_w * dest_h * 5) {
            dest_y = dest_h;
            dest_h = (dest_w * src_h * 6 / 5 / src_w);
            dest_y = (dest_y - dest_h) / 2;
        } else {
            dest_x = dest_w;
            dest_w = (dest_h * src_w * 5 / 6 / src_h);
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
    unsigned int actual_w, actual_h;
    int flags, bpp, rsize, gsize, bsize;

    log_message("SDL: resize %ix%i (%s)\n", w, h, opt_fullscreen ? "full" : "window");

    if (opt_force_sw) {
        return 0;
    }

    if ((w < 0) || (h < 0)) {
        w = video.hwrenderbuf->w;
        h = video.hwrenderbuf->h;
    }

    flags = SDL_OPENGL | SDL_RESIZABLE;

    if (opt_fullscreen) {
        flags |= SDL_FULLSCREEN;
        if (opt_screen_fsw && opt_screen_fsh) {
            actual_w = opt_screen_fsw;
            actual_h = opt_screen_fsh;
        } else {
            actual_w = video.bestmode.w;
            actual_h = video.bestmode.h;
        }
    } else {
        opt_screen_winw = actual_w = w;
        opt_screen_winh = actual_h = h;
    }
    if (hw_opt_bpp == 0) {
        bpp = video.bestmode.bpp;
    } else {
        bpp = hw_opt_bpp;
    }

    switch (bpp) {
    case 15:
        rsize = 5;  gsize = 5;  bsize = 5;
        break;
    case 16:
        rsize = 5;  gsize = 6;  bsize = 5;
        break;
    case 24:
    case 32:
        rsize = 8;  gsize = 8;  bsize = 8;
        break;
    default:
        log_error("hwsdl: %d bpp mode is not supported!\n", bpp);
        goto fail;
        break;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, rsize);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, gsize);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, bsize);

    log_message("SDL_SetVideoMode(%i, %i, %i, 0x%x)\n", actual_w, actual_h, bpp, flags);
    video.screen = SDL_SetVideoMode(actual_w, actual_h, bpp, flags);
    if (!video.screen) {
        log_error("SDL_SetVideoMode failed!\n");
        goto fail;
    }
    set_viewport(video.hwrenderbuf->w, video.hwrenderbuf->h, actual_w, actual_h);
    if (opt_fullscreen) {
        hw_mouse_grab();
    }
    return 0;
    fail:
    return -1;
#else
    return 0;
#endif
}

int hw_video_toggle_fullscreen(void)
{
    opt_fullscreen = !opt_fullscreen;
    if (i_hw_video.setmode(opt_screen_winw, opt_screen_winh)) {
        opt_fullscreen = !opt_fullscreen; /* restore the setting for the config file */
        return -1;
    }
#ifdef HAVE_SDL1GL
    if (opt_force_sw)
#endif /* HAVE_SDL1GL */
    {
        hw_video_refresh_palette();
    }
    return 0;
}

int hw_video_init(int w, int h)
{
    video.source[0] = SDL_CreateRGBSurfaceFrom(vgabuf_get_front(), w, h, 8, w, 0, 0, 0, 0);
    video.source[1] = SDL_CreateRGBSurfaceFrom(vgabuf_get_back(), w, h, 8, w, 0, 0, 0, 0);
    {
        const SDL_VideoInfo *p = SDL_GetVideoInfo();
        video.bestmode.w = p->current_w;
        video.bestmode.h = p->current_h;
        video.bestmode.bpp = p->vfmt->BitsPerPixel;
        log_message("SDL_GetVideoInfo -> %ix%i %ibpp\n", video.bestmode.w, video.bestmode.h, video.bestmode.bpp);
    }

#ifdef HAVE_SDL1GL
    if (opt_force_sw)
#endif
    {
        i_hw_video.setmode = video_setmode_8bpp;
        i_hw_video.render = video_render_8bpp;
        i_hw_video.update = video_update_8bpp;
        i_hw_video.setpal = video_setpal_8bpp;
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
            log_error("SDL_CreateRGBSurface failed!\n");
            return -1;
        }
        if ((video.hwrenderbuf->pitch % sizeof(Uint32)) != 0) {
            log_warning("SDL renderbuf pitch mod %i == %i\n", sizeof(Uint32), video.hwrenderbuf->pitch);
        }
        i_hw_video.setmode = hw_video_resize;
        i_hw_video.render = video_render_gl_32bpp;
        i_hw_video.update = video_update_gl_32bpp;
        i_hw_video.setpal = video_setpal_gl_32bpp;
        if (opt_aspect_ratio_correct) {
            h = h * 6 / 5;
        }
        if ((opt_screen_winw != 0) && (opt_screen_winh != 0)) {
            if ((opt_screen_winw < w) || (opt_screen_winh < h)) {
                log_warning("ignoring too small configured resolution %ix%i < %ix%i\n", opt_screen_winw, opt_screen_winh, w, h);
            } else {
                w = opt_screen_winw;
                h = opt_screen_winh;
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
    for (int i = 0; i < 2; ++i) {
        if (video.source[i]) {
            SDL_FreeSurface(video.source[i]);
            video.source[i] = NULL;
        }
    }
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
