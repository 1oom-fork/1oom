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

void hw_video_set_palette(const uint8_t *pal, int first, int num)
{
    video.setpal(pal, first, num);
}

uint8_t hw_video_get_palette_byte(int i)
{
    return video.pal[i];
}

void hw_video_set_palette_byte(int i, uint8_t b)
{
    video.pal[i] = b & 0x3f;
}

void hw_video_refresh_palette(void)
{
    hw_video_set_palette(video.pal, 0, 256);
}

uint8_t *hw_video_get_buf(void)
{
    return video.buf[video.bufi];
}

uint8_t *hw_video_get_buf_front(void)
{
    return video.buf[video.bufi ^ 1];
}

uint8_t *hw_video_draw_buf(void)
{
    hw_video_refresh(0);
    if (video.flag_screenshot) {
        video.flag_screenshot = false;
        screenshot_save(video.buf[video.bufi], video.pal, video.bufw, video.bufh);
    }
    video.bufi ^= 1;
    return video.buf[video.bufi];
}

void hw_video_redraw_front(void)
{
    hw_video_refresh(1);
}

void hw_video_copy_buf(void)
{
    memcpy(video.buf[video.bufi], video.buf[video.bufi ^ 1], video.bufw * video.bufh);
}

void hw_video_copy_buf_out(uint8_t *buf)
{
    memcpy(buf, video.buf[video.bufi], video.bufw * video.bufh);
}

void hw_video_copy_back_to_page2(void)
{
    memcpy(video.buf[2], video.buf[video.bufi], video.bufw * video.bufh);
}

void hw_video_copy_back_from_page2(void)
{
    memcpy(video.buf[video.bufi], video.buf[2], video.bufw * video.bufh);
}

void hw_video_copy_back_to_page3(void)
{
    memcpy(video.buf[3], video.buf[video.bufi], video.bufw * video.bufh);
}

void hw_video_copy_back_from_page3(void)
{
    memcpy(video.buf[video.bufi], video.buf[3], video.bufw * video.bufh);
}

void hw_video_screenshot(void)
{
    video.flag_screenshot = true;
}
