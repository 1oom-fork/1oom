#ifndef INC_1OOM_HWWIN32_H
#define INC_1OOM_HWWIN32_H

#include <windows.h>

#include "types.h"

/* main window handle, set by hwwin32_video.c */
extern HWND hwwin32_hwnd;

/* window handling, hwwin32.c */
extern int hwwin32_window_create(int w, int h);
extern void hwwin32_window_destroy(void);

/* called from hw_event_handle to pump audio callbacks, hwwin32_audio.c */
extern void hw_audio_update(void);

#endif
