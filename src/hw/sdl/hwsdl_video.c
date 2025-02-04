void hw_video_refresh(void)
{
    if (SDL_MUSTLOCK(video.screen)) {
        if (SDL_LockSurface(video.screen) < 0) {
            return;
        }
    }

    video.render();

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
    vgabuf_select_back();
    video.buf = vgabuf_get_front();
    hw_video_refresh();
    return vgabuf_get_back();
}

void hw_video_redraw_front(void)
{
    hw_video_refresh();
}
