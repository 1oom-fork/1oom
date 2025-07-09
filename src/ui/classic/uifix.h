#ifndef INC_1OOM_UI_FIX_H
#define INC_1OOM_UI_FIX_H

#include "types.h"

/* Fix cursor offset when navigating with cursor keys. */
extern bool ui_fix_cursor_nav_offset;

/* Empire tech report technology text fix, all Controlled Colonies show ENVIRON. */
extern bool ui_fix_empirereport_enviro;

/* Fix the initial position of the planet list. */
extern bool ui_fix_planet_list_pos;

/* Fix displayed spy production to match actual. */
extern bool ui_fix_spy_cost;

/* Fix graphics bug with brighter stars drawn further away than dimmer ones. */
extern bool ui_fix_starmap_background;

/* Fix displayed tech complete 1/2 probability. */
extern bool ui_fix_tech_complete_probability;

/* Allow navigation through objects with assigned hotkeys using the cursor keys. */
extern bool ui_qol_cursor_nav_all_obj;

/* Disable LMB in some inappropriate cases */
extern bool ui_qol_no_cancel_via_lmb;

/* Allow scrolling beyond map boundaries */
extern bool ui_qol_starmap_ext_scroll;

/* Move messages so that the selected star is always visible. */
extern bool ui_qol_starmap_msg_pos;

/* Disable question mark cursor. */
extern bool ui_qol_starmap_no_qmark_cursor;

/* Stars in nebulas get a purple instead of a red frame */
extern bool ui_qol_starmap_planet_neb;

/* Enable planet image button for all explored planets */
extern bool ui_qol_starmap_planet_pic;

#endif
