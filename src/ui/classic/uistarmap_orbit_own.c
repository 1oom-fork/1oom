#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_save.h"
#include "game_str.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "log.h"
#include "types.h"
#include "uicursor.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uiobj.h"
#include "uisound.h"
#include "uistarmap_common.h"

/* -------------------------------------------------------------------------- */

static inline bool ui_starmap_orbit_own_in_frange(struct starmap_data_s *d)
{
    uint8_t pi = d->g->planet_focus_i[d->api];
    const planet_t *p = &d->g->planet[pi];
    return (p->within_frange[d->api] == 1) || ((p->within_frange[d->api] == 2) && d->oo.sn0.have_reserve_fuel);
}

static void ui_starmap_orbit_own_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *pf = &g->planet[d->oo.from];
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    char buf[0x80];

    ui_starmap_draw_basic(d);
    {
        int x, y;
        x = (pf->x - ui_data.starmap.x) * 2 + 23;
        y = (pf->y - ui_data.starmap.y) * 2 + 5;
        lbxgfx_draw_frame_offs(x, y, ui_data.gfx.starmap.shipbord, STARMAP_LIMITS, UI_SCREEN_W);
    }
    ui_draw_filled_rect(225, 8, 314, 192, 7);
    lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.move_shi, UI_SCREEN_W);
    if (d->oo.sn0.num < NUM_SHIPDESIGNS) {
        lbxgfx_draw_frame(224, 151, ui_data.gfx.starmap.movextra, UI_SCREEN_W);
        ui_draw_filled_rect(228, 155, 309, 175, 7);
    }
    lbxfont_select_set_12_4(0, 5, 0, 0);
    lbxfont_print_str_center(268, 11, game_str_sm_fleetdep, UI_SCREEN_W);

    if (g->planet_focus_i[d->api] != d->oo.from) {
        int dist = game_get_min_dist(g, d->api, g->planet_focus_i[d->api]);
        int x0, y0, x1, y1;
        const uint8_t *ctbl;
        uint8_t *gfx;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 8;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 8;
        lbxgfx_draw_frame_offs(x1, y1, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W);
        x0 = (pf->x - ui_data.starmap.x) * 2 + 26;
        y0 = (pf->y - ui_data.starmap.y) * 2 + 8;
        ctbl = ui_starmap_orbit_own_in_frange(d) ? colortbl_line_green : colortbl_line_red;
        ui_draw_line_limit_ctbl(x0 + 3, y0 + 1, x1 + 6, y1 + 6, ctbl, 5, ui_data.starmap.line_anim_phase);
        gfx = ui_data.gfx.starmap.smalship[g->eto[d->api].banner];
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame_offs(x0, y0, gfx, STARMAP_LIMITS, UI_SCREEN_W);
        if (!ui_starmap_orbit_own_in_frange(d)) {
            if (d->oo.sn0.num < 7) { /* FIXME ?? always true */
                sprintf(buf, "%s %i %s", game_str_sm_destoor, dist, game_str_sm_parsfromcc);
                lbxfont_select(2, 0, 0, 0);
                lbxfont_set_gap_h(2);
                lbxfont_print_str_split(228, 156, 81, buf, 2, UI_SCREEN_W, UI_SCREEN_H);
            } else {
                /* dont care */
            }
        } else {
            if ((pt->owner == d->api) && (pf->owner == d->api) && pt->have_stargate && pf->have_stargate) {
                strcpy(buf, game_str_sm_stargate);
            } else if (d->oo.shiptypenon0numsel > 0) {
                const shipdesign_t *sd = &(g->srd[d->api].design[0]);
                int eta, speed = 20;
                for (int i = 0; i < d->oo.sn0.num; ++i) {
                    int st;
                    st = d->oo.sn0.type[i];
                    if (d->oo.ships[st] > 0) {
                        SETMIN(speed, sd[st].engine);
                    }
                }
                ++speed;
                eta = game_calc_eta(g, speed, pt->x, pt->y, pf->x, pf->y);
                sprintf(buf, "%s %i %s", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
            } else {
                buf[0] = '\0';
            }
            lbxfont_select_set_12_4(0, 0, 0, 0);
            if (d->oo.sn0.num >= 6) {
                ui_draw_filled_rect(228, 9, 309, 17, 7);
                lbxfont_print_str_center(268, 11, buf, UI_SCREEN_W);
            } else {
                lbxfont_print_str_center(268, 163, buf, UI_SCREEN_W);
            }
        }
    } else {
        if (d->oo.sn0.num < 6) {
            lbxfont_select_set_12_4(2, 0xe, 0, 0);
            lbxfont_print_str_split(230, 159, 80, game_str_sm_chdest, 2, UI_SCREEN_W, UI_SCREEN_H);
        }
    }
    for (int i = 0; i < d->oo.sn0.num; ++i) {
        const shipdesign_t *sd = &(g->srd[d->api].design[0]);
        uint8_t *gfx;
        int st;
        ui_draw_filled_rect(227, 22 + i * 26, 259, 46 + i * 26, 0);
        ui_draw_filled_rect(264, 34 + i * 26, 310, 46 + i * 26, 0);
        ui_data.starmap.stars_xoff1 = 0;
        ui_data.starmap.stars_xoff2 = 0;
        ui_draw_stars(227, 22 + i * 26, 0, 32);
        st = d->oo.sn0.type[i];
        gfx = ui_data.gfx.ships[sd[st].look];
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame(227, 22 + i * 26, gfx, UI_SCREEN_W);
        lbxfont_select(0, 0xd, 0, 0);
        {
            int y;
            y = 40 + i * 26;
            if (ui_extra_enabled) {
                lbxfont_print_num_right(258, y, d->oo.sn0.ships[i], UI_SCREEN_W);
                y = 24 + i * 26;
            }
            lbxfont_print_num_right(258, y, d->oo.ships[st], UI_SCREEN_W);
        }
        lbxfont_select_set_12_1(2, 0, 0, 0);
        lbxfont_print_str_center(287, 25 + i * 26, sd[st].name, UI_SCREEN_W);
    }
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
    lbxgfx_draw_frame(271, 180, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W);
}

