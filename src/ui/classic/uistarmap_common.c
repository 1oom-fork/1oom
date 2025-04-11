#include "config.h"

#include <stdio.h>

#include "uistarmap_common.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_str.h"
#include "game_tech.h"
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
#include "uiobj.h"
#include "uisound.h"

/* -------------------------------------------------------------------------- */

const uint8_t colortbl_textbox[5] = { 0x18, 0x17, 0x16, 0x15, 0x14 };
const uint8_t colortbl_line_red[5] = { 0x44, 0x43, 0x42, 0x41, 0x40 };
const uint8_t colortbl_line_reloc[5] = { 0x14, 0x15, 0x16, 0x17, 0x18 };
const uint8_t colortbl_line_green[5] = { 0xb0, 0xb1, 0xb2, 0xb3, 0xb4 };

/* -------------------------------------------------------------------------- */

static void ui_starmap_draw_planetinfo_do(const struct game_s *g, player_id_t api, uint8_t planet_i, bool explored, bool show_plus)
{
    const planet_t *p = &g->planet[planet_i];
    if (explored) {
        lbxfont_select_set_12_4(4, 0xf, 0, 0);
        lbxfont_print_str_center(269, 10, p->name, UI_SCREEN_W);
        if (p->type == PLANET_TYPE_NOT_HABITABLE) {
            lbxfont_select(0, 0xe, 0, 0);
            lbxfont_print_str_center(269, 32, game_str_sm_nohabit, UI_SCREEN_W);
            lbxfont_print_str_center(269, 41, game_str_sm__planets, UI_SCREEN_W);
        } else {
            const char *str = NULL;
            lbxgfx_draw_frame(229, 27, ui_data.gfx.planets.planet[p->infogfx], UI_SCREEN_W);
            lbxfont_select(0, 0xd, 0, 0);
            lbxfont_print_str_right(305, 28, game_str_tbl_sm_pltype[p->type], UI_SCREEN_W);
            if (g->evn.have_plague && (g->evn.plague_planet_i == planet_i)) {
                str = game_str_sm_plague;
            } else if (g->evn.have_nova && (g->evn.nova_planet_i == planet_i)) {
                str = game_str_sm_nova;
            } else if (g->evn.have_comet && (g->evn.comet_planet_i == planet_i)) {
                str = game_str_sm_comet;
            } else if (g->evn.have_pirates && (g->evn.pirates_planet_i == planet_i)) {
                str = game_str_sm_pirates;
            } else if (p->unrest == PLANET_UNREST_REBELLION) {
                str = game_str_sm_rebellion;
            } else if (p->unrest == PLANET_UNREST_UNREST) {
                str = game_str_sm_unrest;
            } else if (g->evn.have_accident && (g->evn.accident_planet_i == planet_i)) {
                str = game_str_sm_accident;
            }
            if (str) {
                lbxfont_select(5, 5, 0, 0);
                lbxfont_print_str_right(305, 40, str, UI_SCREEN_W);
            } else {
                int x, max_pop = p->max_pop3;
                lbxfont_select(0, 1, 0, 0);
                if (p->special == PLANET_SPECIAL_NORMAL) {
                    str = game_str_tbl_sm_pgrowth[p->growth];
                } else {
                    str = game_str_tbl_sm_pspecial[p->special];
                }
                lbxfont_print_str_right(305, 36, str, UI_SCREEN_W);
                lbxfont_select(2, 0xe, 0, 0);
                if (show_plus && (p->owner == api) && ((p->max_pop2 + g->eto[api].have_terraform_n) > max_pop) && (max_pop < game_num_max_pop)) {
                    lbxfont_print_str_normal(289, 44, "+", UI_SCREEN_W);
                    str = game_str_sm_popmaxw;
                    x = 287;
                } else {
                    str = game_str_sm_popmaxn;
                    x = 291;
                }
                /* FIXME split the popmax strings and use x for positioning */
                lbxfont_print_str_right(305, 45, str, UI_SCREEN_W);
                if ((!show_plus) || (g->eto[api].race != RACE_SILICOID)) {
                    max_pop -= p->waste;
                }
                SETMAX(max_pop, 10);
                lbxfont_print_num_right(x, 45, max_pop, UI_SCREEN_W);
            }
        }
    } else {
        lbxfont_select(5, 0xe, 0, 0);
        lbxfont_print_str_center(269, show_plus ? 27 : 35, game_str_sm_unexplored, UI_SCREEN_W);
    }
}

static void ui_starmap_draw_range_parsec(struct starmap_data_s *d, int y)
{
    const struct game_s *g = d->g;
    int dist = game_get_min_dist(g, d->api, g->planet_focus_i[d->api]);
    char buf[64];
    sprintf(buf, "%s %i %s", game_str_sm_range, dist, (dist == 1) ? game_str_sm_parsec : game_str_sm_parsecs);
    lbxfont_select_set_12_4(0, 4, 0, 0);
    lbxfont_print_str_center(269, y, buf, UI_SCREEN_W);
}

static int tenths_2str(char *buf, int num)
{
    int pos;
    pos = sprintf(buf, "%i", num / 10);
    if ((num < 100) && ((num % 10) != 0)) {
        pos += sprintf(&buf[pos], ".%i", num % 10);
    }
    return pos;
}

