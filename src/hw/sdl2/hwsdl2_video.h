#ifndef INC_1OOM_HWSDL2_VIDEO_H
#define INC_1OOM_HWSDL2_VIDEO_H

#include "types.h"

extern bool hw_mouse_enabled;

extern int hw_video_resize(int w, int h);

extern void hw_mouse_grab(void);
extern void hw_mouse_ungrab(void);

extern unsigned int hw_video_get_window_id(void);

#endif
