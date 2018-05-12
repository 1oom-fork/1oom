#include "config.h"

#include <stdio.h>
#include <string.h>

#include "SDL.h"

#include "hw.h"
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
    SDL_Surface *rgbabuffer;
    SDL_Texture *texture;
    SDL_Texture *texture_upscaled;

    SDL_Rect blit_rect;
    uint32_t pixel_format;

    /* SDL display number on which to run. */
    int display;

    int w_upscale, h_upscale;
    int actualheight;

    void (*render)(void);
    void (*update)(void);
    void (*setpal)(uint8_t *pal, int first, int num);

    uint8_t *buf;
    int bufw;
    int bufh;

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

    /* When the screen or window dimensions do not match the aspect ratio
       of the texture, the rendered area is scaled down to fit. Calculate
       the actual dimensions of the rendered area.
    */
    if (w * video.actualheight < h * video.bufw) {
        /* Tall window. */
        h = w * video.actualheight / video.bufw;
    } else {
        /* Wide window. */
        w = h * video.bufw / video.actualheight;
    }

    /* Pick texture size the next integer multiple of the screen dimensions.
       If one screen dimension matches an integer multiple of the original
       resolution, there is no need to overscale in this direction.
    */
    w_upscale = (w + video.bufw - 1) / video.bufw;
    h_upscale = (h + video.bufh - 1) / video.bufh;

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
                                w_upscale * video.bufw,
                                h_upscale * video.bufh
                             );

}

/* -------------------------------------------------------------------------- */

static void video_render(void)
{
    int pitch = video.screen->pitch;
    Uint8 *p = (Uint8 *)video.screen->pixels;
    uint8_t *q = video.buf;
    for (int y = 0; y < video.bufh; ++y) {
        memcpy(p, q, video.bufw);
        p += pitch;
        q += video.bufw;
    }
}

static void video_update(void)
{
    /* TODO SDL2 need resize */
    if (video.palette_to_set) {
        SDL_SetPaletteColors(video.screen->format->palette, video.color, 0, 256);
        video.palette_to_set = false;
    }

    /* Blit from the paletted 8-bit screen buffer to the intermediate
       32-bit RGBA buffer that we can load into the texture.
    */
    SDL_LowerBlit(video.screen, &video.blit_rect, video.rgbabuffer, &video.blit_rect);

    /* Update the intermediate texture with the contents of the RGBA buffer. */
    SDL_UpdateTexture(video.texture, NULL, video.rgbabuffer->pixels, video.rgbabuffer->pitch);

    /* Make sure the pillarboxes are kept clear each frame. */
    SDL_RenderClear(video.renderer);

    /* Render this intermediate texture into the upscaled texture
       using "nearest" integer scaling.
    */
    SDL_SetRenderTarget(video.renderer, video.texture_upscaled);
    SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);

    /* Finally, render this upscaled texture to screen using linear scaling. */
    SDL_SetRenderTarget(video.renderer, NULL);
    SDL_RenderCopy(video.renderer, video.texture_upscaled, NULL, NULL);

    /* Draw! */
    SDL_RenderPresent(video.renderer);
}

