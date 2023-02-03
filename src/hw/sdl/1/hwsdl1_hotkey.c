static bool hw_kbd_check_hotkey(SDL1or2Key key, SDL1or2Mod smod, char c)
{
    if ((smod & KMOD_CTRL) && (!(smod & KMOD_ALT))) {
        if (key == SDLK_ESCAPE) {
            log_message("SDL: got Ctrl-ESC, quitting now\n");
            hw_audio_shutdown_pre();
            exit(EXIT_SUCCESS);
        } else if (key == SDLK_F5) {
            hw_video_screenshot();
            return true;
#ifdef FEATURE_MODEBUG
        } else if (key == SDLK_INSERT) {
            hw_opt_overlay_pal ^= 1;
            return true;
#endif
        }
    } else if ((smod & (KMOD_MODE | KMOD_ALT)) && (!(smod & KMOD_CTRL))) {
        if (key == SDLK_RETURN) {
            if (!hw_video_toggle_fullscreen()) {
                log_message("SDL: fs toggle failure, quitting now\n");
                exit(EXIT_FAILURE);
            }
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------- */

void hw_log_message(const char *msg)
{
    fputs(msg, stdout);
}

void hw_log_warning(const char *msg)
{
    fputs(msg, stderr);
}

void hw_log_error(const char *msg)
{
    fputs(msg, stderr);
}

/* -------------------------------------------------------------------------- */

int64_t hw_get_time_us(void)
{
    return SDL_GetTicks() * 1000;
}