/* -------------------------------------------------------------------------- */

void ui_starmap_orbit_own(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_scroll, oi_cancel, oi_accept,
            oi_f2, oi_f3, oi_f4, oi_f5, oi_f6, oi_f7, oi_f8, oi_f9, oi_f10,
            oi_tbl_p[NUM_SHIPDESIGNS],
            oi_tbl_m[NUM_SHIPDESIGNS],
            oi_tbl_a[NUM_SHIPDESIGNS],
            oi_tbl_n[NUM_SHIPDESIGNS]
            ;
    uint16_t scrollx = 0, scrolly = 0;
    struct starmap_data_s d;
    const fleet_orbit_t *r;
    const shipcount_t *os;
    const uint8_t shiptypes[NUM_SHIPDESIGNS] = { 0, 1, 2, 3, 4, 5 };

    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;
    d.oo.from = g->planet_focus_i[active_player];

    r = &(g->eto[active_player].orbit[d.oo.from]);

    if (BOOLVEC_IS_CLEAR(r->visible, PLAYER_NUM)) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        return;
    }

    os = &(r->ships[0]);
    for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
        d.oo.ships[i] = os[i];
    }
    ui_starmap_sn0_setup(&d.oo.sn0, NUM_SHIPDESIGNS, d.oo.ships);

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        STARMAP_UIOBJ_CLEAR_FX(); \
        oi_accept = UIOBJI_INVALID; \
        oi_cancel = UIOBJI_INVALID; \
        UIOBJI_SET_TBL4_INVALID(oi_tbl_p, oi_tbl_m, oi_tbl_a, oi_tbl_n); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(16);
    uiobj_set_callback_and_delay(ui_starmap_orbit_own_draw_cb, &d, STARMAP_DELAY);

    while (!flag_done) {
        int16_t oi1, oi2;
        ui_starmap_update_reserve_fuel(g, &d.oo.sn0, d.oo.ships, active_player);
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if (oi1 == d.oi_gameopts) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_GAMEOPTS;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_design) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_DESIGN;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_fleet) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_FLEET;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_map) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_MAP;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_races) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_RACES;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_planets) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_PLANETS;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_tech) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_TECH;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_next_turn) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_NEXT_TURN;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_f2) {
            int i;
            i = d.oo.from;
            do {
                if (--i < 0) { i = g->galaxy_stars - 1; }
            } while (g->planet[i].owner != active_player);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
            d.oo.from = i;
            /* flag_have_colony_lbx = false */
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_f3) {
            int i;
            i = d.oo.from;
            do {
                i = (i + 1) % g->galaxy_stars;
            } while (g->planet[i].owner != active_player);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
            d.oo.from = i;
            /* flag_have_colony_lbx = false */
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (((oi1 == oi_f8) || (oi1 == oi_f9)) && g->eto[active_player].have_ia_scanner) {
            int i, pi;
            ui_sound_play_sfx_24();
            pi = g->planet_focus_i[active_player];
            i = ui_starmap_enemy_incoming(g, active_player, pi, (oi1 == oi_f8));
            if (i != pi) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                d.oo.from = i;
                /* flag_have_colony_lbx = false */
                flag_done = true;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
            }
        } else if (oi1 == oi_f10) {
            game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", g);
        } else if (oi1 == oi_f4) {
            bool found;
            int i, pi;
            i = pi = d.oo.from;
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
                d.oo.from = i;
                ui_sound_play_sfx_24();
                ui_data.starmap.orbit_player = active_player;
                flag_done = true;
            }
        } else if (oi1 == oi_f5) {
            bool found;
            int i, pi;
            i = pi = d.oo.from;
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
                d.oo.from = i;
                ui_sound_play_sfx_24();
                ui_data.starmap.orbit_player = active_player;
                flag_done = true;
            }
        } else if (oi1 == oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, d.oo.from);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            d.oo.from = i;
            ui_sound_play_sfx_24();
            if (BOOLVEC_IS1(g->eto[active_player].orbit[i].visible, active_player)) {
                ui_data.starmap.orbit_player = active_player;
            } else {
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
            }
            flag_done = true;
        } else if (oi1 == oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, d.oo.from);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            d.oo.from = i;
            ui_sound_play_sfx_24();
            if (BOOLVEC_IS1(g->eto[active_player].orbit[i].visible, active_player)) {
                ui_data.starmap.orbit_player = active_player;
            } else {
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
            }
            flag_done = true;
        }
        for (int i = 0; i < g->enroute_num; ++i) {
            if (oi1 == d.oi_tbl_enroute[i]) {
                ui_data.starmap.fleet_selected = i;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_ENROUTE_SEL;
                ui_sound_play_sfx_24();
                flag_done = true;
                break;
            }
        }
        for (int i = 0; i < g->transport_num; ++i) {
            if (oi1 == d.oi_tbl_transport[i]) {
                ui_data.starmap.fleet_selected = i;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_TRANSPORT_SEL;
                ui_sound_play_sfx_24();
                flag_done = true;
                break;
            }
        }
        for (int i = 0; i < g->galaxy_stars; ++i) {
            for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                if (oi1 == d.oi_tbl_pl_stars[j][i]) {
                    g->planet_focus_i[active_player] = i;  /* not done on MOO1! */
                    d.oo.from = i;
                    ui_data.starmap.orbit_player = j;
                    ui_data.ui_main_loop_action = (j == active_player) ? UI_MAIN_LOOP_ORBIT_OWN_SEL : UI_MAIN_LOOP_ORBIT_EN_SEL;
                    ui_sound_play_sfx_24();
                    flag_done = true;
                    j = g->players; i = g->galaxy_stars;
                }
            }
        }
        if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_accept) {
do_accept:
            ui_sound_play_sfx_24();
            if (ui_starmap_orbit_own_in_frange(&d)) {
                game_send_fleet_from_orbit(g, active_player, d.oo.from, g->planet_focus_i[active_player], d.oo.ships, shiptypes, 6);
                game_update_visibility(g);
            }
            if ((!ui_extra_enabled) || BOOLVEC_IS_CLEAR(r->visible, PLAYER_NUM)) {
                flag_done = true;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
            } else {
                ui_starmap_sn0_setup(&d.oo.sn0, NUM_SHIPDESIGNS, os);
                for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
                    SETMIN(d.oo.ships[i], os[i]);
                }
            }
        } else if (oi1 == oi_scroll) {
            ui_starmap_scroll(g, scrollx, scrolly);
        }
        ui_starmap_handle_oi_ctrl(&d, oi1);
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if ((oi1 == d.oi_tbl_stars[i]) && !g->evn.build_finished_num[active_player]) {
                if (ui_extra_enabled && (oi_accept != UIOBJI_INVALID) && (g->planet_focus_i[active_player] == i)) {
                    oi1 = oi_accept;
                    goto do_accept;
                }
                g->planet_focus_i[active_player] = i;
                ui_sound_play_sfx_24();
                break;
            }
        }

        for (int i = 0; i < d.oo.sn0.num; ++i) {
            int si, per10num;
            si = d.oo.sn0.type[i];
            per10num = os[si] / 10;
            SETMAX(per10num, 1);
            if (oi1 == oi_tbl_p[i]) {
                shipcount_t t;
                t = d.oo.ships[si] + per10num;
                SETMIN(t, os[si]);
                d.oo.ships[si] = t;
                ui_sound_play_sfx_24();
                break;
            } else if (oi1 == oi_tbl_m[i]) {
                shipcount_t t;
                t = d.oo.ships[si];
                t = (t < per10num) ? 0 : (t - per10num);
                d.oo.ships[si] = t;
                ui_sound_play_sfx_24();
                break;
            } else if (oi1 == oi_tbl_a[i]) {
                d.oo.ships[si] = os[si];
                ui_sound_play_sfx_24();
                break;
            } else if (oi1 == oi_tbl_n[i]) {
                d.oo.ships[si] = 0;
                ui_sound_play_sfx_24();
                break;
            }
        }
        d.oo.shiptypenon0numsel = 0;
        for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
            if (d.oo.ships[i] != 0) {
                ++d.oo.shiptypenon0numsel;
            }
        }
        if (!flag_done) {
            d.bottom_highlight = -1;
            if (oi2 == d.oi_gameopts) {
                d.bottom_highlight = 0;
            } else if (oi2 == d.oi_design) {
                d.bottom_highlight = 1;
            } else if (oi2 == d.oi_fleet) {
                d.bottom_highlight = 2;
            } else if (oi2 == d.oi_map) {
                d.bottom_highlight = 3;
            } else if (oi2 == d.oi_races) {
                d.bottom_highlight = 4;
            } else if (oi2 == d.oi_planets) {
                d.bottom_highlight = 5;
            } else if (oi2 == d.oi_tech) {
                d.bottom_highlight = 6;
            } else if (oi2 == d.oi_next_turn) {
                d.bottom_highlight = 7;
            }
            ui_starmap_orbit_own_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            STARMAP_UIOBJ_FILL_FX();
            ui_starmap_fill_oi_tbls(&d, true);
            ui_starmap_fill_oi_tbl_stars(&d);
            oi_cancel = uiobj_add_t0(227, 180, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            if (ui_starmap_orbit_own_in_frange(&d) && d.oo.shiptypenon0numsel) {
                oi_accept = uiobj_add_t0(271, 180, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            }
            oi_scroll = uiobj_add_tb(6, 6, 2, 2, 108, 86, &scrollx, &scrolly);
            ui_starmap_fill_oi_ctrl(&d);
            for (int i = 0; i < d.oo.sn0.num; ++i) {
                oi_tbl_p[i] = uiobj_add_t0(288, 35 + i * 26, "", ui_data.gfx.starmap.move_but_p, MOO_KEY_UNKNOWN);
                oi_tbl_m[i] = uiobj_add_t0(277, 35 + i * 26, "", ui_data.gfx.starmap.move_but_m, MOO_KEY_UNKNOWN);
                oi_tbl_a[i] = uiobj_add_t0(299, 35 + i * 26, "", ui_data.gfx.starmap.move_but_a, MOO_KEY_UNKNOWN);
                oi_tbl_n[i] = uiobj_add_t0(265, 35 + i * 26, "", ui_data.gfx.starmap.move_but_n, MOO_KEY_UNKNOWN);
            }
            ui_starmap_add_oi_bottom_buttons(&d);
            d.oi_tech = UIOBJI_INVALID;
            d.oi_next_turn = UIOBJI_INVALID;
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_table_clear();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.oo.from;
}
