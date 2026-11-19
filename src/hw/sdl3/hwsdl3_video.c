#include "config.h"

#include <string.h>

#include <SDL3/SDL.h>

#include "hw_internal.h"

#include "hwsdl3_opt.h"
#include "hwsdl3_video.h"

#define RESIZE_DELAY 500

static struct sdl_video_s {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *screen;
    SDL_Texture *texture;
    SDL_Texture *texture_upscaled;
    SDL_Palette *palette;
    SDL_Rect blit_rect;
    SDL_DisplayID display;
    int w_upscale;
    int h_upscale;
    int actualw;
    int actualh;
    bool screen_visible;
    bool need_resize;
    Uint64 last_resize_time;
    SDL_Color color[256];
    bool palette_to_set;
} video = { 0 };

bool hw_mouse_enabled = false;
bool hw_use_mouse = true;

static void video_create_upscaled_texture(bool force)
{
    int w, h;
    int src_w, src_h;
    int h_upscale, w_upscale;

    if (!SDL_GetCurrentRenderOutputSize(video.renderer, &w, &h)) {
        log_fatal_and_die("SDL3: Failed to get renderer output size: %s\n", SDL_GetError());
    }
    src_w = video.screen->w;
    src_h = video.screen->h;
    if (!hw_opt_smooth_pixel_scaling || ((w % src_w == 0) && (h % src_h == 0))) {
        if (video.texture_upscaled) {
            SDL_DestroyTexture(video.texture_upscaled);
            video.texture_upscaled = NULL;
        }
        return;
    }
    if (w * video.actualh < h * video.actualw) {
        h = (w * video.actualh) / video.actualw;
    } else {
        w = (h * video.actualw) / video.actualh;
    }
    w_upscale = (w + src_w - 1) / src_w;
    h_upscale = (h + src_h - 1) / src_h;
    if (w_upscale < 1) {
        w_upscale = 1;
    }
    if (h_upscale < 1) {
        h_upscale = 1;
    }
    if (!force && video.texture_upscaled
      && h_upscale == video.h_upscale && w_upscale == video.w_upscale) {
        return;
    }
    video.h_upscale = h_upscale;
    video.w_upscale = w_upscale;
    if (video.texture_upscaled) {
        SDL_DestroyTexture(video.texture_upscaled);
        video.texture_upscaled = NULL;
    }
    log_message("SDL_CreateTexture: upscaled, %d, %d\n",
                w_upscale * src_w, h_upscale * src_h);
    video.texture_upscaled = SDL_CreateTexture(video.renderer,
                                 SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_TARGET,
                                 w_upscale * src_w,
                                 h_upscale * src_h);
    if (!video.texture_upscaled) {
        log_error("SDL3: Error creating upscaled texture: %s\n", SDL_GetError());
        return;
    }
    SDL_SetTextureScaleMode(video.texture_upscaled, SDL_SCALEMODE_LINEAR);
}

static void video_render(void)
{
    if (!video.screen) {
        return;
    }
    if (video.screen->pixels != vgabuf_get_front()) {
        video.screen->pixels = vgabuf_get_front();
        video.palette_to_set = true;
    }
}

static void video_adjust_window_size(int *wptr, int *hptr)
{
    int w = *wptr;
    int h = *hptr;
    if (hw_opt_int_scaling) {
        int scale = w / video.actualw;
        if ((h / video.actualh) < scale) {
            scale = h / video.actualh;
        }
        if (scale < 1) {
            scale = 1;
        }
        w = video.actualw * scale;
        h = video.actualh * scale;
    } else if ((w * video.actualh) <= (h * video.actualw)) {
        h = (w * video.actualh + video.actualw - 1) / video.actualw;
    } else {
        w = (h * video.actualw) / video.actualh;
    }
    *wptr = w;
    *hptr = h;
}

