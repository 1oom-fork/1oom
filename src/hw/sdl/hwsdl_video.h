#ifndef INC_1OOM_HWSDL_VIDEO_H
#define INC_1OOM_HWSDL_VIDEO_H

extern int hw_video_resize(int w, int h);
extern void hw_video_shrink(void);
extern void hw_video_enlarge(void);
extern bool hw_video_toggle_fullscreen(void);
extern void hw_video_input_grab(bool grab);

#endif
