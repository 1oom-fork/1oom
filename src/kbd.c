#include "config.h"

#include <stdlib.h>

#include "hw.h"
#include "kbd.h"
#include "log.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define KBD_BUFSIZE 8
static struct {
    bool full;
    int head;
    int tail;
    uint32_t buf[KBD_BUFSIZE];
} kbd = { 0 };

/* -------------------------------------------------------------------------- */

bool kbd_handle_hotkey(mookey_t key, uint32_t mod)
{
    if (mod & MOO_MOD_CTRL) {
        if (key == MOO_KEY_F10) {
            hw_mouse_toggle_grab();
            return true;
        } else if (key == MOO_KEY_RIGHTBRACKET) {
            if (mod & MOO_MOD_SHIFT) {
                hw_audio_music_volume(opt_music_volume + 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume + 4);
            }
            return true;
        } else if (key == MOO_KEY_LEFTBRACKET) {
            if (mod & MOO_MOD_SHIFT) {
                hw_audio_music_volume(opt_music_volume - 4);
            } else {
                hw_audio_sfx_volume(opt_sfx_volume - 4);
            }
            return true;
        } else if (key == MOO_KEY_ESCAPE) {
            log_message("Got Ctrl-ESC, quitting now\n");
            exit(EXIT_SUCCESS);
        }
    }

    return false;
}

void kbd_add_keypress(mookey_t key, uint32_t mod, char c)
{
    if ((key <= MOO_KEY_UNKNOWN) || (key >= MOO_KEY_NUMLOCK)) {
        return;
    }
    uint32_t value = ((uint32_t)key) | mod | (((uint32_t)c) << 8);
    if (kbd.full) {
        log_warning("kbd: full while inserting 0x%x\n", value);
    } else {
        kbd.buf[kbd.head] = value;
        if (++kbd.head == KBD_BUFSIZE) { kbd.head = 0; }
        if (kbd.head == kbd.tail) {
            kbd.full = true;
        }
    }
}

bool kbd_have_keypress(void)
{
    return kbd.full || (kbd.head != kbd.tail);
}

uint32_t kbd_get_keypress(void)
{
    uint32_t key = MOO_KEY_UNKNOWN;
    if (kbd_have_keypress()) {
        key = kbd.buf[kbd.tail];
        if (++kbd.tail == KBD_BUFSIZE) { kbd.tail = 0; }
        kbd.full = false;
    }
    return key;
}
