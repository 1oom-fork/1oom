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
    char c;
    int x0 = x;
    while ((c = *str++)) {
        if (c == '\n') {
            x = x0;
            y += 8;
            continue;
        }
        font8x8_drawchar(x, y, pitch, c, fg, bg);
        x += 8;
        if (x > (pitch - 8)) {
            x = 0;
            y += 8;
        }
    }
}

void font8x8_drawstrlen(int x, int y, uint16_t pitch, const char *str, int len, uint8_t fg, uint8_t bg)
{
    char c;
    int x0 = x;
    while (len--) {
        c = *str++;
        if (c == '\n') {
            x = x0;
            y += 8;
            continue;
        }
        font8x8_drawchar(x, y, pitch, c, fg, bg);
        x += 8;
        if (x > (pitch - 8)) {
            x = 0;
            y += 8;
        }
    }
}
