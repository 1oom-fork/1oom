#ifndef INC_1OOM_MOUSE_H
#define INC_1OOM_MOUSE_H

#include "types.h"

#define MOUSE_BUTTON_MASK_LEFT  1
#define MOUSE_BUTTON_MASK_RIGHT 2

extern int mouse_x;
extern int mouse_y;
extern int mouse_buttons;
extern int mouse_stored_x;
extern int mouse_stored_y;
extern int mouse_click_x;
extern int mouse_click_y;
extern int mouse_click_buttons;

extern void mouse_set_xy_from_hw(int mx, int my);
extern void mouse_set_buttons_from_hw(int buttons);
extern void mouse_set_click_xy(int mx, int my);
extern void mouse_set_xy(int mx, int my);

extern bool mouse_getclear_hmm4(void);
extern bool mouse_getclear_hmm5(void);

#endif
