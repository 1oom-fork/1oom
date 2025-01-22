#ifndef INC_1OOM_VGABUF_H
#define INC_1OOM_VGABUF_H

#include "types.h"

/* double buffering + 2 aux buffers */
#define NUM_VIDEOBUF    4

#define VGABUF_W    320
#define VGABUF_H    200
#define VGABUF_SIZE     (VGABUF_W * VGABUF_H)

/* FIXME */
#define VGABUF_EXTRA_H    200

#define VGABUF_SIZE_EXT     (VGABUF_W * (VGABUF_H + VGABUF_EXTRA_H))

/* Return back buffer. */
extern uint8_t *vgabuf_get_back(void);
/* Return front buffer. */
extern uint8_t *vgabuf_get_front(void);
/* Swap front and back buffers. */
extern void vgabuf_select_back(void);
/* Copy front buffer to back buffer. */
extern void vgabuf_copy_buf(void);
/* Copy back buffer to pointed buffer. */
extern void vgabuf_copy_buf_out(uint8_t *buf);
extern void vgabuf_copy_back_to_page2(void);
extern void vgabuf_copy_back_from_page2(void);
extern void vgabuf_copy_back_to_page3(void);
extern void vgabuf_copy_back_from_page3(void);

#endif
