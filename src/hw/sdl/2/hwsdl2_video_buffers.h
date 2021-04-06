#ifndef INC_1OOM_HWSDL2_VIDEO_BUFFERS_H
#define INC_1OOM_HWSDL2_VIDEO_BUFFERS_H
#include "types.h"

/* SDL uses OpenGL to rescale the image anyway (=free pixel format conversion)
 * so for simplicity's sake lets use RGB888.
 * We want a 4-byte format to keep hwsdl_video_texture.c code simple */
#define VIDEO_TEXTURE_FORMAT SDL_PIXELFORMAT_RGB888

typedef struct hwsdl_video_buffer_s {
    const uint8_t *pixels;
    const uint32_t *palette;
    int w, h;
} hwsdl_video_buffer_t;

/* To have less memory leaks and malloc calls we allocate buffers statically.
 * But someone set UI_SCALE_MAX to 10 even though
 * a "huge" galaxy is completely in view at uiscale=5 already.
 * This wastes at least 40 megabytes of RAM. Virtual memory should deal with it though */
#define MAX_VIDEO_X (10*320)
#define MAX_VIDEO_Y (10*200)

/* return true on success */
bool hwsdl_video_buffers_alloc(int w, int h);

void hwsdl_video_screenshot(void);

#endif

