void hw_video_refresh(int front)
{
    if (SDL_MUSTLOCK(video.screen)) {
        if (SDL_LockSurface(video.screen) < 0) {
            return;
        }
    }

    i_hw_video.render();

    if (SDL_MUSTLOCK(video.screen)) {
        SDL_UnlockSurface(video.screen);
    }

    i_hw_video.update();
}

void hw_video_refresh_palette(void)
{
    i_hw_video.setpal(vgapal, 0, 256);
}

void hw_video_redraw_front(void)
{
    hw_video_refresh(1);
}
