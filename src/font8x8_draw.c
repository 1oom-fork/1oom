#include "types.h"

#include "font8x8.h"
#include "hw.h"

void font8x8_drawchar(int dx, int dy, uint16_t pitch, uint8_t c, uint8_t fg, uint8_t bg)
{
    uint8_t *p = hw_video_get_buf() + dx + dy * pitch;
    for (int y = 0; y < 8; ++y) {
        uint8_t b;
        b = font8x8_basic[c][y];
        for (int x = 0; x < 8; ++x) {
            p[x] = (b & (1 << (x))) ? fg : bg;
        }
        p += pitch;
    }
}

void font8x8_drawstr(int x, int y, uint16_t pitch, const char *str, uint8_t fg, uint8_t bg)
{
    int cx = x, cy = y;
    char c;
    while ((c = *str++)) {
        if (c == '\n') {
            cx = x;
            cy += 8;
            continue;
        }
        font8x8_drawchar(cx, cy, pitch, c, fg, bg);
        cx += 8;
        if (cx > (pitch - 8)) {
            cx = x;
            cy += 8;
        }
    }
}

void font8x8_drawstrlen(int x, int y, uint16_t pitch, const char *str, int len, uint8_t fg, uint8_t bg)
{
    int cx = x, cy = y;
    char c;
    while (len--) {
        c = *str++;
        if (c == '\n') {
            cx = x;
            cy += 8;
            continue;
        }
        font8x8_drawchar(cx, cy, pitch, c, fg, bg);
        cx += 8;
        if (cx > (pitch - 8)) {
            cx = x;
            cy += 8;
        }
    }
}
