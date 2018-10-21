#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_cheat.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_save.h"
#include "game_str.h"
#include "game_tech.h"
#include "game_techtypes.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "uicursor.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uihelp.h"
#include "uiobj.h"
#include "uisearch.h"
#include "uisound.h"
#include "uistarmap_common.h"

/* -------------------------------------------------------------------------- */

static void ui_starmap_draw_cb1(void *vptr)
{
    struct starmap_data_s *d = vptr;
    ui_starmap_draw_basic(d);
    if (d->g->gaux->flag_cheat_events) {
        ui_draw_text_overlay(0, 0, game_str_no_events);
    }
}

static void ui_starmap_planet_slider_cb(void *ctx, uint8_t i, int16_t value)
{
    struct starmap_data_s *d = ctx;
    struct game_s *g = d->g;
    planet_t *p = &g->planet[g->planet_focus_i[d->api]];
    if (!p->slider_lock[i]) {
        game_adjust_slider_group(p->slider, i, p->slider[i], PLANET_SLIDER_NUM, p->slider_lock);
    }
}

static bool ui_starmap_remove_build_finished(struct game_s *g, player_id_t api, planet_t *p)
{
    int num = g->evn.build_finished_num[api];
    if (num) {
        g->evn.build_finished_num[api] = --num;
        for (planet_finished_t i = 0; i < FINISHED_NUM; ++i) {
            if ((i != FINISHED_SHIP) && BOOLVEC_IS1(p->finished, i)) {
                BOOLVEC_SET0(p->finished, i);
                break;
            }
        }
        if (!num) {
            return true;
        }
    }
    return false;
}

static void ui_starmap_fill_oi_slider(struct starmap_data_s *d, planet_t *p)
{
    const struct game_s *g = d->g;
    d->sm.oi_ship = UIOBJI_INVALID;
    d->sm.oi_reloc = UIOBJI_INVALID;
    d->sm.oi_trans = UIOBJI_INVALID;
    UIOBJI_SET_TBL3_INVALID(d->sm.oi_tbl_slider_lock, d->sm.oi_tbl_slider_minus, d->sm.oi_tbl_slider_plus);
    if ((p->owner == d->api) && (p->unrest != PLANET_UNREST_REBELLION)) {
        for (planet_slider_i_t i = PLANET_SLIDER_SHIP; i < PLANET_SLIDER_NUM; ++i) {
            int y0;
            y0 = 81 + i * 11;
            if (!p->slider_lock[i]) {
                uiobj_add_slider_func(253, y0 + 3, 0, 100, 25, 9, &p->slider[i], ui_starmap_planet_slider_cb, d, i);
                d->sm.oi_tbl_slider_minus[i] = uiobj_add_mousearea(247, y0 + 1, 251, y0 + 8, MOO_KEY_UNKNOWN);
                d->sm.oi_tbl_slider_plus[i] = uiobj_add_mousearea(280, y0 + 1, 283, y0 + 8, MOO_KEY_UNKNOWN);
            }
            d->sm.oi_tbl_slider_lock[i] = uiobj_add_mousearea(226, y0, 245, y0 + 9, MOO_KEY_UNKNOWN);
        }
        d->sm.oi_ship = uiobj_add_t0(282, 140, "", ui_data.gfx.starmap.col_butt_ship, MOO_KEY_s);
        if (p->buildship != BUILDSHIP_STARGATE) {
            d->sm.oi_reloc = uiobj_add_t0(282, 152, "", ui_data.gfx.starmap.col_butt_reloc, MOO_KEY_l);
        }
        if (g->evn.have_plague && (g->evn.plague_planet_i == g->planet_focus_i[d->api])) {
            lbxgfx_set_frame(ui_data.gfx.starmap.col_butt_trans, 1);
            lbxgfx_draw_frame(282, 164, ui_data.gfx.starmap.col_butt_trans, UI_SCREEN_W, ui_scale);
        } else {
            d->sm.oi_trans = uiobj_add_t0(282, 164, "", ui_data.gfx.starmap.col_butt_trans, MOO_KEY_x);
        }
    }
}

