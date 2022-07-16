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

struct starmap_planet_data_s {
    int16_t oi_b;
    int16_t oi_equals;
    int16_t oi_hash;
    int16_t oi_wheelname;
    int16_t oi_starview1;
    int16_t oi_starview2;
    int16_t oi_shippic;
    int16_t oi_wheelshippic;
    int16_t oi_build_everywhere;
    int16_t oi_ctrl_s;
    int16_t oi_ship;
    int16_t oi_reloc;
    int16_t oi_trans;
    int16_t oi_tbl_slider_lock[PLANET_SLIDER_NUM];
    int16_t oi_tbl_slider_minus[PLANET_SLIDER_NUM];
    int16_t oi_tbl_slider_plus[PLANET_SLIDER_NUM];
};

struct starmap_governor_data_s {
    int16_t oi_governor;
    int16_t oi_ship;
    int16_t oi_reserve;
    int16_t oi_tech;
    int16_t oi_bases;
    int16_t oi_wheel_bases;
    int16_t oi_boost;
};

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
    struct starmap_planet_data_s *pd = d->sm.planet_data;
    pd->oi_ship = UIOBJI_INVALID;
    pd->oi_reloc = UIOBJI_INVALID;
    pd->oi_trans = UIOBJI_INVALID;
    UIOBJI_SET_TBL3_INVALID(pd->oi_tbl_slider_lock, pd->oi_tbl_slider_minus, pd->oi_tbl_slider_plus);
    if ((p->owner == d->api) && (p->unrest != PLANET_UNREST_REBELLION)) {
        for (planet_slider_i_t i = PLANET_SLIDER_SHIP; i < PLANET_SLIDER_NUM; ++i) {
            int y0;
            y0 = 81 + i * 11;
            if (!p->slider_lock[i]) {
                uiobj_add_slider_func(253, y0 + 3, 0, 100, 25, 9, &p->slider[i], ui_starmap_planet_slider_cb, d, i);
                pd->oi_tbl_slider_minus[i] = uiobj_add_mousearea(247, y0 + 1, 251, y0 + 8, MOO_KEY_UNKNOWN);
                pd->oi_tbl_slider_plus[i] = uiobj_add_mousearea(280, y0 + 1, 283, y0 + 8, MOO_KEY_UNKNOWN);
            }
            pd->oi_tbl_slider_lock[i] = uiobj_add_mousearea(226, y0, 245, y0 + 9, MOO_KEY_UNKNOWN);
        }
        pd->oi_ship = uiobj_add_t0(282, 140, "", ui_data.gfx.starmap.col_butt_ship, MOO_KEY_s);
        if (p->buildship != BUILDSHIP_STARGATE) {
            pd->oi_reloc = uiobj_add_t0(282, 152, "", ui_data.gfx.starmap.col_butt_reloc, MOO_KEY_r);
        }
        if (g->evn.have_plague && (g->evn.plague_planet_i == g->planet_focus_i[d->api])) {
            lbxgfx_set_frame(ui_data.gfx.starmap.col_butt_trans, 1);
            lbxgfx_draw_frame(282, 164, ui_data.gfx.starmap.col_butt_trans, UI_SCREEN_W, ui_scale);
        } else {
            pd->oi_trans = uiobj_add_t0(282, 164, "", ui_data.gfx.starmap.col_butt_trans, MOO_KEY_x);
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

static bool ui_starmap_handle_planet_controls(struct game_s *g, struct starmap_data_s *d, int16_t scrollmisc, bool *flag_done) {
    struct starmap_planet_data_s *pd = d->sm.planet_data;
    planet_t *p = &g->planet[g->planet_focus_i[d->api]];
    if (d->oi1 == pd->oi_reloc) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_RELOC;
        *flag_done = true;
        ui_sound_play_sfx_24();
        return true;
    }
    if (d->oi1 == pd->oi_trans) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_TRANS;
        *flag_done = true;
        ui_sound_play_sfx_24();
        return true;
    }
    if (d->oi1 == pd->oi_starview1 || d->oi1 == pd->oi_starview2) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARVIEW;
        *flag_done = true;
        ui_sound_play_sfx_24();
        return true;
    }
    if (d->oi1 == pd->oi_b) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_SCRAP_BASES;
        *flag_done = true;
        ui_sound_play_sfx_24();
        return true;
    }
    if (d->oi1 == pd->oi_equals || d->oi1 == pd->oi_hash) {
        if (p->reserve > 0) {
            g->eto[d->api].reserve_bc += p->reserve;
            p->reserve = 0;
            ui_sound_play_sfx_24();
        } else {
            int v = p->prod_after_maint - 2 * p->reserve;
            SETMIN(v, g->eto[d->api].reserve_bc);
            p->reserve += v;
            g->eto[d->api].reserve_bc -= v;
            ui_sound_play_sfx_24();
        }
        game_update_production(g);
        return true;
    }
    if (d->oi1 == pd->oi_wheelname) {
        int i;
        i = g->planet_focus_i[d->api];
        i += scrollmisc;
        if (i < 0) {
            i = g->galaxy_stars - 1;
        } else if (i >= g->galaxy_stars) {
            i = 0;
        }
        g->planet_focus_i[d->api] = i;
        return true;
    }
    if (d->oi1 == pd->oi_build_everywhere || d->oi1 == pd->oi_ctrl_s) {
        game_planet_ship_build_everywhere(g, d->api, p->buildship);
        return true;
    }
    if (0
      || (d->oi1 == pd->oi_ship) || (d->oi1 == pd->oi_shippic)
      || ((d->oi1 == pd->oi_wheelshippic) && (scrollmisc < 0))
    ) {
        int n;
        n = p->buildship + 1;
        if (n >= g->eto[d->api].shipdesigns_num) {
            if (n >= (NUM_SHIPDESIGNS + 1)) {
                n = 0;
            } else if (g->eto[d->api].have_stargates && !p->have_stargate) {
                n = BUILDSHIP_STARGATE;
            } else {
                n = 0;
            }
        }
        p->buildship = n;
        return true;
    }
    if (0
      || ((d->oi1 == UIOBJI_ESC) && ((d->oi2 == pd->oi_ship) || (d->oi2 == pd->oi_shippic)))
      || ((d->oi1 == pd->oi_wheelshippic) && (scrollmisc > 0))
    ) {
        int n;
        n = p->buildship - 1;
        if (n >= g->eto[d->api].shipdesigns_num) {
            n = g->eto[d->api].shipdesigns_num - 1;
        } else if (n < 0) {
            if (g->eto[d->api].have_stargates && !p->have_stargate) {
                n = BUILDSHIP_STARGATE;
            } else {
                n = g->eto[d->api].shipdesigns_num - 1;
            }
        }
        p->buildship = n;
        return true;
    }
    for (planet_slider_i_t i = 0; i < PLANET_SLIDER_NUM; ++i) {
        if (d->oi1 == pd->oi_tbl_slider_lock[i]) {
            p->slider_lock[i] = !p->slider_lock[i];
            ui_sound_play_sfx_24();
            return true;
        } else if (!p->slider_lock[i]) {
            bool do_adj = false;
            int v;
            if (d->oi1 == pd->oi_tbl_slider_minus[i]) {
                if (kbd_is_modifier(MOO_MOD_CTRL)) v = p->slider[i] - 1;
                else v = p->slider[i] - 4;
                SETMAX(v, 0);
                p->slider[i] = v;
                do_adj = true;
            } else if (d->oi1 == pd->oi_tbl_slider_plus[i]) {
                if (kbd_is_modifier(MOO_MOD_CTRL)) v = p->slider[i] + 1;
                else v = p->slider[i] + 4;
                SETMIN(v, 100);
                p->slider[i] = v;
                do_adj = true;
            }
            if (do_adj) {
                game_adjust_slider_group(p->slider, i, p->slider[i], PLANET_SLIDER_NUM, p->slider_lock);
                return true;
            }
        }
    }
    return false;
}