static void ui_starmap_draw_sliders_and_prod(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    const planet_t *p = &g->planet[g->planet_focus_i[d->api]];
    int x = 311;
    char buf[64];

    for (planet_slider_i_t i = PLANET_SLIDER_SHIP; i < PLANET_SLIDER_NUM; ++i) {
        ui_draw_filled_rect(227, 81 + 11 * i, 244, 90 + 11 * i, p->slider_lock[i] ? 0x22 : 0);
    }

    lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.yourplnt, UI_SCREEN_W);
    lbxfont_select(2, 0xd, 0xe, 0);
    sprintf(buf, "%i \x02(%i)\x01", p->prod_after_maint, p->total_prod);
    lbxfont_print_str_right(x, 72, buf, UI_SCREEN_W);
    lbxfont_select(2, 0xd, 0, 0);
    lbxfont_print_num_right(265, 61, p->pop, UI_SCREEN_W);
    lbxfont_print_num_right(x, 61, p->missile_bases, UI_SCREEN_W);

    for (planet_slider_i_t i = PLANET_SLIDER_SHIP; i < PLANET_SLIDER_NUM; ++i) {
        ui_draw_filled_rect(253, 84 + 11 * i, 278, 84 + 11 * i + 3, 0x2f);
        if (p->slider[i] != 0) {
            ui_draw_slider(253, 84 + 11 * i + 1, 252 + p->slider[i] / 4, p->slider_lock[i] ? 0x22 : 0x73);
        }
    }

    lbxfont_select(2, 0xa, 0, 0);
    if (p->buildship == BUILDSHIP_STARGATE) {
        ui_draw_filled_rect(229, 141, 274, 166, 0);
        lbxgfx_draw_frame(229, 141, ui_data.gfx.starmap.stargate, UI_SCREEN_W);
        lbxfont_print_str_center(251, 169, game_str_sm_stargate, UI_SCREEN_W);
    } else {
        const shipdesign_t *sd = &g->srd[d->api].design[p->buildship];
        uint8_t *gfx = ui_data.gfx.ships[sd->look];
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame(236, 142, gfx, UI_SCREEN_W);
        lbxfont_print_str_center(252, 169, sd->name, UI_SCREEN_W);
    }

    lbxfont_select(2, 6, 0, 0);
    {
        const shipdesign_t *sd;
        struct planet_prod_s prod;
        int cost;
        game_planet_get_ship_prod(p, &prod, false);
        if (p->buildship == BUILDSHIP_STARGATE) {
            cost = game_num_stargate_cost;
        } else {
            sd = &g->srd[d->api].design[p->buildship];
            cost = sd->cost;
        }
        if ((prod.vtotal < cost) || (p->buildship == BUILDSHIP_STARGATE)) {
            if (prod.vthis < 1) {
                lbxfont_print_str_right(x, 83, game_str_sm_prodnone, UI_SCREEN_W);
                lbxfont_select(0, 0xd, 0, 0);
                lbxfont_print_str_right(271, 160, "0", UI_SCREEN_W);
            } else {
                int num = 0, over;
                over = cost - p->bc_to_ship;
                while (over > 0) {
                    over -= prod.vthis;
                    ++num;
                }
                SETMAX(num, 1);
                sprintf(buf, "%i %s", num, game_str_sm_prod_y);
                lbxfont_print_str_right(x, 83, buf, UI_SCREEN_W);
                lbxfont_select(0, 0xd, 0, 0);
                if (p->buildship != BUILDSHIP_STARGATE) {
                    lbxfont_print_str_right(271, 160, "1", UI_SCREEN_W);
                }
            }
        } else {
            int num;
            /* TODO this adjusted sd->cost directly! */
            SETMAX(cost, 1);
            num = prod.vtotal / cost;
            sprintf(buf, "1 %s", game_str_sm_prod_y);
            lbxfont_print_str_right(x, 83, buf, UI_SCREEN_W);
            lbxfont_select(0, 0xd, 0, 0);
            lbxfont_print_num_right(271, 160, num, UI_SCREEN_W);
        }
    }
    lbxfont_select(2, 6, 0, 0);
    {
        struct planet_prod_s prod;
        int cost, v8, va;
        cost = game_get_base_cost(g, d->api);
        game_planet_get_def_prod(p, &prod);
        v8 = p->bc_upgrade_base;
        if (prod.vthis == 0) {
            lbxfont_print_str_right(x, 94, game_str_sm_prodnone, UI_SCREEN_W);
        } else if (prod.vtotal <= v8) {
            lbxfont_print_str_right(x, 94, game_str_sm_defupg, UI_SCREEN_W);
        } else {
            prod.vtotal -= v8;
            SETMAX(prod.vtotal, 0);
            va = g->eto[d->api].planet_shield_cost - p->bc_to_shield;
            if (p->battlebg == 0) {
                va = 0;
            }
            SETMAX(va, 0);
            if (prod.vtotal <= va) {
                lbxfont_print_str_right(x, 94, game_str_sm_defshld, UI_SCREEN_W);
            } else {
                int num, over;
                prod.vtotal -= va;
                SETMAX(prod.vtotal, 0);
                if ((cost * 2) > prod.vtotal) {
                    num = 1;
                    over = cost - prod.vtotal;
                    while (over > 0) {
                        over -= prod.vthis;
                        ++num;
                    }
                    sprintf(buf, "%i %s", num, game_str_sm_prod_y);
                } else {
                    num = prod.vtotal / cost;
                    sprintf(buf, "%i/%s", num, game_str_sm_prod_y);
                }
                lbxfont_print_str_right(x, 94, buf, UI_SCREEN_W);
            }
        }
    }
    {
        const char *str = game_str_sm_prodnone;
        int vthis, cost;
        cost = g->eto[d->api].factory_adj_cost;
        vthis = game_adjust_prod_by_special((p->prod_after_maint * p->slider[PLANET_SLIDER_IND]) / 100, p->special);
        if (vthis != 0) {
            int v20;
            v20 = (vthis * 10) / cost;
            if ((v20 / 10 + p->factories) > ((p->pop - p->trans_num) * p->pop_oper_fact)) {
                if (p->pop_oper_fact < g->eto[d->api].colonist_oper_factories) {
                    if (g->eto[d->api].race != RACE_MEKLAR) {
                        str = game_str_sm_refit;
                    } else {
                        int pos = tenths_2str(buf, v20);
                        sprintf(&buf[pos], "/%s", game_str_sm_prod_y);
                        str = buf;
                    }
                } else {
                    str = game_str_sm_indres;
                    if (p->factories < (p->max_pop3 * g->eto[d->api].colonist_oper_factories)) {
                        str = game_str_sm_indmax;
                    }
                }
            } else {
                int pos = tenths_2str(buf, v20);
                sprintf(&buf[pos], "/%s", game_str_sm_prod_y);
                str = buf;
            }
        }
        lbxfont_print_str_right(x, 105, str, UI_SCREEN_W);
    }
    {
        const char *str = NULL;
        int vthis, factoper, waste, adjwaste;
        bool flag_tform = false, flag_ecoproj = false;
        vthis = (p->prod_after_maint * p->slider[PLANET_SLIDER_ECO]) / 100;
        factoper = (p->pop - p->trans_num) * g->eto[d->api].colonist_oper_factories;
        SETMIN(factoper, p->factories);
        waste = (factoper * g->eto[d->api].ind_waste_scale) / 10;
        if (g->eto[d->api].race == RACE_SILICOID) {
            adjwaste = 0;
        } else {
            adjwaste = (waste + p->waste) / g->eto[d->api].have_eco_restoration_n;
        }
        if ((vthis < adjwaste) || (vthis == 0)) {
            str = (vthis < adjwaste) ? game_str_sm_ecowaste : game_str_sm_prodnone;
        } else {
            if (g->eto[d->api].race != RACE_SILICOID) {
                vthis -= adjwaste;
                SETMAX(vthis, 0);
                if ((vthis > 0) && (g->eto[d->api].have_atmos_terra) && (p->growth == PLANET_GROWTH_HOSTILE)) {
                    vthis -= game_num_atmos_cost - p->bc_to_ecoproj;
                    if (vthis < 0) {
                        flag_ecoproj = true;
                        str = game_str_sm_ecoatmos;
                    }
                }
                if ((vthis > 0) && (g->eto[d->api].have_soil_enrich) && (p->growth == PLANET_GROWTH_NORMAL)) {
                    vthis -= game_num_soil_cost - p->bc_to_ecoproj;
                    if (vthis < 0) {
                        flag_ecoproj = true;
                        str = game_str_sm_ecosoil;
                    }
                }
                if ((vthis > 0) && (g->eto[d->api].have_adv_soil_enrich) && ((p->growth == PLANET_GROWTH_NORMAL) || (p->growth == PLANET_GROWTH_FERTILE))) {
                    vthis -= game_num_adv_soil_cost - p->bc_to_ecoproj;
                    if (vthis < 0) {
                        flag_ecoproj = true;
                        str = game_str_sm_ecogaia;
                    }
                }
            }
            flag_tform = false;
            if (vthis > 0) {
                if (p->max_pop3 < game_num_max_pop) {
                    adjwaste = (p->max_pop2 + (g->eto[d->api].have_terraform_n - p->max_pop3)) * g->eto[d->api].terraform_cost_per_inc;
                } else {
                    adjwaste = 0;
                }
                if (vthis < adjwaste) {
                    str = game_str_sm_ecotform;
                } else {
                    int growth, growth2;
                    if (adjwaste > 0) {
                        flag_tform = true;
                    }
                    vthis -= adjwaste;
                    growth = game_get_pop_growth_max(g, g->planet_focus_i[d->api], p->max_pop3) + p->pop_tenths;
                    if (((p->pop - p->trans_num) + (growth / 10)) > p->max_pop3) {
                        growth = (p->max_pop3 - (p->pop - p->trans_num)) * 10;
                    }
                    growth2 = game_get_pop_growth_for_eco(g, g->planet_focus_i[d->api], vthis) + growth;
                    if (((p->pop - p->trans_num) + (growth2 / 10)) > p->max_pop3) {
                        growth2 = (p->max_pop3 - (p->pop - p->trans_num)) * 10;
                    }
                    growth = growth2 / 10 - growth / 10;
                    if (growth <= 0) {
                        str = flag_tform ? game_str_sm_ecotform : game_str_sm_ecoclean;
                    } else {
                        sprintf(buf, "+%i", growth);
                        lbxfont_print_str_right(297, 116, buf, UI_SCREEN_W);
                        str = game_str_sm_ecopop;
                    }
                }
            } else {
                if ((flag_ecoproj == false) && (g->eto[d->api].race != RACE_SILICOID)) {
                    str = game_str_sm_ecoclean;
                }
            }
        }
        if (str) {
            lbxfont_print_str_right(x, 116, str, UI_SCREEN_W);
        }
    }
    {
        int v = game_get_tech_prod(p->prod_after_maint, p->slider[PLANET_SLIDER_TECH], g->eto[d->api].race, p->special);
        sprintf(buf, "%i", v);
        if (v > 9999) {
            ui_draw_filled_rect(288, 127, 312, 132, 7);
        } else {
            x -= 9;
        }
        lbxfont_print_str_right(x, 127, buf, UI_SCREEN_W);
    }
}

