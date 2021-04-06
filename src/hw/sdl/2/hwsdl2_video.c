#include "config.h"

#include <stdio.h>
#include <string.h>

#include "SDL.h"

#include "hw.h"
#include "hw_video.h"
#include "comp.h"
#include "hwsdl_video.h"
#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "vgabuf.h"
#include "vgapal.h"

/* -------------------------------------------------------------------------- */

/* Most of the code and comments adapted from Chocolate Doom 3.0 i_video.c.
   Copyright(C) 2005-2014 Simon Howard
*/

/* -------------------------------------------------------------------------- */

#define RESIZE_DELAY 500

static struct sdl_video_s {
    /* These are (1) the window (or the full screen) that our game is rendered to
       and (2) the renderer that scales the texture (see below) into this window.
    */
    SDL_Window *window;
    SDL_Renderer *renderer;
    /* These are (1) the 320x200x8 paletted buffer that we copy the active buffer to,
       (2) the 320x200x32 RGBA intermediate buffer that we blit the former buffer to,
       (3) the intermediate 320x200 texture that we load the RGBA buffer to and that
       we render into another texture (4) which is upscaled by an integer factor
       UPSCALE using "nearest" scaling and which in turn is finally rendered to screen
       using "linear" scaling.
    */
    SDL_Surface *screen;
    SDL_Surface *interbuffer;
    SDL_Texture *texture;
    SDL_Texture *texture_upscaled;

    SDL_Rect blit_rect;
    uint32_t pixel_format;

    /* SDL display number on which to run. */
    int display;

    int w_upscale, h_upscale;
    int actualh;

    bool need_resize;
    int last_resize_time;

    /* palette as used by SDL */
    SDL_Color color[256];
    bool palette_to_set;
} video = { 0 };

/* -------------------------------------------------------------------------- */

static void video_create_upscaled_texture(bool force)
{
    int w, h;
    int h_upscale, w_upscale;

    /* Get the size of the renderer output. The units this gives us will be
       real world pixels, which are not necessarily equivalent to the screen's
       window size (because of highdpi).
    */
    if (SDL_GetRendererOutputSize(video.renderer, &w, &h) != 0) {
        log_fatal_and_die("SDL2: Failed to get renderer output size: %s\n", SDL_GetError());
    }

    if (!hw_opt_smooth_pixel_scaling || ((w % video.screen->w == 0) && (h % video.screen->h == 0))) {
        if (video.texture_upscaled) {
            SDL_DestroyTexture(video.texture_upscaled);
            video.texture_upscaled = NULL;
        }
        return;
    }

    /* When the screen or window dimensions do not match the aspect ratio
       of the texture, the rendered area is scaled down to fit. Calculate
       the actual dimensions of the rendered area.
    */
    if (w * video.actualh < h * video.screen->w) {
        /* Tall window. */
        h = (w * video.actualh) / video.screen->w;
    } else {
        /* Wide window. */
        w = (h * video.screen->w) / video.actualh;
    }

    /* Pick texture size the next integer multiple of the screen dimensions.
       If one screen dimension matches an integer multiple of the original
       resolution, there is no need to overscale in this direction.
    */
    w_upscale = (w + video.screen->w - 1) / video.screen->w;
    h_upscale = (h + video.screen->h - 1) / video.screen->h;

    /* Minimum texture dimensions of 320x200. */
    SETMAX(w_upscale, 1);
    SETMAX(h_upscale, 1);

    /* LimitTextureSize(&w_upscale, &h_upscale); TODO SDL2 */

    /* Create a new texture only if the upscale factors have actually changed. */
    if (h_upscale == video.h_upscale && w_upscale == video.w_upscale && !force) {
        return;
    }
    video.h_upscale = h_upscale;
    video.w_upscale = w_upscale;

    if (video.texture_upscaled) {
        SDL_DestroyTexture(video.texture_upscaled);
    }

    /* Set the scaling quality for rendering the upscaled texture to "linear",
       which looks much softer and smoother than "nearest" but does a better
       job at downscaling from the upscaled texture to screen.
    */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    video.texture_upscaled = SDL_CreateTexture(video.renderer,
                                video.pixel_format,
                                SDL_TEXTUREACCESS_TARGET,
                                w_upscale * video.screen->w,
                                h_upscale * video.screen->h
                             );

}

/* -------------------------------------------------------------------------- */

static void video_render(const uint8_t *buf)
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

