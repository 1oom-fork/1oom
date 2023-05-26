#ifndef INC_1OOM_FONT8X8_DRAW_H
#define INC_1OOM_FONT8X8_DRAW_H

#include "types.h"

extern void font8x8_drawchar(int dx, int dy, uint16_t pitch, uint8_t c, uint8_t fg, uint8_t bg);
extern void font8x8_drawstr(int x, int y, uint16_t pitch, const char *str, uint8_t fg, uint8_t bg);
extern void font8x8_drawstrlen(int x, int y, uint16_t pitch, const char *str, int len, uint8_t fg, uint8_t bg);

#endif
