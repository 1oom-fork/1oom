#include "SDL.h"
#include "SDL_pixels.h"
#include "types.h"
#include "log.h"
#include "hwsdl_opt.h"
#include "hwsdl_mouse.h"
#include "hwsdl_aspect.h"
#include "hwsdl2_video_buffers.h"
#include "hwsdl2_video_texture.h"

static uint32_t temp_mem_static[MAX_VIDEO_X * MAX_VIDEO_Y];
static SDL_Texture *texture = NULL;
size_t texture_w=0, texture_h=0;

static void setup_texture(SDL_Renderer *rdr, uint32_t pixfmt, size_t w, size_t h)
{
    if (texture) {
        if (texture_w == w && texture_h == h) {
            return;
        }
        SDL_DestroyTexture(texture);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, hw_opt_texture_filtering ? "best" : "nearest");
    texture = SDL_CreateTexture(rdr, pixfmt, SDL_TEXTUREACCESS_STREAMING, w, h);
    texture_w = w;
    texture_h = h;

    if (!texture) {
        log_fatal_and_die("failed to create %dx%d texture\n", w, h);
    }
}

static void write_output(const hwsdl_video_buffer_t *buf, void *dst, size_t dst_pitch)
{
    const uint8_t *src = buf->pixels;
    size_t y, x, w=buf->w, h=buf->h;
    for(y=0; y<h; ++y, src += w) {
        uint32_t *dst_line = (uint32_t*)((uint8_t*) dst + y * dst_pitch);
        for(x=0; x<w; ++x) {
            dst_line[x] = buf->palette[src[x]];
        }
    }
}

void hwsdl_texture_update(SDL_Renderer *rdr, const hwsdl_video_buffer_t *buf)
{
    struct _pixelformat_assertion_ {
        int compile_error_if_pixelformat_not_4_bytes :
            ( SDL_BYTESPERPIXEL(VIDEO_TEXTURE_FORMAT) == 4 ? 1 : -1 );
    };

    setup_texture(rdr, VIDEO_TEXTURE_FORMAT, buf->w, buf->h);

    if (hw_opt_lock_texture) {
        void *pixels;
        int pitch;
        if (SDL_LockTexture(texture, NULL, &pixels, &pitch) == 0) {
            write_output(buf, pixels, pitch);
            SDL_UnlockTexture(texture);
        } else {
            log_error("SDL_LockTexture(): %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }
    } else {
        /* this was faster in my testing */
        int pitch = buf->w * 4;
        write_output(buf, temp_mem_static, pitch);
        SDL_UpdateTexture(texture, NULL, temp_mem_static, pitch);
    }
}

void hwsdl_texture_output(SDL_Renderer *rdr)
{
    SDL_Rect box = {0};
    int rdr_w, rdr_h;

    SDL_GetRendererOutputSize(rdr, &rdr_w, &rdr_h);
    
    if (hw_opt_aspect != 0) {
        box.w = (int64_t) texture_h * 1000000 / hw_opt_aspect;
        box.h = texture_h;

        if (rdr_w < box.w || rdr_h < box.h) {
            /* if int_scaling is enabled and the window is smaller than logical coords
             * then SDL computes the viewport to be -2147483648x-2147483648
             * so make sure the logical coords are smaller than output size. */
            box.w = rdr_w;
            box.h = rdr_h;
            shrink_to_aspect_ratio(&box.w, &box.h, 1000000, hw_opt_aspect);
        }

        SDL_RenderSetLogicalSize(rdr, box.w, box.h);
    } else {
        /* aspect disabled */
        SDL_RenderSetLogicalSize(rdr, rdr_w, rdr_h); /* nonzero w,h for the mouse coordinates */
        box.w = rdr_w;
        box.h = rdr_h;
    }

    /* what the user clicks has to match what was drawn on screen.
     * thus the best place to set mouse coordinate range is right here
     * NOTE box.x and box.y are 0 because SDL deals with repositioning of the picture */
    hw_mouse_set_win_range(box.x, box.y, box.w, box.h);

    SDL_SetRenderTarget(rdr, NULL);
    SDL_RenderCopy(rdr, texture, NULL, &box);
}

void hwsdl_texture_delete(void) {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
        texture_w = texture_h = 0;
    }
}

