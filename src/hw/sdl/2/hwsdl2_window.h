#ifndef INC_1OOM_HWSDL2_VIDEO_H
#define INC_1OOM_HWSDL2_VIDEO_H
#include "SDL.h"

#if SDL_VERSION_ATLEAST(2, 0, 5)
#define HAVE_INT_SCALING
/* have SDL_RenderSetIntegerScale */ 
#endif

int hwsdl_win_init(int moo_w, int moo_h); /* first time setup. called on startup only once */
void hwsdl_video_shutdown(void);
int hwsdl_video_resized(int w, int h); /* called after a resize event */

/* For the UI options. Return true on success */
bool hwsdl_video_toggle_autotrim(void);
bool hwsdl_video_toggle_fullscreen(void);
bool hwsdl_video_toggle_vsync(void);
#ifdef HAVE_INT_SCALING
bool hwsdl_video_toggle_intscaling(void);
#endif
bool hw_video_update_aspect(void);

/* put a new frame on display and show it */
struct hwsdl_video_buffer_s;
void hwsdl_video_next_frame(const struct hwsdl_video_buffer_s *buf);

/* show the same old thing again. called after expose event */
void hwsdl_video_update(void);

/* for hwsdl_opt.c */
#define hw_video_toggle_fullscreen hwsdl_video_toggle_fullscreen

#define FRAME_TIME_DUMP
#ifdef FRAME_TIME_DUMP
extern char *hw_opt_frame_time_dump_filename;
#endif

#endif
