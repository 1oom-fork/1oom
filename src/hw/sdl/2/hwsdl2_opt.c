#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "cfg.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "hwsdl2_window.h"
#include "lib.h"
#include "log.h"
#include "menu.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

bool hw_opt_force_sw = false;
#ifdef HAVE_INT_SCALING
bool hw_opt_int_scaling = false;
#endif
bool hw_opt_lock_texture = false;
bool hw_opt_autotrim = true;
bool hw_opt_vsync = true;
bool hw_opt_texture_filtering = false;

#ifdef FRAME_TIME_DUMP
char *hw_opt_frame_time_dump_filename = NULL;
#endif

#ifdef HAVE_SDL2MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL2MIXER */

/* -------------------------------------------------------------------------- */

const struct cfg_items_s hw_cfg_items_extra[] = {
    CFG_ITEM_BOOL("force_sw", &hw_opt_force_sw),
#ifdef HAVE_INT_SCALING
    CFG_ITEM_BOOL("int_scaling", &hw_opt_int_scaling),
#endif
    CFG_ITEM_BOOL("autotrim", &hw_opt_autotrim),
    CFG_ITEM_BOOL("locktex", &hw_opt_lock_texture),
    CFG_ITEM_BOOL("vsync", &hw_opt_vsync),
    CFG_ITEM_BOOL("texturefiltering", &hw_opt_texture_filtering),
    CFG_ITEM_END
};

const struct uiopt_s hw_uiopts_extra[] = {
#ifdef HAVE_INT_SCALING
    UIOPT_ITEM_BOOL("Integer scaling", hw_opt_int_scaling, hwsdl_video_toggle_intscaling),
#endif
    UIOPT_ITEM_BOOL("Auto-trim window", hw_opt_autotrim, hwsdl_video_toggle_autotrim),
    UIOPT_ITEM_BOOL("V-sync", hw_opt_vsync, hwsdl_video_toggle_vsync),
    UIOPT_ITEM_END
};

const struct cmdline_options_s hw_cmdline_options_extra[] = {
    { "-forcesw", 0,
      options_enable_bool_var, (void *)&hw_opt_force_sw,
      NULL, "Force software rendering" },
    { "-noforcesw", 0,
      options_disable_bool_var, (void *)&hw_opt_force_sw,
      NULL, "Do not force software rendering" },
#ifdef HAVE_INT_SCALING
    { "-intscaling", 0,
      options_enable_bool_var, (void *)&hw_opt_int_scaling,
      NULL, "Force integer scaling" },
    { "-nointscaling", 0,
      options_disable_bool_var, (void *)&hw_opt_int_scaling,
      NULL, "Do not force integer scaling" },
#endif
    { "-locktex", 0,
      options_enable_bool_var, &hw_opt_lock_texture,
      NULL, "Use SDL_LockTexture instead of SDL_UpdateTexture. May affect performance" },
    { "-autotrim", 0,
      options_enable_bool_var, &hw_opt_autotrim,
      NULL, "Enable automatic resize (trimming of black bars)" },
    { "-noautotrim", 0,
      options_disable_bool_var, &hw_opt_autotrim,
      NULL, "Disable automatic resize (trimming of black bars)" },
    { "-vsync", 0,
      options_enable_bool_var, &hw_opt_vsync,
      NULL, "Enable V-sync" },
    { "-novsync", 0,
      options_disable_bool_var, &hw_opt_vsync,
      NULL, "Disable V-sync" },
#ifdef FRAME_TIME_DUMP
    { "-frame-time-dump", 1,
      options_set_str_var, &hw_opt_frame_time_dump_filename,
      "FILEPATH", "dump timing of each frame into a CSV file" },
#endif
    { NULL, 0, NULL, NULL, NULL, NULL }
};

#include "hwsdl_opt.c"

static void hwsdl2_mm_toggle_fullscreen(void *vptr)
{
    hw_video_toggle_fullscreen();
}

#ifdef HAVE_INT_SCALING
static void hwsdl2_mm_toggle_intscaling(void *vptr)
{
    hwsdl_video_toggle_intscaling();
}
#endif

static void hwsdl2_mm_toggle_autotrim(void *vptr)
{
    hwsdl_video_toggle_autotrim();
}

static void hwsdl2_mm_toggle_vsync(void *vptr)
{
    hwsdl_video_toggle_vsync();
}

#ifdef HAVE_SDLX_ASPECT
static void hwsdl2_mm_aspect_next(void *vptr)
{
    hw_uiopt_cb_aspect_next();
    hw_video_update_aspect();
}
#endif

static struct main_menu_item_data_s hwsdl2_mm_opt_items[] = {
    {
        MAIN_MENU_ITEM_TYPE_BOOL,
        hwsdl2_mm_toggle_fullscreen, NULL,
        "Fullscreen", NULL, &hw_opt_fullscreen, 0,
        0, 0,
        MOO_KEY_f,
    },
#ifdef HAVE_SDLX_ASPECT
    {
        MAIN_MENU_ITEM_TYPE_STR,
        hwsdl2_mm_aspect_next, NULL,
        "Aspect Ratio", NULL, hw_uiopt_cb_aspect_get, 0,
        0, 0,
        MOO_KEY_a,
    },
#endif
#ifdef HAVE_INT_SCALING
    {
        MAIN_MENU_ITEM_TYPE_BOOL,
        hwsdl2_mm_toggle_intscaling, NULL,
        "Integer scaling", NULL, &hw_opt_int_scaling, 0,
        0, 0,
        MOO_KEY_i,
    },
#endif
    {
        MAIN_MENU_ITEM_TYPE_BOOL,
        hwsdl2_mm_toggle_autotrim, NULL,
        "Auto-trim window", NULL, &hw_opt_autotrim, 0,
        0, 0,
        MOO_KEY_t,
    },
    {
        MAIN_MENU_ITEM_TYPE_BOOL,
        hwsdl2_mm_toggle_vsync, NULL,
        "V-sync", NULL, &hw_opt_vsync, 0,
        0, 0,
        MOO_KEY_v,
    },
    {
        MAIN_MENU_ITEM_TYPE_NONE,
        NULL, NULL,
        NULL, NULL, NULL, 0,
        0, 0,
        MOO_KEY_UNKNOWN,
    },
};

void *hw_video_get_menu_item(int i)
{
    if (hwsdl2_mm_opt_items[i].type == MAIN_MENU_ITEM_TYPE_NONE) {
        return NULL;
    }
    return &hwsdl2_mm_opt_items[i];
}