static void ui_starmap_draw_textbox_finished(const struct game_s *g, player_id_t api, int pi)
{
    const planet_t *p = &g->planet[pi];
    char *buf = ui_data.strbuf;
    int num;
    planet_finished_t i;
    for (i = 0; i < FINISHED_NUM; ++i) {
        if (BOOLVEC_IS1(p->finished, i)) {
            break;
        }
    }
    switch (i) {
        case FINISHED_FACT:
            num = p->max_pop3 * g->eto[p->owner].colonist_oper_factories;
            sprintf(buf, "%s %s %s %i %s. %s", p->name, game_str_sm_hasreached, game_str_sm_indmaxof, num, game_str_sm_factories, game_str_sm_extrares);
            break;
        case FINISHED_POPMAX:
            num = p->max_pop3;
            sprintf(buf, "%s %s %s %i %s. %s", p->name, game_str_sm_hasreached, game_str_sm_popmaxof, num, game_str_sm_colonists, game_str_sm_extrares);
            break;
        case FINISHED_SOILATMOS:
            sprintf(buf, "%s %s %s %s %s%s.", p->name, game_str_sm_hasterraf, game_str_tbl_sm_terraf[p->growth - 1], game_str_sm_envwith, game_str_tbl_sm_envmore[p->growth - 1], game_str_sm_stdgrow);
            break;
        case FINISHED_STARGATE:
            sprintf(buf, "%s %s.", p->name, game_str_sm_hasfsgate);
            break;
        case FINISHED_SHIELD:
            sprintf(buf, "%s %s %s %s.", p->name, game_str_sm_hasfshield, game_str_tbl_roman[p->shield], game_str_sm_planshield);
            break;
        default:
            break;
    }
    ui_draw_textbox_2str("", buf, 54);
    ui_draw_textbox_2str("", game_str_sm_planratio, 110);
}

