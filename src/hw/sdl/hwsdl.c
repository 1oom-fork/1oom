static bool hw_kbd_check_hotkey(SDL1or2Key key, SDL1or2Mod smod, char c)
{
    if ((smod & KMOD_CTRL) && !(smod & KMOD_ALT)) {
        if (key == SDLK_ESCAPE) {
            log_message("SDL: got Ctrl-ESC, quitting now\n");
            exit(EXIT_SUCCESS);
        } else if (key == SDLK_F10) {
            hw_mouse_toggle_grab();
            return true;
        } else if (c == '+') {
            if (smod & KMOD_SHIFT) {
                hw_audio_music_volume(opt_music_volume + 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume + 4);
            }
            return true;
        } else if (c == '-') {
            if (smod & KMOD_SHIFT) {
                hw_audio_music_volume(opt_music_volume - 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume - 4);
            }
            return true;
        }
    } else if ((smod & KMOD_ALT) && !(smod & KMOD_CTRL)) {
        if (key == SDLK_RETURN) {
            if (hw_video_toggle_fullscreen() < 0) {
                log_message("SDL: fs toggle failure, quitting now\n");
                exit(EXIT_FAILURE);
            }
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------- */

uint32_t hw_get_time_us(void)
{
    return SDL_GetTicks() * 1000;
}
