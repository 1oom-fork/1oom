static int video_check_opt_screen_winwh(void)
{
    int minw = video.bufw;
    int minh = video.bufh;
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

    video.render(video.bufi ^ front);

    if (SDL_MUSTLOCK(video.screen)) {
        SDL_UnlockSurface(video.screen);
    }

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