static void ui_starmap_do_help(struct game_s *g, player_id_t api)
{
    const empiretechorbit_t *e = &(g->eto[api]);
    const shipresearch_t *srd = &(g->srd[api]);
    const planet_t *p = &(g->planet[g->evn.home[api]]);
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 0) && (g->year == 1)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 0);
        ui_help(0x00);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 2) && ((e->contact[0] & (~(1 << api))) != 0)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 2);
        ui_help(0x13);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 1) && (p->missile_bases == 0) && (p->pop >= 70)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 1);
        ui_help(0x12);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 3) && (g->year >= 70)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 3);
        ui_help(0x14);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 4) && (g->year >= 30)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 4);
        ui_help(0x16);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 5)) {
        for (int i = 1; i < e->tech.completed[TECH_FIELD_WEAPON]; ++i) {
            if (srd->researchcompleted[TECH_FIELD_WEAPON][i] != 2) {
                BOOLVEC_TBL_SET1(g->evn.help_shown, api, 5);
                ui_help(0x17);
                return;
            }
        }
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 6) && (g->year < 70)) {
        bool have_explored = false, can_colonize = true;
        for (int i = 0; i < g->galaxy_stars; ++i) {
            const planet_t *p2 = &(g->planet[i]);
            if ((p2 != p) && BOOLVEC_IS1(p2->explored, api)) {
                have_explored = true;
                if ((p2->type < e->have_colony_for) || (p2->owner == api)) {
                    can_colonize = false;
                    break;
                }
            }
        }
        if (have_explored && can_colonize) {
            BOOLVEC_TBL_SET1(g->evn.help_shown, api, 6);
            ui_help(0x19);
            return;
        }
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 8) && (p->factories >= 50) && (e->race != RACE_SILICOID)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 8);
        ui_help(0x1a);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 9)) {
        for (int i = 0; i < 8; ++i) {
            if (game_tech_player_has_tech(g, TECH_FIELD_PROPULSION, TECH_PROP_NUCLEAR_ENGINES + i * 6, api)) {
                BOOLVEC_TBL_SET1(g->evn.help_shown, api, 9);
                ui_help(0x18);
                return;
            }
        }
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 10)) {
        for (int i = 0; i < 4; ++i) {
            if (game_tech_player_has_tech(g, TECH_FIELD_FORCE_FIELD, TECH_FFLD_CLASS_V_PLANETARY_SHIELD + i * 10, api)) {
                BOOLVEC_TBL_SET1(g->evn.help_shown, api, 10);
                ui_help(0x1c);
                return;
            }
        }
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 11) && game_tech_player_has_tech(g, TECH_FIELD_PLANETOLOGY, TECH_PLAN_SOIL_ENRICHMENT, api)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 11);
        ui_help(0x1d);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 12) && game_tech_player_has_tech(g, TECH_FIELD_PLANETOLOGY, TECH_PLAN_ATMOSPHERIC_TERRAFORMING, api)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 12);
        ui_help(0x1e);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 13) && game_tech_player_has_tech(g, TECH_FIELD_PLANETOLOGY, TECH_PLAN_ADVANCED_SOIL_ENRICHMENT, api)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 13);
        ui_help(0x1f);
        return;
    }
    if (BOOLVEC_TBL_IS0(g->evn.help_shown, api, 14) && game_tech_player_has_tech(g, TECH_FIELD_PROPULSION, TECH_PROP_INTERGALACTIC_STAR_GATES, api)) {
        BOOLVEC_TBL_SET1(g->evn.help_shown, api, 14);
        ui_help(0x20);
        return;
    }
}

/* -------------------------------------------------------------------------- */

void ui_starmap_set_pos_focus(const struct game_s *g, player_id_t active_player)
{
    const planet_t *p = &g->planet[g->planet_focus_i[active_player]];
    ui_starmap_set_pos(g, p->x, p->y);
}

