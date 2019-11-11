/* includes already added by X/hwsdlX_opt.c which includes this */

/* -------------------------------------------------------------------------- */

#define HW_MOUSE_SPEED_MAX  200

/* -------------------------------------------------------------------------- */

bool hw_opt_fullscreen = HW_DEFAULT_FULLSCREEN;
int hw_opt_screen_winw = 0;
int hw_opt_screen_winh = 0;
int hw_opt_screen_fsw = 0;
int hw_opt_screen_fsh = 0;
int hw_opt_mousespd = 100;
bool hw_opt_relmouse = false;
#ifdef FEATURE_MODEBUG
int hw_opt_overlay_pal = 0;
#endif
#ifdef HAVE_SDLX_ASPECT
int hw_opt_aspect = HW_DEFAULT_ASPECT;
#endif
char *hw_opt_sdlmixer_sf = NULL;

/* -------------------------------------------------------------------------- */

static bool check_mouse_speed(void *var)
{
    int v = (int)(intptr_t)var;
    if ((v > 0) && (v <= HW_MOUSE_SPEED_MAX)) {
        return true;
    } else {
        log_error("invalid mousespd %i, must be 0 < N <= %i\n", v, HW_MOUSE_SPEED_MAX);
        return false;
    }
}

const struct cfg_items_s hw_cfg_items[] = {
    CFG_ITEM_BOOL("fs", &hw_opt_fullscreen),
    CFG_ITEM_INT("winw", &hw_opt_screen_winw, 0),
    CFG_ITEM_INT("winh", &hw_opt_screen_winh, 0),
    CFG_ITEM_INT("fsw", &hw_opt_screen_fsw, 0),
    CFG_ITEM_INT("fsh", &hw_opt_screen_fsh, 0),
    CFG_ITEM_INT("mousespd", &hw_opt_mousespd, check_mouse_speed),
    CFG_ITEM_BOOL("relmouse", &hw_opt_relmouse),
#ifdef HAVE_SDLMIXER
    CFG_ITEM_STR("sdlmixersf", &hw_opt_sdlmixer_sf, 0),
#endif
#ifdef HAVE_SDLX_ASPECT
    CFG_ITEM_INT("aspect", &hw_opt_aspect, 0),
#endif
    CFG_ITEM_END
};

/* -------------------------------------------------------------------------- */

static bool hw_uiopt_cb_mousespd(void)
{
    hw_opt_mousespd = 100;
    return true;
}

#ifdef HAVE_SDLX_ASPECT
static const char *hw_uiopt_cb_aspect_get(void)
{
    if (hw_opt_aspect == HW_DEFAULT_ASPECT) {
        return "VGA";
    } else if (hw_opt_aspect == 1000000) {
        return "1:1";
    } else if (hw_opt_aspect == 0) {
        return "Off";
    } else {
        return "Custom";
    }
}

static bool hw_uiopt_cb_aspect_next(void)
{
    if (hw_opt_aspect == HW_DEFAULT_ASPECT) {
        hw_opt_aspect = 1000000;
    } else if (hw_opt_aspect == 1000000) {
        hw_opt_aspect = 0;
    } else {
        hw_opt_aspect = HW_DEFAULT_ASPECT;
    }
    return hw_video_update_aspect();
}
#endif /* HAVE_SDLX_ASPECT */

/* -------------------------------------------------------------------------- */

const struct uiopt_s hw_uiopts[] = {
    UIOPT_ITEM_FUNC("Mouse spd", hw_uiopt_cb_mousespd),
    UIOPT_ITEM_SLIDER_INT(hw_opt_mousespd, 1, HW_MOUSE_SPEED_MAX),
    UIOPT_ITEM_BOOL("Fullscreen", hw_opt_fullscreen, hw_video_toggle_fullscreen),
#ifdef HAVE_SDLX_ASPECT
    UIOPT_ITEM_CYCLE("Aspect", hw_uiopt_cb_aspect_get, hw_uiopt_cb_aspect_next),
#endif /* HAVE_SDLX_ASPECT */
    UIOPT_ITEM_END
};

/* -------------------------------------------------------------------------- */

#ifdef HAVE_SDLMIXER
static int hw_opt_set_sdlmixer_sf(char **argv, void *var)
{
    hw_opt_sdlmixer_sf = lib_stralloc(argv[1]);
    return hw_audio_set_sdlmixer_sf(hw_opt_sdlmixer_sf);
}
#endif

/* -------------------------------------------------------------------------- */

static int hw_options_set_mousespd(char **argv, void *var)
{
    int v = atoi(argv[1]);
    if (check_mouse_speed((void *)(intptr_t)v)) {
        hw_opt_mousespd = v;
        return 0;
    }
    return -1;
}

const struct cmdline_options_s hw_cmdline_options[] = {
    { "-fs", 0,
      options_enable_bool_var, (void *)&hw_opt_fullscreen,
      NULL, "Enable fullscreen" },
    { "-window", 0,
      options_disable_bool_var, (void *)&hw_opt_fullscreen,
      NULL, "Use windowed mode" },
    { "-winw", 1,
      options_set_int_var, (void *)&hw_opt_screen_winw,
      "WIDTH", "Set window width" },
    { "-winh", 1,
      options_set_int_var, (void *)&hw_opt_screen_winh,
      "HEIGHT", "Set window height" },
    { "-fsw", 1,
      options_set_int_var, (void *)&hw_opt_screen_fsw,
      "WIDTH", "Set fullscreen width" },
    { "-fsh", 1,
      options_set_int_var, (void *)&hw_opt_screen_fsh,
      "HEIGHT", "Set fullscreen height" },
    { "-mousespd", 1,
      hw_options_set_mousespd, 0,
      "SPEED", "Set mouse speed (default = 100)" },
    { "-relmouse", 0,
      options_enable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Use relative mouse mode (default)" },
    { "-norelmouse", 0,
      options_disable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Do not use relative mouse mode\nAllows playing in windowed mode without mouse capture.\nDisables forced mouse moves and mouse settings." },
#ifdef HAVE_SDLMIXER
    { "-sdlmixersf", 1,
      hw_opt_set_sdlmixer_sf, NULL,
      "FILE.SF2", "Set SDL_mixer soundfont" },
#endif
#ifdef HAVE_SDLX_ASPECT
    { "-aspect", 1,
      options_set_int_var, (void *)&hw_opt_aspect,
      "ASPECT", "Set aspect ratio (*1000000, 0 = off)" },
#endif
    { NULL, 0, NULL, NULL, NULL, NULL }
};
