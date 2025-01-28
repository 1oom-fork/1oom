#ifndef INC_1OOM_VGAPAL_H
#define INC_1OOM_VGAPAL_H

#include "types.h"

/* palette as set by UI, 6bpp */
extern uint8_t vgapal[256 * 3];

static inline void vgapal_set_byte(int i, uint8_t b)
{
    vgapal[i] = b & 0x3f;
}

extern void vgapal_set(const uint8_t *pal, int first, int num);

#endif
