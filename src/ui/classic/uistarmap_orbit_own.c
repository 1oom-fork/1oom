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
#include "lib.h"
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

static bool ui_starmap_orbit_own_in_frange(const struct starmap_data_s *d, uint8_t planet_i)
{
    struct planet_s *p = &d->g->planet[planet_i];
    return (p->within_frange[d->api] == 1) || ((p->within_frange[d->api] == 2) && d->oo.sn0.have_reserve_fuel);
}

static void ui_starmap_orbit_own_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *pf = &g->planet[d->from_i];
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    char buf[0x80];
    STARMAP_LIM_INIT();

    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    {
        int x, y;
        x = (pf->x - ui_data.starmap.x) * 2 + 23;
        y = (pf->y - ui_data.starmap.y) * 2 + 5;
        lbxgfx_draw_frame_offs(x, y, ui_data.gfx.starmap.shipbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    }
    ui_draw_filled_rect(225, 8, 314, 192, 7, ui_scale);
    lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.move_shi, UI_SCREEN_W, ui_scale);
    if (d->oo.sn0.num < NUM_SHIPDESIGNS) {
        lbxgfx_draw_frame(224, 151, ui_data.gfx.starmap.movextra, UI_SCREEN_W, ui_scale);
        ui_draw_filled_rect(228, 155, 309, 175, 7, ui_scale);
    }
    lbxfont_select_set_12_4(0, 5, 0, 0);
    lbxfont_print_str_center(268, 11, game_str_sm_fleetdep, UI_SCREEN_W, ui_scale);

    if (g->planet_focus_i[d->api] != d->from_i) {
        int dist = game_get_min_dist(g, d->api, g->planet_focus_i[d->api]);
        int x0, y0, x1, y1;
        const uint8_t *ctbl;
        uint8_t *gfx;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 8;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 8;
        lbxgfx_draw_frame_offs(x1, y1, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        x0 = (pf->x - ui_data.starmap.x) * 2 + 26;
        y0 = (pf->y - ui_data.starmap.y) * 2 + 8;
        ctbl = ui_starmap_orbit_own_in_frange(d, g->planet_focus_i[d->api]) ? colortbl_line_green : colortbl_line_red;
        if (ui_modern_controls) {
            ui_draw_planet_frame_limit_ctbl(x1, y1, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
        }
        ui_draw_line_limit_ctbl(x0 + 3, y0 + 1, x1 + 6, y1 + 6, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
        gfx = ui_data.gfx.starmap.smalship[g->eto[d->api].banner];
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame_offs(x0, y0, gfx, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        if (!ui_starmap_orbit_own_in_frange(d, g->planet_focus_i[d->api])) {
            if (d->oo.sn0.num < NUM_SHIPDESIGNS) { /* WASBUG MOO1 compares to 7, resulting in text below last ship */
                lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_destoor, dist, game_str_sm_parsfromcc);
                lbxfont_select(2, 0, 0, 0);
                lbxfont_set_gap_h(2);
                lbxfont_print_str_split(228, 156, 81, buf, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
            } else {
                lib_sprintf(buf, sizeof(buf), "%s (%i)", game_str_sm_destoor2, dist);
                ui_draw_filled_rect(228, 9, 309, 17, 7, ui_scale);
                lbxfont_print_str_center(268, 11, buf, UI_SCREEN_W, ui_scale);
            }
        } else {
            if ((pt->owner == d->api) && (pf->owner == d->api) && pt->have_stargate && pf->have_stargate) {
                lib_strcpy(buf, game_str_sm_stargate, sizeof(buf));
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
                eta = game_calc_eta_ship(g, speed, pt->x, pt->y, pf->x, pf->y);
                lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
            } else {
                buf[0] = '\0';
            }
            lbxfont_select_set_12_4(0, 0, 0, 0);
            if (d->oo.sn0.num >= NUM_SHIPDESIGNS) {
                ui_draw_filled_rect(228, 9, 309, 17, 7, ui_scale);
                lbxfont_print_str_center(268, 11, buf, UI_SCREEN_W, ui_scale);
            } else {
                lbxfont_print_str_center(268, 163, buf, UI_SCREEN_W, ui_scale);
            }
        }
    } else {
        if (d->oo.sn0.num < NUM_SHIPDESIGNS) {
            lbxfont_select_set_12_4(2, 0xe, 0, 0);
            lbxfont_print_str_split(230, 159, 80, game_str_sm_chdest, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        }
    }
    for (int i = 0; i < d->oo.sn0.num; ++i) {
        const shipdesign_t *sd = &(g->srd[d->api].design[0]);
        struct draw_stars_s ds;
        uint8_t *gfx;
        int st;
        ui_draw_filled_rect(227, 22 + i * 26, 259, 46 + i * 26, 0, ui_scale);
        ui_draw_filled_rect(264, 34 + i * 26, 310, 46 + i * 26, 0, ui_scale);
        ds.xoff1 = 0;
        ds.xoff2 = 0;
        ui_draw_stars(227, 22 + i * 26, 0, 32, &ds, ui_scale);
        st = d->oo.sn0.type[i];
        gfx = ui_data.gfx.ships[sd[st].look];
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame(227, 22 + i * 26, gfx, UI_SCREEN_W, ui_scale);
        lbxfont_select(0, 0xd, 0, 0);
        {
            int y;
            y = 40 + i * 26;
            lbxfont_print_num_right(258, y, d->oo.ships[st], UI_SCREEN_W, ui_scale);
            if (ui_extra_enabled) {
                y = 24 + i * 26;
                lbxfont_select(0, 0x7, 0, 0);
                lbxfont_print_num_right(258, y, d->oo.sn0.ships[i], UI_SCREEN_W, ui_scale);
                lbxfont_select(0, 0xd, 0, 0);
            }
        }
        lbxfont_select_set_12_1(2, 0, 0, 0);
        lbxfont_print_str_center(287, 25 + i * 26, sd[st].name, UI_SCREEN_W, ui_scale);
        if (i == d->oo.cursor_over) {
            ui_draw_box1(226, 21 + i * 26, 261, 47 + i * 26, 0x56, 0x56, ui_scale);
        }
    }
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
    lbxgfx_draw_frame(271, 180, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W, ui_scale);
}

/* -------------------------------------------------------------------------- */

static const uint8_t shiptypes[NUM_SHIPDESIGNS] = { 0, 1, 2, 3, 4, 5 };

static bool ui_starmap_orbit_own_update(struct starmap_data_s *d, bool first_time)
{
    const fleet_orbit_t *r = &(d->g->eto[d->api].orbit[d->from_i]);
    if (BOOLVEC_IS_CLEAR(r->visible, PLAYER_NUM)) {
        return false;
    }
    for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
        if (first_time || d->oo.ships[i] > r->ships[i]) {
            d->oo.ships[i] = r->ships[i];
        }
    }
    ui_starmap_sn0_setup(&d->oo.sn0, NUM_SHIPDESIGNS, r->ships);
    if (first_time || d->oo.cursor_over >= d->oo.sn0.num) {
        d->oo.cursor_over = 0;
    }
    return true;
}

static bool ui_starmap_orbit_own_valid_destination(const struct starmap_data_s *d, int planet_i)
{
    return ui_starmap_orbit_own_in_frange(d, planet_i) && d->oo.shiptypenon0numsel;
}

static void ui_starmap_orbit_own_do_accept(struct starmap_data_s *d)
{
    struct game_s *g = d->g;
    uint8_t planet_i = g->planet_focus_i[d->api];
    planet_t *p = &g->planet[planet_i];

    if ((p->within_frange[d->api] == 1) || ((p->within_frange[d->api] == 2) && d->oo.sn0.have_reserve_fuel)) {
        game_send_fleet_from_orbit(g, d->api, d->from_i, planet_i, d->oo.ships, shiptypes, 6);
        game_update_visibility(g);
    }
}

void ui_starmap_orbit_own(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_cancel, oi_cycle,
            oi_f4, oi_f5, oi_f6, oi_f7, oi_f10,
            oi_plus, oi_minus, oi_equals, oi_a,
            oi_tbl_p[NUM_SHIPDESIGNS],
            oi_tbl_m[NUM_SHIPDESIGNS],
            oi_tbl_a[NUM_SHIPDESIGNS],
            oi_tbl_n[NUM_SHIPDESIGNS],
            oi_tbl_s[NUM_SHIPDESIGNS]
            ;
    int16_t scrollship = 0;
    struct starmap_data_s d;
    const shipcount_t *os;

    ui_starmap_common_init(g, &d, active_player);
    d.update_cb = ui_starmap_orbit_own_update;
    d.valid_target_cb = ui_starmap_orbit_own_valid_destination;
    d.on_accept_cb = ui_starmap_orbit_own_do_accept;

    if (!d.update_cb(&d, true)) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        return;
    }
    os = &(g->eto[active_player].orbit[d.from_i].ships[0]);

    ui_starmap_common_late_init(&d, ui_starmap_orbit_own_draw_cb, true);

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        STARMAP_UIOBJ_CLEAR_FX(); \
        oi_cancel = UIOBJI_INVALID; \
        oi_cycle = UIOBJI_INVALID; \
        oi_plus = UIOBJI_INVALID; \
        oi_minus = UIOBJI_INVALID; \
        oi_equals = UIOBJI_INVALID; \
        oi_a = UIOBJI_INVALID; \
        UIOBJI_SET_TBL5_INVALID(oi_tbl_p, oi_tbl_m, oi_tbl_a, oi_tbl_n, oi_tbl_s); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(16);

    while (!flag_done) {
        int16_t oi1, oi2;
        ui_starmap_update_reserve_fuel(g, &d.oo.sn0, d.oo.ships, active_player);
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if (ui_starmap_common_handle_oi(g, &d, &flag_done, oi1, oi2)) {
        } else if (oi1 == oi_f10) {
            game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", g);
        } else if (oi1 == oi_f4) {
            bool found;
            int i, pi;
            i = pi = d.from_i;
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
                d.from_i = i;
                ui_sound_play_sfx_24();
                ui_data.starmap.orbit_player = active_player;
                flag_done = true;
            }
        } else if (oi1 == oi_f5) {
            bool found;
            int i, pi;
            i = pi = d.from_i;
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
                d.from_i = i;
                ui_sound_play_sfx_24();
                ui_data.starmap.orbit_player = active_player;
                flag_done = true;
            }
        } else if (oi1 == oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, d.from_i);
            if (i != PLANET_NONE) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                d.from_i = i;
                ui_sound_play_sfx_24();
                if (BOOLVEC_IS1(g->eto[active_player].orbit[i].visible, active_player)) {
                    ui_data.starmap.orbit_player = active_player;
                } else {
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                }
                flag_done = true;
            }
        } else if (oi1 == oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, d.from_i);
            if (i != PLANET_NONE) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                d.from_i = i;
                ui_sound_play_sfx_24();
                if (BOOLVEC_IS1(g->eto[active_player].orbit[i].visible, active_player)) {
                    ui_data.starmap.orbit_player = active_player;
                } else {
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                }
                flag_done = true;
            }
        }
        if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        }
        ui_starmap_set_ruler(&d, oi2);
        d.ruler_from_fleet = true;
        for (int i = 0; i < d.oo.sn0.num; ++i) {
            if (0
              || (oi2 == oi_tbl_p[i])
              || (oi2 == oi_tbl_m[i])
              || (oi2 == oi_tbl_a[i])
              || (oi2 == oi_tbl_n[i])
              || (oi2 == oi_tbl_s[i])
            ) {
                d.oo.cursor_over = i;
                break;
            }
        }
        if (oi1 == oi_cycle) {
            if (++d.oo.cursor_over >= d.oo.sn0.num) {
                d.oo.cursor_over = 0;
            }
        }
        for (int i = 0; i < d.oo.sn0.num; ++i) {
            int si, per10num;
            si = d.oo.sn0.type[i];
            if (kbd_is_modifier(MOO_MOD_CTRL)) {
                per10num = 1;
            } else {
                per10num = os[si] / 10;
            }
            SETMAX(per10num, 1);
            if ((oi1 == oi_tbl_p[i])
            || ((oi1 == oi_tbl_s[i]) && ((scrollship > 0) != ui_mwi_counter))
            || ((oi1 == oi_plus || oi1 == oi_equals) && i == d.oo.cursor_over)) {
                shipcount_t t;
                t = d.oo.ships[si] + per10num;
                SETMIN(t, os[si]);
                d.oo.ships[si] = t;
                if (scrollship) {
                    scrollship = 0;
                } else {
                    ui_sound_play_sfx_24();
                }
                break;
            } else if ((oi1 == oi_tbl_m[i])
                   || ((oi1 == oi_tbl_s[i]) && ((scrollship < 0) != ui_mwi_counter))
                   || ((oi1 == oi_minus) && i == d.oo.cursor_over)) {
                shipcount_t t;
                t = d.oo.ships[si];
                t = (t < per10num) ? 0 : (t - per10num);
                d.oo.ships[si] = t;
                ui_sound_play_sfx_24();
                if (scrollship) {
                    scrollship = 0;
                } else {
                    ui_sound_play_sfx_24();
                }
                break;
            } else if (oi1 == oi_a && i == d.oo.cursor_over) {
                d.oo.ships[si] = d.oo.ships[si] != os[si] ? os[si] : 0;
                ui_sound_play_sfx_24();
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
            ui_starmap_select_bottom_highlight(&d, oi2);
            ui_starmap_orbit_own_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
            oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
            oi_f6 = uiobj_add_inputkey(MOO_KEY_F6);
            oi_f7 = uiobj_add_inputkey(MOO_KEY_F7);
            oi_f10 = uiobj_add_inputkey(MOO_KEY_F10);
            oi_plus = uiobj_add_inputkey(MOO_KEY_PLUS);
            oi_minus = uiobj_add_inputkey(MOO_KEY_MINUS);
            oi_equals = uiobj_add_inputkey(MOO_KEY_EQUALS);
            oi_a = uiobj_add_inputkey(MOO_KEY_a);
            ui_starmap_common_fill_oi(&d);
            oi_cancel = uiobj_add_t0(227, 180, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            if (d.valid_target_cb(&d, g->planet_focus_i[active_player])) {
                d.oi_accept = uiobj_add_t0(271, 180, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            }
            oi_cycle = uiobj_add_inputkey(MOO_KEY_TAB);
            for (int i = 0; i < d.oo.sn0.num; ++i) {
                oi_tbl_p[i] = uiobj_add_t0(288, 35 + i * 26, "", ui_data.gfx.starmap.move_but_p, MOO_KEY_UNKNOWN);
                oi_tbl_m[i] = uiobj_add_t0(277, 35 + i * 26, "", ui_data.gfx.starmap.move_but_m, MOO_KEY_UNKNOWN);
                oi_tbl_a[i] = uiobj_add_t0(299, 35 + i * 26, "", ui_data.gfx.starmap.move_but_a, MOO_KEY_UNKNOWN);
                oi_tbl_n[i] = uiobj_add_t0(265, 35 + i * 26, "", ui_data.gfx.starmap.move_but_n, MOO_KEY_UNKNOWN);
                oi_tbl_s[i] = uiobj_add_mousewheel(227, 22 + i * 26, 319, 46 + i * 26, &scrollship);
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_table_clear();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.from_i;
}
