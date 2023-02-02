#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
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
            d->sm.oi_reloc = uiobj_add_t0(282, 152, "", ui_data.gfx.starmap.col_butt_reloc, MOO_KEY_r);
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

void ui_starmap_do(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_b, oi_c, oi_starview1, oi_starview2, oi_shippic, oi_equals, oi_hash,
            oi_f4, oi_f5, oi_f6, oi_f7, oi_f10,
            oi_governor, oi_wheelname, oi_wheelshippic, oi_filter;
    int16_t scrollmisc = 0;
    struct starmap_data_s d;

    ui_starmap_common_init(g, &d, active_player);

    ui_delay_1();
    ui_sound_stop_music();  /* or fade? */
    uiobj_set_downcount(1);
    game_update_production(g);
    game_update_visibility(g);
    ui_data.gfx.colonies.current = NULL;

    ui_starmap_common_late_init(&d, ui_starmap_draw_cb1, false);

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        STARMAP_UIOBJ_CLEAR_FX(); \
        oi_b = UIOBJI_INVALID; \
        oi_c = UIOBJI_INVALID; \
        oi_starview1 = UIOBJI_INVALID; \
        oi_starview2 = UIOBJI_INVALID; \
        oi_shippic = UIOBJI_INVALID; \
        oi_equals = UIOBJI_INVALID; \
        oi_hash = UIOBJI_INVALID; \
        oi_governor = UIOBJI_INVALID; \
        oi_wheelname = UIOBJI_INVALID; \
        oi_wheelshippic = UIOBJI_INVALID; \
        oi_filter = UIOBJI_INVALID; \
        d.sm.oi_ship = UIOBJI_INVALID; \
        d.sm.oi_reloc = UIOBJI_INVALID; \
        d.sm.oi_trans = UIOBJI_INVALID; \
        UIOBJI_SET_TBL3_INVALID(d.sm.oi_tbl_slider_lock, d.sm.oi_tbl_slider_minus, d.sm.oi_tbl_slider_plus); \
        d.sm.oi_gov_ship = UIOBJI_INVALID; \
        d.sm.oi_gov_reserve = UIOBJI_INVALID; \
        d.sm.oi_gov_tech = UIOBJI_INVALID; \
        d.sm.oi_gov_bases = UIOBJI_INVALID; \
        d.sm.oi_gov_wheel_bases = UIOBJI_INVALID; \
        d.sm.oi_gov_boost = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    while (!flag_done) {
        planet_t *p;
        int16_t oi1, oi2;
        p = &g->planet[g->planet_focus_i[active_player]];
        uiobj_set_help_id((p->owner == active_player) ? 0 : 3);
        scrollmisc = 0;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if (ui_starmap_common_handle_oi(g, &d, &flag_done, oi1, oi2)) {
        } else if (oi1 == d.sm.oi_reloc) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_RELOC;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.sm.oi_trans) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_TRANS;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if ((oi1 == oi_starview1) || (oi1 == oi_starview2)) {
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
        } else if (ui_starmap_handle_oi_finished(g, &d, &flag_done, oi1, oi2)) {
            oi1 = 0;
        } else if (oi1 == oi_f10) {
            game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", g);
        } else if (oi1 == d.sm.oi_gov_ship) {
            ui_sound_play_sfx_24();
            BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOVERNOR);
            BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP);
            BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND);
            game_planet_govern(g, p);
        } else if (oi1 == d.sm.oi_gov_reserve) {
            ui_sound_play_sfx_24();
            BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOVERNOR);
            BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP);
            BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND);
            game_planet_govern(g, p);
        } else if (oi1 == d.sm.oi_gov_tech) {
            ui_sound_play_sfx_24();
            BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOVERNOR);
            BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP);
            BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND);
            game_planet_govern(g, p);
        } else if (oi1 == d.sm.oi_gov_bases || (oi1 == d.sm.oi_gov_wheel_bases && scrollmisc > 0)) {
            if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) {
                ui_sound_play_sfx_24();
                int t = p->target_bases + 1;
                SETMAX(t, p->missile_bases);
                SETMIN(t, 250);
                if (t != p->target_bases) {
                    ui_sound_play_sfx_24();
                    p->target_bases = t;
                    game_planet_govern(g, p);
                }
            }
        } else if (oi1 == d.sm.oi_gov_wheel_bases && scrollmisc < 0) {
            if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR) && p->target_bases) {
                int t = p->target_bases;
                --t;
                if (t < p->missile_bases) t = 0;
                SETMAX(t, p->missile_bases);
                if (t != p->target_bases) {
                    ui_sound_play_sfx_24();
                    p->target_bases = t;
                    game_planet_govern(g, p);
                }
            }
        } else if (oi1 == d.sm.oi_gov_boost) {
            if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) {
                if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOV_BOOST_PROD)) {
                    BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_BOOST_BUILD);
                    BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_BOOST_PROD);
                } else if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOV_BOOST_BUILD)) {
                    BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOV_BOOST_BUILD);
                    BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOV_BOOST_PROD);
                } else {
                    BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOV_BOOST_BUILD);
                    BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_BOOST_PROD);
                }
                game_planet_govern(g, p);
            }
        }
        if (oi1 == d.sm.oi_ship) {
            ui_sound_play_sfx_24();
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_PLANET_SHIPS;
            flag_done = true;
            oi1 = 0;
        }
        if (0
          || (oi1 == oi_shippic)
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
          || ((oi1 == UIOBJI_ESC) && (oi2 == oi_shippic))
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
                bool do_adj = false;
                bool upd_eco = false;
                int v;
                if (oi1 == d.sm.oi_tbl_slider_minus[i]) {
                    if (kbd_is_modifier(MOO_MOD_ALT) && i == 3) {
                        upd_eco = true;
                    } else if (kbd_is_modifier(MOO_MOD_ALT)) {
                        p->slider[i] = 0;
                        do_adj = true;
                    } else {
                        if (kbd_is_modifier(MOO_MOD_CTRL)) v = p->slider[i] - 1;
                        else v = p->slider[i] - 4;
                        SETMAX(v, 0);
                        p->slider[i] = v;
                        do_adj = true;
                    }
                } else if (oi1 == d.sm.oi_tbl_slider_plus[i]) {
                    if (kbd_is_modifier(MOO_MOD_ALT)) {
                        p->slider[i] = 100;
                        do_adj = true;
                        if (i != 3) upd_eco = true;
                    } else {
                        if (kbd_is_modifier(MOO_MOD_CTRL)) v = p->slider[i] + 1;
                        else v = p->slider[i] + 4;
                        SETMIN(v, 100);
                        p->slider[i] = v;
                        do_adj = true;
                    }
                }
                if (do_adj) {
                    game_adjust_slider_group(p->slider, i, p->slider[i], PLANET_SLIDER_NUM, p->slider_lock);
                }
                if (upd_eco && !p->slider_lock[3]) {
                    game_planet_update_eco_on_waste(g, p, d.api, true);
                }
            }
        }
        if (oi1 == oi_equals || oi1 == oi_hash) {
            if (2 * p->reserve >= p->prod_after_maint || g->eto[active_player].reserve_bc == 0) {
                g->eto[active_player].reserve_bc += p->reserve;
                p->reserve = 0;
                ui_sound_play_sfx_24();
            } else {
                int v = p->prod_after_maint - 2 * p->reserve;
                SETMIN(v, g->eto[active_player].reserve_bc);
                p->reserve += v;
                g->eto[active_player].reserve_bc -= v;
                ui_sound_play_sfx_24();
            }
            game_update_production(g);
        } else if (oi1 == oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, g->planet_focus_i[active_player]);
            if (i != PLANET_NONE) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
                if (BOOLVEC_IS1(g->eto[active_player].orbit[i].visible, active_player)) {
                    ui_data.starmap.orbit_player = active_player;
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_ORBIT_OWN_SEL;
                    flag_done = true;
                }
            }
        } else if (oi1 == oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, g->planet_focus_i[active_player]);
            if (i != PLANET_NONE) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
                if (BOOLVEC_IS1(g->eto[active_player].orbit[i].visible, active_player)) {
                    ui_data.starmap.orbit_player = active_player;
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_ORBIT_OWN_SEL;
                    flag_done = true;
                }
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
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_GOVERN;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_filter) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_MSGFILTER;
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

        d.from_i = g->planet_focus_i[active_player];
        ui_starmap_set_ruler(&d, oi2);
        d.ruler_from_fleet = false;
        d.gov_highlight = 0;

        p = &(g->planet[g->planet_focus_i[active_player]]);
        ui_starmap_select_bottom_highlight(&d, oi2);
        if (oi2 == d.sm.oi_gov_ship) {
            d.gov_highlight = 1;
        } else if (oi2 == d.sm.oi_gov_reserve) {
            d.gov_highlight = 2;
        } else if (oi2 == d.sm.oi_gov_tech) {
            d.gov_highlight = 3;
        } else if (oi2 == d.sm.oi_gov_bases) {
            if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) d.gov_highlight = 4;
        } else if (oi2 == d.sm.oi_gov_boost) {
            d.gov_highlight = 5;
        }
        if (!flag_done) {
            ui_starmap_draw_cb1(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            if (p->owner == active_player) {
                if (BOOLVEC_IS0(p->extras, PLANET_EXTRAS_GOVERNOR)) {
                    oi_equals = uiobj_add_mousearea(227, 70, 312, 78, MOO_KEY_EQUALS);
                } else {
                    oi_equals = uiobj_add_inputkey(MOO_KEY_EQUALS);
                }
                oi_hash = uiobj_add_inputkey(MOO_KEY_HASH);
            }
            if (!g->evn.build_finished_num[active_player]) {
                oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
                oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
                oi_f6 = uiobj_add_inputkey(MOO_KEY_F6);
                oi_f7 = uiobj_add_inputkey(MOO_KEY_F7);
                oi_f10 = uiobj_add_inputkey(MOO_KEY_F10);
            }
            if ((p->owner == active_player) && p->missile_bases) {
                if (BOOLVEC_IS0(p->extras, PLANET_EXTRAS_GOVERNOR)) {
                    oi_b = uiobj_add_mousearea(272, 59, 312, 67, MOO_KEY_b);
                } else {
                    oi_b = uiobj_add_inputkey(MOO_KEY_b);
                }
            }
            oi_c = uiobj_add_inputkey(MOO_KEY_c);
            if (p->owner == active_player) {
                oi_shippic = uiobj_add_mousearea(228, 139, 275, 175, MOO_KEY_UNKNOWN);
                oi_wheelshippic = uiobj_add_mousewheel(228, 139, 275, 175, &scrollmisc);
                if (ui_governor_enabled) {
                    oi_governor = uiobj_add_mousearea(227, 59, 268, 67, MOO_KEY_v);
                    if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) {
                        d.sm.oi_gov_ship = uiobj_add_mousearea( 288, 82, 312, 88, MOO_KEY_UNKNOWN );
                        d.sm.oi_gov_reserve = uiobj_add_mousearea( 288, 104, 312, 110, MOO_KEY_UNKNOWN );
                        d.sm.oi_gov_tech = uiobj_add_mousearea( 288, 126, 312, 132, MOO_KEY_UNKNOWN );
                        d.sm.oi_gov_bases = uiobj_add_mousearea( 273, 60, 312, 66, MOO_KEY_UNKNOWN );
                        d.sm.oi_gov_wheel_bases = uiobj_add_mousewheel( 273, 60, 312, 66, &scrollmisc);
                        d.sm.oi_gov_boost = uiobj_add_mousearea( 226, 71, 312, 77, MOO_KEY_UNKNOWN );
                    }
                }
            }
            if (ui_governor_enabled) {
                oi_filter = uiobj_add_inputkey(MOO_KEY_i);
            }
            if (!g->evn.build_finished_num[active_player]) {
                oi_wheelname = uiobj_add_mousewheel(227, 8, 310, 20, &scrollmisc);
            }
            ui_starmap_common_fill_oi(&d);
            if (BOOLVEC_IS1(p->explored, active_player)) {
                oi_starview1 = d.oi_tbl_stars[g->planet_focus_i[active_player]];
                d.oi_tbl_stars[g->planet_focus_i[active_player]] = UIOBJI_INVALID;
                oi_starview2 = uiobj_add_mousearea(227, 24, 310, 53, MOO_KEY_UNKNOWN);
            }
            ui_starmap_fill_oi_slider(&d, p);
            if (g->evn.build_finished_num[active_player]) {
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[0]);
            } else {
                ui_cursor_setup_area(2, &ui_cursor_area_tbl[3]);
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