static void video_update(void)
{
    SDL_Surface *interbuffer;

    if (!video.screen_visible) {
        return;
    }
    if (video.need_resize) {
        if (SDL_GetTicks() <= (video.last_resize_time + RESIZE_DELAY)) {
            return;
        }
        if (!(SDL_GetWindowFlags(video.window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED))) {
            int w, h;
            if (SDL_GetWindowSize(video.window, &w, &h)) {
                video_adjust_window_size(&w, &h);
                log_message("SDL_SetWindowSize: %d, %d\n", w, h);
                SDL_SetWindowSize(video.window, w, h);
                opt_screen_winw = w;
                opt_screen_winh = h;
            }
        }
        video_create_upscaled_texture(false);
        video.need_resize = false;
        video.palette_to_set = true;
    }
    if (!video.texture) {
        return;
    }
    if (video.palette_to_set) {
        SDL_SetPaletteColors(video.palette, video.color, 0, 256);
        video.palette_to_set = false;
    }
    interbuffer = NULL;
    if (!SDL_LockTextureToSurface(video.texture, &video.blit_rect, &interbuffer)) {
        log_error("SDL3: SDL_LockTextureToSurface failed: %s\n", SDL_GetError());
        return;
    }
    if (!SDL_BlitSurfaceUnchecked(video.screen, &video.blit_rect, interbuffer, &video.blit_rect)) {
        log_error("SDL3: SDL_BlitSurfaceUnchecked failed: %s\n", SDL_GetError());
    }
    SDL_UnlockTexture(video.texture);
    SDL_SetRenderDrawColor(video.renderer, 0, 0, 0, 255);
    SDL_RenderClear(video.renderer);
    if (hw_opt_smooth_pixel_scaling && video.texture_upscaled) {
        SDL_SetRenderTarget(video.renderer, video.texture_upscaled);
        SDL_RenderTexture(video.renderer, video.texture, NULL, NULL);
        SDL_SetRenderTarget(video.renderer, NULL);
        SDL_RenderTexture(video.renderer, video.texture_upscaled, NULL, NULL);
    } else {
        SDL_SetRenderTarget(video.renderer, NULL);
        SDL_RenderTexture(video.renderer, video.texture, NULL, NULL);
    }
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

void hw_video_set_visible(bool visible)
{
    video.screen_visible = visible;
}

static bool video_get_display_bounds(SDL_Rect *bounds)
{
    if (SDL_GetDisplayBounds(video.display, bounds)) {
        return true;
    }
    log_warning("SDL3: Failed to read display bounds for display #%u!\n", (unsigned int)video.display);
    video.display = SDL_GetPrimaryDisplay();
    if (SDL_GetDisplayBounds(video.display, bounds)) {
        return true;
    }
    log_error("SDL3: Failed to read display bounds for primary display: %s\n", SDL_GetError());
    return false;
}

static void video_get_window_position(int *x, int *y, int w, int h)
{
    SDL_Rect bounds;

    *x = SDL_WINDOWPOS_UNDEFINED;
    *y = SDL_WINDOWPOS_UNDEFINED;
    if (!opt_fullscreen || !video_get_display_bounds(&bounds)) {
        return;
    }
    if (bounds.w > w) {
        *x = bounds.x + ((bounds.w - w) / 2);
    } else {
        *x = bounds.x;
    }
    if (bounds.h > h) {
        *y = bounds.y + ((bounds.h - h) / 2);
    } else {
        *y = bounds.y;
    }
}

static SDL_Renderer *video_create_renderer(void)
{
    SDL_PropertiesID props;
    SDL_Renderer *renderer;
    const char *renderer_name;

    props = SDL_CreateProperties();
    if (props == 0) {
        log_error("SDL3: SDL_CreateProperties failed: %s\n", SDL_GetError());
        return NULL;
    }
    if (!SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, video.window)
      || !SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER,
                                (!opt_force_sw && hw_opt_vsync) ? 1 : 0)) {
        log_error("SDL3: SDL_SetRendererCreateProperty failed: %s\n", SDL_GetError());
        SDL_DestroyProperties(props);
        return NULL;
    }
    renderer_name = opt_force_sw ? "software" : NULL;
    if (renderer_name && !SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, renderer_name)) {
        log_error("SDL3: SDL_SetRendererCreateProperty failed: %s\n", SDL_GetError());
        SDL_DestroyProperties(props);
        return NULL;
    }
    renderer = SDL_CreateRendererWithProperties(props);
    SDL_DestroyProperties(props);
    if (!renderer) {
        log_error("SDL3: Error creating renderer for screen window: %s\n", SDL_GetError());
        return NULL;
    }
    log_message("SDL_GetRendererName: %s\n", SDL_GetRendererName(renderer));
    return renderer;
}

static void video_set_fullscreen_mode(void)
{
    if (opt_screen_fsw && opt_screen_fsh) {
        SDL_DisplayMode mode;
        memset(&mode, 0, sizeof(mode));
        if (SDL_GetClosestFullscreenDisplayMode(video.display, opt_screen_fsw, opt_screen_fsh, 0.0f, false, &mode)) {
            if (!SDL_SetWindowFullscreenMode(video.window, &mode)) {
                log_warning("SDL3: SDL_SetWindowFullscreenMode failed: %s\n", SDL_GetError());
            }
            return;
        }
        log_warning("SDL3: SDL_GetClosestFullscreenDisplayMode failed: %s\n", SDL_GetError());
    }
    if (!SDL_SetWindowFullscreenMode(video.window, NULL)) {
        log_warning("SDL3: SDL_SetWindowFullscreenMode failed: %s\n", SDL_GetError());
    }
}

