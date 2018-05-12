#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_FULLSCREEN   false
#define HW_DEFAULT_ASPECT   833333

#ifdef HAVE_OPENGL
bool hw_opt_use_gl = true;
int hw_opt_aspect = HW_DEFAULT_ASPECT;
int hw_opt_gl_filter = 1;
#endif

#ifdef HAVE_SDL1MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL1MIXER */

/* -------------------------------------------------------------------------- */

#include "hwsdl_opt.c"

const struct cmdline_options_s hw_cmdline_options_extra[] = {
#ifdef HAVE_OPENGL
    { "-gl", 0,
      options_enable_bool_var, (void *)&hw_opt_use_gl,
      NULL, "Enable OpenGL" },
    { "-nogl", 0,
      options_disable_bool_var, (void *)&hw_opt_use_gl,
      NULL, "Disable OpenGL" },
    { "-aspect", 1,
      options_set_int_var, (void *)&hw_opt_aspect,
      "ASPECT", "Set aspect ratio (*1000000, 0 = off)" },
    { "-filt", 1,
      options_set_int_var, (void *)&hw_opt_gl_filter,
      "FILTER", "Set OpenGL filter (0 = nearest, 1 = linear)" },
#endif
    { NULL, 0, NULL, NULL, NULL, NULL }
};
