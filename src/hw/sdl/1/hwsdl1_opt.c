#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "cfg.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "hwsdl_video.h"
#include "lib.h"
#include "menu.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_FULLSCREEN   false
#define HW_DEFAULT_ASPECT   833333

#ifdef HAVE_SDL1GL
int hw_opt_aspect = HW_DEFAULT_ASPECT;
int hw_opt_gl_filter = 1;
int hw_opt_bpp = 0;
#endif /* HAVE_SDL1GL */

#ifdef HAVE_SDL1MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDL1MIXER */

/* -------------------------------------------------------------------------- */

#ifdef HAVE_SDL1GL
static const char *hw_gl_filter_str[2] = { "Nearest", "Linear" };

static const char *hw_uiopts_filter_get(void)
{
    return hw_gl_filter_str[hw_opt_gl_filter];
}

static bool hw_uiopts_filter_next(void)
{
    hw_opt_gl_filter = (hw_opt_gl_filter + 1) % 2;
    return true;
}
#endif /* HAVE_SDL1GL */

/* -------------------------------------------------------------------------- */

const struct cfg_items_s hw_cfg_items_extra[] = {
#ifdef HAVE_SDL1GL
    CFG_ITEM_INT("aspect", &hw_opt_aspect, 0),
    CFG_ITEM_INT("bpp", &hw_opt_bpp, 0),
    CFG_ITEM_INT("filter", &hw_opt_gl_filter, 0),
#endif /* HAVE_SDL1GL */
    CFG_ITEM_END
};

#include "hwsdl_opt.c"

const struct cmdline_options_s hw_cmdline_options_extra[] = {
#ifdef HAVE_SDL1GL
    { "-aspect", 1,
      options_set_int_var, (void *)&hw_opt_aspect,
      "ASPECT", "Set aspect ratio (*1000000, 0 = off)" },
    { "-bpp", 1,
      options_set_int_var, (void *)&hw_opt_bpp,
      "BPP", "Set bits/pixel (0 = autodetect)" },
    { "-filt", 1,
      options_set_int_var, (void *)&hw_opt_gl_filter,
      "FILTER", "Set OpenGL filter (0 = nearest, 1 = linear)" },
#endif /* HAVE_SDL1GL */
    { NULL, 0, NULL, NULL, NULL, NULL }
};

void hw_opt_menu_make_page_video(void)
{
    menu_make_bool_func(menu_allocate_item(), "Fullscreen", &hw_opt_fullscreen, hw_video_toggle_fullscreen, MOO_KEY_f);
#ifdef HAVE_SDL1GL
    menu_make_str_func(menu_allocate_item(), "Filter", hw_uiopts_filter_get, hw_uiopts_filter_next, MOO_KEY_i);
#endif /* HAVE_SDL1GL */
}
