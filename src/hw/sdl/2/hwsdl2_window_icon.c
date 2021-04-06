#include "SDL.h"
#include "hw.h"
#include "hwsdl2_window_icon.h"
#include "log.h"

extern const uint8_t sixbit_to_8bit[64];
static SDL_Surface *icon = NULL;

void hwsdl_set_icon(struct SDL_Window *window)
{
    if (icon) {
        SDL_SetWindowIcon(window, icon);
    }
}

void hwsdl_delete_icon(void)
{
    if (icon) {
        SDL_FreeSurface(icon);
        icon = NULL;
    }
}

int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h)
{
    SDL_Color color[256];
    Uint8 *p;
    int y;

    icon = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
    if (!icon) {
        log_error("Icon: SDL_CreateRGBSurface failed!\n");
        return -1;
    }

    p = (Uint8 *)icon->pixels;
    for (y = 0; y < h; ++y) {
        memcpy(p, data, w);
        data += w;
        p += icon->pitch;
    }

    for (int i = 0; i < 256; ++i) {
        color[i].r = sixbit_to_8bit[*pal++];
        color[i].g = sixbit_to_8bit[*pal++];
        color[i].b = sixbit_to_8bit[*pal++];
        color[i].a = 255;
    }

    /* Set the first color transparent. Maybe not a good idea if the background is dark
     * But then again color 0 happens to be black so with a=255 we'd get dark blue on black. */
    color[0].a = 0;

    if (SDL_SetPaletteColors(icon->format->palette, color, 0, 256)) {
        log_error("Icon: SetPaletteColors failed!\n");
        SDL_FreeSurface(icon);
        return -1;
    }

    return 0;
}