static void ui_starmap_handle_governor(struct game_s *g, struct starmap_data_s *d, int16_t scrollmisc, bool *flag_done) {
    struct starmap_governor_data_s *gd = d->sm.gov_data;
    planet_t *p;
    p = &g->planet[g->planet_focus_i[d->api]];
    if (d->oi1 == gd->oi_ship) {
        ui_sound_play_sfx_24();
        BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOVERNOR);
        BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP);
        BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND);
        game_planet_govern(g, p);
    } else if (d->oi1 == gd->oi_reserve) {
        ui_sound_play_sfx_24();
        BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOVERNOR);
        BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP);
        BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND);
        game_planet_govern(g, p);
    } else if (d->oi1 == gd->oi_tech) {
        ui_sound_play_sfx_24();
        BOOLVEC_SET1(p->extras, PLANET_EXTRAS_GOVERNOR);
        BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP);
        BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND);
        game_planet_govern(g, p);
    } else if (d->oi1 == gd->oi_bases || (d->oi1 == gd->oi_wheel_bases && scrollmisc > 0)) {
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
    } else if (d->oi1 == gd->oi_wheel_bases && scrollmisc < 0) {
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
    } else if (d->oi1 == gd->oi_boost) {
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
    } else if (d->oi1 == gd->oi_governor) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_GOVERN;
        *flag_done = true;
        ui_sound_play_sfx_24();
    }
}

