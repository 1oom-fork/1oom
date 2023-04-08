#include "config.h"

#include <stdio.h>
#include <string.h>

#include "SDL.h"

#include "hw.h"
#include "comp.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "comp.h"
#include "version.h"
#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "hwsdl_aspect.h"
#include "hwsdl2_window.h"
#include "hwsdl2_video_buffers.h"
#include "hwsdl2_video_texture.h"
#include "hwsdl2_window_icon.h"

/* If a user wants a post stamp then let them do it */
#define MIN_RESX 80
#define MIN_RESY 50

#define RESIZE_DELAY 500

/* A line or two were adapted from Chocolate Doom 3.0 i_video.c by Simon Howard */

/* -------------------------------------------------------------------------- */

static bool need_resize = false;
static uint32_t window_resize_time = 0;

static SDL_Window *the_window = NULL;
static SDL_Renderer *the_renderer = NULL;

static int viewport_w = 0;
static int viewport_h = 0;

/* -------------------------------------------------------------------------- */

void hw_mouse_warp(int x, int y)
{
    if (the_window) {
        SDL_Rect win = {0};
        SDL_Rect vp = {0}; /* viewport in window units */
        int wx=0, wy=0;

        SDL_GetWindowSize(the_window, &win.w, &win.h);

        vp = win;
        if (hw_opt_aspect != 0) {
            /* SDL has no function to query where the picture is drawn?
            Gotta figure it out ourselves and risk being wrong */
            shrink_to_aspect_ratio(&vp.w, &vp.h, 1000000, hw_opt_aspect);
            vp.x = (win.w - vp.w) / 2;
            vp.y = (win.h - vp.h) / 2;
        }

        moo_to_window(x, y, &vp, &wx, &wy);

        /* this function uses window coordinates. not logical units */
        SDL_WarpMouseInWindow(the_window, wx, wy);

        /* eat the generated mouse event */
        SDL_PumpEvents();
        SDL_FlushEvent(SDL_MOUSEMOTION);
    }
}

static void print_format(const char *title, uint32_t p)
{
    uint32_t r, g, b, a;
    int bpp;
    SDL_PixelFormatEnumToMasks(p, &bpp, &r, &g, &b, &a);
    log_message("%s: %s bytes/pixel=%d R=%x G=%x B=%x A=%x\n",
            title, SDL_GetPixelFormatName(p), bpp, r, g, b, a);
}

static void move_window(int dx, int dy)
{
    int x, y;
    SDL_GetWindowPosition(the_window, &x, &y);
    x += dx;
    y += dy;
    SDL_SetWindowPosition(the_window, x, y);
}

static bool is_windowed(int flags)
{
    return (flags & (
        SDL_WINDOW_FULLSCREEN_DESKTOP
        | SDL_WINDOW_FULLSCREEN
        | SDL_WINDOW_MAXIMIZED)) == 0;
}

static void do_the_resize(void)
{
    int flags, w, h;

    if (!hw_opt_autotrim) {
        return;
    }

    flags = SDL_GetWindowFlags(the_window);
    if (!is_windowed(flags)) {
        return;
    }

    SDL_GetWindowSize(the_window, &w, &h);

    if (hw_opt_aspect != 0) {
        SDL_Rect vp;
        float scale[2];
        int new_w, new_h;

        /* here we figure out how many window units the picture is
         * after SDL maybe letterboxed it and maybe did the integer scaling */
        SDL_RenderGetViewport(the_renderer, &vp);
        SDL_RenderGetScale(the_renderer, scale, scale+1);

        if (vp.w > 0 && vp.h > 0) {
            new_w = vp.w * scale[0];
            new_h = vp.h * scale[1];

            /* sometimes SDL gets the viewport size wrong by 1 pixel (e.g. 320x199)
             * this rounding here hopes to prevent that bug */
            new_w = ( new_w + 1 ) & ~1;
            new_h = ( new_h + 1 ) & ~1;

            new_w = MAX(MIN_RESX, new_w);
            new_h = MAX(MIN_RESY, new_h);

            move_window((w-new_w)/2, (h-new_h)/2); /* keep image centered */
            SDL_SetWindowSize(the_window, new_w, new_h);
        } else {
            /* viewport not set? or SDL bug? try again soon */
            log_warning("Strange viewport size: %dx%d\n", vp.w, vp.h);
            need_resize = true;
            window_resize_time = SDL_GetTicks() + RESIZE_DELAY;
        }
    }
}

