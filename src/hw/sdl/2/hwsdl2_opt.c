#include "config.h"

#include <stdio.h>

#include <SDL.h>

#include "hw.h"
#include "cfg.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "hwsdl_video.h"
#include "lib.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_FULLSCREEN   false
#define HAVE_SDLX_ASPECT

bool hw_opt_borderless = false;
bool hw_opt_int_scaling = false;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    bool hw_opt_relmouse = false;
#else
    bool hw_opt_relmouse = true;
#endif
bool hw_opt_two_step_scaling = true;
bool hw_opt_autotrim = true;
bool hw_opt_vsync = true;
int hw_opt_scaling_quality = 0;

#ifdef HAVE_SDL2MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL2MIXER */

/* -------------------------------------------------------------------------- */

const struct cfg_items_s hw_cfg_items_extra[] = {
    CFG_ITEM_BOOL("borderless", &hw_opt_borderless),
    CFG_ITEM_BOOL("int_scaling", &hw_opt_int_scaling),
    CFG_ITEM_BOOL("relmouse", &hw_opt_relmouse),
    CFG_ITEM_BOOL("two_step_scaling", &hw_opt_two_step_scaling),
    CFG_ITEM_BOOL("autotrim", &hw_opt_autotrim),
    CFG_ITEM_BOOL("vsync", &hw_opt_vsync),
    CFG_ITEM_INT("scaling_quality", &hw_opt_scaling_quality, 0),
    CFG_ITEM_END
};

#include "hwsdl_opt.c"

const struct cmdline_options_s hw_cmdline_options_extra[] = {
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
    { "-relmouse", 0,
      options_enable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Use relative mouse mode" },
    { "-norelmouse", 0,
      options_disable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Do not use relative mouse mode" },
    { "-twostep", 0,
      options_enable_bool_var, (void *)&hw_opt_two_step_scaling,
      NULL, "Allow two-step scaling" },
    { "-notwostep", 0,
      options_disable_bool_var, (void *)&hw_opt_two_step_scaling,
      NULL, "Disable two-step scaling" },
    { "-autotrim", 0,
      options_enable_bool_var, &hw_opt_autotrim,
      NULL, "Enable automatic resize" },
    { "-noautotrim", 0,
      options_disable_bool_var, &hw_opt_autotrim,
      NULL, "Disable automatic resize" },
    { "-vsync", 0,
      options_enable_bool_var, &hw_opt_vsync,
      NULL, "Enable V-sync" },
    { "-novsync", 0,
      options_disable_bool_var, &hw_opt_vsync,
      NULL, "Disable V-sync" },
    { "-filt", 1,
     options_set_int_var, (void *)&hw_opt_scaling_quality,
     "FILTER", "Set scaling quality (0 = nearest, 1 = linear, 2 = best)" },
    { NULL, 0, NULL, NULL, NULL, NULL }
};
