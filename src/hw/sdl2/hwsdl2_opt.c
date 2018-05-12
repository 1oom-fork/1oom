#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "hwsdl2_opt.h"
#include "lib.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_FULLSCREEN   false

bool hw_opt_borderless = false;
bool hw_opt_int_scaling = false;
bool hw_opt_vsync = true;

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s hw_cmdline_options[] = {
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
    { "-vsync", 0,
      options_enable_bool_var, &hw_opt_vsync,
      NULL, "Enable V-sync" },
    { "-novsync", 0,
      options_disable_bool_var, &hw_opt_vsync,
      NULL, "Disable V-sync" },
    { NULL, 0, NULL, NULL, NULL, NULL }
};
