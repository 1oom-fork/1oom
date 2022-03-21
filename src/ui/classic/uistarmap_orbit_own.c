#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_num.h"
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

static bool ui_starmap_orbit_own_withinin_frange(const struct game_s *g, const struct starmap_data_s *d, int planet_i) {
    const planet_t *p = &g->planet[planet_i];
    return p->within_frange[d->api] == 1 || (p->within_frange[d->api] == 2 && d->oo.sn0.have_reserve_fuel);
}

static bool ui_starmap_orbit_own_valid_destination(const struct game_s *g, const struct starmap_data_s *d, int planet_i) {
    return 1
      && ui_starmap_orbit_own_withinin_frange(g, d, planet_i)
      && d->oo.shiptypenon0numsel;
}

static void ui_starmap_orbit_own_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *pf = &g->planet[d->from];
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    bool in_frange = ui_starmap_orbit_own_withinin_frange(g, d, g->planet_focus_i[d->api]);
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

    if (g->planet_focus_i[d->api] != d->from) {
        int dist = game_get_min_dist(g, d->api, g->planet_focus_i[d->api]);
        int x0, y0, x1, y1;
        const uint8_t *ctbl;
        uint8_t *gfx;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 8;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 8;
        lbxgfx_draw_frame_offs(x1, y1, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        x0 = (pf->x - ui_data.starmap.x) * 2 + 26;
        y0 = (pf->y - ui_data.starmap.y) * 2 + 8;
        ctbl = in_frange ? colortbl_line_green : colortbl_line_red;
        ui_draw_line_limit_ctbl(x0 + 3, y0 + 1, x1 + 6, y1 + 6, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
        gfx = ui_data.gfx.starmap.smalship[g->eto[d->api].banner];
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame_offs(x0, y0, gfx, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        if (!in_frange || d->from == g->planet_focus_i[d->api]) {
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
    }
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
    lbxgfx_draw_frame(271, 180, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W, ui_scale);
}

/* -------------------------------------------------------------------------- */

static void ui_starmap_orbit_own_do_accept(struct game_s *g, struct starmap_data_s *d) {
    const uint8_t shiptypes[NUM_SHIPDESIGNS] = { 0, 1, 2, 3, 4, 5 };
    if (d->from != g->planet_focus_i[d->api]) {
        game_send_fleet_from_orbit(g, d->api, d->from, g->planet_focus_i[d->api], d->oo.ships, shiptypes, 6);
        game_update_visibility(g);
    }
    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
}

void ui_starmap_orbit_own(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_cycle,
            oi_tbl_p[NUM_SHIPDESIGNS],
            oi_tbl_m[NUM_SHIPDESIGNS],
            oi_tbl_a[NUM_SHIPDESIGNS],
            oi_tbl_n[NUM_SHIPDESIGNS],
            oi_tbl_s[NUM_SHIPDESIGNS]
            ;
    int16_t scrollship = 0;
    struct starmap_data_s d;
    const fleet_orbit_t *r;
    const shipcount_t *os;

    d.scrollx = 0;
    d.scrolly = 0;
    d.scrollz = starmap_scale;
    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;

    d.gov_highlight = 0;
    d.from = g->planet_focus_i[active_player];

    d.controllable = true;
    d.is_valid_destination = ui_starmap_orbit_own_valid_destination;
    d.do_accept = ui_starmap_orbit_own_do_accept;

    r = &(g->eto[active_player].orbit[d.from]);

    if (BOOLVEC_IS_CLEAR(r->visible, PLAYER_NUM)) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        return;
    }

    os = &(r->ships[0]);
    for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
        d.oo.ships[i] = os[i];
    }
    d.oo.shiptypenon0numsel = 0;
    for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
        if (d.oo.ships[i] != 0) {
            ++d.oo.shiptypenon0numsel;
        }
    }
    ui_starmap_sn0_setup(&d.oo.sn0, NUM_SHIPDESIGNS, d.oo.ships);

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        oi_cycle = UIOBJI_INVALID; \
        UIOBJI_SET_TBL5_INVALID(oi_tbl_p, oi_tbl_m, oi_tbl_a, oi_tbl_n, oi_tbl_s); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(16);
    uiobj_set_callback_and_delay(ui_starmap_orbit_own_draw_cb, &d, STARMAP_DELAY);

    while (!flag_done) {
        int cursor_over;
        ui_starmap_update_reserve_fuel(g, &d.oo.sn0, d.oo.ships, active_player);
        ui_delay_prepare();
        ui_starmap_handle_common(g, &d, &flag_done);
        cursor_over = -1;
        for (int i = 0; i < d.oo.sn0.num; ++i) {
            if (0
              || (d.oi2 == oi_tbl_p[i])
              || (d.oi2 == oi_tbl_m[i])
              || (d.oi2 == oi_tbl_a[i])
              || (d.oi2 == oi_tbl_n[i])
            ) {
                cursor_over = i;
                break;
            }
        }
        if (d.oi1 == oi_cycle) {
            if (++cursor_over >= d.oo.sn0.num) {
                cursor_over = 0;
            }
            uiobj_set_focus(oi_tbl_m[cursor_over]);
        }
        for (int i = 0; i < d.oo.sn0.num; ++i) {
            int si, per10num;
            si = d.oo.sn0.type[i];
            per10num = os[si] / 10;
            SETMAX(per10num, 1);
            if ((d.oi1 == oi_tbl_p[i]) || ((d.oi1 == oi_tbl_s[i]) && ((scrollship > 0) != ui_mwi_counter))) {
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
            } else if ((d.oi1 == oi_tbl_m[i]) || ((d.oi1 == oi_tbl_s[i]) && ((scrollship < 0) != ui_mwi_counter))) {
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
            } else if (d.oi1 == oi_tbl_a[i]) {
                d.oo.ships[si] = os[si];
                ui_sound_play_sfx_24();
                break;
            } else if (d.oi1 == oi_tbl_n[i]) {
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
            ui_starmap_select_bottom_highlight(g, &d, d.oi2);
            ui_starmap_orbit_own_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            ui_starmap_add_oi_hotkeys(&d);
            ui_starmap_fill_oi_tbls(&d);
            ui_starmap_fill_oi_tbl_stars(&d);
            d.oi_cancel = uiobj_add_t0(227, 180, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            if (ui_starmap_orbit_own_valid_destination(g, &d, g->planet_focus_i[active_player])) {
                d.oi_accept = uiobj_add_t0(271, 180, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            }
            oi_cycle = uiobj_add_inputkey(MOO_KEY_TAB);
            d.oi_scroll = uiobj_add_tb(6, 6, 2, 2, 108, 86, &d.scrollx, &d.scrolly, &d.scrollz, ui_scale);
            ui_starmap_fill_oi_ctrl(&d);
            for (int i = 0; i < d.oo.sn0.num; ++i) {
                oi_tbl_p[i] = uiobj_add_t0(288, 35 + i * 26, "", ui_data.gfx.starmap.move_but_p, (cursor_over == i) ? MOO_KEY_GREATER : MOO_KEY_UNKNOWN);
                oi_tbl_m[i] = uiobj_add_t0(277, 35 + i * 26, "", ui_data.gfx.starmap.move_but_m, (cursor_over == i) ? MOO_KEY_LESS : MOO_KEY_UNKNOWN);
                oi_tbl_a[i] = uiobj_add_t0(299, 35 + i * 26, "", ui_data.gfx.starmap.move_but_a, MOO_KEY_UNKNOWN);
                oi_tbl_n[i] = uiobj_add_t0(265, 35 + i * 26, "", ui_data.gfx.starmap.move_but_n, MOO_KEY_UNKNOWN);
                oi_tbl_s[i] = uiobj_add_mousewheel(227, 22 + i * 26, 319, 46 + i * 26, &scrollship);
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
    g->planet_focus_i[active_player] = d.from;
}
