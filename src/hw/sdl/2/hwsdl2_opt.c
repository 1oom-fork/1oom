#include "config.h"

#include <stdio.h>

#include <SDL.h>

#include "hw.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "lib.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_FULLSCREEN   false

bool hw_opt_aspect_ratio_correct = true;
bool hw_opt_borderless = false;
bool hw_opt_int_scaling = false;
bool hw_opt_relmouse = false;
bool hw_opt_vsync = true;

#ifdef HAVE_SDL2MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL2MIXER */

/* -------------------------------------------------------------------------- */

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
    { "-relmouse", 0,
      options_enable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Use relative mouse mode" },
    { "-norelmouse", 0,
      options_disable_bool_var, (void *)&hw_opt_relmouse,
      NULL, "Do not use relative mouse mode" },
    { "-vsync", 0,
      options_enable_bool_var, &hw_opt_vsync,
      NULL, "Enable V-sync" },
    { "-novsync", 0,
      options_disable_bool_var, &hw_opt_vsync,
      NULL, "Disable V-sync" },
    { NULL, 0, NULL, NULL, NULL, NULL }
};
