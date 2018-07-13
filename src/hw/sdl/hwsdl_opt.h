#ifndef INC_1OOM_HWSDL_OPT_H
#define INC_1OOM_HWSDL_OPT_H

#define HW_DEFAULT_ASPECT   833333

extern bool hw_opt_fullscreen;
extern int hw_opt_screen_winw;
extern int hw_opt_screen_winh;
extern int hw_opt_screen_fsw;
extern int hw_opt_screen_fsh;
extern char *hw_opt_sdlmixer_sf;
extern int hw_opt_aspect;
extern int hw_opt_mousespd;

/* debug */
extern int hw_opt_overlay_pal;

/* for SDL1 */
extern bool hw_opt_use_gl;
extern int hw_opt_gl_filter;
extern int hw_opt_bpp;

/* for SDL2 */
extern bool hw_opt_force_sw;
extern bool hw_opt_int_scaling;
extern bool hw_opt_relmouse;

#endif
