#include <string.h>

#include "hw.h"
#include "vgabuf.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* double buffering + 2 aux buffers */
#define NUM_VIDEOBUF    4

/* FIXME */
#define VGABUF_EXTRA_H    200

#define VGABUF_SIZE_INTERNAL     (VGABUF_W * (VGABUF_H + VGABUF_EXTRA_H))

/* buffers used by UI */
static uint8_t vgabuf[NUM_VIDEOBUF * VGABUF_SIZE_INTERNAL] = {0};
static int16_t vga_page = 0;

static inline uint8_t *vgabuf_get_i(int16_t i)
{
    return &vgabuf[i * VGABUF_SIZE_INTERNAL];
}

/* -------------------------------------------------------------------------- */

void vgabuf_flip(void)
{
    vga_page = 1 - vga_page;
    hw_video_redraw_front();
    /* vgabuf_select_back(); */
}

uint8_t *vgabuf_get_back(void)
{
    return vgabuf_get_i(1 - vga_page);
}

uint8_t *vgabuf_get_front(void)
{
    return vgabuf_get_i(vga_page);
}

void vgabuf_copy_front_to_back(void)
{
    memcpy(vgabuf_get_back(), vgabuf_get_front(), VGABUF_SIZE);
}

void vgabuf_copy_back_out(uint8_t *buf)
{
    memcpy(buf, vgabuf_get_back(), VGABUF_SIZE);
}

void vgabuf_copy_back_to_page2(void)
{
    memcpy(vgabuf_get_i(2), vgabuf_get_back(), VGABUF_SIZE);
}

void vgabuf_copy_back_from_page2(void)
{
    memcpy(vgabuf_get_back(), vgabuf_get_i(2), VGABUF_SIZE);
}

void vgabuf_copy_back_to_page3(void)
{
    memcpy(vgabuf_get_i(3), vgabuf_get_back(), VGABUF_SIZE);
}

void vgabuf_copy_back_from_page3(void)
{
    memcpy(vgabuf_get_back(), vgabuf_get_i(3), VGABUF_SIZE);
}

void vgabuf_fill_rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
    /* HACK: simplified */
    uint8_t *buf = vgabuf_get();
    if (x1 < x0) {
        return;
    }
    for (; y0 <= y1; ++y0) {
        memset(buf + VGABUF_OFFSET(x0, y0), color, x1 - x0 + 1);
    }
}

void vgabuf_put_pixel(int16_t x, int16_t y, uint8_t color)
{
    uint8_t *buf = vgabuf_get();
    buf[VGABUF_OFFSET(x, y)] = color;
}

void vgabuf_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
    int xslope = 0, yslope = 0, yinc, numpixels;    /* BUG? xslope and yslope not cleared by MOO1 */

    if (x1 < x0) {
        int16_t t;
        t = x1; x1 = x0; x0 = t;
        t = y1; y1 = y0; y0 = t;
    }

    {
        int dx, dy;
        dx = x1 - x0;
        dy = y1 - y0;
        yinc = VGABUF_PITCH;
        if (dy < 0) {
            dy = -dy;
            yinc = -VGABUF_PITCH;
        }
        if (dx < dy) {
            numpixels = dy + 1;
            yslope = 0x100;
            if (dy != 0) {
                xslope = (dx << 8) / dy;
            }
        } else {
            numpixels = dx + 1;
            if (dx != 0) {
                xslope = 0x100;
                yslope = (dy << 8) / dx;
            }
        }
    }

    {
        uint8_t *p = vgabuf_get() + VGABUF_OFFSET(x0, y0);
        int xerr, yerr;

        xerr = 0x100 / 2;
        yerr = 0x100 / 2;

        while (numpixels--) {
            *p = color;
            xerr += xslope;
            if ((xerr & 0xff00) != 0) {
                xerr &= 0xff;
                ++p;
            }
            yerr += yslope;
            if ((yerr & 0xff00) != 0) {
                yerr &= 0xff;
                p += yinc;
            }
        }
    }
}

