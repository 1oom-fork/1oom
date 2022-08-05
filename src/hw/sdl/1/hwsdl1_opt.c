#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "cfg.h"
#include "hwsdl_opt.h"
#include "hwsdl_audio.h"
#include "hwsdl1_video.h"
#include "lib.h"
#include "log.h"
#include "menu.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#ifdef HAVE_SDL1GL
bool hw_opt_use_gl = true;
int hw_opt_gl_filter = 1;
int hw_opt_bpp = 0;
#define HAVE_SDLX_ASPECT
#endif /* HAVE_SDL1GL */

#ifdef HAVE_SDL1MIXER
#define HAVE_SDLMIXER
#endif /* HAVE_SDLMIXER1 */

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
    CFG_ITEM_BOOL("gl", &hw_opt_use_gl),
    CFG_ITEM_INT("bpp", &hw_opt_bpp, 0),
    CFG_ITEM_INT("filter", &hw_opt_gl_filter, 0),
#endif /* HAVE_SDL1GL */
    CFG_ITEM_END
};


const struct uiopt_s hw_uiopts_extra[] = {
#ifdef HAVE_SDL1GL
    UIOPT_ITEM_CYCLE("Filter", hw_uiopts_filter_get, hw_uiopts_filter_next),
#endif /* HAVE_SDL1GL */
    UIOPT_ITEM_END
};

#include "hwsdl_opt.c"

const struct cmdline_options_s hw_cmdline_options_extra[] = {
#ifdef HAVE_SDL1GL
    { "-gl", 0,
      options_enable_bool_var, (void *)&hw_opt_use_gl,
      NULL, "Enable OpenGL" },
    { "-nogl", 0,
      options_disable_bool_var, (void *)&hw_opt_use_gl,
      NULL, "Disable OpenGL" },
    { "-bpp", 1,
      options_set_int_var, (void *)&hw_opt_bpp,
      "BPP", "Set bits/pixel (0 = autodetect)" },
    { "-filt", 1,
      options_set_int_var, (void *)&hw_opt_gl_filter,
      "FILTER", "Set OpenGL filter (0 = nearest, 1 = linear)" },
#endif /* HAVE_SDL1GL */
    { NULL, 0, NULL, NULL, NULL, NULL }
};

static void hwsdl1_mm_toggle_fullscreen(void *vptr)
{
    hw_video_toggle_fullscreen();
}

#ifdef HAVE_SDLX_ASPECT
static void hwsdl1_mm_aspect_next(void *vptr)
{
    hw_uiopt_cb_aspect_next();
    hw_video_update_aspect();
}
#endif

static void hwsdl1_mm_filter_next(void *vptr)
{
    hw_uiopts_filter_next();
}

static struct main_menu_item_data_s hwsdl1_mm_opt_items[] = {
    {
        MAIN_MENU_ITEM_TYPE_BOOL,
        hwsdl1_mm_toggle_fullscreen, NULL,
        "Fullscreen", NULL, &hw_opt_fullscreen, 0,
        0, 0,
        MOO_KEY_f,
    },
#ifdef HAVE_SDLX_ASPECT
    {
        MAIN_MENU_ITEM_TYPE_STR,
        hwsdl1_mm_aspect_next, NULL,
        "Aspect Ratio", NULL, hw_uiopt_cb_aspect_get, 0,
        0, 0,
        MOO_KEY_a,
    },
#endif
#ifdef HAVE_SDL1GL
    {
        MAIN_MENU_ITEM_TYPE_STR,
        hwsdl1_mm_filter_next, NULL,
        "Filter", NULL, hw_uiopts_filter_get, 0,
        0, 0,
        MOO_KEY_i,
    },
#endif /* HAVE_SDL1GL */
    {
        MAIN_MENU_ITEM_TYPE_NONE,
        NULL, NULL,
        NULL, NULL, NULL, 0,
        0, 0,
        MOO_KEY_UNKNOWN,
    },
};

void *hw_video_get_menu_item(int i)
{
    if (hwsdl1_mm_opt_items[i].type == MAIN_MENU_ITEM_TYPE_NONE) {
        return NULL;
    }
    return &hwsdl1_mm_opt_items[i];
}
