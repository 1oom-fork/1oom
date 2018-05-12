/* already included by includer */
#if 0
#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "hwsdl_opt.h"
#include "options.h"
#include "types.h"
#endif

/* -------------------------------------------------------------------------- */

#define HW_DEFAULT_ASPECT   1000000

int hw_opt_fullscreen = HW_DEFAULT_FULLSCREEN;
int hw_opt_screen_winw = 0;
int hw_opt_screen_winh = 0;
int hw_opt_screen_fsw = 0;
int hw_opt_screen_fsh = 0;
#ifdef HAVE_SDLX_ASPECT
int hw_opt_aspect = HW_DEFAULT_ASPECT;
#endif
const char *hw_opt_sdlmixer_sf = NULL;

/* -------------------------------------------------------------------------- */

#ifdef HAVE_SDLMIXER
int hw_opt_set_sdlmixer_sf(char **argv, void *var)
{
    hw_opt_sdlmixer_sf = argv[1];
    return hw_audio_set_sdlmixer_sf(hw_opt_sdlmixer_sf);
}
#endif

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s hw_cmdline_options[] = {
    { "-fs", 0,
      options_enable_var, (void *)&hw_opt_fullscreen,
      NULL, "Enable fullscreen" },
    { "-window", 0,
      options_disable_var, (void *)&hw_opt_fullscreen,
      NULL, "Use windowed mode" },
    { "-winw", 1,
      options_set_int_var, (void *)&hw_opt_screen_winw,
      "WIDTH", "Set window width" },
    { "-winh", 1,
      options_set_int_var, (void *)&hw_opt_screen_winh,
      "HEIGHT", "Set window height" },
    { "-fsw", 1,
      options_set_int_var, (void *)&hw_opt_screen_fsw,
      "WIDTH", "Set fullscreen width" },
    { "-fsh", 1,
      options_set_int_var, (void *)&hw_opt_screen_fsh,
      "HEIGHT", "Set fullscreen height" },
#ifdef HAVE_SDLMIXER
    { "-sdlmixersf", 1,
      hw_opt_set_sdlmixer_sf, NULL,
      "FILE.SF2", "Set SDL_mixer soundfont" },
#endif
#ifdef HAVE_SDLX_ASPECT
    { "-aspect", 1,
      options_set_int_var, (void *)&hw_opt_aspect,
      "ASPECT", "Set aspect ratio (*1000000, 0 = off)" },
#endif
    { NULL, 0, NULL, NULL, NULL, NULL }
};