void ui_starmap_set_pos(const struct game_s *g, int x, int y)
{
    x -= (54 * ui_scale) / starmap_scale;
    SETRANGE(x, 0, g->galaxy_maxx - ((108 * ui_scale) / starmap_scale));
    ui_data.starmap.x = x;
    ui_data.starmap.x2 = x;
    y -= (43 * ui_scale) / starmap_scale;
    SETRANGE(y, 0, g->galaxy_maxy - ((86 * ui_scale) / starmap_scale));
    ui_data.starmap.y = y;
    ui_data.starmap.y2 = y;
}

void ui_starmap_do(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_b, oi_c, oi_scroll, oi_starview1, oi_starview2, oi_shippic, oi_finished, oi_equals,
            oi_f2, oi_f3, oi_f4, oi_f5, oi_f6, oi_f7, oi_f8, oi_f9, oi_f10,
            oi_alt_galaxy, oi_alt_m, oi_alt_c, oi_alt_p, oi_alt_r, oi_alt_events,
            oi_governor, oi_wheelname, oi_wheelshippic, oi_xtramenu, oi_search
            ;
    int16_t scrollx = 0, scrolly = 0, scrollmisc = 0;
    uint8_t scrollz = starmap_scale;
    struct starmap_data_s d;

    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;

    ui_delay_1();
    ui_sound_stop_music();  /* or fade? */
    uiobj_set_downcount(1);
    game_update_production(g);
    game_update_visibility(g);
    ui_data.gfx.colonies.current = NULL;
    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        STARMAP_UIOBJ_CLEAR_FX(); \
        oi_b = UIOBJI_INVALID; \
        oi_c = UIOBJI_INVALID; \
        oi_starview1 = UIOBJI_INVALID; \
        oi_starview2 = UIOBJI_INVALID; \
        oi_shippic = UIOBJI_INVALID; \
        oi_finished = UIOBJI_INVALID; \
        oi_equals = UIOBJI_INVALID; \
        oi_governor = UIOBJI_INVALID; \
        oi_wheelname = UIOBJI_INVALID; \
        oi_wheelshippic = UIOBJI_INVALID; \
        oi_xtramenu = UIOBJI_INVALID; \
        d.sm.oi_ship = UIOBJI_INVALID; \
        d.sm.oi_reloc = UIOBJI_INVALID; \
        d.sm.oi_trans = UIOBJI_INVALID; \
        UIOBJI_SET_TBL3_INVALID(d.sm.oi_tbl_slider_lock, d.sm.oi_tbl_slider_minus, d.sm.oi_tbl_slider_plus); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    oi_alt_galaxy = uiobj_add_alt_str("galaxy");
    oi_alt_m = uiobj_add_alt_str("m");
    oi_alt_c = uiobj_add_alt_str("c");
    oi_alt_p = uiobj_add_alt_str("p");
    oi_alt_r = uiobj_add_alt_str("r");
    oi_alt_events = uiobj_add_alt_str("events");

    uiobj_set_callback_and_delay(ui_starmap_draw_cb1, &d, STARMAP_DELAY);

    while (!flag_done) {
        planet_t *p;
        int16_t oi1, oi2;
        p = &g->planet[g->planet_focus_i[active_player]];
        uiobj_set_help_id((p->owner == active_player) ? 0 : 3);
        scrollmisc = 0;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
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
        } else if (oi1 == d.sm.oi_reloc) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_RELOC;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.sm.oi_trans) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_TRANS;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (((oi1 == oi_starview1) && BOOLVEC_IS1(p->explored, active_player)) || (oi1 == oi_starview2)) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARVIEW;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_b) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_SCRAP_BASES;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_c) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_SPIES_CAUGHT;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if ((oi1 == oi_alt_r) && (p->owner == active_player)) {
            for (int i = 0; i < g->galaxy_stars; ++i) {
                planet_t *p2 = &(g->planet[i]);
                if ((p2->owner == active_player) && (p2->reloc != i)) {
                    p2->reloc = g->planet_focus_i[active_player];
                }
            }
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_alt_m) {
            ui_data.starmap.flag_show_grid = !ui_data.starmap.flag_show_grid;
            ui_sound_play_sfx_24();
        } else if ((oi1 == oi_finished) || ((oi1 == UIOBJI_ESC) && (oi_finished != UIOBJI_INVALID))) {
            if (ui_starmap_remove_build_finished(g, active_player, p)) {
                if (ui_extra_enabled) {
                    g->planet_focus_i[active_player] = ui_data.start_planet_focus_i;
                    ui_starmap_set_pos_focus(g, active_player);
                }
            }
            ui_sound_play_sfx_24();
            flag_done = true;
            ui_delay_1();
            oi1 = 0;
        } else if (oi1 == oi_alt_galaxy) {
            if (game_cheat_galaxy(g, active_player)) {
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_alt_events) {
            if (game_cheat_events(g, active_player)) {
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f10) {
            game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", g);
        } else if (oi1 == oi_alt_p) {
            game_cheat_traits(g, active_player);
        } else if (oi1 == oi_search) {
            ui_sound_play_sfx_24();
            ui_search_set_pos(g, active_player);
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
                    g->planet_focus_i[active_player] = i;
                    ui_data.starmap.orbit_player = j;
                    ui_data.ui_main_loop_action = (j == active_player) ? UI_MAIN_LOOP_ORBIT_OWN_SEL : UI_MAIN_LOOP_ORBIT_EN_SEL;
                    ui_sound_play_sfx_24();
                    flag_done = true;
                    j = g->players; i = g->galaxy_stars;
                }
            }
        }
        if (0
          || (oi1 == d.sm.oi_ship) || (oi1 == oi_shippic)
          || ((oi1 == oi_wheelshippic) && (scrollmisc < 0))
        ) {
            int n;
            n = p->buildship + 1;
            if (n >= g->eto[active_player].shipdesigns_num) {
                if (n >= (NUM_SHIPDESIGNS + 1)) {
                    n = 0;
                } else if (g->eto[active_player].have_stargates && !p->have_stargate) {
                    n = BUILDSHIP_STARGATE;
                } else {
                    n = 0;
                }
            }
            p->buildship = n;
        }
        if (0
          || ((oi1 == UIOBJI_ESC) && ((oi2 == d.sm.oi_ship) || (oi2 == oi_shippic)))
          || ((oi1 == oi_wheelshippic) && (scrollmisc > 0))
        ) {
            int n;
            n = p->buildship - 1;
            if (n >= g->eto[active_player].shipdesigns_num) {
                n = g->eto[active_player].shipdesigns_num - 1;
            } else if (n < 0) {
                if (g->eto[active_player].have_stargates && !p->have_stargate) {
                    n = BUILDSHIP_STARGATE;
                } else {
                    n = g->eto[active_player].shipdesigns_num - 1;
                }
            }
            p->buildship = n;
        }
        for (planet_slider_i_t i = 0; i < PLANET_SLIDER_NUM; ++i) {
            if (oi1 == d.sm.oi_tbl_slider_lock[i]) {
                p->slider_lock[i] = !p->slider_lock[i];
                ui_sound_play_sfx_24();
            } else if (!p->slider_lock[i]) {
                bool do_adj;
                do_adj = false;
                if (oi1 == d.sm.oi_tbl_slider_minus[i]) {
                    int v = p->slider[i] - 4;
                    SETMAX(v, 0);
                    p->slider[i] = v;
                    do_adj = true;
                } else if (oi1 == d.sm.oi_tbl_slider_plus[i]) {
                    int v = p->slider[i] + 4;
                    SETMIN(v, 100);
                    p->slider[i] = v;
                    do_adj = true;
                }
                if (do_adj) {
                    game_adjust_slider_group(p->slider, i, p->slider[i], PLANET_SLIDER_NUM, p->slider_lock);
                }
            }
        }
        if ((oi1 == oi_scroll) && !g->evn.build_finished_num[active_player]) {
            ui_starmap_scroll(g, scrollx, scrolly, scrollz);
        }
        ui_starmap_handle_oi_ctrl(&d, oi1);
        ui_starmap_handle_tag(&d, oi1, true);
        if (oi1 == oi_f2) {
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
        } else if (oi1 == oi_alt_c) {
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_equals) {
            if (p->prod_after_maint < p->reserve) {
                ui_sound_play_sfx_06();
            } else {
                int v = p->prod_after_maint;
                if (v > g->eto[active_player].reserve_bc) {
                    v = g->eto[active_player].reserve_bc;
                }
                v -= p->reserve;
                p->reserve = v;
                g->eto[active_player].reserve_bc -= v;
                p->total_prod += v;
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, g->planet_focus_i[active_player]);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
            if (BOOLVEC_IS1(g->eto[active_player].orbit[i].visible, active_player)) {
                ui_data.starmap.orbit_player = active_player;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_ORBIT_OWN_SEL;
                flag_done = true;
            }
        } else if (oi1 == oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, g->planet_focus_i[active_player]);
            g->planet_focus_i[active_player] = i;
            ui_starmap_set_pos_focus(g, active_player);
            ui_sound_play_sfx_24();
            if (BOOLVEC_IS1(g->eto[active_player].orbit[i].visible, active_player)) {
                ui_data.starmap.orbit_player = active_player;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_ORBIT_OWN_SEL;
                flag_done = true;
            }
        } else if (((oi1 == oi_f8) || (oi1 == oi_f9)) && g->eto[active_player].have_ia_scanner) {
            int i, pi;
            ui_sound_play_sfx_24();
            pi = g->planet_focus_i[active_player];
            i = ui_starmap_enemy_incoming(g, active_player, pi, (oi1 == oi_f8));
            if (i != pi) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
            }
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
                ui_data.starmap.orbit_player = active_player;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_ORBIT_OWN_SEL;
                flag_done = true;
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
                ui_data.starmap.orbit_player = active_player;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_ORBIT_OWN_SEL;
                flag_done = true;
            }
        } else if (oi1 == oi_governor) {
            /* invert the governor flag */
            BOOLVEC_TOGGLE(p->extras, PLANET_EXTRAS_GOVERNOR);
            if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) {
                game_planet_govern(g, p);
            }
        } else if ((oi1 == UIOBJI_ESC) && (oi2 == oi_governor)) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_GOVERN;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_xtramenu) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_XTRAMENU;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_wheelname) {
            int i;
            i = g->planet_focus_i[active_player];
            i += scrollmisc;
            if (i < 0) {
                i = g->galaxy_stars - 1;
            } else if (i >= g->galaxy_stars) {
                i = 0;
            }
            g->planet_focus_i[active_player] = i;
        }
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if ((oi1 == d.oi_tbl_stars[i]) && !g->evn.build_finished_num[active_player]) {
                g->planet_focus_i[active_player] = i;
                ui_sound_play_sfx_24();
                break;
            }
        }
        p = &(g->planet[g->planet_focus_i[active_player]]);
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
        if (!flag_done) {
            ui_starmap_draw_cb1(&d);
            uiobj_table_set_last(oi_alt_events);
            UIOBJ_CLEAR_LOCAL();
            if (p->owner == active_player) {
                oi_equals = uiobj_add_inputkey(MOO_KEY_EQUALS);
            }
            oi_f2 = uiobj_add_inputkey(MOO_KEY_F2);
            oi_f3 = uiobj_add_inputkey(MOO_KEY_F3);
            oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
            oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
            oi_f6 = uiobj_add_inputkey(MOO_KEY_F6);
            oi_f7 = uiobj_add_inputkey(MOO_KEY_F7);
            oi_f8 = uiobj_add_inputkey(MOO_KEY_F8);
            oi_f9 = uiobj_add_inputkey(MOO_KEY_F9);
            oi_f10 = uiobj_add_inputkey(MOO_KEY_F10);
            if ((p->owner == active_player) && p->missile_bases) {
                oi_b = uiobj_add_mousearea(272, 59, 312, 67, MOO_KEY_b);
            }
            oi_c = uiobj_add_inputkey(MOO_KEY_c);
            ui_starmap_add_oi_bottom_buttons(&d);
            if (g->evn.build_finished_num[active_player]) {
                oi_finished = uiobj_add_mousearea(6, 6, 225, 180, MOO_KEY_SPACE);
            }
            if (p->owner == active_player) {
                oi_starview2 = uiobj_add_mousearea(227, 24, 310, 53, MOO_KEY_UNKNOWN);
                oi_shippic = uiobj_add_mousearea(228, 139, 275, 175, MOO_KEY_UNKNOWN);
                oi_wheelshippic = uiobj_add_mousewheel(228, 139, 275, 175, &scrollmisc);
                if (ui_extra_enabled) {
                    oi_governor = uiobj_add_mousearea(227, 8, 310, 20, MOO_KEY_UNKNOWN);
                }
            }
            if (ui_extra_enabled) {
                oi_xtramenu = uiobj_add_mousearea(0, 195, 5, 199, MOO_KEY_e);
            }
            oi_wheelname = uiobj_add_mousewheel(227, 8, 310, 20, &scrollmisc);
            ui_starmap_fill_oi_tbls(&d);
            {
                int x0, y0;
                x0 = (p->x - ui_data.starmap.x) * 2 + 6;
                y0 = (p->y - ui_data.starmap.y) * 2 + 6;
                oi_starview1 = uiobj_add_mousearea_limited(x0, y0, x0 + 16, y0 + 16, starmap_scale, MOO_KEY_UNKNOWN);
            }
            ui_starmap_fill_oi_tbl_stars(&d);
            ui_starmap_fill_oi_slider(&d, p);
            oi_scroll = uiobj_add_tb(6, 6, 2, 2, 108, 86, &scrollx, &scrolly, &scrollz, ui_scale);
            oi_search = uiobj_add_inputkey(MOO_KEY_SLASH);
            ui_starmap_fill_oi_ctrl(&d);
            if (1) {
                int x0, y0, x1, y1;
                x0 = (p->x - ui_data.starmap.x) * 2 + 6;
                y0 = (p->y - ui_data.starmap.y) * 2 + 6;
                x1 = x0 + 16;
                y1 = y0 + 16;
                ui_cursor_area_tbl[7].x0 = x0;
                ui_cursor_area_tbl[7].x1 = x1;
                ui_cursor_area_tbl[7].y0 = y0;
                ui_cursor_area_tbl[7].y1 = y1;
                if (1
                  && BOOLVEC_IS1(p->explored, active_player)
                  && (x0 >= 7 * ui_scale)
                  && (x1 <= 221 * ui_scale)
                  && (y0 >= 7 * ui_scale)
                  && (y1 <= 177 * ui_scale)
                ) {
                    /* FIXME why were these here? these only seem to break stuff */
                    /*
                    SETMAX(ui_cursor_area_tbl[5].x0, 7);
                    SETMIN(ui_cursor_area_tbl[5].x1, 221);
                    SETMAX(ui_cursor_area_tbl[5].y0, 7);
                    SETMIN(ui_cursor_area_tbl[5].y1, 177);
                    */
                    ui_cursor_setup_area(3, &ui_cursor_area_tbl[5]);
                } else {
                    ui_cursor_setup_area(2, &ui_cursor_area_tbl[3]);
                }
            }
            if (g->evn.build_finished_num[active_player]) {
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[0]);
            }
            ui_draw_finish();
            if (g->difficulty < DIFFICULTY_AVERAGE) {
                ui_starmap_do_help(g, active_player);
            }
            game_rng_step(g);
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_table_clear();
    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    ui_delay_1();
}
