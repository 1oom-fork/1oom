/* includes already added by X/hwsdlX_opt.c which includes this */

/* -------------------------------------------------------------------------- */

bool hw_opt_fullscreen = HW_DEFAULT_FULLSCREEN;
int hw_opt_screen_winw = 0;
int hw_opt_screen_winh = 0;
int hw_opt_screen_fsw = 0;
int hw_opt_screen_fsh = 0;
int hw_opt_mousespd = 100;
#ifdef FEATURE_MODEBUG
int hw_opt_overlay_pal = 0;
#endif
#ifdef HAVE_SDLX_ASPECT
int hw_opt_aspect = HW_DEFAULT_ASPECT;
#endif
char *hw_opt_sdlmixer_sf = NULL;

/* -------------------------------------------------------------------------- */

const struct cfg_items_s hw_cfg_items[] = {
    CFG_ITEM_BOOL("fs", &hw_opt_fullscreen),
    CFG_ITEM_INT("winw", &hw_opt_screen_winw, 0),
    CFG_ITEM_INT("winh", &hw_opt_screen_winh, 0),
    CFG_ITEM_INT("fsw", &hw_opt_screen_fsw, 0),
    CFG_ITEM_INT("fsh", &hw_opt_screen_fsh, 0),
    CFG_ITEM_INT("mousespd", &hw_opt_mousespd, 0),
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

/* -------------------------------------------------------------------------- */

const struct uiopt_s hw_uiopts[] = {
    UIOPT_ITEM_BOOL("Fullscreen", hw_opt_fullscreen, hw_video_toggle_fullscreen),
    UIOPT_ITEM_FUNC("Mouse spd", hw_uiopt_cb_mousespd),
    UIOPT_ITEM_SLIDER_INT(hw_opt_mousespd, 1, 200),
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
      options_set_int_var, (void *)&hw_opt_mousespd,
      "SPEED", "Set mouse speed (default = 100)" },
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