static int video_sw_set(int w, int h)
{
    Uint64 window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_RendererLogicalPresentation presentation;
    int x, y;

    if (hw_opt_borderless) {
        window_flags |= SDL_WINDOW_BORDERLESS;
    }
    if (opt_fullscreen) {
        window_flags |= SDL_WINDOW_HIDDEN;
    }
    video_get_window_position(&x, &y, w, h);
    if (!video.window) {
        log_message("SDL_CreateWindow(%i, %i, 0x%llx)\n", w, h, (unsigned long long)window_flags);
        video.window = SDL_CreateWindow("1oom", w, h, window_flags);
        if (!video.window) {
            log_error("SDL_CreateWindow failed: %s\n", SDL_GetError());
            return -1;
        }
        if (!SDL_SetWindowPosition(video.window, x, y)) {
            log_warning("SDL3: SDL_SetWindowPosition failed: %s\n", SDL_GetError());
        }
        SDL_SetWindowMinimumSize(video.window, video.actualw, video.actualh);
    }
    if (opt_fullscreen) {
        video_set_fullscreen_mode();
        if (!SDL_SetWindowFullscreen(video.window, true)) {
            log_error("SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
        }
        SDL_ShowWindow(video.window);
    }
    if (video.renderer) {
        SDL_DestroyRenderer(video.renderer);
        video.renderer = NULL;
        video.texture = NULL;
        video.texture_upscaled = NULL;
    }
    video.renderer = video_create_renderer();
    if (!video.renderer) {
        return -1;
    }
    presentation = hw_opt_int_scaling ? SDL_LOGICAL_PRESENTATION_INTEGER_SCALE : SDL_LOGICAL_PRESENTATION_LETTERBOX;
    if (!SDL_SetRenderLogicalPresentation(video.renderer, video.actualw, video.actualh, presentation)) {
        log_error("SDL3: SDL_SetRenderLogicalPresentation failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetRenderDrawColor(video.renderer, 0, 0, 0, 255);
    SDL_RenderClear(video.renderer);
    SDL_RenderPresent(video.renderer);
    video.texture = SDL_CreateTexture(video.renderer,
                                 SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING,
                                 video.screen->w,
                                 video.screen->h);
    if (!video.texture) {
        log_error("SDL3: Error creating texture: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetTextureScaleMode(video.texture, SDL_SCALEMODE_NEAREST);
    video.w_upscale = 0;
    video.h_upscale = 0;
    video_create_upscaled_texture(true);
    return 0;
}

static void video_position_cursor(int mx, int my)
{
    float x, y;
    float xcheck, ycheck;

    if (!hw_opt_relmouse && hw_mouse_enabled && video.renderer) {
        if (opt_aspect_ratio_correct) {
            my = my * 6 / 5;
        }
        SDL_RenderCoordinatesToWindow(video.renderer, (float)mx, (float)my, &x, &y);
        SDL_RenderCoordinatesFromWindow(video.renderer, x, y, &xcheck, &ycheck);
        if ((float)mx > xcheck) {
            x += 1.0f;
        }
        if ((float)my > ycheck) {
            y += 1.0f;
        }
        SDL_WarpMouseInWindow(video.window, x, y);
    }
}

uint32_t hw_video_get_window_id(void)
{
    if (!video.window) {
        return 0;
    }
    return (uint32_t)SDL_GetWindowID(video.window);
}

int hw_video_resize(int w, int h)
{
    (void)w;
    (void)h;
    video.need_resize = true;
    video.last_resize_time = SDL_GetTicks();
    return 0;
}

int hw_video_toggle_fullscreen(void)
{
    if (opt_screen_fsw && opt_screen_fsh) {
        return 0;
    }
    opt_fullscreen = !opt_fullscreen;
    video_set_fullscreen_mode();
    if (!SDL_SetWindowFullscreen(video.window, opt_fullscreen)) {
        log_error("SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
        return -1;
    }
    hw_video_resize(0, 0);
    return 0;
}

int hw_video_init(int w, int h)
{
    video.window = NULL;
    video.renderer = NULL;
    video.screen = NULL;
    video.texture = NULL;
    video.texture_upscaled = NULL;
    video.palette = NULL;
    video.display = SDL_GetPrimaryDisplay();
    video.w_upscale = 0;
    video.h_upscale = 0;
    video.screen_visible = true;
    video.need_resize = false;
    video.last_resize_time = 0;
    video.palette_to_set = false;
    memset(video.color, 0, sizeof(video.color));
    i_hw_video.setmode = video_sw_set;
    i_hw_video.render = video_render;
    i_hw_video.update = video_update;
    i_hw_video.setpal = video_setpal;
    i_hw_video.position_cursor = video_position_cursor;

    video.screen = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_INDEX8, vgabuf_get_front(), w);
    if (!video.screen) {
        log_error("SDL3: SDL_CreateSurfaceFrom failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_FillSurfaceRect(video.screen, NULL, 0);
    video.palette = SDL_CreateSurfacePalette(video.screen);
    if (!video.palette) {
        log_error("SDL3: SDL_CreateSurfacePalette failed: %s\n", SDL_GetError());
        SDL_DestroySurface(video.screen);
        video.screen = NULL;
        return -1;
    }
    SDL_SetPaletteColors(video.palette, video.color, 0, 256);
    video.blit_rect.x = 0;
    video.blit_rect.y = 0;
    video.blit_rect.w = w;
    video.blit_rect.h = h;
    video.actualw = w;
    if (opt_aspect_ratio_correct) {
        video.actualh = h * 6 / 5;
        h = video.actualh;
    } else {
        video.actualh = h;
    }
    if ((opt_screen_winw != 0) && (opt_screen_winh != 0)) {
        w = opt_screen_winw;
        h = opt_screen_winh;
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
    } else {
        if (video.texture) {
            SDL_DestroyTexture(video.texture);
            video.texture = NULL;
        }
        if (video.texture_upscaled) {
            SDL_DestroyTexture(video.texture_upscaled);
            video.texture_upscaled = NULL;
        }
    }
    if (video.window) {
        SDL_DestroyWindow(video.window);
        video.window = NULL;
    }
    if (video.screen) {
        SDL_DestroySurface(video.screen);
        video.screen = NULL;
        video.palette = NULL;
    }
}

int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h)
{
    SDL_Color color[256];
    SDL_Surface *icon;
    SDL_Palette *palette;
    Uint8 *p;

    icon = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_INDEX8);
    if (!icon) {
        log_error("Icon: SDL_CreateSurface failed!\n");
        return -1;
    }
    p = (Uint8 *)icon->pixels;
    for (int y = 0; y < h; ++y) {
        memcpy(p, data, (size_t)w);
        data += w;
        p += icon->pitch;
    }
    for (int i = 0; i < 256; ++i) {
        color[i].r = vgapal_6bit_to_8bit(*pal++);
        color[i].g = vgapal_6bit_to_8bit(*pal++);
        color[i].b = vgapal_6bit_to_8bit(*pal++);
        color[i].a = 255;
    }
    color[0].a = 0;
    palette = SDL_CreatePalette(256);
    if (!palette) {
        log_error("Icon: SDL_CreatePalette failed!\n");
        SDL_DestroySurface(icon);
        return -1;
    }
    if (!SDL_SetPaletteColors(palette, color, 0, 256)
      || !SDL_SetSurfacePalette(icon, palette)
      || !SDL_SetWindowIcon(video.window, icon)) {
        log_error("Icon: failed to set window icon: %s\n", SDL_GetError());
        SDL_DestroyPalette(palette);
        SDL_DestroySurface(icon);
        return -1;
    }
    SDL_DestroyPalette(palette);
    SDL_DestroySurface(icon);
    return 0;
}

void hw_mouse_grab(void)
{
    bool grab;

    if (hw_mouse_enabled) {
        return;
    }
    hw_mouse_enabled = true;
    grab = hw_opt_relmouse || !hw_opt_nograbmouse;
    SDL_HideCursor();
    SDL_SetWindowKeyboardGrab(video.window, grab);
    SDL_SetWindowMouseGrab(video.window, grab);
    SDL_SetWindowRelativeMouseMode(video.window, hw_opt_relmouse);
}

void hw_mouse_ungrab(void)
{
    if (!hw_mouse_enabled) {
        return;
    }
    hw_mouse_enabled = false;
    SDL_SetWindowRelativeMouseMode(video.window, false);
    SDL_SetWindowMouseGrab(video.window, false);
    SDL_SetWindowKeyboardGrab(video.window, false);
    SDL_ShowCursor();
}

void hw_mouse_toggle_grab(void)
{
    if (hw_mouse_enabled) {
        hw_mouse_ungrab();
    } else {
        hw_mouse_grab();
    }
}
