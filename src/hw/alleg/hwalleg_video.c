bool hw_video_in_gfx = false;

void hw_video_redraw_front(void)
{
    video.render(UI_VIDEO_BUF_FRONT);
    video.update();
}

void hw_video_update(void)
{
    video.update();
}

void hw_video_set_palette(const uint8_t *pal, int first, int num)
{
    ui_palette_set(pal, first, num);
    video.setpal(pal, first, num);
}

void hw_video_set_palette_color(int i, uint8_t r, uint8_t g, uint8_t b)
{
    ui_palette_set_color(i, r, g, b);
}

void hw_video_refresh_palette(void)
{
    video.setpal(ui_palette, 0, 256);
}

uint8_t *hw_video_draw_buf(void)
{
    UI_VIDEO_BUF_SWAP;
    hw_video_redraw_front();
    return hw_video_get_buf();
}
