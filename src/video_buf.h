#ifndef INC_1OOM_VIDEO_BUF_H
#define INC_1OOM_VIDEO_BUF_H

#include <string.h>

#include "types.h"

/* double buffering + 2 aux buffers */
#define UI_NUM_VIDEOBUF    4
#define UI_VIDEO_BUFW   320
#define UI_VIDEO_BUFH   200
#define UI_VIDEO_BUF_SIZE   UI_VIDEO_BUFW * UI_VIDEO_BUFH
#define UI_VIDEO_BUF_FRONT  &ui_video_buf[UI_VIDEO_BUF_SIZE * (ui_video_bufi ^ 1)]

/* buffers used by UI */
extern uint8_t ui_video_buf[UI_NUM_VIDEOBUF * UI_VIDEO_BUF_SIZE];
extern int ui_video_bufi;

#endif
