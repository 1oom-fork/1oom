bool hw_video_in_gfx = false;

void hw_video_redraw_front(void)
{
    if (hw_opt_scale == 1) {
        video.render(UI_VIDEO_BUF_FRONT);
    } else {
        video_set_render_target(0, 0, 320, 200, hw_opt_scale);
        video.render_target(UI_VIDEO_BUF_FRONT, 0, 0, 320);
    }
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
