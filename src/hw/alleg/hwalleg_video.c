bool hw_video_in_gfx = false;

void hw_video_refresh(int front)
{
    video.render(video.bufi ^ front);
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
    video.bufi = vgabuf_select_back();
    hw_video_refresh(1);
    return vgabuf_get_back();
}

void hw_video_redraw_front(void)
{
    hw_video_refresh(1);
}
