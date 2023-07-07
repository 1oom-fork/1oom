#ifndef INC_1OOM_PALETTE_H
#define INC_1OOM_PALETTE_H

#include <string.h>

#include "types.h"

extern uint8_t ui_palette[256*3]; /* 6bit palette from the game */

static inline void ui_palette_clear(void)
{
    memset(ui_palette, 0, sizeof(ui_palette));
}

static inline void ui_palette_set_byte(int i, uint8_t b)
{
    ui_palette[i] = b & 0x3f;
}

static inline void ui_palette_set(const uint8_t *pal, int first, int num)
{
    memcpy(&ui_palette[first * 3], pal, num * 3);
}

static inline uint8_t palette_6bit_to_8bit(uint8_t six_bit)
{
    return (six_bit << 2) | ((six_bit >> 4) & 3);
}

#endif
