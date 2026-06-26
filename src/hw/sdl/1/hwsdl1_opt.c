#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "hwsdl_opt.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#ifdef HAVE_OPENGL
int hw_opt_gl_filter = 1;
#endif

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s hw_cmdline_options[] = {
#ifdef HAVE_OPENGL
    { "-filt", 1,
      options_set_int_var, (void *)&hw_opt_gl_filter,
      "FILTER", "Set OpenGL filter (0 = nearest, 1 = linear)" },
#endif
    { NULL, 0, NULL, NULL, NULL, NULL }
};
