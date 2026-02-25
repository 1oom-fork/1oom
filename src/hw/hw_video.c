struct i_hw_video_s i_hw_video = { 0 };

void hw_video_refresh(void)
{
    i_hw_video.render(vgabuf_get_front());
    i_hw_video.update();
}

void hw_video_update(void)
{
    i_hw_video.update();
}

void hw_video_refresh_palette(void)
{
    i_hw_video.setpal(vgapal, 0, 256);
}

uint8_t *hw_video_draw_buf(void)
{
    vgabuf_select_back();
    hw_video_refresh();
    return vgabuf_get_back();
}

void hw_video_redraw_front(void)
{
    hw_video_refresh();
}
