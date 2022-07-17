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

static int ui_starmap_ships_get_x(int i)
{
    static const int offset = UI_SM_PL_SHIPS_WIDTH + UI_SM_PL_SHIPS_BORDER_WIDTH * 2 + 1;
    return 228 + offset * (i / 3);
}

static int ui_starmap_ships_get_y(int i)
{
    static const int offset = UI_SM_PL_SHIPS_HEIGHT + UI_SM_PL_SHIPS_BORDER_HEIGHT * 2 + 1;
    return 83 + offset * (i % 3);
}

static void ui_starmap_ships_draw_cb1(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *p = &(g->planet[g->planet_focus_i[d->api]]);
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
}

/* -------------------------------------------------------------------------- */

static bool ui_starmap_ships_is_valid_selection(const struct game_s *g, const struct starmap_data_s *d, int i)
{
    const planet_t *p = &(g->planet[g->planet_focus_i[d->api]]);
    return p->owner == d->api;
}

void ui_starmap_ships(struct game_s *g, player_id_t active_player)
{
    struct starmap_data_s d;
    ui_starmap_init_common_data(g, &d, active_player);

    int16_t oi_ship_design[NUM_SHIPDESIGNS];

    d.controllable = false;
    d.draw_own_routes = true;
    d.tags_enabled = false;
    d.is_valid_selection = ui_starmap_ships_is_valid_selection;
    d.do_accept = NULL;

    planet_t *p = &(g->planet[g->planet_focus_i[active_player]]);

    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);

    uiobj_table_clear();
    STARMAP_UIOBJ_CLEAR_COMMON();

    ui_starmap_fill_oi_common(&d);
    d.oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
    d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
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

    uiobj_set_callback_and_delay(ui_starmap_ships_draw_cb1, &d, STARMAP_DELAY);

    while (!d.flag_done) {
        ui_delay_prepare();
        if (ui_starmap_handle_common(g, &d)) {
        } else if (d.oi1 == d.oi_accept) {
            ui_sound_play_sfx_24();
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
            d.flag_done = true;
        }
        for (int i = 0; i < g->eto[active_player].shipdesigns_num; ++i) {
            if (d.oi1 == oi_ship_design[i]) {
                ui_sound_play_sfx_24();
                p->buildship = i;
            }
        }
        if (!d.flag_done) {
            ui_starmap_select_bottom_highlight(g, &d);
            ui_starmap_ships_draw_cb1(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }

    uiobj_unset_callback();
    uiobj_table_clear();
}
