#ifndef INC_1OOM_UI_FIX_H
#define INC_1OOM_UI_FIX_H

#include "types.h"

/* Fix cursor offset when navigating with cursor keys. */
extern bool ui_fix_cursor_nav_offset;

/* Fix graphics bug with brighter stars drawn further away than dimmer ones. */
extern bool ui_fix_starmap_background;

/* Allow navigation through objects with assigned hotkeys using the cursor keys. */
extern bool ui_qol_cursor_nav_all_obj;

/* Allow use of numeric key bindings when expected */
extern bool ui_qol_numeric_key_bindings;

/* Allow scrolling beyond map boundaries */
extern bool ui_qol_starmap_ext_scroll;

/* Move messages so that the selected star is always visible. */
extern bool ui_qol_starmap_msg_pos;

/* Disable question mark cursor. */
extern bool ui_qol_starmap_no_qmark_cursor;

/* Enable planet image button for all explored planets */
extern bool ui_qol_starmap_planet_pic;

extern void ui_enable_fix_bugs(void);
extern void ui_enable_fix_qol(void);

#endif
