static mookey_t key_xlat[SDLK_LAST];

static void build_key_xlat(void)
{
    memset(key_xlat, 0, sizeof(key_xlat));
    key_xlat[SDLK_FIRST] = MOO_KEY_FIRST;
    key_xlat[SDLK_BACKSPACE] = MOO_KEY_BACKSPACE;
    key_xlat[SDLK_TAB] = MOO_KEY_TAB;
    key_xlat[SDLK_CLEAR] = MOO_KEY_CLEAR;
    key_xlat[SDLK_RETURN] = MOO_KEY_RETURN;
    key_xlat[SDLK_PAUSE] = MOO_KEY_PAUSE;
    key_xlat[SDLK_ESCAPE] = MOO_KEY_ESCAPE;
    key_xlat[SDLK_SPACE] = MOO_KEY_SPACE;
    key_xlat[SDLK_EXCLAIM] = MOO_KEY_EXCLAIM;
    key_xlat[SDLK_QUOTEDBL] = MOO_KEY_QUOTEDBL;
    key_xlat[SDLK_HASH] = MOO_KEY_HASH;
    key_xlat[SDLK_DOLLAR] = MOO_KEY_DOLLAR;
    key_xlat[SDLK_AMPERSAND] = MOO_KEY_AMPERSAND;
    key_xlat[SDLK_QUOTE] = MOO_KEY_QUOTE;
    key_xlat[SDLK_LEFTPAREN] = MOO_KEY_LEFTPAREN;
    key_xlat[SDLK_RIGHTPAREN] = MOO_KEY_RIGHTPAREN;
    key_xlat[SDLK_ASTERISK] = MOO_KEY_ASTERISK;
    key_xlat[SDLK_PLUS] = MOO_KEY_PLUS;
    key_xlat[SDLK_COMMA] = MOO_KEY_COMMA;
    key_xlat[SDLK_MINUS] = MOO_KEY_MINUS;
    key_xlat[SDLK_PERIOD] = MOO_KEY_PERIOD;
    key_xlat[SDLK_SLASH] = MOO_KEY_SLASH;
    key_xlat[SDLK_0] = MOO_KEY_0;
    key_xlat[SDLK_1] = MOO_KEY_1;
    key_xlat[SDLK_2] = MOO_KEY_2;
    key_xlat[SDLK_3] = MOO_KEY_3;
    key_xlat[SDLK_4] = MOO_KEY_4;
    key_xlat[SDLK_5] = MOO_KEY_5;
    key_xlat[SDLK_6] = MOO_KEY_6;
    key_xlat[SDLK_7] = MOO_KEY_7;
    key_xlat[SDLK_8] = MOO_KEY_8;
    key_xlat[SDLK_9] = MOO_KEY_9;
    key_xlat[SDLK_COLON] = MOO_KEY_COLON;
    key_xlat[SDLK_SEMICOLON] = MOO_KEY_SEMICOLON;
    key_xlat[SDLK_LESS] = MOO_KEY_LESS;
    key_xlat[SDLK_EQUALS] = MOO_KEY_EQUALS;
    key_xlat[SDLK_GREATER] = MOO_KEY_GREATER;
    key_xlat[SDLK_QUESTION] = MOO_KEY_QUESTION;
    key_xlat[SDLK_AT] = MOO_KEY_AT;
    key_xlat[SDLK_LEFTBRACKET] = MOO_KEY_LEFTBRACKET;
    key_xlat[SDLK_BACKSLASH] = MOO_KEY_BACKSLASH;
    key_xlat[SDLK_RIGHTBRACKET] = MOO_KEY_RIGHTBRACKET;
    key_xlat[SDLK_CARET] = MOO_KEY_CARET;
    key_xlat[SDLK_UNDERSCORE] = MOO_KEY_UNDERSCORE;
    key_xlat[SDLK_BACKQUOTE] = MOO_KEY_BACKQUOTE;
    key_xlat[SDLK_a] = MOO_KEY_a;
    key_xlat[SDLK_b] = MOO_KEY_b;
    key_xlat[SDLK_c] = MOO_KEY_c;
    key_xlat[SDLK_d] = MOO_KEY_d;
    key_xlat[SDLK_e] = MOO_KEY_e;
    key_xlat[SDLK_f] = MOO_KEY_f;
    key_xlat[SDLK_g] = MOO_KEY_g;
    key_xlat[SDLK_h] = MOO_KEY_h;
    key_xlat[SDLK_i] = MOO_KEY_i;
    key_xlat[SDLK_j] = MOO_KEY_j;
    key_xlat[SDLK_k] = MOO_KEY_k;
    key_xlat[SDLK_l] = MOO_KEY_l;
    key_xlat[SDLK_m] = MOO_KEY_m;
    key_xlat[SDLK_n] = MOO_KEY_n;
    key_xlat[SDLK_o] = MOO_KEY_o;
    key_xlat[SDLK_p] = MOO_KEY_p;
    key_xlat[SDLK_q] = MOO_KEY_q;
    key_xlat[SDLK_r] = MOO_KEY_r;
    key_xlat[SDLK_s] = MOO_KEY_s;
    key_xlat[SDLK_t] = MOO_KEY_t;
    key_xlat[SDLK_u] = MOO_KEY_u;
    key_xlat[SDLK_v] = MOO_KEY_v;
    key_xlat[SDLK_w] = MOO_KEY_w;
    key_xlat[SDLK_x] = MOO_KEY_x;
    key_xlat[SDLK_y] = MOO_KEY_y;
    key_xlat[SDLK_z] = MOO_KEY_z;
    key_xlat[SDLK_DELETE] = MOO_KEY_DELETE;
    key_xlat[SDLK_KP0] = MOO_KEY_KP0;
    key_xlat[SDLK_KP1] = MOO_KEY_KP1;
    key_xlat[SDLK_KP2] = MOO_KEY_KP2;
    key_xlat[SDLK_KP3] = MOO_KEY_KP3;
    key_xlat[SDLK_KP4] = MOO_KEY_KP4;
    key_xlat[SDLK_KP5] = MOO_KEY_KP5;
    key_xlat[SDLK_KP6] = MOO_KEY_KP6;
    key_xlat[SDLK_KP7] = MOO_KEY_KP7;
    key_xlat[SDLK_KP8] = MOO_KEY_KP8;
    key_xlat[SDLK_KP9] = MOO_KEY_KP9;
    key_xlat[SDLK_KP_PERIOD] = MOO_KEY_KP_PERIOD;
    key_xlat[SDLK_KP_DIVIDE] = MOO_KEY_KP_DIVIDE;
    key_xlat[SDLK_KP_MULTIPLY] = MOO_KEY_KP_MULTIPLY;
    key_xlat[SDLK_KP_MINUS] = MOO_KEY_KP_MINUS;
    key_xlat[SDLK_KP_PLUS] = MOO_KEY_KP_PLUS;
    key_xlat[SDLK_KP_ENTER] = MOO_KEY_KP_ENTER;
    key_xlat[SDLK_KP_EQUALS] = MOO_KEY_KP_EQUALS;
    key_xlat[SDLK_UP] = MOO_KEY_UP;
    key_xlat[SDLK_DOWN] = MOO_KEY_DOWN;
    key_xlat[SDLK_RIGHT] = MOO_KEY_RIGHT;
    key_xlat[SDLK_LEFT] = MOO_KEY_LEFT;
    key_xlat[SDLK_INSERT] = MOO_KEY_INSERT;
    key_xlat[SDLK_HOME] = MOO_KEY_HOME;
    key_xlat[SDLK_END] = MOO_KEY_END;
    key_xlat[SDLK_PAGEUP] = MOO_KEY_PAGEUP;
    key_xlat[SDLK_PAGEDOWN] = MOO_KEY_PAGEDOWN;
    key_xlat[SDLK_F1] = MOO_KEY_F1;
    key_xlat[SDLK_F2] = MOO_KEY_F2;
    key_xlat[SDLK_F3] = MOO_KEY_F3;
    key_xlat[SDLK_F4] = MOO_KEY_F4;
    key_xlat[SDLK_F5] = MOO_KEY_F5;
    key_xlat[SDLK_F6] = MOO_KEY_F6;
    key_xlat[SDLK_F7] = MOO_KEY_F7;
    key_xlat[SDLK_F8] = MOO_KEY_F8;
    key_xlat[SDLK_F9] = MOO_KEY_F9;
    key_xlat[SDLK_F10] = MOO_KEY_F10;
    key_xlat[SDLK_F11] = MOO_KEY_F11;
    key_xlat[SDLK_F12] = MOO_KEY_F12;
    key_xlat[SDLK_F13] = MOO_KEY_F13;
    key_xlat[SDLK_F14] = MOO_KEY_F14;
    key_xlat[SDLK_F15] = MOO_KEY_F15;
    key_xlat[SDLK_NUMLOCK] = MOO_KEY_NUMLOCK;
    key_xlat[SDLK_CAPSLOCK] = MOO_KEY_CAPSLOCK;
    key_xlat[SDLK_SCROLLOCK] = MOO_KEY_SCROLLOCK;
    key_xlat[SDLK_RSHIFT] = MOO_KEY_RSHIFT;
    key_xlat[SDLK_LSHIFT] = MOO_KEY_LSHIFT;
    key_xlat[SDLK_RCTRL] = MOO_KEY_RCTRL;
    key_xlat[SDLK_LCTRL] = MOO_KEY_LCTRL;
    key_xlat[SDLK_RALT] = MOO_KEY_RALT;
    key_xlat[SDLK_LALT] = MOO_KEY_LALT;
    key_xlat[SDLK_RMETA] = MOO_KEY_RMETA;
    key_xlat[SDLK_LMETA] = MOO_KEY_LMETA;
    key_xlat[SDLK_LSUPER] = MOO_KEY_LSUPER;
    key_xlat[SDLK_RSUPER] = MOO_KEY_RSUPER;
    key_xlat[SDLK_MODE] = MOO_KEY_MODE;
    key_xlat[SDLK_COMPOSE] = MOO_KEY_COMPOSE;
    key_xlat[SDLK_HELP] = MOO_KEY_HELP;
    key_xlat[SDLK_PRINT] = MOO_KEY_PRINT;
    key_xlat[SDLK_SYSREQ] = MOO_KEY_SYSREQ;
    key_xlat[SDLK_BREAK] = MOO_KEY_BREAK;
    key_xlat[SDLK_MENU] = MOO_KEY_MENU;
    key_xlat[SDLK_POWER] = MOO_KEY_POWER;
    key_xlat[SDLK_EURO] = MOO_KEY_EURO;
    key_xlat[SDLK_UNDO] = MOO_KEY_UNDO;
}

/* -------------------------------------------------------------------------- */

static bool hw_kbd_check_hotkey(SDLKey key, SDLMod smod, char c)
{
    if ((smod & KMOD_CTRL) && (!(smod & KMOD_ALT))) {
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
    } else if ((smod & KMOD_ALT) && (!(smod & KMOD_CTRL))) {
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
