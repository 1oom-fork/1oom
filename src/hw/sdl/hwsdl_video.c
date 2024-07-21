static int video_check_opt_screen_winwh(void)
{
    int minw = UI_VIDEO_BUFW;
    int minh = UI_VIDEO_BUFH;
    if ((hw_opt_screen_winw != 0) && (hw_opt_screen_winh != 0)) {
        if ((hw_opt_screen_winw < minw) || (hw_opt_screen_winh < minh)) {
            log_warning("ignoring too small configured resolution %ix%i < %ix%i\n", hw_opt_screen_winw, hw_opt_screen_winh, minw, minh);
        } else {
            return 0;
        }
    }
    return -1;
}

/* -------------------------------------------------------------------------- */

void hw_video_refresh(int front)
{
    if (SDL_MUSTLOCK(video.screen)) {
        if (SDL_LockSurface(video.screen) < 0) {
            return;
        }
    }

    video.render(UI_VIDEO_BUF(front));

    if (SDL_MUSTLOCK(video.screen)) {
        SDL_UnlockSurface(video.screen);
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

uint8_t *hw_video_get_buf(void)
{
    return UI_VIDEO_BUF_BACK;
}

uint8_t *hw_video_get_buf_front(void)
{
    return UI_VIDEO_BUF_FRONT;
}

uint8_t *hw_video_draw_buf(void)
{
    hw_video_refresh(0);
    UI_VIDEO_BUF_SWAP;
    return UI_VIDEO_BUF_BACK;
}

void hw_video_redraw_front(void)
{
    hw_video_refresh(1);
}

void hw_video_copy_buf(void)
{
    UI_VIDEO_COPY_BUF;
}

void hw_video_copy_buf_out(uint8_t *buf)
{
    UI_VIDEO_COPY_BUF_OUT(buf);
}

void hw_video_copy_back_to_page2(void)
{
    UI_VIDEO_COPY_BACK_TO_PAGE2;
}

void hw_video_copy_back_from_page2(void)
{
    UI_VIDEO_COPY_BACK_FROM_PAGE2;
}

void hw_video_copy_back_to_page3(void)
{
    UI_VIDEO_COPY_BACK_TO_PAGE3;
}

void hw_video_copy_back_from_page3(void)
{
    UI_VIDEO_COPY_BACK_FROM_PAGE3;
}