/* -------------------------------------------------------------------------- */

void ui_starmap_draw_basic(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    const planet_t *p = &g->planet[g->planet_focus_i[d->api]];

    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    ui_draw_filled_rect(224, 5, 314, 178, 0);
    if (BOOLVEC_IS0(p->explored, d->api)) {
        lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.unexplor, UI_SCREEN_W);
        lbxfont_select_set_12_4(5, 1, 0, 0);
        lbxfont_print_str_split(232, 74, 76, game_str_tbl_sm_stinfo[p->star_type], 2, UI_SCREEN_W, UI_SCREEN_H);
        ui_starmap_draw_range_parsec(d, 165);
    } else {
        player_id_t owner = p->owner;
        int pi = g->planet_focus_i[d->api];
        if (BOOLVEC_IS0(p->within_srange, d->api) && ((owner == PLAYER_NONE) || BOOLVEC_IS0(g->eto[d->api].within_frange, owner))) {
            owner = g->seen[d->api][pi].owner;
        }
        if (owner == PLAYER_NONE) {
            lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.no_colny, UI_SCREEN_W);
            ui_data.gfx.colonies.current = ui_data.gfx.colonies.d[p->type * 2];
            lbxgfx_draw_frame(227, 73, ui_data.gfx.colonies.current, UI_SCREEN_W);
            ui_draw_box1(227, 73, 310, 174, 0, 0);
            ui_starmap_draw_range_parsec(d, 80);
        } else if ((owner != d->api) || (p->unrest == PLANET_UNREST_REBELLION)) {
            char buf[64];
            int pop, bases, range_y;
            lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.en_colny, UI_SCREEN_W);
            ui_data.gfx.colonies.current = ui_data.gfx.colonies.d[p->type * 2 + 1];
            lbxgfx_draw_frame(227, 73, ui_data.gfx.colonies.current, UI_SCREEN_W);
            ui_draw_box1(227, 73, 310, 174, 0, 0);
            sprintf(buf, "%s %s", game_str_tbl_race[g->eto[owner].race], game_str_sm_colony);
            if (BOOLVEC_IS1(p->within_srange, d->api)) {
                lbxfont_select_set_12_4(5, tbl_banner_fontparam[g->eto[owner].banner], 0, 0);
                lbxfont_print_str_center(270, 84, buf, UI_SCREEN_W);
                pop = p->pop;
                bases = p->missile_bases;
                range_y = 95;
            } else {
                lbxfont_select_set_12_4(0, tbl_banner_fontparam[g->eto[owner].banner], 0, 0);
                lbxfont_print_str_center(269, 75, game_str_sm_lastrep, UI_SCREEN_W);
                lbxfont_print_str_center(268, 83, buf, UI_SCREEN_W);    /* TODO combine with above */
                pop = g->seen[d->api][pi].pop;
                bases = g->seen[d->api][pi].bases;
                range_y = 92;
            }
            lbxfont_select(0, 0xd, 0, 0);
            lbxfont_print_num_right(265, 61, pop, UI_SCREEN_W);
            lbxfont_print_num_right(310, 61, bases, UI_SCREEN_W);
            ui_starmap_draw_range_parsec(d, range_y);
        } else {
            ui_starmap_draw_sliders_and_prod(d);
            lbxgfx_set_frame_0(ui_data.gfx.starmap.col_butt_ship);
            lbxgfx_set_frame_0(ui_data.gfx.starmap.col_butt_reloc);
            if (p->buildship == BUILDSHIP_STARGATE) {
                lbxgfx_set_frame(ui_data.gfx.starmap.col_butt_reloc, 1);
            }
            if ((g->evn.have_plague == 0) || (g->evn.plague_planet_i != g->planet_focus_i[d->api])) {
                lbxgfx_set_frame_0(ui_data.gfx.starmap.col_butt_trans);
            } else {
                lbxgfx_set_frame(ui_data.gfx.starmap.col_butt_trans, 1);
            }
            lbxgfx_draw_frame(282, 140, ui_data.gfx.starmap.col_butt_ship, UI_SCREEN_W);
            lbxgfx_draw_frame(282, 152, ui_data.gfx.starmap.col_butt_reloc, UI_SCREEN_W);
            lbxgfx_draw_frame(282, 164, ui_data.gfx.starmap.col_butt_trans, UI_SCREEN_W);
        }
    }
    ui_starmap_draw_planetinfo(g, d->api, g->planet_focus_i[d->api]);
    if (g->evn.build_finished_num[d->api]) {
        ui_starmap_draw_textbox_finished(g, d->api, g->planet_focus_i[d->api]);
    }
}