static void ui_starmap_select_governor_highlight(const struct game_s *g, struct starmap_data_s *d) {
    struct starmap_governor_data_s *gd = d->sm.gov_data;
    const planet_t *p = &g->planet[g->planet_focus_i[d->api]];
    d->gov_highlight = 0;
    if (d->oi2 == gd->oi_ship) {
        d->gov_highlight = 1;
    } else if (d->oi2 == gd->oi_reserve) {
        d->gov_highlight = 2;
    } else if (d->oi2 == gd->oi_tech) {
        d->gov_highlight = 3;
    } else if (d->oi2 == gd->oi_bases) {
        if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) d->gov_highlight = 4;
    } else if (d->oi2 == gd->oi_boost) {
        d->gov_highlight = 5;
    }
}

static void ui_starmap_fill_oi_planet(const struct game_s *g, struct starmap_data_s *d, int16_t *scrollmisc) {
    struct starmap_planet_data_s *pd = d->sm.planet_data;
    const planet_t *p = &g->planet[g->planet_focus_i[d->api]];

    if (p->owner == d->api && (!ui_extra_enabled || BOOLVEC_IS0(p->extras, PLANET_EXTRAS_GOVERNOR))) {
        pd->oi_equals = uiobj_add_mousearea(227, 70, 312, 78, MOO_KEY_EQUALS);
        pd->oi_hash = uiobj_add_inputkey(MOO_KEY_HASH);
    }
    if ((p->owner == d->api) && p->missile_bases) {
        if (!ui_extra_enabled || BOOLVEC_IS0(p->extras, PLANET_EXTRAS_GOVERNOR)) {
            pd->oi_b = uiobj_add_mousearea(272, 59, 312, 67, MOO_KEY_b);
        } else {
            pd->oi_b = uiobj_add_inputkey(MOO_KEY_b);
        }
    }
    if (p->owner == d->api) {
        if (ui_extra_enabled && kbd_is_modifier(MOO_MOD_CTRL)) {
            pd->oi_build_everywhere = uiobj_add_mousearea(228, 139, 275, 175, MOO_KEY_UNKNOWN);
        } else {
            pd->oi_shippic = uiobj_add_mousearea(228, 139, 275, 175, MOO_KEY_UNKNOWN);
            pd->oi_wheelshippic = uiobj_add_mousewheel(228, 139, 275, 175, scrollmisc);
        }
        pd->oi_ctrl_s = uiobj_add_inputkey(MOO_KEY_s | MOO_MOD_CTRL);
    }
    pd->oi_wheelname = uiobj_add_mousewheel(227, 8, 310, 20, scrollmisc);
    pd->oi_starview2 = uiobj_add_mousearea(227, 24, 310, 53, MOO_KEY_UNKNOWN);
}

static void ui_starmap_fill_oi_governor(struct starmap_data_s *d, int16_t *scrollmisc) {
    struct starmap_governor_data_s *gd = d->sm.gov_data;
    const planet_t *p = &d->g->planet[d->g->planet_focus_i[d->api]];
    gd->oi_governor = uiobj_add_mousearea(227, 59, 268, 67, MOO_KEY_v);
    if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) {
        gd->oi_ship = uiobj_add_mousearea( 288, 82, 312, 88, MOO_KEY_UNKNOWN );
        gd->oi_reserve = uiobj_add_mousearea( 288, 104, 312, 110, MOO_KEY_UNKNOWN );
        gd->oi_tech = uiobj_add_mousearea( 288, 126, 312, 132, MOO_KEY_UNKNOWN );
        gd->oi_bases = uiobj_add_mousearea( 273, 60, 312, 66, MOO_KEY_UNKNOWN );
        gd->oi_wheel_bases = uiobj_add_mousewheel( 273, 60, 312, 66, scrollmisc);
        gd->oi_boost = uiobj_add_mousearea( 226, 71, 312, 77, MOO_KEY_UNKNOWN );
    }
}