static void video_setpal(uint8_t *pal, int first, int num)
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

    if (!hw_opt_aspect) {
        video.actualheight = video.bufh;
    } else {
        video.actualheight = (uint32_t)(video.bufh * 1000000) / hw_opt_aspect;
        SETMAX(h, video.actualheight);
    }

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
    video.blit_rect.x = 0;
    video.blit_rect.y = 0;
    video.blit_rect.w = video.bufw;
    video.blit_rect.h = video.bufh;
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
        SDL_SetWindowMinimumSize(video.window, video.bufw, video.actualheight);
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
    renderer_flags |= SDL_RENDERER_PRESENTVSYNC;

    if (hw_opt_force_sw) {
        renderer_flags |= SDL_RENDERER_SOFTWARE;
        renderer_flags &= ~SDL_RENDERER_PRESENTVSYNC;
    }

    if (video.renderer) {
        SDL_DestroyRenderer(video.renderer);
    }
    video.renderer = SDL_CreateRenderer(video.window, -1, renderer_flags);
    if (video.renderer == NULL) {
        log_error("SDL2: Error creating renderer for screen window: %s\n", SDL_GetError());
        return -1;
    }
    /* Important: Set the "logical size" of the rendering context. At the same
       time this also defines the aspect ratio that is preserved while scaling
       and stretching the texture into the window.
    */
    SDL_RenderSetLogicalSize(video.renderer, video.bufw, video.actualheight);

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

    /* Create the 8-bit paletted and the 32-bit RGBA screenbuffer surfaces. */
    if (video.screen == NULL) {
        video.screen = SDL_CreateRGBSurface(0, video.bufw, video.bufh, 8, 0, 0, 0, 0);
        SDL_FillRect(video.screen, NULL, 0);
    }
    /* Format of rgbabuffer must match the screen pixel format because we
       import the surface data into the texture.
    */
    if (video.rgbabuffer == NULL) {
        unsigned int rmask, gmask, bmask, amask;
        int unused_bpp;
        SDL_PixelFormatEnumToMasks(video.pixel_format, &unused_bpp, &rmask, &gmask, &bmask, &amask);
        video.rgbabuffer = SDL_CreateRGBSurface(0, video.bufw, video.bufw, 32, rmask, gmask, bmask, amask);
        SDL_FillRect(video.rgbabuffer, NULL, 0);
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
                                video.bufw, video.bufh);

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
    /* TODO SDL 2 */
    return 0;
}

int hw_video_toggle_fullscreen(void)
{
    log_warning("%s: unimpl\n", __func__);
#if 0
    /* TODO SDL2 */
    unsigned int flags = 0;
    hw_opt_fullscreen ^= 1;
    if (hw_opt_fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    SDL_SetWindowFullscreen(video.window, flags);
    if (!hw_opt_fullscreen) {
        AdjustWindowSize();
        SDL_SetWindowSize(video.window, window_width, window_height);
    }
    if (func(hw_opt_screen_winw, hw_opt_screen_winh)) {
        hw_opt_fullscreen ^= 1; /* restore the setting for the config file */
        return -1;
    }
#endif
    return 0;
}

int hw_video_init(int w, int h)
{
    hw_mouse_set_limits(w, h);
    video.buf = vgabuf_get_front();
    video.bufw = w;
    video.bufh = h;
    video.window = NULL;
    video.renderer = NULL;
    video.screen = NULL;
    video.rgbabuffer = NULL;
    video.texture = NULL;
    video.texture_upscaled = NULL;
    video.display = 0;
    video.w_upscale = 0;
    video.h_upscale = 0;
    video.render = video_render;
    video.update = video_update;
    video.setpal = video_setpal;
    if ((hw_opt_screen_winw != 0) && (hw_opt_screen_winh != 0)) {
        w = hw_opt_screen_winw;
        h = hw_opt_screen_winh;
    }
    if (video_sw_set(w, h)) {
        return -1;
    }
    return 0;
}

void hw_video_shutdown(void)
{
    if (video.window) {
        SDL_DestroyWindow(video.window);
        video.window = NULL;
    }
    if (video.screen) {
        SDL_FreeSurface(video.screen);
        video.screen = NULL;
    }
    if (video.rgbabuffer) {
        SDL_FreeSurface(video.rgbabuffer);
        video.rgbabuffer = NULL;
    }
    if (video.texture) {
        SDL_DestroyTexture(video.texture);
        video.texture = NULL;
    }
    if (video.texture_upscaled) {
        SDL_DestroyTexture(video.texture_upscaled);
        video.texture_upscaled = NULL;
    }
    video.buf = NULL;
}

void hw_video_input_grab(bool grab)
{
    SDL_SetWindowGrab(video.window, grab);
    SDL_SetRelativeMouseMode(grab);
}

#include "hwsdl_video.c"
