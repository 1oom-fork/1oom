#ifndef INC_1OOM_HWSDL_ASPECT_H
#define INC_1OOM_HWSDL_ASPECT_H
#include "types.h"
#include "SDL.h"

#define HAVE_SDLX_ASPECT

/* 1000000:625000 matches 1oom video output size of 320x200 */
#define HW_DEFAULT_ASPECT   625000

extern int hw_opt_aspect; /* hwsdl_opt.c */
bool hw_video_update_aspect(void); /* in some file under 1/ or 2/ */

const char *hw_uiopt_cb_aspect_get(void);
bool hw_uiopt_cb_aspect_next(void);

/* shrink either w or h to meet ratio rx:ry */
void shrink_to_aspect_ratio(int w[1], int h[1], int rx, int ry);

#endif

