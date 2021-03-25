#ifndef INC_1OOM_GFXSCALE_H
#define INC_1OOM_GFXSCALE_H

#include "types.h"

static inline uint8_t *gfxscale_draw_pixel(uint8_t *q, uint8_t b, uint16_t pitch, int scale)
{
    for (int y = 0; y < scale; ++y) {
        for (int x = 0; x < scale; ++x) {
            q[x] = b;
        }
        q += pitch;
    }
    return q;
}

/* used by lbxgfx.c for rle format 1 */
static inline uint8_t *gfxscale_draw_pixel_fmt1(uint8_t *q, const uint8_t tbl[], uint16_t pitch, int scale)
{
    for (int sy = 0; sy < scale; ++sy) {
        for (int sx = 0; sx < scale; ++sx) {
            q[sx] = tbl[q[sx]];
        }
        q += pitch;
    }
    return q;
}

#endif
