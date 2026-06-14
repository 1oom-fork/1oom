#ifndef INC_1OOM_VGABUF_H
#define INC_1OOM_VGABUF_H

#include "comp.h"
#include "hw.h"
#include "types.h"

extern struct vgabuf_s {
    uint8_t *buf;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    int16_t pitch;
} vgabuf_seg;

#define VGABUF_W    320
#define VGABUF_H    200
#define VGABUF_SIZE     (VGABUF_W * VGABUF_H)
#define VGABUF_RECT     0, 0, (VGABUF_W - 1), (VGABUF_H - 1)

#define VGABUF_SEG_RECT     vgabuf_seg.x, vgabuf_seg.y, vgabuf_seg.w - 1, vgabuf_seg.h - 1
#define VGABUF_PITCH    vgabuf_seg.pitch
#define VGABUF_OFFSET(x, y)   ((x) + (y) * (VGABUF_PITCH))

#define VGABUF_CURSOR_W    16
#define VGABUF_CURSOR_H    16
#define VGABUF_CURSOR_SIZE  VGABUF_CURSOR_W * VGABUF_CURSOR_H

extern int16_t vgabuf_scale;

extern bool vgabuf_hw_cursor_enabled;

extern void vgabuf_select_back(void);
extern void vgabuf_select_front(void);
extern void vgabuf_flip(void);

extern void vgabuf_clear_hw_cursor(void);
extern uint8_t *vgabuf_get_hw_cursor(void);

/* Return selected buffer. */
extern uint8_t *vgabuf_get(void);
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
    vgabuf_fill_rect(VGABUF_SEG_RECT, 0);
}

static inline void vgabuf_fill(uint8_t color)
{
    vgabuf_fill_rect(VGABUF_SEG_RECT, color);
}

extern void vgabuf_limits_get(int16_t *x0, int16_t *y0, int16_t *x1, int16_t *y1);
extern bool vgabuf_limits_outside(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
extern void vgabuf_limits_clamp_rect(int16_t *x0, int16_t *y0, int16_t *x1, int16_t *y1);
extern void vgabuf_limits_set(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
extern void vgabuf_limits_set_all(void);

static inline bool vgabuf_set_scale(int16_t scale)
{
    int16_t result = hw_video_set_scale(scale);
    if (result == scale) {
        vgabuf_scale = scale;
        return true;
    }
    return false;
}

#endif
