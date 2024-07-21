#ifndef INC_1OOM_VIDEO_BUF_H
#define INC_1OOM_VIDEO_BUF_H

#include <string.h>

#include "types.h"

/* double buffering + 2 aux buffers */
#define UI_NUM_VIDEOBUF    4
#define UI_VIDEO_BUFW   320
#define UI_VIDEO_BUFH   200
#define UI_VIDEO_BUF_SIZE   UI_VIDEO_BUFW * UI_VIDEO_BUFH
#define UI_VIDEO_BUF_BACK  &ui_video_buf[UI_VIDEO_BUF_SIZE * ui_video_bufi]
#define UI_VIDEO_BUF_FRONT  &ui_video_buf[UI_VIDEO_BUF_SIZE * (ui_video_bufi ^ 1)]
#define UI_VIDEO_BUF(front)  &ui_video_buf[UI_VIDEO_BUF_SIZE * (ui_video_bufi ^ front)]
#define UI_VIDEO_BUF_N(n)  &ui_video_buf[UI_VIDEO_BUF_SIZE * n]
#define UI_VIDEO_BUF_SWAP   ui_video_bufi ^= 1

#define UI_VIDEO_COPY_BUF   memcpy(UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_FRONT, UI_VIDEO_BUF_SIZE)
#define UI_VIDEO_COPY_BUF_OUT(buf)   memcpy(buf, UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_SIZE)
#define UI_VIDEO_COPY_BACK_TO_PAGE2     memcpy(UI_VIDEO_BUF_N(2), UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_SIZE)
#define UI_VIDEO_COPY_BACK_FROM_PAGE2     memcpy(UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_N(2), UI_VIDEO_BUF_SIZE)
#define UI_VIDEO_COPY_BACK_TO_PAGE3     memcpy(UI_VIDEO_BUF_N(3), UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_SIZE)
#define UI_VIDEO_COPY_BACK_FROM_PAGE3     memcpy(UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_N(3), UI_VIDEO_BUF_SIZE)

/* buffers used by UI */
extern uint8_t ui_video_buf[UI_NUM_VIDEOBUF * UI_VIDEO_BUF_SIZE];
extern int ui_video_bufi;

#endif