/* Adjust window_width / window_height variables to be an an aspect
   ratio consistent with the aspect_ratio_correct variable.
*/
static void video_adjust_window_size(int *wptr, int *hptr)
{
    int w = *wptr, h = *hptr;
    if ((w * video.actualh) <= (h * video.screen->w)) {
        /* We round up window_height if the ratio is not exact; this leaves the result stable. */
        h = (w * video.actualh + video.screen->w - 1) / video.screen->w;
    } else {
        w = (h * video.screen->w) / video.actualh;
    }
    *wptr = w;
    *hptr = h;
}

static void video_update(void)
{
    if (video.need_resize) {
        if (SDL_GetTicks() > (video.last_resize_time + RESIZE_DELAY)) {
            int flags, w, h;
            /* When the window is resized (we're not in fullscreen mode and not maximized),
               save the new window size.
            */
            flags = SDL_GetWindowFlags(video.window);
            if ((flags & (SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED)) == 0) {
                SDL_GetWindowSize(video.window, &w, &h);
                /* Adjust the window by resizing again so that the window is the right aspect ratio. */
                video_adjust_window_size(&w, &h);
                SDL_SetWindowSize(video.window, w, h);
                hw_opt_screen_winw = w;
                hw_opt_screen_winh = h;
            }
            video_create_upscaled_texture(false);
            video.need_resize = false;
            video.palette_to_set = true;
        } else {
            return;
        }
    }

    if (video.palette_to_set) {
        SDL_SetPaletteColors(video.screen->format->palette, video.color, 0, 256);
        video.palette_to_set = false;
    }

    /* Blit from the paletted 8-bit screen buffer to the intermediate
       32-bit RGBA buffer that we can load into the texture.
    */
    SDL_LowerBlit(video.screen, &video.blit_rect, video.interbuffer, &video.blit_rect);

    /* Update the intermediate texture with the contents of the RGBA buffer. */
    SDL_UpdateTexture(video.texture, NULL, video.interbuffer->pixels, video.interbuffer->pitch);

    /* Make sure the pillarboxes are kept clear each frame. */
    SDL_RenderClear(video.renderer);

    if (hw_opt_smooth_pixel_scaling && (video.texture_upscaled != NULL)) {
        /* Render this intermediate texture into the upscaled texture
           using "nearest" integer scaling.
        */
        SDL_SetRenderTarget(video.renderer, video.texture_upscaled);
        SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);

        /* Finally, render this upscaled texture to screen using linear scaling. */
        SDL_SetRenderTarget(video.renderer, NULL);
        SDL_RenderCopy(video.renderer, video.texture_upscaled, NULL, NULL);
    } else {
        /* Render this intermediate texture directly to screen using "nearest" scaling. */
        SDL_SetRenderTarget(video.renderer, NULL);
        SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);
    }

    /* Draw! */
    SDL_RenderPresent(video.renderer);
}

static void video_setpal(const uint8_t *pal, int first, int num)
{
    for (int i = first; i < (first + num); ++i) {
        video.color[i].r = vgapal_6bit_to_8bit(*pal++);
        video.color[i].g = vgapal_6bit_to_8bit(*pal++);
        video.color[i].b = vgapal_6bit_to_8bit(*pal++);
        video.color[i].a = 255;
    }
    video.palette_to_set = true;
    video_update();
}

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

/* Check the display bounds of the display referred to by 'video_display' and
   set x and y to a location that places the window in the center of that
   display.
*/
static void video_center_window(int *x, int *y, int w, int h)
{
    SDL_Rect bounds;

    if (SDL_GetDisplayBounds(video.display, &bounds) < 0) {
        log_warning("SDL2: Failed to read display bounds for display #%d!\n", video.display);
        return;
    }

    *x = bounds.x + SDL_max((bounds.w - w) / 2, 0);
    *y = bounds.y + SDL_max((bounds.h - h) / 2, 0);
}

static void video_get_window_position(int *x, int *y, int w, int h)
{
    /* TODO SDL2 stored x/y */
    *x = SDL_WINDOWPOS_UNDEFINED;
    *y = SDL_WINDOWPOS_UNDEFINED;

    /* Check that video_display corresponds to a display that really exists,
       and if it doesn't, reset it. */
    if (video.display < 0 || video.display >= SDL_GetNumVideoDisplays()) {
        log_warning("SDL2: We were configured to run on display #%d, "
                    "but it no longer exists (max %d). Moving to display 0.\n",
                    video.display, SDL_GetNumVideoDisplays() - 1
                   );
        video.display = 0;
    }

    /* in fullscreen mode, the window "position" still matters, because
       we use it to control which display we run fullscreen on.
    */
    if (hw_opt_fullscreen) {
        video_center_window(x, y, w, h);
    }
}