void vgabuf_draw_line_ctbl(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint8_t *colortbl, int colornum, int pos)
{
    int xslope = 0, yslope = 0, yinc, numpixels;    /* BUG? xslope and yslope not cleared by MOO1 */

    if (x1 < x0) {
        int16_t t;
        t = x1; x1 = x0; x0 = t;
        t = y1; y1 = y0; y0 = t;
    }

    {
        int dx, dy;
        dx = x1 - x0;
        dy = y1 - y0;
        yinc = VGABUF_PITCH;
        if (dy < 0) {
            dy = -dy;
            yinc = -VGABUF_PITCH;
        }
        if (dx < dy) {
            numpixels = dy + 1;
            yslope = 0x100;
            if (dy != 0) {
                xslope = (dx << 8) / dy;
            }
        } else {
            numpixels = dx + 1;
            if (dx != 0) {
                xslope = 0x100;
                yslope = (dy << 8) / dx;
            }
        }
    }

    {
        uint8_t *p = vgabuf_get() + VGABUF_OFFSET(x0, y0);
        int xerr, yerr;

        xerr = 0x100 / 2;
        yerr = 0x100 / 2;

        while (numpixels--) {
            uint8_t color;
            color = colortbl[pos++];
            if (pos >= colornum) { pos = 0; }
            if (color != 0) {
                *p = color;
            }
            xerr += xslope;
            if ((xerr & 0xff00) != 0) {
                xerr &= 0xff;
                ++p;
            }
            yerr += yslope;
            if ((yerr & 0xff00) != 0) {
                yerr &= 0xff;
                p += yinc;
            }
        }
    }
}

void vgabuf_draw_copy_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool flag_hmm)
{
    int xslope = 0, yslope = 0, yinc, numpixels;    /* BUG? xslope and yslope not cleared by MOO1 */

    if (x1 < x0) {
        int16_t t;
        t = x1; x1 = x0; x0 = t;
        t = y1; y1 = y0; y0 = t;
    }

    {
        int dx, dy;
        dx = x1 - x0;
        dy = y1 - y0;
        yinc = VGABUF_PITCH;
        if (dy < 0) {
            dy = -dy;
            yinc = -VGABUF_PITCH;
        }
        if (dx < dy) {
            numpixels = dy + 1;
            yslope = 0x100;
            if (dy != 0) {
                xslope = (dx << 8) / dy;
            }
        } else {
            numpixels = dx + 1;
            if (dx != 0) {
                xslope = 0x100;
                yslope = (dy << 8) / dx;
            }
        }
    }

    if (flag_hmm != 0) {
        numpixels <<= 1;
        xslope /= 2;
        yslope /= 2;
    }

    {
        uint8_t *q = vgabuf_get_back() + VGABUF_OFFSET(x0, y0);
        uint8_t *p = vgabuf_get_front() + VGABUF_OFFSET(x0, y0);
        int xerr, yerr;

        xerr = 0x100 / 2;
        yerr = 0x100 / 2;

        while (numpixels--) {
            *p = *q;
            xerr += xslope;
            if ((xerr & 0xff00) != 0) {
                xerr &= 0xff;
                ++p;
                ++q;
            }
            yerr += yslope;
            if ((yerr & 0xff00) != 0) {
                yerr &= 0xff;
                p += yinc;
                q += yinc;
            }
        }
    }
}

void vgabuf_draw_line_3h(int16_t x0, int16_t y0, int16_t x1, uint8_t color)
{
    vgabuf_draw_line(x0, y0, x1, y0, color);
    vgabuf_draw_line(x0, y0 + 1, x1, y0 + 1, color);
    vgabuf_draw_line(x0, y0 + 2, x1, y0 + 2, color);
}

void vgabuf_draw_box1(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color1, uint8_t color2)
{
    vgabuf_draw_line(x0, y0, x1, y0, color1);
    vgabuf_draw_line(x0, y0, x0, y1, color1);
    vgabuf_draw_line(x0 + 1, y1, x1, y1, color2);
    vgabuf_draw_line(x1, y0 + 1, x1, y1, color2);
}

void vgabuf_draw_box2(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color1, uint8_t color2, uint8_t color3, uint8_t color4)
{
    vgabuf_draw_box1(x0, y0, x1, y1, color1, color3);
    ++x0; ++y0; --x1; --y1;
    vgabuf_draw_box1(x0, y0, x1, y1, color2, color4);
}
