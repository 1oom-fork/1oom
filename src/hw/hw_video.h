#ifndef INC_1OOM_HW_VIDEO_H
#define INC_1OOM_HW_VIDEO_H

#include "types.h"

extern struct i_hw_video_s {
    int (*setmode)(int w, int h);
    void (*render)(const uint8_t *buf);
    void (*update)(void);
    void (*setpal)(const uint8_t *pal, int first, int num);
} i_hw_video;

extern void hw_video_shutdown(void);
extern void hw_video_refresh(void);
extern void hw_video_update(void);

#endif
