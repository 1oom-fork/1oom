#ifndef INC_1OOM_HW_INTERNAL_H
#define INC_1OOM_HW_INTERNAL_H

#include "types.h"
#include "vgapal.h"

extern struct i_hw_video_s {
    int (*setmode)(int w, int h);
    void (*render)(int bufi);
    void (*update)(void);
    void (*setpal)(const uint8_t *pal, int first, int num);
} i_hw_video;

extern int hw_audio_init(void);
extern void hw_audio_shutdown(void);

extern void hw_video_shutdown(void);

#endif