void ui_starmap_draw_starmap(struct starmap_data_s *d)
{
    struct game_s *g = d->g;
    int x, y, tx, ty;
    {
        int v;
        v = ui_data.starmap.x2;
        x = ui_data.starmap.x;
        if (v != x) {
            if (v < x) {
                x -= STARMAP_SCROLLSTEP;
                SETMAX(x, v);
            } else {
                x += STARMAP_SCROLLSTEP;
                SETMIN(x, v);
            }
            ui_data.starmap.x = x;
        }
        v = ui_data.starmap.y2;
        y = ui_data.starmap.y;
        if (v != y) {
            if (v < y) {
                y -= STARMAP_SCROLLSTEP;
                SETMAX(y, v);
            } else {
                y += STARMAP_SCROLLSTEP;
                SETMIN(y, v);
            }
            ui_data.starmap.y = y;
        }
    }
    ui_draw_filled_rect(STARMAP_LIMITS, 0);
    lbxgfx_draw_frame(0, 0, ui_data.gfx.starmap.mainview, UI_SCREEN_W);
    uiobj_set_limits(STARMAP_LIMITS);
    {
        int x0, y0, x1, y1;
        x0 = (-x / 4) + 6;
        y0 = (-y / 4) + 6;
        x1 = ((-x + 1) / 2) + 6;
        y1 = ((-y + 1) / 2) + 6;
        lbxgfx_draw_frame_offs(x0, y0, ui_data.gfx.starmap.starback, UI_SCREEN_W);
        lbxgfx_draw_frame_offs(x0 + 320, y0, ui_data.gfx.starmap.starback, UI_SCREEN_W);
        lbxgfx_draw_frame_offs(x0, y0 + 200, ui_data.gfx.starmap.starback, UI_SCREEN_W);
        lbxgfx_draw_frame_offs(x0 + 320, y0 + 200, ui_data.gfx.starmap.starback, UI_SCREEN_W);
        lbxgfx_draw_frame_offs(x1, y1, ui_data.gfx.starmap.starbak2, UI_SCREEN_W);
        lbxgfx_draw_frame_offs(x1 + 320, y1, ui_data.gfx.starmap.starbak2, UI_SCREEN_W);
        lbxgfx_draw_frame_offs(x1, y1 + 200, ui_data.gfx.starmap.starbak2, UI_SCREEN_W);
        lbxgfx_draw_frame_offs(x1 + 320, y1 + 200, ui_data.gfx.starmap.starbak2, UI_SCREEN_W);
    }
    for (int i = 0; i < g->nebula_num; ++i) {
        int tx, ty;
        tx = (g->nebula_x[i] - x) * 2 + 7;
        ty = (g->nebula_y[i] - y) * 2 + 7;
        lbxgfx_draw_frame_offs(tx, ty, ui_data.gfx.starmap.nebula[i], UI_SCREEN_W);
    }
    if (ui_data.starmap.flag_show_grid) {
        int x0, y0, x1, y1;
        for (y0 = 10; y0 < (g->galaxy_maxy * 2); y0 += 50) {
            int ty;
            x0 = (-x) * 2 + 6;
            x1 = (g->galaxy_maxx - x) * 2 + 6;
            ty = (y0 - y) * 2 + 6;
            ui_draw_line_limit(x0, ty, x1, ty, 4);
        }
        for (x0 = 10; x0 < (g->galaxy_maxx * 2); x0 += 50) {
            int tx;
            y0 = (-y) * 2 + 6;
            y1 = (g->galaxy_maxy - y) * 2 + 6;
            tx = (x0 - x) * 2 + 6;
            ui_draw_line_limit(tx, y0, tx, y1, 4);
        }
    }
    for (int pi = 0; pi < g->galaxy_stars; ++pi) {
        const planet_t *p = &g->planet[pi];
        if ((p->owner == d->api) && (p->reloc != pi)) {
            const planet_t *p2 = &g->planet[p->reloc];
            int x0, y0, x1, y1;
            x0 = (p->x - x) * 2 + 14;
            x1 = (p2->x - x) * 2 + 14;
            y0 = (p->y - y) * 2 + 14;
            y1 = (p2->y - y) * 2 + 14;
            ui_draw_line_limit_ctbl(x0, y0, x1, y1, colortbl_line_reloc, 5, ui_data.starmap.line_anim_phase);
        }
    }
    for (int pi = 0; pi < g->galaxy_stars; ++pi) {
        planet_t *p = &g->planet[pi];
        uint8_t *gfx = ui_data.gfx.starmap.stars[p->star_type + p->look];
        lbxgfx_set_new_frame(gfx, (p->frame < 4) ? p->frame : 0);
        gfx_aux_draw_frame_to(gfx, &ui_data.starmap.star_aux);
        if (p->look > 0) {
            tx = (p->x - x) * 2 + 8;
            ty = (p->y - y) * 2 + 9;
        } else {
            tx = (p->x - x) * 2 + 11;
            ty = (p->y - y) * 2 + 11;
        }
        gfx_aux_draw_frame_from_limit(tx, ty, &ui_data.starmap.star_aux, UI_SCREEN_W);
        if (p->frame == 4) {
            p->frame = rnd_0_nm1(50, &g->seed);
        } else {
            p->frame = (p->frame + 1) % 50;
        }
        if (p->owner != PLAYER_NONE) {
            bool do_print;
            do_print = BOOLVEC_IS1(p->within_srange, d->api);
            if (do_print || (pi == g->evn.planet_orion_i)) {
                do_print = true;
                lbxfont_select(2, tbl_banner_fontparam[g->eto[p->owner].banner], 0, 0);
                tx = (p->x - x) * 2 + 14;
                ty = (p->y - y) * 2 + 22;
                lbxfont_print_str_center_limit(tx, ty, p->name, UI_SCREEN_W);
            } else if (BOOLVEC_IS1(g->eto[d->api].within_frange, p->owner) || (p->within_frange[d->api] == 1)) {
                do_print = true;
                lbxfont_select(2, 0, 0, 0);
                lbxfont_set_color0(tbl_banner_color[g->eto[p->owner].banner]);
            }
            if (do_print) {
                tx = (p->x - x) * 2 + 14;
                ty = (p->y - y) * 2 + 22;
                lbxfont_print_str_center_limit(tx, ty, p->name, UI_SCREEN_W);
            }
        }
    }
    if (1
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_RELOC)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_TRANS)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_ORBIT_OWN_SEL)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_TRANSPORT_SEL)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_ENROUTE_SEL)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_ORBIT_EN_SEL)
    ) {
        const planet_t *p = &g->planet[g->planet_focus_i[d->api]];
        tx = (p->x - x) * 2 + 8;
        ty = (p->y - y) * 2 + 8;
        lbxgfx_draw_frame_offs(tx, ty, ui_data.gfx.starmap.planbord, UI_SCREEN_W);
    }
    if (--ui_data.starmap.line_anim_phase < 0) {
        ui_data.starmap.line_anim_phase = 4;
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &g->enroute[i];
        if (BOOLVEC_IS1(r->visible, d->api)) {
            uint8_t *gfx = ui_data.gfx.starmap.smalship[g->eto[r->owner].banner];
            const planet_t *p = &g->planet[r->dest];
            tx = (r->x - x) * 2 + 8;
            ty = (r->y - y) * 2 + 8;
            if (p->x < r->x) {
                lbxgfx_set_new_frame(gfx, 1);
            } else {
                lbxgfx_set_frame_0(gfx);
            }
            p = &g->planet[g->planet_focus_i[d->api]];
            if (g->eto[d->api].have_ia_scanner && (p->owner == d->api) && (r->owner != d->api) && (r->dest == g->planet_focus_i[d->api])) {
                ui_draw_line_limit_ctbl(tx + 5, ty + 2, (p->x - x) * 2 + 14, (p->y - y) * 2 + 14, colortbl_line_red, 5, ui_data.starmap.line_anim_phase);
            }
            lbxgfx_draw_frame_offs(tx, ty, gfx, UI_SCREEN_W);
        }
    }
    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &g->transport[i];
        if (BOOLVEC_IS1(r->visible, d->api)) {
            uint8_t *gfx = ui_data.gfx.starmap.smaltran[g->eto[r->owner].banner];
            const planet_t *p = &g->planet[r->dest];
            tx = (r->x - x) * 2 + 8;
            ty = (r->y - y) * 2 + 8;
            if (p->x < r->x) {
                lbxgfx_set_new_frame(gfx, 1);
            } else {
                lbxgfx_set_frame_0(gfx);
            }
            p = &g->planet[g->planet_focus_i[d->api]];
            if (g->eto[d->api].have_ia_scanner && (p->owner == d->api) && (r->owner != d->api) && (r->dest == g->planet_focus_i[d->api])) {
                ui_draw_line_limit_ctbl(tx + 5, ty + 2, (p->x - x) * 2 + 14, (p->y - y) * 2 + 14, colortbl_line_red, 5, ui_data.starmap.line_anim_phase);
            }
            lbxgfx_draw_frame_offs(tx, ty, gfx, UI_SCREEN_W);
        }
    }
    if (g->evn.crystal.exists && (g->evn.crystal.killer == PLAYER_NONE)) {
        tx = (g->evn.crystal.x - x) * 2 + 8;
        ty = (g->evn.crystal.y - y) * 2 + 8;
        lbxgfx_draw_frame_offs(tx, ty, ui_data.gfx.planets.smonster, UI_SCREEN_W);
        lbxfont_select(2, 8, 0, 0);
        lbxfont_print_str_center_limit(tx + 2, ty + 5, game_str_sm_crystal, UI_SCREEN_W);
    }
    if ((g->evn.amoeba.exists != 0) && (g->evn.amoeba.killer == PLAYER_NONE)) {
        tx = (g->evn.amoeba.x - x) * 2 + 8;
        ty = (g->evn.amoeba.y - y) * 2 + 8;
        lbxgfx_draw_frame_offs(tx, ty, ui_data.gfx.planets.smonster, UI_SCREEN_W);
        lbxfont_select(2, 8, 0, 0);
        lbxfont_print_str_center_limit(tx + 2, ty + 5, game_str_sm_amoeba, UI_SCREEN_W);
    }
    for (int pi = 0; pi < g->galaxy_stars; ++pi) {
        const planet_t *p = &g->planet[pi];
        if (BOOLVEC_IS1(p->within_srange, d->api)) {
            player_id_t tblorbit[PLAYER_NUM];
            player_id_t num;
            num = 0;
            for (player_id_t i = PLAYER_0; i < g->players; ++i) {
                const empiretechorbit_t *e = &g->eto[i];
                for (int j = 0; j < e->shipdesigns_num; ++j) {
                    if (e->orbit[pi].ships[j]) {
                        tblorbit[num++] = i;
                        break;
                    }
                }
            }
            tx = (p->x - x) * 2 + 25;
            if (p->have_stargate) {
                lbxgfx_draw_frame_offs(tx, (p->y - y) * 2 + 7, ui_data.gfx.starmap.stargate2, UI_SCREEN_W);
            }
            ++tx;
            for (player_id_t i = PLAYER_0; i < num; ++i) {
                player_id_t i2 = tblorbit[i];
                uint8_t *gfx = ui_data.gfx.starmap.smalship[g->eto[i2].banner];
                lbxgfx_set_frame_0(gfx);
                ty = (p->y - y) * 2 + i * 6 + 8;
                lbxgfx_draw_frame_offs(tx, ty, gfx, UI_SCREEN_W);
            }
        }
    }
}

