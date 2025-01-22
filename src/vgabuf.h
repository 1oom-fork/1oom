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

/* buffers used by UI */
extern uint8_t vgabuf[NUM_VIDEOBUF * VGABUF_SIZE_EXT];

static inline uint8_t *vgabuf_get(int i)
{
    return &vgabuf[i * VGABUF_SIZE_EXT];
}

#endif
