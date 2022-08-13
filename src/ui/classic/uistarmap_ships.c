#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_str.h"
#include "game_tech.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uicursor.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "uistarmap_common.h"

/* -------------------------------------------------------------------------- */

#define UI_SM_PL_SHIPS_WIDTH 37
#define UI_SM_PL_SHIPS_BORDER_WIDTH 3
#define UI_SM_PL_SHIPS_HEIGHT 23
#define UI_SM_PL_SHIPS_BORDER_HEIGHT 1

static const int offset_x = UI_SM_PL_SHIPS_WIDTH + UI_SM_PL_SHIPS_BORDER_WIDTH * 2 + 1;
static const int offset_y = UI_SM_PL_SHIPS_HEIGHT + UI_SM_PL_SHIPS_BORDER_HEIGHT * 2 + 1;

static int ui_starmap_ships_get_x(int i)
{
    return 228 + offset_x * (i / 3);
}

static int ui_starmap_ships_get_y(int i)
{
    return 83 + offset_y * (i % 3);
}

static void ui_starmap_ships_draw_cb1(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *p = &(g->planet[g->planet_focus_i[d->api]]);
    char buf[0x80];
    ui_starmap_draw_basic(d);
    lbxgfx_draw_frame(222, 80, ui_data.gfx.starmap.relocate, UI_SCREEN_W, ui_scale);
    ui_draw_filled_rect(225, 81, 312, 160, 0, ui_scale);

    const uint8_t n = g->eto[d->api].shipdesigns_num;
    struct draw_stars_s ds;
    ds.xoff1 = 0;
    ds.xoff2 = 0;
    for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
        int x = ui_starmap_ships_get_x(i);
        int y = ui_starmap_ships_get_y(i);
        ui_draw_stars(x, y, i * 5, 32, &ds, ui_scale);
    }
    for (int i = 0; i < n; ++i) {
        const shipdesign_t *sd = &g->srd[d->api].design[i];
        uint8_t *gfx = ui_data.gfx.ships[sd->look];
        int x = ui_starmap_ships_get_x(i);
        int y = ui_starmap_ships_get_y(i);
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame(x, y, gfx, UI_SCREEN_W, ui_scale);
        lbxfont_select(2, 0xd, 0, 0);
        lbxfont_select_set_12_1(2, 0xa, 0, 0);
        lbxfont_print_str_center(x + 19, y + 18, sd->name, UI_SCREEN_W, ui_scale);
    }
    int i = p->buildship;
    if (i < n) {
        int x = ui_starmap_ships_get_x(i);
        int y = ui_starmap_ships_get_y(i);
        ui_draw_box1(x - UI_SM_PL_SHIPS_BORDER_WIDTH,
                     y - UI_SM_PL_SHIPS_BORDER_HEIGHT,
                     x + UI_SM_PL_SHIPS_BORDER_WIDTH + UI_SM_PL_SHIPS_WIDTH,
                     y + UI_SM_PL_SHIPS_BORDER_HEIGHT + UI_SM_PL_SHIPS_HEIGHT,
                     0x56, 0x56, ui_scale);
    }
    if (kbd_is_modifier(MOO_MOD_CTRL)) {
        lbxfont_select_set_12_1(2, 0xd, 0, 0);
        lbxfont_print_str_center(228 + offset_x, 84, game_str_sm_sh_everywhere, UI_SCREEN_W, ui_scale);
    } else if (kbd_is_modifier(MOO_MOD_ALT)) {
        const shipdesign_t *sd = &g->srd[d->api].design[p->buildship];
        lbxfont_select_set_12_1(2, 0xd, 0, 0);
        lib_sprintf(buf, sizeof(buf), "%s %s", game_str_sm_sh_replace, sd->name);
        lbxfont_print_str_center(228 + offset_x, 84, buf, UI_SCREEN_W, ui_scale);
    }
}

/* -------------------------------------------------------------------------- */

void ui_starmap_ships(struct game_s *g, player_id_t active_player)
{
    struct starmap_data_s d;
    ui_starmap_common_init(g, &d, active_player);
    d.disable_tags = true;

    int16_t oi_ship_design[NUM_SHIPDESIGNS];
    int16_t oi_s, oi_cancel;
    bool flag_done = false;

    planet_t *p = &(g->planet[g->planet_focus_i[active_player]]);

    ui_starmap_common_late_init(&d, ui_starmap_ships_draw_cb1, false);

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        UIOBJI_SET_TBL_INVALID(oi_ship_design); \
        oi_s = UIOBJI_INVALID; \
        oi_cancel = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    while (!flag_done) {
        int16_t oi1, oi2;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if (ui_starmap_common_handle_oi(g, &d, &flag_done, oi1, oi2)) {
        } else if (ui_starmap_handle_oi_finished(g, &d, &flag_done, oi1, oi2)) {
            oi1 = 0;
        } else if (oi1 == d.oi_accept) {
            ui_sound_play_sfx_24();
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
            flag_done = true;
        } else if (oi1 == oi_s) {
            p->buildship = (p->buildship + 1) % g->eto[active_player].shipdesigns_num;
        } else if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        }
        for (int i = 0; i < g->eto[active_player].shipdesigns_num; ++i) {
            if (oi1 == oi_ship_design[i]) {
                ui_sound_play_sfx_24();
                if (kbd_is_modifier(MOO_MOD_CTRL)) {
                    game_planet_ship_build_everywhere(g, p->owner, i);
                } else if (kbd_is_modifier(MOO_MOD_ALT)) {
                    game_planet_ship_replace_everywhere(g, p->owner, p->buildship, i);
                } else {
                    p->buildship = i;
                }
            }
        }
        if (!flag_done) {
            ui_starmap_select_bottom_highlight(&d, oi2);
            ui_starmap_ships_draw_cb1(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            ui_starmap_common_fill_oi(&d);
            oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            oi_s = uiobj_add_inputkey(MOO_KEY_s);
            for (int i = 0; i < g->eto[active_player].shipdesigns_num; ++i) {
                int x = ui_starmap_ships_get_x(i);
                int y = ui_starmap_ships_get_y(i);
                oi_ship_design[i] = uiobj_add_mousearea(
                            x - UI_SM_PL_SHIPS_BORDER_WIDTH,
                            y - UI_SM_PL_SHIPS_BORDER_HEIGHT,
                            x + UI_SM_PL_SHIPS_BORDER_WIDTH + UI_SM_PL_SHIPS_WIDTH,
                            y + UI_SM_PL_SHIPS_BORDER_HEIGHT + UI_SM_PL_SHIPS_HEIGHT,
                            MOO_KEY_1 + i);
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }

    uiobj_unset_callback();
    uiobj_table_clear();
}
