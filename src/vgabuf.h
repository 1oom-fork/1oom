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

#define vgabuf_erase ui_draw_erase_buf
#define vgabuf_fill ui_draw_color_buf
#define vgabuf_fill_rect ui_draw_filled_rect
#define vgabuf_put_pixel ui_draw_pixel
#define vgabuf_draw_line ui_draw_line1
#define vgabuf_draw_line_ctbl ui_draw_line_ctbl
#define vgabuf_draw_line_limit ui_draw_line_limit
#define vgabuf_draw_line_limit_ctbl ui_draw_line_limit_ctbl
#define vgabuf_draw_copy_line ui_draw_copy_line
#define vgabuf_draw_line_3h ui_draw_line_3h
#define vgabuf_draw_box1 ui_draw_box1
#define vgabuf_draw_box2 ui_draw_box2

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

#define vgabuf_limits_set uiobj_set_limits
#define vgabuf_limits_set_all uiobj_set_limits_all

#endif