void ui_starmap_draw_button_text(struct starmap_data_s *d, bool highlight)
{
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 0)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(10, 184, game_str_sm_game, UI_SCREEN_W);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 1)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(44, 184, game_str_sm_design, UI_SCREEN_W);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 2)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(83, 184, game_str_sm_fleet, UI_SCREEN_W);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 3)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(119, 184, game_str_sm_map, UI_SCREEN_W);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 4)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(147, 184, game_str_sm_races, UI_SCREEN_W);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 5)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(184, 184, game_str_sm_planets, UI_SCREEN_W);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 6)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(230, 184, game_str_sm_tech, UI_SCREEN_W);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 7)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(263, 184, game_str_sm_next_turn, UI_SCREEN_W);
}

void ui_starmap_handle_oi_ctrl(struct starmap_data_s *d, int16_t oi)
{
#define XSTEP   0x1b
#define YSTEP   0x15
    const struct game_s *g = d->g;
    bool changed = false;
    int x, y;
    if (g->evn.build_finished_num[d->api]) {
        return;
    }
    x = ui_data.starmap.x;
    y = ui_data.starmap.y;
    if (oi == d->oi_ctrl_ul) {
        x -= XSTEP;
        y -= YSTEP;
        changed = true;
    } else if ((oi == d->oi_ctrl_up) || (oi == d->oi_ctrl_u2)) {
        y -= YSTEP;
        changed = true;
    } else if (oi == d->oi_ctrl_ur) {
        x += XSTEP;
        y -= YSTEP;
        changed = true;
    } else if ((oi == d->oi_ctrl_left) || (oi == d->oi_ctrl_l2)) {
        x -= XSTEP;
        changed = true;
    } else if ((oi == d->oi_ctrl_right) || (oi == d->oi_ctrl_r2)) {
        x += XSTEP;
        changed = true;
    } else if (oi == d->oi_ctrl_dl) {
        x -= XSTEP;
        y += YSTEP;
        changed = true;
    } else if ((oi == d->oi_ctrl_down) || (oi == d->oi_ctrl_d2)) {
        y += YSTEP;
        changed = true;
    } else if (oi == d->oi_ctrl_dr) {
        x += XSTEP;
        y += YSTEP;
        changed = true;
    }
    if (changed) {
        SETRANGE(x, 0, g->galaxy_maxx - 0x6c);
        SETRANGE(y, 0, g->galaxy_maxy - 0x56);
        ui_data.starmap.x2 = x;
        ui_data.starmap.y2 = y;
    }
#undef XSTEP
#undef YSTEP
}