void hwsdl_video_next_frame(const hwsdl_video_buffer_t *buf)
{
    hwsdl_texture_update(the_renderer, buf);
    hwsdl_video_update();
}

#ifdef FRAME_TIME_DUMP
static void frame_time_dump(void)
{
    extern char *hw_opt_frame_time_dump_filename;
    static FILE *fp = NULL;
    static unsigned frame_id = 0;
    static uint32_t prev_t = 0;
    if (!fp) {
        char *fn = hw_opt_frame_time_dump_filename;
        if (fn) {
            fp = fopen(fn, "w");
            if (fp) {
                log_message("Dumping frame timings to %s\n", fn);
                fprintf(fp, "frame,dt\n");
                prev_t = SDL_GetTicks();
            } else {
                log_error("failed to open file: %s\n", fn);
            }
        }
    }
    if (fp) {
        uint32_t now = SDL_GetTicks();
        unsigned dt = now - prev_t;
        prev_t = now;
        fprintf(fp, "%u,%u\n", ++frame_id, dt);
    }
}
#endif

void hwsdl_video_update(void)
{
    if (need_resize && SDL_GetTicks() >= window_resize_time) {
        need_resize = false;
        do_the_resize();
    }

    /* Make sure the pillarboxes are kept clear each frame. */
    SDL_RenderSetViewport(the_renderer, NULL);
    SDL_SetRenderDrawBlendMode(the_renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(the_renderer, 0, 0, 0, 0);
    SDL_RenderClear(the_renderer);

    /* Put the output texture on screen */
    hwsdl_texture_output(the_renderer);

    /* Draw! */
    SDL_RenderPresent(the_renderer);

#ifdef FRAME_TIME_DUMP
    frame_time_dump();
#endif
}

/* -------------------------------------------------------------------------- */

static void destroy_window(void)
{
    hwsdl_texture_delete();
    if (the_renderer) {
        SDL_DestroyRenderer(the_renderer);
        the_renderer = NULL;
    }
    if (the_window) {
        SDL_DestroyWindow(the_window);
        the_window = NULL;
    }
}

static bool create_window(int w, int h)
{
    const int x = SDL_WINDOWPOS_UNDEFINED;
    const int y = SDL_WINDOWPOS_UNDEFINED;
    uint32_t window_flags = 0;

    /* In windowed mode, the window can be resized while the game is running. */
    window_flags = SDL_WINDOW_RESIZABLE;

    /* Set the highdpi flag - this makes a big difference on Macs with
       retina displays, especially when using small window sizes. */
    window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;

    if (hw_opt_fullscreen) {
        if (hw_opt_screen_fsw && hw_opt_screen_fsh) {
            w = hw_opt_screen_fsw;
            h = hw_opt_screen_fsh;
            window_flags |= SDL_WINDOW_FULLSCREEN;
        } else {
            window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
    }

    /* If "window_flags" contains the fullscreen flag (see above), then w and h are ignored. */
    if (!the_window) {
        log_message("SDL_CreateWindow(0, %i, %i, %i, %i, 0x%x)\n", x, y, w, h, window_flags);
        the_window = SDL_CreateWindow(0, x, y, w, h, window_flags);
        if (!the_window) {
            log_error("SDL_CreateWindow failed: %s\n", SDL_GetError());
            return false;
        }
        SDL_SetWindowMinimumSize(the_window, MIN_RESX, MIN_RESY);
        SDL_SetWindowTitle(the_window, PACKAGE_NAME " " VERSION_STR);
        hwsdl_set_icon(the_window);

        print_format("Display format", SDL_GetWindowPixelFormat(the_window));
    }

    return true;
}

static bool create_renderer(void)
{
    uint32_t renderer_flags = 0;
    SDL_RendererInfo info;

    if (hw_opt_vsync) {
        /* Turn on vsync */
        renderer_flags = SDL_RENDERER_PRESENTVSYNC;
    }

    if (hw_opt_force_sw) {
        renderer_flags |= SDL_RENDERER_SOFTWARE;
        renderer_flags &= ~SDL_RENDERER_PRESENTVSYNC;
    }

    if (the_renderer) {
        SDL_DestroyRenderer(the_renderer);
    }

    the_renderer = SDL_CreateRenderer(the_window, -1, renderer_flags);
    if (the_renderer == NULL) {
        log_error("SDL2: Error creating renderer for screen window: %s\n", SDL_GetError());
        return false;
    }

#ifdef HAVE_INT_SCALING
    SDL_RenderSetIntegerScale(the_renderer, hw_opt_int_scaling);
#endif

    if (!SDL_GetRendererInfo(the_renderer, &info)) {
        const char *y = "yes";
        const char *n = "no";
        
        /* We want the extraui checkbox to be consistent with the actual state of the flag */
        hw_opt_vsync = (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;

        log_message("SDL renderer: %s\n"
                "... accelerated %s\n"
                "... V-sync %s\n"
                "... max_texture_width: %d\n"
                "... max_texture_height: %d\n",
                info.name,
                (info.flags & SDL_RENDERER_ACCELERATED) ? y : n,
                hw_opt_vsync ? y : n,
                info.max_texture_width,
                info.max_texture_height);
    }

    return true;
}

static int set_video_mode(int w, int h)
{
    if (!create_window(w, h)) {
        return -1;
    }
    if (!create_renderer()) {
        return -1;
    }
    hw_mouse_init();
    hwsdl_video_resized(w, h);
    return 0;
}

/* -------------------------------------------------------------------------- */

int hwsdl_video_resized(int w, int h) /* called from the event handler */
{
    need_resize = true;
    window_resize_time = SDL_GetTicks() + RESIZE_DELAY;
    return 0;
}

bool hwsdl_video_toggle_autotrim(void)
{
    hw_opt_autotrim = !hw_opt_autotrim;
    hwsdl_video_resized(-1, -1); /* w,h not used so far */
    return true;
}

#ifdef HAVE_INT_SCALING
bool hwsdl_video_toggle_intscaling(void)
{
    hw_opt_int_scaling = !hw_opt_int_scaling;
    SDL_RenderSetIntegerScale(the_renderer, hw_opt_int_scaling);
    return true;
}
#endif

static bool reset_video_mode(void)
{
    /* About to destroy the window and re-create it.
     * this means we lose the GL context and every texture with it */
    hwsdl_texture_delete(); /* recreate the texture when needed next time */
    destroy_window();
    int w = viewport_w;
    int h = viewport_h;
    if (hw_opt_aspect != 0) {
        shrink_to_aspect_ratio(&w, &h, 1000000, hw_opt_aspect);
    }
    return set_video_mode(w, h) == 0;
}

bool hw_video_update_aspect(void)
{
    /* called by hwsdl_opt.c 
     * if autotrim is enabled and in windowed mode then gotta resize */
    return reset_video_mode();
}

bool hwsdl_video_toggle_fullscreen(void)
{
    hw_opt_fullscreen = !hw_opt_fullscreen;
    if (!reset_video_mode()) {
        hw_opt_fullscreen = !hw_opt_fullscreen; /* restore the setting for the config file */
        return false;
    }
    return true;
}

bool hwsdl_video_toggle_vsync(void)
{
    hw_opt_vsync = !hw_opt_vsync;
    if (!reset_video_mode()) {
        hw_opt_vsync = !hw_opt_vsync; /* restore the setting for the config file */
        return false;
    }
    return true;
}

int hwsdl_win_init(int moo_w, int moo_h)
{
    int w = moo_w;
    int h = moo_h;
    if (hw_opt_screen_winw != 0 && hw_opt_screen_winh != 0) {
        if (hw_opt_screen_winw < MIN_RESX || hw_opt_screen_winh < MIN_RESY) {
            log_warning("ignoring too small configured resolution %ix%i < %ix%i\n",
                    hw_opt_screen_winw, hw_opt_screen_winh, MIN_RESX, MIN_RESY);
        } else {
            w = hw_opt_screen_winw;
            h = hw_opt_screen_winh;
        }
    }
    viewport_w = w;
    viewport_h = h;
    if (hw_opt_aspect != 0) {
        shrink_to_aspect_ratio(&w, &h, 1000000, hw_opt_aspect);
    }
    if (set_video_mode(w, h)) {
        return -1;
    }
    hw_video_refresh_palette();
    return 0;
}

void hwsdl_video_shutdown(void)
{
    hwsdl_delete_icon();
    destroy_window();
}

