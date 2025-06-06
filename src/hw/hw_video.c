void hw_video_refresh(void)
{
    video.render(vgabuf_get_front());
    video.update();
}

void hw_video_update(void)
{
    video.update();
}

void hw_video_refresh_palette(void)
{
    video.setpal(vgapal, 0, 256);
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