void ui_starmap_add_oi_bottom_buttons(struct starmap_data_s *d)
{
    d->oi_gameopts = uiobj_add_mousearea(5, 181, 36, 194, MOO_KEY_g, -1);
    d->oi_design = uiobj_add_mousearea(40, 181, 75, 194, MOO_KEY_d, -1);
    d->oi_fleet = uiobj_add_mousearea(79, 181, 111, 194, MOO_KEY_f, -1);
    d->oi_map = uiobj_add_mousearea(115, 181, 139, 194, MOO_KEY_m, -1);
    d->oi_races = uiobj_add_mousearea(143, 181, 176, 194, MOO_KEY_r, -1);
    d->oi_planets = uiobj_add_mousearea(180, 181, 221, 194, MOO_KEY_p, -1);
    d->oi_tech = uiobj_add_mousearea(225, 181, 254, 194, MOO_KEY_t, -1);
    d->oi_next_turn = uiobj_add_mousearea(258, 181, 314, 194, MOO_KEY_n, -1);
}

void ui_starmap_fill_oi_tbls(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    int x = ui_data.starmap.x;
    int y = ui_data.starmap.y;
    uiobj_set_limits(STARMAP_LIMITS);
    UIOBJI_SET_TBL_INVALID(d->oi_tbl_enroute);
    UIOBJI_SET_TBL_INVALID(d->oi_tbl_transport);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        for (int j = 0; j < g->players; ++j) {
            d->oi_tbl_pl_stars[j][i] = UIOBJI_INVALID;
        }
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if (BOOLVEC_IS1(p->within_srange, d->api)) {
            int numorbits, x0, y0;
            player_id_t tblpl[PLAYER_NUM];
            numorbits = 0;
            for (int j = 0; j < g->players; ++j) {
                const fleet_orbit_t *r = &(g->eto[j].orbit[i]);
                for (int k = 0; k < g->eto[j].shipdesigns_num; ++k) {
                    if (r->ships[k]) {
                        tblpl[numorbits++] = j;
                        break;
                    }
                }
            }
            x0 = (p->x - x) * 2 + 26;
            y0 = (p->y - y) * 2 + 8;
            for (int j = 0; j < numorbits; ++j, y0 += 6) {
                d->oi_tbl_pl_stars[tblpl[j]][i] = uiobj_add_mousearea_limited(x0, y0, x0 + 8, y0 + 4, MOO_KEY_UNKNOWN, -1);
            }
        }
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        if (BOOLVEC_IS1(r->visible, d->api)) {
            int x0 = (r->x - x) * 2 + 8;
            int y0 = (r->y - y) * 2 + 8;
            d->oi_tbl_enroute[i] = uiobj_add_mousearea_limited(x0, y0, x0 + 8, y0 + 4, MOO_KEY_UNKNOWN, -1);
        }
    }
    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &(g->transport[i]);
        if (BOOLVEC_IS1(r->visible, d->api)) {
            int x0 = (r->x - x) * 2 + 8;
            int y0 = (r->y - y) * 2 + 8;
            d->oi_tbl_transport[i] = uiobj_add_mousearea_limited(x0, y0, x0 + 8, y0 + 4, MOO_KEY_UNKNOWN, -1);
        }
    }
}

void ui_starmap_fill_oi_tbl_stars(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    int x = ui_data.starmap.x;
    int y = ui_data.starmap.y;
    uiobj_set_limits(STARMAP_LIMITS);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        int x0, y0;
        x0 = (p->x - x) * 2 + 8;
        y0 = (p->y - y) * 2 + 8;
        d->oi_tbl_stars[i] = uiobj_add_mousearea_limited(x0, y0, x0 + 13, y0 + 13, MOO_KEY_UNKNOWN, -1);
    }
}

