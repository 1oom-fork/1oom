#ifndef INC_1OOM_VGAPAL_H
#define INC_1OOM_VGAPAL_H

#include "types.h"

/* palette as set by UI, 6bpp */
extern uint8_t vgapal[256 * 3];

static inline uint8_t vgapal_6bit_to_8bit(uint8_t _6bit)
{
    return (_6bit << 2) | ((_6bit >> 4) & 3);
}

static inline void vgapal_set_byte(uint16_t i, uint8_t b)
{
    vgapal[i] = b & 0x3f;
}

extern void vgapal_set(const uint8_t *pal, uint16_t first, uint16_t num);

#endif
