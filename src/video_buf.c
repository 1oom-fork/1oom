#include "video_buf.h"

uint8_t ui_video_buf[UI_NUM_VIDEOBUF * UI_VIDEO_BUF_SIZE];
int ui_video_bufi = 0;

#define UI_VIDEO_BUF_BACK  &ui_video_buf[UI_VIDEO_BUF_SIZE * ui_video_bufi]
#define UI_VIDEO_BUF_FRONT  &ui_video_buf[UI_VIDEO_BUF_SIZE * (ui_video_bufi ^ 1)]
#define UI_VIDEO_BUF_N(n)  &ui_video_buf[UI_VIDEO_BUF_SIZE * n]

uint8_t *hw_video_get_buf(void)
{
    return UI_VIDEO_BUF_BACK;
}

uint8_t *hw_video_get_buf_front(void)
{
    return UI_VIDEO_BUF_FRONT;
}

void hw_video_copy_buf(void)
{
    memcpy(UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_FRONT, UI_VIDEO_BUF_SIZE);
}

void hw_video_copy_buf_out(uint8_t *buf)
{
    memcpy(buf, UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_SIZE);
}

void hw_video_copy_back_to_page2(void)
{
    memcpy(UI_VIDEO_BUF_N(2), UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_SIZE);
}

void hw_video_copy_back_from_page2(void)
{
    memcpy(UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_N(2), UI_VIDEO_BUF_SIZE);
}

void hw_video_copy_back_to_page3(void)
{
    memcpy(UI_VIDEO_BUF_N(3), UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_SIZE);
}

void hw_video_copy_back_from_page3(void)
{
    memcpy(UI_VIDEO_BUF_BACK, UI_VIDEO_BUF_N(3), UI_VIDEO_BUF_SIZE);
}