void ui_starmap_fill_oi_tbl_stars_own(struct starmap_data_s *d, player_id_t owner)
{
    const struct game_s *g = d->g;
    int x = ui_data.starmap.x;
    int y = ui_data.starmap.y;
    uiobj_set_limits(STARMAP_LIMITS);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if (p->owner == owner) {
            int x0, y0;
            x0 = (p->x - x) * 2 + 8;
            y0 = (p->y - y) * 2 + 8;
            d->oi_tbl_stars[i] = uiobj_add_mousearea_limited(x0, y0, x0 + 13, y0 + 13, MOO_KEY_UNKNOWN, -1);
        }
    }
}

void ui_starmap_clear_oi_ctrl(struct starmap_data_s *d)
{
    d->oi_ctrl_left = UIOBJI_INVALID;
    d->oi_ctrl_l2 = UIOBJI_INVALID;
    d->oi_ctrl_right = UIOBJI_INVALID;
    d->oi_ctrl_r2 = UIOBJI_INVALID;
    d->oi_ctrl_ul = UIOBJI_INVALID;
    d->oi_ctrl_ur = UIOBJI_INVALID;
    d->oi_ctrl_up = UIOBJI_INVALID;
    d->oi_ctrl_u2 = UIOBJI_INVALID;
    d->oi_ctrl_dl = UIOBJI_INVALID;
    d->oi_ctrl_down = UIOBJI_INVALID;
    d->oi_ctrl_d2 = UIOBJI_INVALID;
    d->oi_ctrl_dr = UIOBJI_INVALID;
}

void ui_starmap_fill_oi_ctrl(struct starmap_data_s *d)
{
    d->oi_ctrl_left = uiobj_add_inputkey(MOO_KEY_LEFT | MOO_MOD_CTRL);
    if (MOO_KEY_LEFT != MOO_KEY_KP4) {
        d->oi_ctrl_l2 = uiobj_add_inputkey(MOO_KEY_KP4 | MOO_MOD_CTRL);
    }
    d->oi_ctrl_right = uiobj_add_inputkey(MOO_KEY_RIGHT | MOO_MOD_CTRL);
    if (MOO_KEY_RIGHT != MOO_KEY_KP6) {
        d->oi_ctrl_r2 = uiobj_add_inputkey(MOO_KEY_KP6 | MOO_MOD_CTRL);
    }
    d->oi_ctrl_ul = uiobj_add_inputkey(MOO_KEY_KP7 | MOO_MOD_CTRL);
    d->oi_ctrl_ur = uiobj_add_inputkey(MOO_KEY_KP9 | MOO_MOD_CTRL);
    d->oi_ctrl_up = uiobj_add_inputkey(MOO_KEY_UP | MOO_MOD_CTRL);
    if (MOO_KEY_UP != MOO_KEY_KP8) {
        d->oi_ctrl_u2 = uiobj_add_inputkey(MOO_KEY_KP8 | MOO_MOD_CTRL);
    }
    d->oi_ctrl_dl = uiobj_add_inputkey(MOO_KEY_KP1 | MOO_MOD_CTRL);
    d->oi_ctrl_down = uiobj_add_inputkey(MOO_KEY_DOWN | MOO_MOD_CTRL);
    if (MOO_KEY_DOWN != MOO_KEY_KP2) {
        d->oi_ctrl_d2 = uiobj_add_inputkey(MOO_KEY_KP2 | MOO_MOD_CTRL);
    }
    d->oi_ctrl_dr = uiobj_add_inputkey(MOO_KEY_KP3 | MOO_MOD_CTRL);
}

void ui_starmap_sn0_setup(struct shipnon0_s *sn0, int sd_num, const shipcount_t *ships)
{
    sn0->num = 0;
    for (int i = 0; i < sd_num; ++i) {
        shipcount_t n;
        n = ships[i];
        sn0->ships[i] = n;
        if (n) {
            sn0->type[sn0->num++] = i;
        }
    }
}

void ui_starmap_update_reserve_fuel(struct game_s *g, struct shipnon0_s *sn0, const shipcount_t *ships, player_id_t pi)
{
    const bool *hrf = &(g->srd[pi].have_reserve_fuel[0]);
    for (int i = 0; i < sn0->num; ++i) {
        int st;
        st = sn0->type[i];
        if ((!hrf[st]) && (ships[st] != 0)) {
            sn0->have_reserve_fuel = false;
            return;
        }
    }
    sn0->have_reserve_fuel = true;
}

void ui_starmap_draw_planetinfo(const struct game_s *g, player_id_t api, int planet_i)
{
    const planet_t *p = &(g->planet[planet_i]);
    ui_starmap_draw_planetinfo_do(g, api, planet_i, BOOLVEC_IS1(p->explored, api), true);
}

void ui_starmap_draw_planetinfo_2(const struct game_s *g, int p1, int p2, int planet_i)
{
    const planet_t *p = &(g->planet[planet_i]);
    player_id_t api = (p1 < PLAYER_NUM) ? p1 : p2;
    bool explored = true;
    if (0
      || (IS_HUMAN(g, p1) && BOOLVEC_IS0(p->explored, p1))
      || (IS_HUMAN(g, p2) && BOOLVEC_IS0(p->explored, p2))
    ) {
        explored = false;
    }
    ui_starmap_draw_planetinfo_do(g, api, planet_i, explored, false);
}

int ui_starmap_newship_next(const struct game_s *g, player_id_t pi, int i)
{
    int t = i;
    const planet_t *p;
    do {
        i = (i + 1) % g->galaxy_stars;
        p = &(g->planet[i]);
    } while ((!((p->owner == pi) && BOOLVEC_IS1(p->finished, FINISHED_SHIP))) && (i != t));
    return i;
}

int ui_starmap_newship_prev(const struct game_s *g, player_id_t pi, int i)
{
    int t = i;
    const planet_t *p;
    do {
        if (--i < 0) { i = g->galaxy_stars - 1; }
        p = &(g->planet[i]);
    } while ((!((p->owner == pi) && BOOLVEC_IS1(p->finished, FINISHED_SHIP))) && (i != t));
    return i;
}

