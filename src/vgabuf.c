#include "config.h"

#include <string.h>

#include "vgabuf.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* buffers used by UI */
static uint8_t vgabuf[NUM_VIDEOBUF * VGABUF_SIZE_EXT] = {0};
static int vgabuf_i = 0;

static inline uint8_t *vgabuf_get(int i)
{
    return &vgabuf[i * VGABUF_SIZE_EXT];
}

/* -------------------------------------------------------------------------- */

uint8_t *vgabuf_get_back(void)
{
    return vgabuf_get(vgabuf_i);
}

uint8_t *vgabuf_get_front(void)
{
    return vgabuf_get(vgabuf_i ^ 1);
}

void vgabuf_select_back(void)
{
    vgabuf_i ^= 1;
}

void vgabuf_copy_buf(void)
{
    memcpy(vgabuf_get_back(), vgabuf_get_front(), VGABUF_SIZE);
}

void vgabuf_copy_buf_out(uint8_t *buf)
{
    memcpy(buf, vgabuf_get_back(), VGABUF_SIZE);
}

void vgabuf_copy_back_to_page2(void)
{
    memcpy(vgabuf_get(2), vgabuf_get_back(), VGABUF_SIZE);
}

void vgabuf_copy_back_from_page2(void)
{
    memcpy(vgabuf_get_back(), vgabuf_get(2), VGABUF_SIZE);
}

void vgabuf_copy_back_to_page3(void)
{
    memcpy(vgabuf_get(3), vgabuf_get_back(), VGABUF_SIZE);
}

void vgabuf_copy_back_from_page3(void)
{
    memcpy(vgabuf_get_back(), vgabuf_get(3), VGABUF_SIZE);
}
