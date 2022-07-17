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

static void ui_starmap_bases_draw_cb1(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *p = &(g->planet[g->planet_focus_i[d->api]]);
    ui_starmap_draw_basic(d);
    lbxgfx_draw_frame(222, 80, ui_data.gfx.starmap.relocate, UI_SCREEN_W, ui_scale);
    lbxgfx_draw_frame(230, 123, ui_data.gfx.starmap.tran_bar, UI_SCREEN_W, ui_scale);
    ui_draw_filled_rect(233, 126, 246, 130, 0x7, ui_scale);
    ui_draw_filled_rect(233, 138, 304, 143, 0x7, ui_scale);
    ui_draw_line1(233, 125, 246, 125, 0xfb, ui_scale);
    ui_draw_line1(233, 137, 304, 137, 0xfb, ui_scale);
    if (d->ba.slider_var > 0) {
        ui_draw_slider(258, 127, d->ba.slider_var * 40, p->missile_bases, 0, 0x73, ui_scale);
    }

    lbxfont_select_set_12_1(5, 5, 0, 0);
    lbxfont_print_str_center(269, 90, game_str_bs_line1, UI_SCREEN_W, ui_scale);
    lbxfont_select(0, 6, 0, 0);
    lbxfont_print_str_split(229, 105, 80, game_str_bs_line2, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
    lbxfont_print_str_normal(234, 137, "Scrap", UI_SCREEN_W, ui_scale);
    lbxfont_print_str_normal(234, 125, "Bas", UI_SCREEN_W, ui_scale);
    {
        int n = d->ba.slider_var;
        lbxfont_select_set_12_1(0, 1, 0, 0);
        lbxfont_print_num_center(268, 137, n, UI_SCREEN_W, ui_scale);
        lbxfont_select(0, 6, 0, 0);
        lbxfont_print_str_normal(283, 137, (n == 1) ? game_str_bs_base : game_str_bs_bases, UI_SCREEN_W, ui_scale);
    }
}

/* -------------------------------------------------------------------------- */

static bool ui_starmap_bases_is_valid_selection(const struct game_s *g, const struct starmap_data_s *d, int i)
{
    const planet_t *p = &(g->planet[g->planet_focus_i[d->api]]);
    return p->owner == d->api;
}

void ui_starmap_bases(struct game_s *g, player_id_t active_player)
{
    struct starmap_data_s d;
    ui_starmap_init_common_data(g, &d, active_player);

    d.controllable = false;
    d.draw_own_routes = true;
    d.is_valid_selection = ui_starmap_bases_is_valid_selection;
    d.do_accept = NULL;

    int16_t oi_plus, oi_minus;
    planet_t *p = &(g->planet[g->planet_focus_i[active_player]]);

    d.ba.slider_var = 0;
    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        oi_plus = UIOBJI_INVALID; \
        oi_minus = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_callback_and_delay(ui_starmap_bases_draw_cb1, &d, STARMAP_DELAY);

    while (!d.flag_done) {
        ui_delay_prepare();
        if (ui_starmap_handle_common(g, &d)) {
        } else if (d.oi1 == d.oi_accept) {
            int n;
            ui_sound_play_sfx_24();
            n = d.ba.slider_var;
            p->missile_bases -= n;
            g->eto[active_player].reserve_bc += (n * game_get_base_cost(g, active_player)) / 4;
            d.flag_done = true;
            SETMAX(p->missile_bases, 0);
        } else if (d.oi1 == oi_minus) {
            --d.ba.slider_var;
            SETMAX(d.ba.slider_var, 0);
        } else if (d.oi1 == oi_plus) {
            ++d.ba.slider_var;
            SETMIN(d.ba.slider_var, p->missile_bases);
        }
        if (!d.flag_done) {
            ui_starmap_select_bottom_highlight(g, &d);
            ui_starmap_bases_draw_cb1(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            ui_starmap_fill_oi_common(&d);
            d.oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            if (p->missile_bases > 0) {
                uiobj_add_slider_int(258, 124, 0, p->missile_bases, 41, 8, &d.ba.slider_var);
                oi_minus = uiobj_add_mousearea(252, 124, 256, 131, MOO_KEY_UNKNOWN);
                oi_plus = uiobj_add_mousearea(301, 124, 305, 131, MOO_KEY_UNKNOWN);
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }

    uiobj_unset_callback();
    uiobj_table_clear();
}