static int video_sw_set(int w, int h)
{
    int x, y;
    int window_flags = 0, renderer_flags = 0;
    SDL_DisplayMode mode;
    SDL_RendererInfo info;

    /* In windowed mode, the window can be resized while the game is running. */
    window_flags = SDL_WINDOW_RESIZABLE;
    /* Set the highdpi flag - this makes a big difference on Macs with
       retina displays, especially when using small window sizes. */
    window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    if (hw_opt_borderless) {
        window_flags |= SDL_WINDOW_BORDERLESS;
    }
    if (hw_opt_fullscreen) {
        if (hw_opt_screen_fsw && hw_opt_screen_fsh) {
            w = hw_opt_screen_fsw;
            h = hw_opt_screen_fsh;
            window_flags |= SDL_WINDOW_FULLSCREEN;
        } else {
            window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
    }
    /* Create window and renderer contexts. We leave the window position "undefined".
       If "window_flags" contains the fullscreen flag (see above), then w and h are ignored.
    */
    video_get_window_position(&x, &y, w, h);
    if (!video.window) {
        log_message("SDL_CreateWindow(0, %i, %i, %i, %i, 0x%x)\n", x, y, w, h, window_flags);
        video.window = SDL_CreateWindow(0, x, y, w, h, window_flags);
        if (!video.window) {
            log_error("SDL_CreateWindow failed: %s\n", SDL_GetError());
            return -1;
        }
        video.pixel_format = SDL_GetWindowPixelFormat(video.window);
        SDL_SetWindowMinimumSize(video.window, video.screen->w, video.actualh);
        SDL_SetWindowTitle(video.window, "1oom");
    }
    /* The SDL_RENDERER_TARGETTEXTURE flag is required to render the
       intermediate texture into the upscaled texture.
    */
    renderer_flags = SDL_RENDERER_TARGETTEXTURE;
    if (SDL_GetCurrentDisplayMode(video.display, &mode) != 0) {
        log_error("SDL2: Could not get display mode for video display #%d: %s\n", video.display, SDL_GetError());
        return -1;
    }
    /* Turn on vsync */
    if (hw_opt_vsync) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    if (hw_opt_force_sw) {
        renderer_flags |= SDL_RENDERER_SOFTWARE;
        renderer_flags &= ~SDL_RENDERER_PRESENTVSYNC;
    }

    if (video.renderer) {
        SDL_DestroyRenderer(video.renderer);
        // all associated textures get destroyed
        video.texture = NULL;
        video.texture_upscaled = NULL;
    }
    video.renderer = SDL_CreateRenderer(video.window, -1, renderer_flags);
    if (video.renderer == NULL) {
        log_error("SDL2: Error creating renderer for screen window: %s\n", SDL_GetError());
        return -1;
    }
    if (!SDL_GetRendererInfo(video.renderer, &info)) {
        log_message("SDL_GetRendererInfo: %s%s%s\n",
                    info.name,
                    (info.flags & SDL_RENDERER_ACCELERATED) ? ", accelerated" : "",
                    (info.flags & SDL_RENDERER_PRESENTVSYNC) ? ", vsync" : "");
    }
    /* Important: Set the "logical size" of the rendering context. At the same
       time this also defines the aspect ratio that is preserved while scaling
       and stretching the texture into the window.
    */
    SDL_RenderSetLogicalSize(video.renderer, video.screen->w, video.actualh);

    /* Force integer scales for resolution-independent rendering. */
#if SDL_VERSION_ATLEAST(2, 0, 5)
    SDL_RenderSetIntegerScale(video.renderer, hw_opt_int_scaling);
#endif

    /* Blank out the full screen area in case there is any junk in
       the borders that won't otherwise be overwritten.
    */
    SDL_SetRenderDrawColor(video.renderer, 0, 0, 0, 255);
    SDL_RenderClear(video.renderer);
    SDL_RenderPresent(video.renderer);

    /* Create the 32-bit RGBA screenbuffer surface. */
    /* Format of interbuffer must match the screen pixel format because we
       import the surface data into the texture.
    */
    if (video.interbuffer != NULL) {
        SDL_FreeSurface(video.interbuffer);
        video.interbuffer = NULL;
    }
    if (video.interbuffer == NULL) {
        unsigned int rmask, gmask, bmask, amask;
        int unused_bpp;
        SDL_PixelFormatEnumToMasks(video.pixel_format, &unused_bpp, &rmask, &gmask, &bmask, &amask);
        video.interbuffer = SDL_CreateRGBSurface(0, video.screen->w, video.screen->h, 32, rmask, gmask, bmask, amask);
        SDL_FillRect(video.interbuffer, NULL, 0);
    }
    if (video.texture != NULL) {
        SDL_DestroyTexture(video.texture);
    }
    /* Set the scaling quality for rendering the intermediate texture into
       the upscaled texture to "nearest", which is gritty and pixelated and
       resembles software scaling pretty well.
    */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    /* Create the intermediate texture that the RGBA surface gets loaded into.
       The SDL_TEXTUREACCESS_STREAMING flag means that this texture's content
       is going to change frequently.
    */
    video.texture = SDL_CreateTexture(video.renderer,
                                video.pixel_format,
                                SDL_TEXTUREACCESS_STREAMING,
                                video.screen->w, video.screen->h);

    /* Initially create the upscaled texture for rendering to screen */
    video_create_upscaled_texture(true);
    return 0;
}

/* -------------------------------------------------------------------------- */

int hw_video_get_window_id(void)
{
    return SDL_GetWindowID(video.window);
}

int hw_video_resize(int w, int h)
{
    video.need_resize = true;
    video.last_resize_time = SDL_GetTicks();
    return 0;
}

int hw_video_toggle_fullscreen(void)
{
    unsigned int flags = 0;

    // TODO: Consider implementing fullscreen toggle for SDL_WINDOW_FULLSCREEN
    // (mode-changing) setup. This is hard because we have to shut down and
    // restart again.
    if (hw_opt_screen_fsw && hw_opt_screen_fsh) {
        return 0;
    }
    hw_opt_fullscreen = !hw_opt_fullscreen;
    if (hw_opt_fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    SDL_SetWindowFullscreen(video.window, flags);
    if (!hw_opt_fullscreen) {
        hw_video_resize(0, 0);
    }
    return 0;
}

int hw_video_init(int w, int h)
{
    video.window = NULL;
    video.renderer = NULL;
    video.screen = NULL;
    video.interbuffer = NULL;
    video.texture = NULL;
    video.texture_upscaled = NULL;
    video.display = 0;
    video.w_upscale = 0;
    video.h_upscale = 0;
    video.need_resize = false;
    video.last_resize_time = 0;
    i_hw_video.setmode = video_sw_set;
    i_hw_video.render = video_render;
    i_hw_video.update = video_update;
    i_hw_video.setpal = video_setpal;

    /* Create the 8-bit paletted surface. */
    video.screen = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
    SDL_FillRect(video.screen, NULL, 0);
    video.blit_rect.x = 0;
    video.blit_rect.y = 0;
    video.blit_rect.w = w;
    video.blit_rect.h = h;

    if (hw_opt_aspect_ratio_correct) {
        video.actualh = h * 6 / 5;
        h = video.actualh;
    } else {
        video.actualh = h;
    }
    if ((hw_opt_screen_winw != 0) && (hw_opt_screen_winh != 0)) {
        w = hw_opt_screen_winw;
        h = hw_opt_screen_winh;
    }
    if (i_hw_video.setmode(w, h)) {
        return -1;
    }
    return 0;
}

void hw_video_shutdown(void)
{
    if (video.renderer) {
        SDL_DestroyRenderer(video.renderer);
        video.renderer = NULL;
        video.texture = NULL;
        video.texture_upscaled = NULL;
    }
    if (video.window) {
        SDL_DestroyWindow(video.window);
        video.window = NULL;
    }
    if (video.screen) {
        SDL_FreeSurface(video.screen);
        video.screen = NULL;
    }
    if (video.interbuffer) {
        SDL_FreeSurface(video.interbuffer);
        video.interbuffer = NULL;
    }
}

void hw_video_input_grab(bool grab)
{
    SDL_SetWindowGrab(video.window, grab);
    if (hw_opt_relmouse) {
        SDL_SetRelativeMouseMode(grab);
    }
}

void hw_video_position_cursor(int mx, int my)
{
#if SDL_VERSION_ATLEAST(2, 0, 18)
    if (!hw_opt_relmouse && hw_mouse_enabled) {
        int x, y;
        float xcheck, ycheck;
        if (hw_opt_aspect_ratio_correct) {
            my = my * 6 / 5;
        }
        SDL_RenderLogicalToWindow(video.renderer, mx, my, &x, &y);
        SDL_RenderWindowToLogical(video.renderer, x, y, &xcheck, &ycheck);
        if (mx > xcheck) {
            ++x;
        }
        if (my > ycheck) {
            ++y;
        }
        SDL_WarpMouseInWindow(video.window, x, y);
    }
#endif
}

#include "hw_video.c"
