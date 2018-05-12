#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "lib.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_FULLSCREEN   false
#define HAVE_SDLX_ASPECT

bool hw_opt_int_scaling = false;

#ifdef HAVE_SDL2MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL2MIXER */

/* -------------------------------------------------------------------------- */

#include "hwsdl_opt.c"

const struct cmdline_options_s hw_cmdline_options_extra[] = {
    { "-intscaling", 0,
      options_enable_var, (void *)&hw_opt_int_scaling,
      NULL, "Force integer scaling" },
    { "-nointscaling", 0,
      options_disable_var, (void *)&hw_opt_int_scaling,
      NULL, "Do not force integer scaling" },
    { NULL, 0, NULL, NULL, NULL, NULL }
};
