#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_save.h"
#include "game_str.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "log.h"
#include "types.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uiobj.h"
#include "uisearch.h"
#include "uisound.h"
#include "uistarmap_common.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static void ui_starmap_trans_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *pf = &g->planet[d->tr.from];
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    char buf[0x80];
    int x0, y0, trans_max = pf->pop / 2;
    STARMAP_LIM_INIT();
    uiobj_set_help_id(17);
    ui_starmap_draw_basic(d);
    x0 = (pf->x - ui_data.starmap.x) * 2 + 8;
    y0 = (pf->y - ui_data.starmap.y) * 2 + 8;
    if (pt->owner == d->api) {
        lbxgfx_draw_frame(222, 80, ui_data.gfx.starmap.relocate, UI_SCREEN_W, ui_scale);
        /* if (pt->unrest == PLANET_UNREST_REBELLION) {} never true, trans button disabled in uistarmap.c */
    } else {
        lbxgfx_draw_frame_offs(222, 80, ui_data.gfx.starmap.relocate, 0, 83, 310, 199, UI_SCREEN_W, ui_scale);
        if (BOOLVEC_IS0(pt->explored, d->api)) {
            ui_draw_filled_rect(227, 57, 310, 159, 0, ui_scale);
            lbxgfx_draw_frame_offs(224, 5, ui_data.gfx.starmap.unexplor, 227, 57, 310, 159, UI_SCREEN_W, ui_scale);
        } else {
            ui_draw_filled_rect(227, 73, 310, 159, 7, ui_scale);
            ui_draw_box1(227, 73, 310, 159, 4, 4, ui_scale);
        }
    }
    lbxgfx_draw_frame_offs(x0, y0, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
    lbxgfx_draw_frame(271, 163, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W, ui_scale);
    if (d->tr.from != g->planet_focus_i[d->api]) {
        const uint8_t *ctbl;
        int x1, y1;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 14;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 14;
        if (0
          || (!d->tr.other)
          || (pt->within_frange[d->api] != 1)
          || BOOLVEC_IS0(pt->explored, d->api)
          || (pt->owner == PLAYER_NONE)
          || (pt->type < g->eto[d->api].have_colony_for)
        ) {
            ctbl = colortbl_line_red;
        } else {
            ctbl = colortbl_line_green;
        }
        ui_draw_line_limit_ctbl(x0 + 6, y0 + 6, x1, y1, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
    }
    if (d->tr.from != g->planet_focus_i[d->api]) {
        if (pt->within_frange[d->api] != 1) {
            int mindist = game_get_min_dist(g, d->api, g->planet_focus_i[d->api]);
            lbxfont_select_set_12_1(0, 0xe, 5, 0);
            lbxfont_set_gap_h(1);
            lbxfont_print_str_split(228, 94, 82, game_str_sm_notrange, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
            sprintf(buf, "%s %i %s %i %s", game_str_sm_notrange1, mindist, game_str_sm_notrange2, g->eto[d->api].fuel_range, game_str_sm_notrange3);
            lbxfont_print_str_split(229, 125, 80, game_str_sm_seltr, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else if (BOOLVEC_IS0(pt->explored, d->api)) {
            lbxfont_select(0, 0xe, 0, 0);
            lbxfont_print_str_split(229, 105, 80, game_str_sm_trfirste, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else if (pt->type < g->eto[d->api].have_colony_for) {
            int pos;
            lbxfont_select(0, 6, 0, 0);
            lbxfont_set_gap_h(1);
            pos = sprintf(buf, "%s ", game_str_sm_trcontr1);
            sprintf(&buf[pos], "%s ", game_str_tbl_sm_pltype[pt->type]);
            util_str_tolower(&buf[pos]);
            strcat(&buf[pos], game_str_sm_trcontr2);
            lbxfont_print_str_split(228, 111, 82, buf, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else if (pt->owner == PLAYER_NONE) {
            lbxfont_select(0, 0xe, 0, 0);
            lbxfont_print_str_split(230, 105, 80, game_str_sm_trfirstc, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else {
            treaty_t treaty = TREATY_NONE;
            uiobj_set_help_id(1);
            if (pf->have_stargate && pt->have_stargate && (pf->owner == pt->owner)) {
                strcpy(buf, game_str_sm_stargate);
            } else {
                int eta, engine = g->eto[d->api].have_engine;
                eta = game_calc_eta(g, engine, pf->x, pf->y, pt->x, pt->y);
                sprintf(buf, "%s %i %s", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
            }
            lbxfont_select(0, 0, 0, 0);
            lbxfont_print_str_center(268, 149, buf, UI_SCREEN_W, ui_scale);
            lbxgfx_draw_frame(230, 123, ui_data.gfx.starmap.tran_bar, UI_SCREEN_W, ui_scale);
            lbxfont_select(0, 6, 0, 0);
            if (pt->owner != PLAYER_NONE) {/* WASBUG MOO1 tests for == PLAYER_NONE, reading from eto offs 0xcc */
                treaty = g->eto[d->api].treaty[pt->owner];
            }
            if ((treaty == TREATY_NONAGGRESSION) || (treaty == TREATY_ALLIANCE)) {
                lbxfont_print_str_split(228, 105, 84, game_str_sm_trwarna, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
            } else {
                int v = 0;
                if (pf->owner == pt->owner) {
                    v = pt->pop;
                }
                v += d->tr.num;
                if (pt->max_pop3 < v) {
                    if (d->anim_delay == 0) {
                        d->tr.blink = !d->tr.blink;
                    }
                    if (d->tr.blink) {
                        lbxfont_select(0, 5, 0, 0);
                        sprintf(buf, "%s %i %s", game_str_sm_trwarnm1, pt->max_pop3, game_str_sm_trwarnm2);
                        lbxfont_print_str_split(228, 101, 80, buf, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
                    }
                } else {
                    const char *s = (pt->owner == pf->owner) ? game_str_sm_trchnum1 : game_str_sm_trchnum2;
                    lbxfont_print_str_split(228, 105, 84, s, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
                }
            }
            lbxfont_select_set_12_1(0, 1, 0, 0);
            lbxfont_print_num_right(273, 137, d->tr.num, UI_SCREEN_W, ui_scale);
            ui_draw_filled_rect(258, 127, 299, 129, 0x2f, ui_scale);
            if (d->tr.num > 0) {
                ui_draw_slider(258, 127, d->tr.num * 40, trans_max, 0, 0x73, ui_scale);
            }
        }
    } else {
        lbxfont_select(0, 6, 0, 0);
        lbxfont_print_str_split(229, 110, 80, game_str_sm_seltr, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
    }
    lbxfont_select_set_12_1(5, 5, 0, 0);
    {
        int y;
        y = (d->tr.other && (pt->within_frange[d->api] != 1)) ? 77 : 90;
        lbxfont_print_str_center(269, y, game_str_sm_transs, UI_SCREEN_W, ui_scale);
    }
}

/* -------------------------------------------------------------------------- */

void ui_starmap_trans(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_scroll, oi_cancel, oi_accept, oi_plus, oi_minus, oi_search,
            oi_f2, oi_f3, oi_f4, oi_f5, oi_f6, oi_f7, oi_f8, oi_f9, oi_f10
            ;
    int16_t scrollx = 0, scrolly = 0;
    uint8_t scrollz = starmap_scale;
    struct starmap_data_s d;
    uint8_t olddest;
    planet_t *p;
    int16_t trans_max;
    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;
    d.tr.blink = false;
    {
        uint8_t pi = g->planet_focus_i[active_player];
        d.tr.from = pi;
        p = &(g->planet[pi]);
        olddest = p->trans_dest;
        if (p->trans_num != 0) {
            d.tr.other = true;
            g->planet_focus_i[active_player] = p->trans_dest;
        } else {
            d.tr.other = false;
            g->planet_focus_i[active_player] = pi;
        }
    }
    d.tr.num = p->trans_num;
    trans_max = p->pop / 2;

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        STARMAP_UIOBJ_CLEAR_FX(); \
        oi_accept = UIOBJI_INVALID; \
        oi_cancel = UIOBJI_INVALID; \
        oi_plus = UIOBJI_INVALID; \
        oi_minus = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_callback_and_delay(ui_starmap_trans_draw_cb, &d, STARMAP_DELAY);

    while (!flag_done) {
        int16_t oi1, oi2;
        const planet_t *pt;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        pt = &(g->planet[g->planet_focus_i[active_player]]);
        p->trans_dest = g->planet_focus_i[active_player];
        ui_starmap_handle_scrollkeys(&d, oi1);
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
        } else if (oi1 == oi_search) {
            ui_sound_play_sfx_24();
            if (ui_search_set_pos(g, active_player)) {
                d.tr.other = true;
            }
        } else if (oi1 == oi_f2) {
            int i;
            i = g->planet_focus_i[active_player];
            do {
                if (--i < 0) { i = g->galaxy_stars - 1; }
            } while (g->planet[i].owner != active_player);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_f3) {
            int i;
            i = g->planet_focus_i[active_player];
            do {
                i = (i + 1) % g->galaxy_stars;
            } while (g->planet[i].owner != active_player);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (((oi1 == oi_f8) || (oi1 == oi_f9)) && g->eto[active_player].have_ia_scanner) {
            int i, pi;
            ui_sound_play_sfx_24();
            pi = g->planet_focus_i[active_player];
            i = ui_starmap_enemy_incoming(g, active_player, pi, (oi1 == oi_f8));
            if (i != pi) {
                g->planet_focus_i[active_player] = i;
                d.tr.other = (i != d.tr.from);
                if (!ui_extra_enabled) {
                    ui_starmap_set_pos_focus(g, active_player);
                }
            }
        } else if (oi1 == oi_f10) {
            game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", g);
        } else if (oi1 == oi_f4) {
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
        } else if (oi1 == oi_f5) {
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
        } else if (oi1 == oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, g->planet_focus_i[active_player]);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, g->planet_focus_i[active_player]);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        }
        if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            p->trans_dest = olddest;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_accept) {
do_accept:
            ui_sound_play_sfx_24();
            flag_done = true;
            if (BOOLVEC_IS1(pt->explored, active_player) && (pt->within_frange[active_player] == 1)) {
                p->trans_dest = g->planet_focus_i[active_player];
                p->trans_num = d.tr.num;
            } else {
                p->trans_dest = olddest;
            }
            if (d.tr.from == g->planet_focus_i[active_player]) {
                p->trans_num = 0;
            }
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_minus) {
            ui_sound_play_sfx_24();
            SUBSAT0(d.tr.num, 1);
        } else if (oi1 == oi_plus) {
            ++d.tr.num;
            SETMIN(d.tr.num, trans_max);
        } else if (oi1 == oi_scroll) {
            ui_starmap_scroll(g, scrollx, scrolly, scrollz);
        }
        ui_starmap_handle_oi_ctrl(&d, oi1);
        if (ui_starmap_handle_tag(&d, oi1, false) != PLANET_NONE) {
            d.tr.other = true;
        }
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if (oi1 == d.oi_tbl_stars[i]) {
                if (ui_extra_enabled && (oi_accept != UIOBJI_INVALID) && (g->planet_focus_i[active_player] == i)) {
                    oi1 = oi_accept;
                    goto do_accept;
                }
                d.tr.other = true;
                g->planet_focus_i[active_player] = i;
                ui_sound_play_sfx_24();
                break;
            }
        }
        if (!flag_done) {
            pt = &(g->planet[g->planet_focus_i[active_player]]);
            p->trans_dest = g->planet_focus_i[active_player];
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
            ui_starmap_trans_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            oi_f2 = uiobj_add_inputkey(MOO_KEY_F2);
            oi_f3 = uiobj_add_inputkey(MOO_KEY_F3);
            oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
            oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
            oi_f6 = uiobj_add_inputkey(MOO_KEY_F6);
            oi_f7 = uiobj_add_inputkey(MOO_KEY_F7);
            oi_f8 = uiobj_add_inputkey(MOO_KEY_F8);
            oi_f9 = uiobj_add_inputkey(MOO_KEY_F9);
            oi_f10 = uiobj_add_inputkey(MOO_KEY_F10);
            ui_starmap_fill_oi_tbl_stars(&d);
            oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            if ((d.tr.other) && (pt->owner != PLAYER_NONE)
              && (pt->within_frange[active_player] == 1)
              && BOOLVEC_IS1(pt->explored, active_player)
              && (pt->type >= g->eto[active_player].have_colony_for)
            ) {
                oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
                uiobj_add_slider_int(258, 124, 0, trans_max, 41, 8, &d.tr.num);
                oi_minus = uiobj_add_mousearea(252, 124, 256, 131, MOO_KEY_MINUS);
                oi_plus = uiobj_add_mousearea(301, 124, 305, 131, MOO_KEY_PLUS);
            }
            oi_scroll = uiobj_add_tb(6, 6, 2, 2, 108, 86, &scrollx, &scrolly, &scrollz, ui_scale);
            oi_search = uiobj_add_inputkey(MOO_KEY_SLASH);
            ui_starmap_fill_oi_ctrl(&d);
            ui_starmap_add_oi_bottom_buttons(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.tr.from;
}
