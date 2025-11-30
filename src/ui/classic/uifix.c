#include "config.h"

#include "uifix.h"

/* -------------------------------------------------------------------------- */

bool ui_fix_cursor_nav_offset = false;
bool ui_fix_empirereport_enviro = false;
bool ui_fix_planet_list_pos = false;
bool ui_fix_spy_cost = false;
bool ui_fix_starmap_background = false;
bool ui_fix_tech_complete_probability = false;

bool ui_qol_cursor_nav_all_obj = false;
bool ui_qol_no_cancel_via_lmb = false;
bool ui_qol_extra_key_bindings = false;
bool ui_qol_starmap_ext_scroll = false;
bool ui_qol_starmap_msg_pos = false;
bool ui_qol_starmap_no_qmark_cursor = false;
bool ui_qol_starmap_planet_neb = false;
bool ui_qol_starmap_planet_pic = false;
bool ui_qol_gmap_scroll = false;

void ui_enable_unofficial_1_3a(void)
{
    ui_fix_empirereport_enviro = true;
    ui_fix_planet_list_pos = true;
    ui_fix_spy_cost = true;
    ui_fix_starmap_background = true;
    ui_fix_tech_complete_probability = true;
    ui_qol_no_cancel_via_lmb = true;
    ui_qol_extra_key_bindings = true;
    ui_qol_starmap_msg_pos = true;
    ui_qol_starmap_no_qmark_cursor = true;
}

void ui_enable_fix_bugs(void)
{
    ui_fix_cursor_nav_offset = true;
}

void ui_enable_fix_qol(void)
{
    ui_qol_cursor_nav_all_obj = true;
    ui_qol_starmap_ext_scroll = true;
    ui_qol_starmap_planet_pic = true;
    ui_qol_gmap_scroll = true;
}
