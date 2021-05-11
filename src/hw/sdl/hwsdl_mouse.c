#include "config.h"

#include "SDL.h"
#include "hw.h"
#include "comp.h"
#include "hwsdl_mouse.h"
#include "hwsdl_opt.h"
#include "log.h"
#include "mouse.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

static struct hwsdl_mouse_s {
    int win_x, win_y;
    int moo_x, moo_y;
    int buttons;
} hwsdl_mouse = {0};

#define M hwsdl_mouse

static int moo_range[2] = {0};
static SDL_Rect win_range = {0, 0, 1, 1};

/* -------------------------------------------------------------------------- */

void window_to_moo(int win_x, int win_y, int *moo_x, int *moo_y)
{
    if (win_range.w <= 1 || win_range.h <= 1) {
        *moo_x = *moo_y = 0; /* avoid division by zero */
        return;
    }
    int x = (win_x - win_range.x) * (moo_range[0]-1) / (win_range.w-1);
    int y = (win_y - win_range.y) * (moo_range[1]-1) / (win_range.h-1);
    /* the game shouldn't crash due to weird mouse coordinates but clamp anyway */
    *moo_x = MAX(0, MIN(x, moo_range[0]-1));
    *moo_y = MAX(0, MIN(y, moo_range[1]-1));
}

void moo_to_window(int moo_x, int moo_y, const struct SDL_Rect *win, int *win_x, int *win_y)
{
    if (!win) {
        /* can't use win_range for SDL2 because SDL_WarpMouseInWindow
         * uses window coordinates, not logical coordinates */
        win = &win_range;
    }
    if (moo_range[0] <= 1 || moo_range[1] <= 1) {
        *win_x = *win_y = 0; /* avoid division by zero */
        return;
    }
    *win_x = win->x + (moo_x * (win->w-1)) / (moo_range[0]-1);
    *win_y = win->y + (moo_y * (win->h-1)) / (moo_range[1]-1);
}

/* called when the window is created. or re-created when toggling fullscreen */
void hw_mouse_init(void)
{
    SDL_ShowCursor(SDL_DISABLE);
}

void hw_mouse_set_moo_range(int w, int h)
{
    moo_range[0] = w;
    moo_range[1] = h;
}

void hw_mouse_set_win_range(int x0, int y0, int w, int h)
{
    win_range.x = x0;
    win_range.y = y0;
    win_range.w = w;
    win_range.h = h;
}

void hw_mouse_set_xy(int x, int y)
{
    M.win_x = x;
    M.win_y = y;
    window_to_moo(x, y, &M.moo_x, &M.moo_y);
    mouse_set_xy_from_hw(M.moo_x, M.moo_y);
}

void hw_mouse_button(int i, int pressed)
{
    static int b = 0;
    if (i == SDL_BUTTON_LEFT) {
        if (pressed) {
            b |= MOUSE_BUTTON_MASK_LEFT;
        } else {
            b &= ~MOUSE_BUTTON_MASK_LEFT;
        }
    } else if (i == SDL_BUTTON_RIGHT) {
        if (pressed) {
            b |= MOUSE_BUTTON_MASK_RIGHT;
        } else {
            b &= ~MOUSE_BUTTON_MASK_RIGHT;
        }
    }
    M.buttons = b;
    mouse_set_buttons_from_hw(b);
}

void hw_mouse_scroll(int scroll)
{
    mouse_set_scroll_from_hw(scroll);
}

#undef M
