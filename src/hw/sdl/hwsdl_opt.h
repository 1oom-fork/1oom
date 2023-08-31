#ifndef INC_1OOM_HWSDL_OPT_H
#define INC_1OOM_HWSDL_OPT_H

#include "types.h"

extern bool hw_opt_fullscreen;
extern int hw_opt_screen_winw;
extern int hw_opt_screen_winh;
extern int hw_opt_screen_fsw;
extern int hw_opt_screen_fsh;
extern char *hw_opt_sdlmixer_sf;
extern int hw_opt_aspect;
extern bool hw_opt_force_sw;

/* for SDL1 */
extern int hw_opt_gl_filter;
extern int hw_opt_bpp;

/* for SDL2 */
extern bool hw_opt_borderless;
extern bool hw_opt_int_scaling;
extern bool hw_opt_relmouse;
extern bool hw_opt_two_step_scaling;
extern bool hw_opt_autotrim;
extern bool hw_opt_vsync;
extern int hw_opt_scaling_quality;

#endif
