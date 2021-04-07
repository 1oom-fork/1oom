#include "SDL.h"
#include "SDL_pixels.h"
#include "hw.h"
#include "log.h"
#include "screenshot.h"
#include "hwsdl2_window.h"
#include "hwsdl2_video_buffers.h"

/* -------------------------------------------------------------------------- */

/* used here and also in hwsdl_window_icon.c */
const uint8_t sixbit_to_8bit[256] = {
0,4,8,12,16,20,24,28,32,36,40,45,49,53,57,61,65,69,73,77,81,85,89,93,97,101,105,109,113,117,121,125,130,134,138,142,146,150,154,158,162,166,170,174,178,182,186,190,194,198,202,206,210,215,219,223,227,231,235,239,243,247,251,255,
#define REP8(x) x,x,x,x,x,x,x,x
#define REP48(x) REP8(x),REP8(x),REP8(x),REP8(x),REP8(x),REP8(x)
#define REP192(x) REP48(x),REP48(x),REP48(x),REP48(x)
REP192(255) /* saturate >6bit values to maximum. even though they should not happen */
};
static uint32_t palette_rgb888[256];
static uint8_t palette_6bit[256*3]; /* 6bit palette from the game */

/* -------------------------------------------------------------------------- */

static bool flag_screenshot = false;

/* double buffering + 2 aux buffers */
#define NUM_VIDEOBUF 4

#define BACK bufi
#define FRONT (bufi^1)
#define COPY_SZ ((size_t)bufw*(size_t)bufh)

/* buffers used by UI */
static uint8_t bufp[NUM_VIDEOBUF][MAX_VIDEO_X*MAX_VIDEO_Y];
static int bufi=0;
static int bufw=0, bufh=0;
static SDL_PixelFormat *pixfmt = NULL;

/* -------------------------------------------------------------------------- */

bool hwsdl_video_buffers_alloc(int w, int h)
{
    if (w >= MAX_VIDEO_X || h >= MAX_VIDEO_Y) {
        log_error("Can't allocate bigger than %dx%d framebuffer\n", w, h);
        return false;
    }
    pixfmt = SDL_AllocFormat(VIDEO_TEXTURE_FORMAT);
    if (!pixfmt) {
    	 log_error("Failed to allocate pixel format\n");
    	 return false;
    }
    bufw = w;
    bufh = h;
    return true;
}

void hwsdl_video_screenshot(void)
{
    flag_screenshot = true;
}

static void do_refresh(int i)
{
    hwsdl_video_buffer_t b;
    b.pixels = bufp[i];
    b.w = bufw;
    b.h = bufh;
    b.palette = palette_rgb888;
    /* send a buffer to be displayed */
    hwsdl_video_next_frame(&b);
}

uint8_t *hw_video_draw_buf(void)
{
    do_refresh(BACK);
    if (flag_screenshot) {
        flag_screenshot = false;
        screenshot_save(bufp[bufi], palette_6bit, bufw, bufh);
    }
    bufi ^= 1;
    return bufp[bufi];
}

static void set_palette_color(int i, uint8_t r, uint8_t g, uint8_t b)
{
    palette_6bit[3*i + 0] = r;
    palette_6bit[3*i + 1] = g;
    palette_6bit[3*i + 2] = b;
    r = sixbit_to_8bit[r];
    g = sixbit_to_8bit[g];
    b = sixbit_to_8bit[b];
    /* using SDL_MapRGB to take care of possible endianness issues */
    palette_rgb888[i] = SDL_MapRGB(pixfmt, r, g, b);
}

void hw_video_set_palette(const uint8_t *pal, int first, int num)
{
    int end = first + num;
    for(int i=first; i<end; ++i) {
        set_palette_color(i, pal[3*i+0], pal[3*i+1], pal[3*i+2]);
    }
    do_refresh(FRONT);
}

/* Gets called in a stupid loop over and over.
 * at the end of the loop there is hw_video_refresh_palette() */
void hw_video_set_palette_byte(int i, uint8_t b)
{
    palette_6bit[i] = b;
}

void hw_video_refresh_palette(void)
{
    hw_video_set_palette(palette_6bit, 0, 256);
    do_refresh(FRONT);
}


static void copy(int to, int from) { memcpy(bufp[to], bufp[from], COPY_SZ); }
void hw_video_redraw_front(void) { do_refresh(FRONT); }
void hw_video_copy_buf_out(uint8_t *out) { memcpy(out, bufp, COPY_SZ); }
void hw_video_copy_buf(void) { copy(BACK, FRONT); }
void hw_video_copy_back_to_page2(void) { copy(2, BACK); }
void hw_video_copy_back_from_page2(void) { copy(BACK, 2); }
void hw_video_copy_back_to_page3(void) { copy(3, BACK); }
void hw_video_copy_back_from_page3(void) { copy(BACK, 3); }
uint8_t *hw_video_get_buf(void) { return bufp[BACK]; }
uint8_t *hw_video_get_buf_front(void) { return bufp[FRONT]; }

