#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_str.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uiobj.h"
#include "uisearch.h"
#include "uisound.h"
#include "uistarmap_common.h"

/* -------------------------------------------------------------------------- */

static void ui_starmap_reloc_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *pf = &g->planet[d->from];
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    char buf[0x40];
    int x0, y0;
    STARMAP_LIM_INIT();
    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    x0 = (pf->x - ui_data.starmap.x) * 2 + 8;
    y0 = (pf->y - ui_data.starmap.y) * 2 + 8;
    if (g->planet_focus_i[d->api] != d->from) {
        int x1, y1;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 14;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 14;
        ui_draw_line_limit_ctbl(x0 + 6, y0 + 6, x1, y1, colortbl_line_green, 5, ui_data.starmap.line_anim_phase, starmap_scale);
    }
    lbxgfx_draw_frame_offs(x0, y0, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    lbxgfx_draw_frame(222, 80, ui_data.gfx.starmap.relocate, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_1(5, 5, 0, 0);
    lbxfont_print_str_center(269, 90, game_str_sm_sreloc, UI_SCREEN_W, ui_scale);
    lbxfont_select(0, 6, 0, 0);
    lbxfont_print_str_split(229, 105, 80, game_str_sm_sreloc2, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
    if (g->planet_focus_i[d->api] != d->from) {
        if (pf->have_stargate && pt->have_stargate) {
            lib_strcpy(buf, game_str_sm_stargate, sizeof(buf));
        } else {
            int eta;
            eta = game_calc_eta_ship(g, g->srd[d->api].design[pf->buildship].engine + 1, pf->x, pf->y, pt->x, pt->y);
            lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_delay, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
        }
        lbxfont_select(0, 0, 0, 0);
        lbxfont_print_str_center(268, 149, buf, UI_SCREEN_W, ui_scale);
    }
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
    lbxgfx_draw_frame(271, 163, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W, ui_scale);
}

/* -------------------------------------------------------------------------- */

void ui_starmap_reloc(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_search
            ;
    struct starmap_data_s d;

    d.scrollx = 0;
    d.scrolly = 0;
    d.scrollz = starmap_scale;
    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;
    d.ruler_from_i = d.ruler_to_i = -1;
    d.gov_highlight = 0;
    {
        uint8_t pi = g->planet_focus_i[active_player];
        d.from = pi;
        g->planet_focus_i[active_player] = g->planet[pi].reloc;
        if (g->planet[g->planet[pi].reloc].owner != active_player) {
            g->planet_focus_i[active_player] = pi;
        }
    }

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(2);
    uiobj_set_callback_and_delay(ui_starmap_reloc_draw_cb, &d, STARMAP_DELAY);

    while (!flag_done) {
        ui_delay_prepare();
        if (ui_starmap_handle_common(g, &d, &flag_done)) {
        } else if (d.oi1 == oi_search) {
            int i;
            i = ui_search(g, active_player);
            if (i >= 0) {
                if (g->planet[i].owner == active_player) {
                    ui_sound_play_sfx_24();
                    g->planet_focus_i[active_player] = i;
                    ui_starmap_set_pos_focus(g, active_player);
                } else {
                    ui_sound_play_sfx_06();
                    flag_done = true;
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                }
            }
        } else if (d.oi1 == d.oi_f2) {
            int i;
            i = g->planet_focus_i[active_player];
            do {
                if (--i < 0) { i = g->galaxy_stars - 1; }
            } while (g->planet[i].owner != active_player);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (d.oi1 == d.oi_f3) {
            int i;
            i = g->planet_focus_i[active_player];
            do {
                i = (i + 1) % g->galaxy_stars;
            } while (g->planet[i].owner != active_player);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (((d.oi1 == d.oi_f8) || (d.oi1 == d.oi_f9)) && g->eto[active_player].have_ia_scanner) {
            int i, pi;
            ui_sound_play_sfx_24();
            pi = g->planet_focus_i[active_player];
            i = ui_starmap_enemy_incoming(g, active_player, pi, (d.oi1 == d.oi_f8));
            if (i != pi) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
            }
        } else if (d.oi1 == d.oi_f4) {
            bool found;
            int i, pi;
            i = pi = g->planet_focus_i[active_player];
            found = false;
            do {
                i = (i + 1) % g->galaxy_stars;
                for (int j = 0; j < g->eto[active_player].shipdesigns_num; ++j) {
                    if (g->eto[active_player].orbit[i].ships[j]) {
                        found = true;
                        break;
                    }
                }
            } while ((!found) && (i != pi));
            if (found) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (d.oi1 == d.oi_f5) {
            bool found;
            int i, pi;
            i = pi = g->planet_focus_i[active_player];
            found = false;
            do {
                if (--i < 0) { i = g->galaxy_stars - 1; }
                for (int j = 0; j < g->eto[active_player].shipdesigns_num; ++j) {
                    if (g->eto[active_player].orbit[i].ships[j]) {
                        found = true;
                        break;
                    }
                }
            } while ((!found) && (i != pi));
            if (found) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (d.oi1 == d.oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, g->planet_focus_i[active_player]);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (d.oi1 == d.oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, g->planet_focus_i[active_player]);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        }
        if (d.oi1 == d.oi_accept) {
do_accept:
            ui_sound_play_sfx_24();
            flag_done = true;
            g->planet[d.from].reloc = g->planet_focus_i[active_player];
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        }
        ui_starmap_handle_oi_ctrl(&d, d.oi1);
        ui_starmap_handle_tag(&d, d.oi1, false);
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if (d.oi1 == d.oi_tbl_stars[i]) {
                if (ui_extra_enabled) {
                    g->planet_focus_i[active_player] = i;
                    d.oi1 = d.oi_accept;
                    goto do_accept;
                }
                g->planet_focus_i[active_player] = i;
                ui_sound_play_sfx_24();
                break;
            }
            else if (d.oi2 == d.oi_tbl_stars[i]) {
                if (ui_extra_enabled && g->planet_focus_i[active_player] != i) {
                    g->planet_focus_i[active_player] = i;
                    break;
                }
            }
        }
        d.ruler_from_i = d.from;
        d.ruler_to_i = ui_starmap_cursor_on_star(g, &d, d.oi2, active_player);
        d.ruler_from_fleet = false;
        if (!flag_done) {
            ui_starmap_select_bottom_highlight(g, &d, d.oi2);
            ui_starmap_reloc_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            ui_starmap_add_oi_hotkeys(&d);
            ui_starmap_fill_oi_tbl_stars_own(&d, active_player);
            d.oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            if (g->planet[d.from].buildship != BUILDSHIP_STARGATE) {
                d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            }
            d.oi_scroll = uiobj_add_tb(6, 6, 2, 2, 108, 86, &d.scrollx, &d.scrolly, &d.scrollz, ui_scale);
            oi_search = uiobj_add_inputkey(MOO_KEY_SLASH);
            ui_starmap_fill_oi_ctrl(&d);
            ui_starmap_add_oi_bottom_buttons(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.from;
}
