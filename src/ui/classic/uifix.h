#ifndef INC_1OOM_UI_FIX_H
#define INC_1OOM_UI_FIX_H

#include "types.h"

/* Fix the initial position of the planet list. */
extern bool ui_fix_planet_list_pos;

/* Fix displayed spy production to match actual. */
extern bool ui_fix_spy_cost;

/* Fix graphics bug with brighter stars drawn further away than dimmer ones. */
extern bool ui_fix_starmap_background;

/* Allow navigation through objects with assigned hotkeys using the cursor keys. */
extern bool ui_qol_cursor_nav_all_obj;

#endif