void ui_starmap_do(struct game_s *g, player_id_t active_player)
{
    int16_t oi_finished, oi_alt_galaxy, oi_alt_p, oi_alt_events
            ;
    int16_t scrollmisc = 0;

    struct starmap_data_s d;
    ui_starmap_init_common_data(g, &d, active_player);

    struct starmap_planet_data_s planet_d;
    struct starmap_governor_data_s gov_d;
    d.sm.planet_data = &planet_d;
    d.sm.gov_data = &gov_d;

    d.controllable = false;
    d.draw_own_routes = true;
    d.is_valid_selection = NULL;
    d.do_accept = NULL;

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
        planet_d.oi_b = UIOBJI_INVALID; \
        planet_d.oi_starview1 = UIOBJI_INVALID; \
        planet_d.oi_starview2 = UIOBJI_INVALID; \
        planet_d.oi_shippic = UIOBJI_INVALID; \
        planet_d.oi_build_everywhere = UIOBJI_INVALID; \
        planet_d.oi_ctrl_s = UIOBJI_INVALID; \
        oi_finished = UIOBJI_INVALID; \
        planet_d.oi_equals = UIOBJI_INVALID; \
        planet_d.oi_hash = UIOBJI_INVALID; \
        planet_d.oi_wheelname = UIOBJI_INVALID; \
        planet_d.oi_wheelshippic = UIOBJI_INVALID; \
        planet_d.oi_ship = UIOBJI_INVALID; \
        planet_d.oi_reloc = UIOBJI_INVALID; \
        planet_d.oi_trans = UIOBJI_INVALID; \
        UIOBJI_SET_TBL3_INVALID(planet_d.oi_tbl_slider_lock, planet_d.oi_tbl_slider_minus, planet_d.oi_tbl_slider_plus); \
        gov_d.oi_governor = UIOBJI_INVALID; \
        gov_d.oi_ship = UIOBJI_INVALID; \
        gov_d.oi_reserve = UIOBJI_INVALID; \
        gov_d.oi_tech = UIOBJI_INVALID; \
        gov_d.oi_bases = UIOBJI_INVALID; \
        gov_d.oi_wheel_bases = UIOBJI_INVALID; \
        gov_d.oi_boost = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    oi_alt_galaxy = uiobj_add_alt_str("galaxy");
    oi_alt_p = uiobj_add_alt_str("p");
    oi_alt_events = uiobj_add_alt_str("events");

    uiobj_set_callback_and_delay(ui_starmap_draw_cb1, &d, STARMAP_DELAY);

    while (!d.flag_done) {
        planet_t *p;
        p = &g->planet[g->planet_focus_i[active_player]];
        uiobj_set_help_id((p->owner == active_player) ? 0 : 3);
        scrollmisc = 0;
        ui_delay_prepare();
        if (ui_starmap_handle_common(g, &d)) {
        } else if (ui_starmap_handle_planet_controls(g, &d, scrollmisc, &d.flag_done)) {
        } else if ((d.oi1 == oi_finished) || ((d.oi1 == UIOBJI_ESC) && (oi_finished != UIOBJI_INVALID))) {
            if (ui_starmap_remove_build_finished(g, active_player, p)) {
                if (ui_extra_enabled) {
                    g->planet_focus_i[active_player] = ui_data.start_planet_focus_i;
                    ui_starmap_set_pos_focus(g, active_player);
                }
            }
            ui_sound_play_sfx_24();
            d.flag_done = true;
            ui_delay_1();
            d.oi1 = 0;
        } else if (d.oi1 == oi_alt_galaxy) {
            if (game_cheat_galaxy(g, active_player)) {
                ui_sound_play_sfx_24();
            }
        } else if (d.oi1 == oi_alt_events) {
            if (game_cheat_events(g, active_player)) {
                ui_sound_play_sfx_24();
            }
        } else if (d.oi1 == oi_alt_p) {
            game_cheat_traits(g, active_player);
        } else {
            ui_starmap_handle_governor(g, &d, scrollmisc, &d.flag_done);
        }

        if (!d.flag_done) {
            ui_starmap_select_bottom_highlight(g, &d);
            ui_starmap_select_governor_highlight(g, &d);
            ui_starmap_draw_cb1(&d);
            uiobj_table_set_last(oi_alt_events);
            UIOBJ_CLEAR_LOCAL();
            p = &g->planet[g->planet_focus_i[active_player]];
            if (g->evn.build_finished_num[active_player]) {
                oi_finished = uiobj_add_mousearea(6, 6, 225, 180, MOO_KEY_SPACE);
            }
            ui_starmap_fill_oi_planet(g, &d, &scrollmisc);
            if (p->owner == active_player && ui_extra_enabled) {
                ui_starmap_fill_oi_governor(&d, &scrollmisc);
            }
            ui_starmap_fill_oi_common(&d);
            if (BOOLVEC_IS1(p->explored, active_player)) {
                planet_d.oi_starview1 = d.oi_tbl_stars[g->planet_focus_i[active_player]];
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
