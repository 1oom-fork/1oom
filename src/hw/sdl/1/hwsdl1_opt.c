#include "config.h"

#include <stdio.h>

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

#ifdef HAVE_SDL1GL
int hw_opt_gl_filter = 1;
int hw_opt_bpp = 0;
#define HAVE_SDLX_ASPECT
#endif /* HAVE_SDL1GL */

#ifdef HAVE_SDL1MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL1MIXER */

/* -------------------------------------------------------------------------- */

const struct cfg_items_s hw_cfg_items_extra[] = {
#ifdef HAVE_SDL1GL
    CFG_ITEM_INT("bpp", &hw_opt_bpp, 0),
    CFG_ITEM_INT("filter", &hw_opt_gl_filter, 0),
#endif /* HAVE_SDL1GL */
    CFG_ITEM_END
};

#include "hwsdl_opt.c"

const struct cmdline_options_s hw_cmdline_options_extra[] = {
#ifdef HAVE_SDL1GL
    { "-bpp", 1,
      options_set_int_var, (void *)&hw_opt_bpp,
      "BPP", "Set bits/pixel (0 = autodetect)" },
    { "-filt", 1,
      options_set_int_var, (void *)&hw_opt_gl_filter,
      "FILTER", "Set OpenGL filter (0 = nearest, 1 = linear)" },
#endif /* HAVE_SDL1GL */
    { NULL, 0, NULL, NULL, NULL, NULL }
};
