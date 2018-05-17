static bool hw_kbd_check_hotkey(uint32_t key, uint32_t smod)
{
    if ((smod & KB_CTRL_FLAG) && (!(smod & KB_ALT_FLAG))) {
        if ((key >> 8) == KEY_ESC) {
            log_message("Alleg: got Ctrl-ESC, quitting now\n");
            exit(EXIT_SUCCESS);
        } else if ((key >> 8) == KEY_PLUS_PAD) {
            if (smod & KB_SHIFT_FLAG) {
                hw_audio_music_volume(opt_music_volume + 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume + 4);
            }
            return true;
        } else if ((key >> 8) == KEY_MINUS_PAD) {
            if (smod & KB_SHIFT_FLAG) {
                hw_audio_music_volume(opt_music_volume - 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume - 4);
            }
            return true;
        }
    }
    return false;
}
