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
