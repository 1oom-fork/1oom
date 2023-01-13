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
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uiobj.h"
#include "uisound.h"
#include "uistarmap_common.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static void ui_starmap_trans_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *pf = &g->planet[d->from_i];
    const planet_t *px = &g->planet[pf->trans_dest];
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
    if (pf != pt || ui_modern_controls) {
        const uint8_t *ctbl;
        int x1, y1;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 14;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 14;
        if (!d->valid_target_cb(d, g->planet_focus_i[d->api])) {
            ctbl = colortbl_line_red;
        } else {
            ctbl = colortbl_line_green;
        }
        if (ui_modern_controls) {
            int x2, y2;
            x2 = (px->x - ui_data.starmap.x) * 2 + 14;
            y2 = (px->y - ui_data.starmap.y) * 2 + 14;
            if (pt != pf) {
                ui_draw_planet_frame_limit_ctbl(x1 - 6, y1 - 6, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
            }
            if (px != pf && pf->trans_num > 0) {
                ui_draw_line_limit_ctbl(x0 + 6, y0 + 6, x2, y2, colortbl_line_green, 5, ui_data.starmap.line_anim_phase, starmap_scale);
            }
        }
        if (pf != pt) {
            ui_draw_line_limit_ctbl(x0 + 6, y0 + 6, x1, y1, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
        }
    }
    if (d->tr.other) {
        if (pt->within_frange[d->api] != 1) {
            int mindist = game_get_min_dist(g, d->api, g->planet_focus_i[d->api]);
            lbxfont_select_set_12_1(0, 0xe, 5, 0);
            lbxfont_set_gap_h(1);
            lbxfont_print_str_split(228, 94, 82, game_str_sm_notrange, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
            lib_sprintf(buf, sizeof(buf), "%s %i %s %i %s", game_str_sm_notrange1, mindist, game_str_sm_notrange2, g->eto[d->api].fuel_range, game_str_sm_notrange3);
            lbxfont_print_str_split(229, 125, 80, game_str_sm_seltr, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else if (BOOLVEC_IS0(pt->explored, d->api)) {
            lbxfont_select(0, 0xe, 0, 0);
            lbxfont_print_str_split(229, 105, 80, game_str_sm_trfirste, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else if (pt->type < g->eto[d->api].have_colony_for) {
            int pos;
            lbxfont_select(0, 6, 0, 0);
            lbxfont_set_gap_h(1);
            pos = lib_sprintf(buf, sizeof(buf), "%s ", game_str_sm_trcontr1);
            lib_sprintf(&buf[pos], sizeof(buf) - pos, "%s ", game_str_tbl_sm_pltype[pt->type]);
            util_str_tolower(&buf[pos], sizeof(buf) - pos);
            lib_strcat(&buf[pos], game_str_sm_trcontr2, sizeof(buf) - pos);
            lbxfont_print_str_split(228, 111, 82, buf, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else if (pt->owner == PLAYER_NONE) {
            lbxfont_select(0, 0xe, 0, 0);
            lbxfont_print_str_split(230, 105, 80, game_str_sm_trfirstc, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else {
            treaty_t treaty = TREATY_NONE;
            uiobj_set_help_id(1);
            if (pf->have_stargate && pt->have_stargate && (pf->owner == pt->owner)) {
                lib_strcpy(buf, game_str_sm_stargate, sizeof(buf));
            } else {
                int eta, engine = g->eto[d->api].have_engine;
                eta = game_calc_eta_trans(g, engine, pf->x, pf->y, pt->x, pt->y);
                lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
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
                        lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_trwarnm1, pt->max_pop3, game_str_sm_trwarnm2);
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

static bool ui_starmap_trans_valid_destination(const struct starmap_data_s *d, int planet_i)
{
    const struct game_s *g = d->g;
    const planet_t *p = &g->planet[planet_i];
    return (p->owner != PLAYER_NONE)
        && (p->within_frange[g->active_player] == 1)
        && BOOLVEC_IS1(p->explored, d->api)
        && (p->type >= g->eto[d->api].have_colony_for);
}

static void ui_starmap_trans_do_accept(struct starmap_data_s *d)
{
    struct game_s *g = d->g;
    uint8_t planet_i = g->planet_focus_i[d->api];
    planet_t *pt = &g->planet[planet_i];
    planet_t *pf = &g->planet[d->from_i];

    if (BOOLVEC_IS1(pt->explored, d->api) && (pt->within_frange[d->api] == 1)) {
        pf->trans_dest = planet_i;
        pf->trans_num = d->tr.num;
    }
    if (d->from_i == planet_i) {
        pf->trans_num = 0;
    }
    if (pf->trans_num && pf->owner != PLAYER_NONE) {
        if(BOOLVEC_IS1(pf->extras, PLANET_EXTRAS_GOVERNOR)) {
            game_update_production(g);
            game_planet_govern(g, pf);
        }
    }
}

void ui_starmap_trans(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_cancel, oi_plus, oi_minus, oi_equals,
            oi_f4, oi_f5, oi_f6, oi_f7, oi_f10,
            oi_a;
    struct starmap_data_s d;
    planet_t *p;
    int16_t trans_max;

    ui_starmap_common_init(g, &d, active_player);
    d.valid_target_cb = ui_starmap_trans_valid_destination;
    d.on_accept_cb = ui_starmap_trans_do_accept;

    d.tr.blink = false;
    {
        uint8_t pi = g->planet_focus_i[active_player];
        p = &(g->planet[pi]);
        if (p->trans_num != 0) {
            d.tr.other = true;
            g->planet_focus_i[active_player] = p->trans_dest;
        } else if (ui_modern_controls) {
            d.tr.other = true;
            g->planet_focus_i[active_player] = pi;
        } else {
            d.tr.other = false;
            g->planet_focus_i[active_player] = pi;
        }
    }
    d.tr.num = p->trans_num;
    trans_max = p->pop / 2;

    ui_starmap_common_late_init(&d, ui_starmap_trans_draw_cb, true);

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        STARMAP_UIOBJ_CLEAR_FX(); \
        oi_cancel = UIOBJI_INVALID; \
        oi_plus = UIOBJI_INVALID; \
        oi_minus = UIOBJI_INVALID; \
        oi_equals = UIOBJI_INVALID; \
        oi_a = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    while (!flag_done) {
        int16_t oi1, oi2;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if (ui_starmap_common_handle_oi(g, &d, &flag_done, oi1, oi2)) {
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
                d.tr.other = true;
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
                d.tr.other = true;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, g->planet_focus_i[active_player]);
            if (i != PLANET_NONE) {
                g->planet_focus_i[active_player] = i;
                d.tr.other = true;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, g->planet_focus_i[active_player]);
            if (i != PLANET_NONE) {
                g->planet_focus_i[active_player] = i;
                d.tr.other = true;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        }
        if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_minus) {
            ui_sound_play_sfx_24();
            if (kbd_is_modifier(MOO_MOD_CTRL)) {
                SUBSAT0(d.tr.num, (trans_max + 9) / 10);
            } else {
                SUBSAT0(d.tr.num, 1);
            }
            d.tr.other = true;
        } else if (oi1 == oi_plus || oi1 == oi_equals) {
            ui_sound_play_sfx_24();
            if (kbd_is_modifier(MOO_MOD_CTRL)) {
                d.tr.num += (trans_max + 9) / 10;
            } else {
                ++d.tr.num;
            }
            SETMIN(d.tr.num, trans_max);
            d.tr.other = true;
        } else if (oi1 == oi_a) {
            ui_sound_play_sfx_24();
            if (d.tr.num < trans_max) {
                d.tr.num = trans_max;
            } else {
                d.tr.num = 0;
            }
            d.tr.other = true;
        }
        ui_starmap_set_ruler(&d, oi2);
        d.ruler_from_fleet = false;
        if (!flag_done) {
            ui_starmap_select_bottom_highlight(&d, oi2);
            ui_starmap_trans_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
            oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
            oi_f6 = uiobj_add_inputkey(MOO_KEY_F6);
            oi_f7 = uiobj_add_inputkey(MOO_KEY_F7);
            oi_f10 = uiobj_add_inputkey(MOO_KEY_F10);
            ui_starmap_common_fill_oi(&d);
            oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            if (d.valid_target_cb(&d, g->planet_focus_i[active_player])) {
                if (d.tr.other) {
                    d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
                    uiobj_add_slider_int(258, 124, 0, trans_max, 41, 8, &d.tr.num);
                    oi_minus = uiobj_add_mousearea(252, 124, 256, 131, MOO_KEY_MINUS);
                    oi_plus = uiobj_add_mousearea(301, 124, 305, 131, MOO_KEY_PLUS);
                } else {
                    oi_minus = uiobj_add_inputkey(MOO_KEY_MINUS);
                    oi_plus = uiobj_add_inputkey(MOO_KEY_PLUS);
                }
                oi_equals = uiobj_add_inputkey(MOO_KEY_EQUALS);
                oi_a = uiobj_add_inputkey(MOO_KEY_a);
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.from_i;

    /* We clear the UI object table because one of them contains a pointer to a local variable. */
    uiobj_table_clear();
}
