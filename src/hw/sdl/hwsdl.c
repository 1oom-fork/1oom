static bool hw_kbd_check_hotkey(SDLKey key, SDLMod smod, char c)
{
    if (smod & KMOD_CTRL) {
        if (key == SDLK_ESCAPE) {
            log_message("SDL: got Ctrl-ESC, quitting now\n");
            exit(EXIT_SUCCESS);
        } else if (key == SDLK_F10) {
            hw_mouse_toggle_grab();
        } else if (c == '+') {
            if (smod & KMOD_SHIFT) {
                hw_audio_music_volume(opt_music_volume + 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume + 4);
            }
        } else if (c == '-') {
            if (smod & KMOD_SHIFT) {
                hw_audio_music_volume(opt_music_volume - 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume - 4);
            }
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
