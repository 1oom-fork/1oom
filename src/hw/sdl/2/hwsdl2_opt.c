#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "cfg.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "hwsdl_video.h"
#include "lib.h"
#include "log.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_FULLSCREEN   false
#define HAVE_SDLX_ASPECT

bool hw_opt_force_sw = false;
bool hw_opt_int_scaling = false;
bool hw_opt_relmouse = true;
bool hw_opt_autotrim = true;
bool hw_opt_vsync = true;

#ifdef HAVE_SDL2MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL2MIXER */

/* -------------------------------------------------------------------------- */

const struct cfg_items_s hw_cfg_items_extra[] = {
    CFG_ITEM_BOOL("force_sw", &hw_opt_force_sw),
    CFG_ITEM_BOOL("int_scaling", &hw_opt_int_scaling),
    CFG_ITEM_BOOL("relmouse", &hw_opt_relmouse),
    CFG_ITEM_BOOL("autotrim", &hw_opt_autotrim),
    CFG_ITEM_BOOL("vsync", &hw_opt_vsync),
    CFG_ITEM_END
};

const struct uiopt_s hw_uiopts_extra[] = {
    UIOPT_ITEM_END
};

#include "hwsdl_opt.c"

const struct cmdline_options_s hw_cmdline_options_extra[] = {
    { "-forcesw", 0,
      options_enable_bool_var, (void *)&hw_opt_force_sw,
      NULL, "Force software rendering" },
    { "-noforcesw", 0,
      options_disable_bool_var, (void *)&hw_opt_force_sw,
      NULL, "Do not force software rendering" },
    { "-intscaling", 0,
      options_enable_bool_var, (void *)&hw_opt_int_scaling,
      NULL, "Force integer scaling" },
    { "-nointscaling", 0,
      options_disable_bool_var, (void *)&hw_opt_int_scaling,
      NULL, "Do not force integer scaling" },
    { "-relmouse", 0,
      options_enable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Use relative mouse mode (default)" },
    { "-norelmouse", 0,
      options_disable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Do not use relative mouse mode\nRelative mouse mode is needed for normal operation.\nDisable it at your own risk!" },
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
    { NULL, 0, NULL, NULL, NULL, NULL }
};
