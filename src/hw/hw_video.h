#ifndef INC_1OOM_HW_VIDEO_H
#define INC_1OOM_HW_VIDEO_H

static inline uint8_t palette_6bit_to_8bit(uint8_t six_bit)
{
    return (six_bit << 2) | ((six_bit >> 4) & 3);
}

#endif
