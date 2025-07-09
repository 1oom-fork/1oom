#ifndef INC_1OOM_UI_FIX_H
#define INC_1OOM_UI_FIX_H

#include "types.h"

/* Fix cursor offset when navigating with cursor keys. */
extern bool ui_fix_cursor_nav_offset;

/* Fix graphics bug with brighter stars drawn further away than dimmer ones. */
extern bool ui_fix_starmap_background;

/* Allow navigation through objects with assigned hotkeys using the cursor keys. */
extern bool ui_qol_cursor_nav_all_obj;

/* Move messages so that the selected star is always visible. */
extern bool ui_qol_starmap_msg_pos;

#endif
