#include "config.h"

#include <stdio.h>

#include <SDL.h>

#include "hw.h"
#include "cfg.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "hwsdl_video.h"
#include "hwsdl2_video.h"
#include "lib.h"
#include "menu.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_FULLSCREEN   false

bool hw_opt_aspect_ratio_correct = true;
bool hw_opt_borderless = false;
bool hw_opt_int_scaling = false;
#ifdef IS_WINDOWS
    bool hw_opt_nograbmouse = true;
#else
    bool hw_opt_nograbmouse = false;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 18)
    bool hw_opt_relmouse = false;
#else
    bool hw_opt_relmouse = true;
#endif
bool hw_opt_smooth_pixel_scaling = true;
bool hw_opt_vsync = true;

#ifdef HAVE_SDL2MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL2MIXER */

/* -------------------------------------------------------------------------- */

const struct cfg_items_s hw_cfg_items_extra[] = {
    CFG_ITEM_BOOL("aspect", &hw_opt_aspect_ratio_correct),
    CFG_ITEM_BOOL("borderless", &hw_opt_borderless),
    CFG_ITEM_BOOL("int_scaling", &hw_opt_int_scaling),
    CFG_ITEM_BOOL("nograbmouse", &hw_opt_nograbmouse),
    CFG_ITEM_BOOL("relmouse", &hw_opt_relmouse),
    CFG_ITEM_BOOL("smooth_scaling", &hw_opt_smooth_pixel_scaling),
    CFG_ITEM_BOOL("vsync", &hw_opt_vsync),
    CFG_ITEM_END
};

#include "hwsdl_opt.c"

const struct cmdline_options_s hw_cmdline_options_extra[] = {
    { "-aspect", 0,
      options_enable_bool_var, (void *)&hw_opt_aspect_ratio_correct,
      NULL, "Enable aspect ratio correction" },
    { "-noaspect", 0,
      options_disable_bool_var, (void *)&hw_opt_aspect_ratio_correct,
      NULL, "Disable aspect ratio correction" },
    { "-borderless", 0,
      options_enable_bool_var, &hw_opt_borderless,
      NULL, "Enable borderless window" },
    { "-noborderless", 0,
      options_disable_bool_var, &hw_opt_borderless,
      NULL, "Disable borderless window" },
    { "-intscaling", 0,
      options_enable_bool_var, (void *)&hw_opt_int_scaling,
      NULL, "Force integer scaling" },
    { "-nointscaling", 0,
      options_disable_bool_var, (void *)&hw_opt_int_scaling,
      NULL, "Do not force integer scaling" },
    { "-grabmouse", 0,
      options_disable_bool_var, (void *)&hw_opt_nograbmouse,
      NULL, "Allow grab mouse" },
    { "-nograbmouse", 0,
      options_enable_bool_var, (void *)&hw_opt_nograbmouse,
      NULL, "Do not allow grab mouse" },
    { "-relmouse", 0,
      options_enable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Use relative mouse mode" },
    { "-norelmouse", 0,
      options_disable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Do not use relative mouse mode" },
    { "-smoothscaling", 0,
      options_enable_bool_var, (void *)&hw_opt_smooth_pixel_scaling,
      NULL, "Enable smooth pixel scaling" },
    { "-nosmoothscaling", 0,
      options_disable_bool_var, (void *)&hw_opt_smooth_pixel_scaling,
      NULL, "Disable smooth pixel scaling" },
    { "-vsync", 0,
      options_enable_bool_var, &hw_opt_vsync,
      NULL, "Enable V-sync" },
    { "-novsync", 0,
      options_disable_bool_var, &hw_opt_vsync,
      NULL, "Disable V-sync" },
    { NULL, 0, NULL, NULL, NULL, NULL }
};

void hw_opt_menu_make_page_video(void)
{
    menu_make_bool_func(menu_allocate_item(), "Fullscreen", &hw_opt_fullscreen, hw_video_toggle_fullscreen, MOO_KEY_f);
    #if SDL_VERSION_ATLEAST(2, 0, 18)
        menu_make_bool_func(menu_allocate_item(), "V-sync", &hw_opt_vsync, hw_video_toggle_vsync, MOO_KEY_v);
    #endif
    #if SDL_VERSION_ATLEAST(2, 0, 5)
        menu_make_bool_func(menu_allocate_item(), "Integer scaling", &hw_opt_int_scaling, hw_video_toggle_int_scaling, MOO_KEY_i);
    #endif
}
