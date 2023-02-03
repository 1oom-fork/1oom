static bool hw_kbd_check_hotkey(uint32_t key, uint32_t smod)
{
    if ((smod & KB_CTRL_FLAG) && (!(smod & KB_ALT_FLAG))) {
        if ((key >> 8) == KEY_ESC) {
            log_message("Alleg: got Ctrl-ESC, quitting now\n");
            exit(EXIT_SUCCESS);
#if 0
        } else if ((key >> 8) == KEY_F10) {
            hw_mouse_toggle_grab();
            return true;
#endif
        } else if ((key >> 8) == KEY_F5) {
            hw_video_screenshot();
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------- */

void hw_log_message(const char *msg)
{
    if (!hw_video_in_gfx) {
        fputs(msg, stdout);
    }
}

void hw_log_warning(const char *msg)
{
    if (!hw_video_in_gfx) {
        fputs(msg, stderr);
    }
}

void hw_log_error(const char *msg)
{
    if (!hw_video_in_gfx) {
        fputs(msg, stderr);
    }
}

int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h)
{
    return 0;
}
