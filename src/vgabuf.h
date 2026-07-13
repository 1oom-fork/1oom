#ifndef INC_1OOM_VGABUF_H
#define INC_1OOM_VGABUF_H

#include "types.h"

#define VGABUF_W    320
#define VGABUF_H    200
#define VGABUF_SIZE     (VGABUF_W * VGABUF_H)
#define VGABUF_RECT     0, 0, (VGABUF_W - 1), (VGABUF_H - 1)

#define VGABUF_PITCH    320
#define VGABUF_OFFSET(x, y)   ((x) + (y) * (VGABUF_PITCH))

#define vgabuf_get vgabuf_get_back

extern void vgabuf_flip(void);

/* Return back buffer. */
extern uint8_t *vgabuf_get_back(void);
/* Return front buffer. */
extern uint8_t *vgabuf_get_front(void);

extern void vgabuf_copy_front_to_back(void);
extern void vgabuf_copy_back_out(uint8_t *buf);
extern void vgabuf_copy_back_to_page2(void);
extern void vgabuf_copy_back_from_page2(void);
extern void vgabuf_copy_back_to_page3(void);
extern void vgabuf_copy_back_from_page3(void);

extern void vgabuf_fill_rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
extern void vgabuf_put_pixel(int16_t x, int16_t y, uint8_t color);
extern void vgabuf_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
extern void vgabuf_draw_line_ctbl(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint8_t *colortbl, int colornum, int pos);
extern void vgabuf_draw_line_limit(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
extern void vgabuf_draw_line_limit_ctbl(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint8_t *colortbl, int colornum, int pos);
extern void vgabuf_draw_copy_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool flag_hmm);
extern void vgabuf_draw_line_3h(int16_t x0, int16_t y0, int16_t x1, uint8_t color);
extern void vgabuf_draw_box1(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color1, uint8_t color2);
extern void vgabuf_draw_box2(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color1, uint8_t color2, uint8_t color3, uint8_t color4);

static inline void vgabuf_erase(void)
{
    vgabuf_fill_rect(VGABUF_RECT, 0);
}

static inline void vgabuf_fill(uint8_t color)
{
    vgabuf_fill_rect(VGABUF_RECT, color);
}

extern bool vgabuf_limits_outside(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
extern void vgabuf_limits_clamp_rect(int16_t *x0, int16_t *y0, int16_t *x1, int16_t *y1);
extern void vgabuf_limits_set(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
extern void vgabuf_limits_set_all(void);

#endif
