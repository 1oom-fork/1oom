#include "config.h"

#include <stdio.h>
#include <stdlib.h> /* abs */
#include <string.h>

#include "game_turn.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_ai.h"
#include "game_audience.h"
#include "game_aux.h"
#include "game_battle.h"
#include "game_design.h"
#include "game_diplo.h"
#include "game_election.h"
#include "game_end.h"
#include "game_event.h"
#include "game_fix.h"
#include "game_fleet.h"
#include "game_ground.h"
#include "game_misc.h"
#include "game_news.h"
#include "game_num.h"
#include "game_shiptech.h"
#include "game_spy.h"
#include "game_stat.h"
#include "game_str.h"
#include "game_tech.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "ui.h"
#include "util.h"
#include "util_math.h"

/* -------------------------------------------------------------------------- */

int copyprot_status = 0;

static void game_turn_limit_ships(struct game_s *g)
{
    for (int ei = 0; ei < g->enroute_num; ++ei) {
        fleet_enroute_t *r = &(g->enroute[ei]);
        for (int si = 0; si < NUM_SHIPDESIGNS; ++si) {
            SETMIN(r->ships[si], game_num_limit_ships);
        }
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            fleet_orbit_t *r = &(g->eto[j].orbit[i]);
            for (int si = 0; si < NUM_SHIPDESIGNS; ++si) {
                SETMIN(r->ships[si], game_num_limit_ships);
            }
        }
    }
    game_remove_empty_fleets(g);
}

static void game_turn_countdown_ceasefire(struct game_s *g)
{
    for (player_id_t j = PLAYER_0; j < g->players; ++j) {
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            if (g->evn.ceasefire[j][i] > 0) {
                --g->evn.ceasefire[j][i];
            }
        }
    }
}

static void game_turn_update_mood_blunder(struct game_s *g)
{
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        for (player_id_t j = PLAYER_1; j < g->players; ++j) {
            if (i != j) {
                int16_t v;
                v = e->diplo_type[j];
                if ((v >= 4) && (v <= 11)) {
                    e->blunder[j] = v;
                } else if ((v >= 42) && (v <= 49)) {
                    e->blunder[j] = v - 30;
                } else if ((v >= 50) && (v <= 57)) {
                    e->blunder[j] = v - 46;
                }
                if (e->treaty[j] != TREATY_FINAL_WAR) {
                    v = e->mood_treaty[j];
                    if (v < 0) {
                        v += rnd_1_n(5, &g->seed);
                    }
                    if (v < 50) {
                        v += rnd_1_n(5, &g->seed);
                    }
                    e->mood_treaty[j] = v;
                    v = e->mood_trade[j];
                    if (v < 0) {
                        v += rnd_1_n(5, &g->seed);
                    }
                    if (v < 50) {
                        v += rnd_1_n(5, &g->seed);
                    }
                    e->mood_trade[j] = v;
                    v = e->mood_tech[j];
                    if (v < 0) {
                        v += rnd_1_n(5, &g->seed);
                    }
                    if (v < 50) {
                        v += rnd_1_n(5, &g->seed);
                    }
                    e->mood_tech[j] = v;
                    v = e->mood_peace[j];
                    if (v < 0) {
                        v += rnd_1_n(5, &g->seed);
                    }
                    if (v < 50) {
                        v += rnd_1_n(5, &g->seed);
                    }
                    SETRANGE(v, -200, 200);
                    e->mood_peace[j] = v;
                }
            }
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            e->diplo_val[j] = 0;
            e->diplo_type[j] = 0;
            if (e->mutual_enemy[j] != PLAYER_NONE) {
                e->hated[j] = PLAYER_NONE;
                e->mutual_enemy[j] = PLAYER_NONE;
            }
            e->au_tech_trade_num[j] = 0;
            e->offer_tech[j] = 0;
            e->au_want_tech[j] = 0;
            e->offer_bc[j] = 0;
        }
    }
}

static void game_turn_init_z_finished(struct game_s *g)
{
    memset(g->evn.spies_caught, 0, sizeof(g->evn.spies_caught));
    memset(g->evn.new_ships, 0, sizeof(g->evn.new_ships));
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        BOOLVEC_CLEAR(p->finished, FINISHED_NUM);
    }
    memset(g->evn.build_finished_num, 0, sizeof(g->evn.build_finished_num));
}

static void game_turn_send_transport(struct game_s *g)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if ((p->owner != PLAYER_NONE) && (p->trans_num > 0)) {
            game_send_transport(g, p);
        }
    }
}

static int game_turn_build_eco_sub1(int fact, int colonist_oper_fact, int pop1, int waste, int max_pop2)
{
    int v;
    v = pop1 * colonist_oper_fact;
    SETMIN(fact, v);
    v = (fact * colonist_oper_fact) / 10;
    return ((100 - ((waste * 100) / max_pop2)) * v) / 100;
}

static inline void game_add_planet_to_eco_finished(struct game_s *g, uint8_t pli, player_id_t owner)
{
    BOOLVEC_SET1(g->planet[pli].finished, FINISHED_SOILATMOS);
}

static void game_turn_build_eco(struct game_s *g)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        player_id_t owner;
        owner = p->owner;
        p->pop_prev = (owner != PLAYER_NONE) ? p->pop : 0;
        if ((owner != PLAYER_NONE) && (p->type != PLANET_TYPE_NOT_HABITABLE) && (p->pop != 0)) {
            const empiretechorbit_t *e;
            int ecorestore, ecoprod;
            e = &(g->eto[owner]);
            {
                int v, fact, waste;
                fact = p->factories;
                v = p->pop * e->colonist_oper_factories;
                SETMIN(fact, v);
                waste = (fact * e->ind_waste_scale) / 10;
                p->waste += ((100 - ((p->waste * 100) / p->max_pop2)) * waste) / 100;
            }
            if (p->unrest == PLANET_UNREST_REBELLION) {
                p->waste = 0;
            }
            ecorestore = (e->race != RACE_SILICOID) ? (p->waste / e->have_eco_restoration_n) : 0;
            p->waste += game_turn_build_eco_sub1(p->factories, e->colonist_oper_factories, p->pop, p->waste, p->max_pop2);
            ecoprod = (p->slider[PLANET_SLIDER_ECO] * p->prod_after_maint) / 100;
            if (e->race != RACE_SILICOID) {
                if (ecorestore > ecoprod) {
                    p->waste -= e->have_eco_restoration_n * ecoprod;
                    ecoprod = 0;
                } else {
                    ecoprod -= ecorestore;
                    p->waste = 0;
                }
            }
            SETMAX(p->waste, 0);
            if ((p->max_pop2 - p->waste) < 10) {
                p->waste = p->max_pop2 - 10;
            }
            SETMAX(p->waste, 0);
            if (e->race != RACE_SILICOID) {
                if ((ecoprod > 0) && e->have_atmos_terra && (p->growth == PLANET_GROWTH_HOSTILE)) {
                    p->bc_to_ecoproj += ecoprod;
                    if (p->bc_to_ecoproj >= game_num_atmos_cost) {
                        int v;
                        if (p->type < PLANET_TYPE_DEAD) {
                            v = 20;
                        } else if (p->type < PLANET_TYPE_BARREN) {
                            v = 10;
                        } else {
                            v = 0;
                        }
                        /* WASBUG max_pop += moved from outside if (bc >= cost) */
                        p->type = PLANET_TYPE_MINIMAL;
                        p->max_pop2 += v;
                        p->max_pop1 += v;
                        p->max_pop3 += v;
                        p->growth = PLANET_GROWTH_NORMAL;
                        ecoprod = p->bc_to_ecoproj - game_num_atmos_cost;
                        p->bc_to_ecoproj -= game_num_atmos_cost;
                        game_add_planet_to_eco_finished(g, i, owner);
                    } else {
                        ecoprod = 0;
                    }
                }
                if ((ecoprod > 0) && e->have_soil_enrich && (p->growth == PLANET_GROWTH_NORMAL)) {
                    p->bc_to_ecoproj += ecoprod;
                    if (p->bc_to_ecoproj > game_num_soil_cost) {
                        int v;
                        p->growth = PLANET_GROWTH_FERTILE;
                        v = (p->max_pop1 / 20) * 5;
                        if ((p->max_pop1 / 20) % 5) {
                            v += 5;
                        }
                        SETMAX(v, 5);
                        p->max_pop2 = p->max_pop1 + v;
                        p->max_pop3 += v;
                        SETMIN(p->max_pop3, (e->have_terraform_n + p->max_pop2));
                        SETMIN(p->max_pop3, game_num_max_pop);
                        ecoprod = p->bc_to_ecoproj - game_num_soil_cost;
                        p->bc_to_ecoproj -= game_num_soil_cost; /* BUG cost was not removed */
                        game_add_planet_to_eco_finished(g, i, owner);
                    } else {
                        ecoprod = 0;
                    }
                }
                if ((ecoprod > 0) && e->have_adv_soil_enrich && (p->growth < PLANET_GROWTH_GAIA) && (p->growth > PLANET_GROWTH_HOSTILE)) {
                    p->bc_to_ecoproj += ecoprod;
                    if (p->bc_to_ecoproj > game_num_adv_soil_cost) {
                        int v;
                        p->growth = PLANET_GROWTH_GAIA;
                        v = (p->max_pop1 / 10) * 5;
                        if ((p->max_pop1 / 10) % 5) {
                            v += 5;
                        }
                        SETMAX(v, 5);
                        p->max_pop2 = p->max_pop1 + v;
                        p->max_pop3 += v;
                        SETMIN(p->max_pop3, (e->have_terraform_n + p->max_pop2));
                        SETMIN(p->max_pop3, game_num_max_pop);
                        ecoprod = p->bc_to_ecoproj - game_num_adv_soil_cost;
                        p->bc_to_ecoproj -= game_num_adv_soil_cost; /* BUG cost was not removed */
                        game_add_planet_to_eco_finished(g, i, owner);
                    } else {
                        ecoprod = 0;
                    }
                }
            }
            if ((ecoprod > 0) && ((p->max_pop3 - p->max_pop2) < e->have_terraform_n) && (p->max_pop3 < game_num_max_pop)) {
                int v, tmax;
                v = ecoprod / e->terraform_cost_per_inc;
                tmax = e->have_terraform_n + p->max_pop2;
                if (tmax < (p->max_pop3 + v)) {
                    ecoprod -= (tmax - p->max_pop3) * e->terraform_cost_per_inc;
                    p->max_pop3 = tmax;
                } else {
                    p->max_pop3 += v;
                    ecoprod = 0;
                }
            }
            SETMIN(p->max_pop3, game_num_max_pop);
            SETMAX(ecoprod, 0);
            {
                int v;
                v = game_get_pop_growth_max(g, i, p->max_pop3) + game_get_pop_growth_for_eco(g, i, ecoprod) + p->pop_tenths;
                p->pop += v / 10;
                p->pop_tenths = v % 10;
            }
            SETRANGE(p->pop, 0, p->max_pop3);
        }
    }
}

static void game_turn_update_trade(struct game_s *g)
{
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        for (player_id_t j = i; j < g->players; ++j) {
            empiretechorbit_t *e2 = &(g->eto[j]);
            uint16_t bc;
            bc = e->trade_bc[j];
            if (bc != 0) {
                if (BOOLVEC_IS0(e->within_frange, j)) {
                    e->trade_bc[j] = 0;
                    e->trade_percent[j] = 0;
                    e->trade_established_bc[j] = 0;
                } else {
                    uint16_t estbc;
                    int16_t v;
                    estbc = e->trade_established_bc[j];
                    if (estbc < bc) {   /* FIXME BUG? never true ; both are set to bc */
                        estbc += bc / 10;
                        SETMIN(estbc, bc);
                        e->trade_established_bc[j] = estbc;
                        e2->trade_established_bc[i] = estbc;
                    }
                    v = (rnd_1_n(200, &g->seed) + e->relation1[j] + 25) / 60;
                    SETMAX(v, 0);
                    ADDSATT(e->trade_percent[j], v, 100);
                    ADDSATT(e2->trade_percent[i], v, 100);
                }
            }
        }
    }
}

static void game_turn_diplo_adjust(struct game_s *g)
{
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        for (player_id_t j = i + 1; j < g->players; ++j) {
            if (!rnd_0_nm1(2, &g->seed)) {
                if (e->treaty[j] == TREATY_NONAGGRESSION) {
                    game_diplo_act(g, rnd_1_n(3, &g->seed), i, j, 0, 0, 0);
                }
                if (e->treaty[j] == TREATY_ALLIANCE) {
                    game_diplo_act(g, rnd_1_n(6, &g->seed), i, j, 0, 0, 0);
                }
                if (e->trade_bc[j] != 0) {
                    game_diplo_act(g, rnd_1_n(3, &g->seed), i, j, 2, 0, 0);
                }
            }
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        int s_ai;
        if (IS_HUMAN(g, i)) {
            continue;
        }
        s_ai = game_stat_fleet(g, i);
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            int s_h;
            if (IS_AI(g, j)) {
                continue;
            }
            s_h = game_stat_fleet(g, j);
            if (1
              && (s_ai < s_h) && (s_ai > 0)
              && (rnd_1_n(100, &g->seed) >= ((s_h * 50) / s_ai))
              && (!rnd_0_nm1(20, &g->seed))
            ) {
                game_diplo_act(g, -10, j, i, 8, 0, 0);
            }
        }
    }
    if ((g->year & 1) == 0) {
        uint8_t tbl_num_pp[PLAYER_NUM];
        int gscale = ((g->galaxy_size + 1) * 3);
        int gdiv = g->galaxy_size + 6 - g->difficulty;
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            tbl_num_pp[i] = 0;
        }
        for (int j = 0; j < g->galaxy_stars; ++j) {
            planet_t *p = &(g->planet[j]);
            if (p->prod_after_maint >= 100) {
                ++tbl_num_pp[p->owner];
            }
        }
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            empiretechorbit_t *e = &(g->eto[i]);
            if ((gscale < tbl_num_pp[i]) && !rnd_0_nm1(4, &g->seed)) {
                int v;
                v = rnd_1_n(4, &g->seed) * ((-(tbl_num_pp[i] - gscale)) / gdiv);
                v = (v / 3) * 4;
                if (IS_HUMAN(g, i)) {
                    v /= 2;
                }
                for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                    if (IS_HUMAN(g, j)) {
                        continue;
                    }
                    if (BOOLVEC_IS1(e->within_frange, j) && (e->treaty[j] == TREATY_ALLIANCE)) {
                        game_diplo_act(g, v, i, j, 12, 0, 0);
                    }
                }
            }
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        if (IS_HUMAN(g, i)) {
            continue;
        }
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            if ((i != j) && (e->diplo_val[j] == 0)) {
                int16_t r1, r2;
                uint8_t v;
                r1 = e->relation1[j];
                r2 = e->relation2[j];
                v = (abs(r1) > rnd_1_n(105, &g->seed)) ? rnd_0_nm1(2, &g->seed) : 0;
                if (r1 < r2) {
                    r1 += v;
                    if (r1 > r2) {
                        r1 = r2;
                    }
                } else {
                    r1 -= v;
                    if (r1 < r2) {
                        r1 = r2;
                    }
                }
                SETRANGE(r1, -100, 100);
                e->relation1[j] = r1;
            }
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            if (i == j) {
                e->relation1[j] = 0;
            } else {
                g->eto[j].relation1[i] = e->relation1[j];
            }
        }
    }
}

static void game_add_planet_to_shield_finished(struct game_s *g, uint8_t pli, player_id_t owner)
{
    BOOLVEC_SET1(g->planet[pli].finished, FINISHED_SHIELD);
}

static void game_turn_build_def(struct game_s *g)
{
    int16_t cost_new[PLAYER_NUM], cost_diff[PLAYER_NUM];
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        int16_t cost_old;
        cost_old = game_get_base_cost(g, i);
        e->base_weapon = game_get_base_weapon(g, i, e->tech.percent[TECH_FIELD_WEAPON]);
        e->base_shield = game_get_best_shield(g, i, e->tech.percent[TECH_FIELD_FORCE_FIELD]);
        e->base_comp = game_get_best_comp(g, i, e->tech.percent[TECH_FIELD_COMPUTER]);
        cost_new[i] = game_get_base_cost(g, i);
        cost_diff[i] = cost_new[i] - cost_old;
        SETMAX(cost_diff[i], 0);
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        player_id_t owner;
        owner = p->owner;
        if (owner != PLAYER_NONE) {
            empiretechorbit_t *e = &(g->eto[owner]);
            struct planet_prod_s prod;
            int toup;
            game_planet_get_def_prod(p, &prod);
            toup = cost_diff[owner] * p->missile_bases + p->bc_upgrade_base;
            if (toup >= prod.vtotal) {
                toup -= prod.vtotal;
                p->bc_upgrade_base = toup;
                p->bc_to_base = 0;
            } else {
                int tosh;
                prod.vtotal -= toup;
                p->bc_upgrade_base = 0;
                tosh = e->planet_shield_cost - p->bc_to_shield;
                SETMAX(tosh, 0);
                if ((p->battlebg == 0) || game_xy_is_in_nebula(g, p->x, p->y)) { /* FIXME later check is redundant */
                    tosh = 0;
                    p->bc_to_shield = 0;
                }
                if (prod.vtotal <= tosh) { /* FIXME just "<" ? */
                    p->bc_to_shield += prod.vtotal;
                    prod.vtotal = 0;
                } else {
                    prod.vtotal -= tosh;
                    p->bc_to_shield = e->planet_shield_cost;
                    while (prod.vtotal >= cost_new[owner]) {
                        prod.vtotal -= cost_new[owner];
                        ++p->missile_bases;
                    }
                }
                p->bc_to_base = prod.vtotal;
            }
            {
                uint8_t newshield;
                int curcost;
                curcost = MIN(e->planet_shield_cost, p->bc_to_shield);
                newshield = 0;
                for (int j = 1; (j < PSHIELD_NUM) && (curcost >= game_num_pshield_cost[j]); ++j) {
                    newshield += 5;
                }
                if (newshield != p->shield) {
                    p->shield = newshield;
                    if (newshield == g->eto[owner].have_planet_shield) {
                        game_add_planet_to_shield_finished(g, i, owner);
                    }
                }
            }
        }
    }
}

static inline void game_add_planet_to_stargate_finished(struct game_s *g, uint8_t pli, player_id_t owner)
{
    BOOLVEC_SET1(g->planet[pli].finished, FINISHED_STARGATE);
}

static void game_turn_build_ship(struct game_s *g)
{
    game_update_maint_costs(g);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        shipresearch_t *srd = &(g->srd[i]);
        struct game_design_s gd;
        gd.sd_num = e->shipdesigns_num;
        gd.player_i = i;
        memcpy(gd.percent, e->tech.percent, sizeof(gd.percent));
        for (int j = 0; j < gd.sd_num; ++j) {
            shipdesign_t *sd = &(srd->design[j]);
            gd.sd = *sd;
            sd->cost = game_design_calc_cost(&gd);
        }
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        player_id_t owner;
        owner = p->owner;
        if (owner != PLAYER_NONE) {
            empiretechorbit_t *e = &(g->eto[owner]);
            shipresearch_t *srd = &(g->srd[owner]);
            struct planet_prod_s prod;
            uint8_t si;
            game_planet_get_ship_prod(p, &prod, true);
            si = p->buildship;
            if (si == BUILDSHIP_STARGATE) {
                if (prod.vtotal >= game_num_stargate_cost) {
                    p->have_stargate = true;
                    p->buildship = 0;
                    p->bc_to_ship = prod.vtotal - game_num_stargate_cost;
                    game_add_planet_to_stargate_finished(g, i, owner);
                } else {
                    p->bc_to_ship = prod.vtotal;
                }
            } else {
                shipdesign_t *sd = &(srd->design[si]);
                int cost, shipnum;
                uint8_t dest;
                cost = sd->cost;
                shipnum = 0;
                while (prod.vtotal >= cost) {
                    ++shipnum;
                    prod.vtotal -= cost;
                }
                if ((shipnum + srd->shipcount[si]) > game_num_limit_ships_all) {
                    shipnum = game_num_limit_ships_all - srd->shipcount[si];
                }
                SETMAX(shipnum, 0);
                srd->shipcount[si] += shipnum;
                dest = p->reloc;
                if (dest == i) {
                    e->orbit[i].ships[si] += shipnum;
                    if (shipnum > 0) {
                        BOOLVEC_SET1(p->finished, FINISHED_SHIP);
                    }
                } else {
                    if (shipnum > 0) {
                        game_send_fleet_reloc(g, owner, i, dest, si, shipnum);
                    }
                }
                g->evn.new_ships[owner][si] += shipnum;
                p->bc_to_ship = prod.vtotal;
            }
        }
    }
    game_update_visibility(g);
}

static void game_turn_reserve(struct game_s *g)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        player_id_t owner;
        owner = p->owner;
        if (owner != PLAYER_NONE) {
            uint32_t v, r;
            v = p->prod_after_maint / 2;
            r = p->reserve;
            if (v < r) {
                r -= v;
            } else {
                r = 0;
            }
            p->reserve = r;
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        e->reserve_bc += (e->total_production_bc * e->tax) / 2000;
    }
}

static inline void game_add_planet_to_build_finished(struct game_s *g, uint8_t pli, player_id_t owner, uint8_t type)
{
    planet_t *p = &(g->planet[pli]);
    BOOLVEC_SET1(p->finished, type);
    ++g->evn.build_finished_num[owner];
}

static void game_turn_build_ind(struct game_s *g)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        player_id_t owner;
        owner = p->owner;
        if (owner != PLAYER_NONE) {
            empiretechorbit_t *e;
            uint16_t fact, factold, num;
            uint8_t cost, bonus;
            struct planet_prod_s prod;
            int v;
            e = &(g->eto[owner]);
            factold = fact = p->factories;
            cost = e->factory_adj_cost;
            game_planet_get_ind_prod(p, &prod);
            bonus = (e->race == RACE_MEKLAR) ? 2 : 0;
            v = e->colonist_oper_factories - p->pop_oper_fact - bonus;
            if (v > 0) {
                v = v * (e->factory_cost / 2);
            } else {
                v = 0;
            }
            if ((prod.vtotal / cost + fact) > (p->pop * p->pop_oper_fact)) {
                p->bc_to_refit += prod.vtotal;
                if (p->bc_to_refit >= v) {
                    p->pop_oper_fact = e->colonist_oper_factories;
                    prod.vtotal = p->bc_to_refit - v;
                    p->bc_to_refit = 0;
                } else {
                    prod.vtotal = 0;
                }
            }
            num = prod.vtotal / cost;
            p->bc_to_factory = prod.vtotal % cost;
            v = p->max_pop3 * e->colonist_oper_factories;
            if (v < (fact + num)) {
                v = (fact + num) - v;
                SETMAX(v, 0);
                SETMIN(v, num);
                num -= v;
            } else {
                v = 0;
            }
            e->reserve_bc += (v * cost) / 2;
            fact += num;
            SETMIN(fact, game_num_max_factories);
            p->factories = fact;
            if ((fact != factold) && (fact >= (p->max_pop3 * e->colonist_oper_factories))) {
                if (IS_HUMAN(g, owner) && (p->slider[PLANET_SLIDER_IND] > 0)) {
                    p->slider[PLANET_SLIDER_TECH] += p->slider[PLANET_SLIDER_IND];
                    p->slider[PLANET_SLIDER_IND] = 0;
                }
                game_add_planet_to_build_finished(g, i, owner, FINISHED_FACT);
            }
        }
    }
}

static void game_turn_move_ships(struct game_s *g)
{
    void *ctx;
    bool local_multiplayer = g->gaux->local_players > 1, move_back = false;
    ctx = ui_gmap_basic_init(g, local_multiplayer);
    if (local_multiplayer) {
        memcpy(g->gaux->move_temp->enroute, g->enroute, g->enroute_num * sizeof(fleet_enroute_t));
        memcpy(g->gaux->move_temp->transport, g->transport, g->transport_num * sizeof(transport_t));
        g->gaux->move_temp->crystal = g->evn.crystal;
        g->gaux->move_temp->amoeba = g->evn.amoeba;
    }
    for (g->active_player = 0; g->active_player < g->players; ++g->active_player) {
        bool flag_more;
        if (BOOLVEC_IS1(g->is_ai, g->active_player)) {
            continue;
        }
        if (move_back) {
            memcpy(g->enroute, g->gaux->move_temp->enroute, g->enroute_num * sizeof(fleet_enroute_t));
            memcpy(g->transport, g->gaux->move_temp->transport, g->transport_num * sizeof(transport_t));
            g->evn.crystal = g->gaux->move_temp->crystal;
            g->evn.amoeba = g->gaux->move_temp->amoeba;
        }
        game_update_visibility(g);
        flag_more = true;
        ui_gmap_basic_start_player(ctx, g->active_player);
        for (int frame = 0; (frame < 20) && flag_more; ++frame) {
            bool odd_frame;
            game_update_visibility(g);
            odd_frame = frame & 1;
            ui_gmap_basic_start_frame(ctx, g->active_player);
            flag_more = false;
            for (int i = 0; i < g->enroute_num; ++i) {
                fleet_enroute_t *r = &(g->enroute[i]);
                if ((r->speed * 2) > frame) {
                    int x, y;
                    x = r->x;
                    y = r->y;
                    if (odd_frame || (!game_xy_is_in_nebula(g, x, y)) || (frame == 0)) {
                        int x1, y1;
                        const planet_t *p;
                        p = &(g->planet[r->dest]);
                        x1 = p->x;
                        y1 = p->y;
                        if (r->speed == FLEET_SPEED_STARGATE) {
                            x = x1;
                            y = y1;
                        } else {
                            flag_more = true;
                            util_math_go_line_dist(&x, &y, x1, y1, odd_frame ? 6 : 5);
                        }
                        r->x = x;
                        r->y = y;
                    }
                }
            }
            for (int i = 0; i < g->transport_num; ++i) {
                transport_t *r = &(g->transport[i]);
                if ((r->speed * 2) > frame) {
                    int x, y;
                    x = r->x;
                    y = r->y;
                    if ((!odd_frame) || (!game_xy_is_in_nebula(g, x, y))) {
                        int x1, y1;
                        const planet_t *p;
                        p = &(g->planet[r->dest]);
                        x1 = p->x;
                        y1 = p->y;
                        if (r->speed == FLEET_SPEED_STARGATE) {
                            x = x1;
                            y = y1;
                        } else {
                            flag_more = true;
                            util_math_go_line_dist(&x, &y, x1, y1, odd_frame ? 6 : 5);
                        }
                        r->x = x;
                        r->y = y;
                    }
                }
            }
            for (int i = 0; i < 2; ++i) {
                monster_t *m;
                m = (i == 0) ? &(g->evn.crystal) : &(g->evn.amoeba);
                if (m->exists && (m->counter <= 0)) {
                    int x, y, x1, y1;
                    const planet_t *p;
                    p = &(g->planet[m->dest]);
                    x1 = p->x;
                    y1 = p->y;
                    x = m->x;
                    y = m->y;
                    if (((x != x1) || (y != y1)) && (frame < 2)) {
                        util_math_go_line_dist(&x, &y, x1, y1, odd_frame ? 6 : 5);
                        m->x = x;
                        m->y = y;
                    }
                }
            }
            ui_gmap_basic_draw_frame(ctx, g->active_player);
            ui_gmap_basic_finish_frame(ctx, g->active_player);
        }
        move_back = local_multiplayer;
    }
    ui_gmap_basic_shutdown(ctx);
    for (int i = 0; i < g->enroute_num; ++i) {
        fleet_enroute_t *r = &(g->enroute[i]);
        const planet_t *p;
        p = &(g->planet[r->dest]);
        if ((r->x == p->x) && (r->y == p->y)) {
            fleet_orbit_t *o;
            o = &(g->eto[r->owner].orbit[r->dest]);
            for (int j = 0; j < NUM_SHIPDESIGNS; ++j) {
                uint32_t s;
                s = o->ships[j] + r->ships[j];
                SETMIN(s, game_num_limit_ships);
                o->ships[j] = s;
            }
            util_table_remove_item_any_order(i, g->enroute, sizeof(fleet_enroute_t), g->enroute_num);
            --g->enroute_num;
            --i;
        }
    }
    game_update_visibility(g);
}

#if 0
static void game_turn_unrest_hmm1(struct game_s *g)
{
    /* this does nothing except churn the rng! */
    int tbl1[PLAYER_NUM], tbl2[PLAYER_NUM];
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        tbl1[i] = 0;
        tbl2[i] = 0;
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        const planet_t *p;
        int pl, v;
        v = tbl2[i] * 2;   /* always 0 */
        pl = game_planet_get_random(g, i);
        if (pl != PLANET_NONE) {
            p = &(g->planet[pi]);
            /* WASBUG unrest != RESOLVED test done even when pl < 0 */
            if ((p->unrest != PLANET_UNREST_REBELLION) || (p->unrest != PLANET_UNREST_RESOLVED)) {
                if (rnd_1_n(100, &g->seed) <= v) {  /* never true */
                    p->unrest = PLANET_UNREST_REBELLION;
                    p->unrest_reported = false;
                    p->rebels = p->pop / 2;
                }
            }
        }
    }
}
#endif

static void game_turn_explore(struct game_s *g, uint8_t *planetptr, player_id_t *playerptr)
{
    for (int pli = 0; pli < g->galaxy_stars; ++pli) {
        planet_t *p = &(g->planet[pli]);
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            if (BOOLVEC_IS0(p->explored, i) || (p->owner == PLAYER_NONE)) {
                empiretechorbit_t *e = &(g->eto[i]);
                bool flag_visible, by_scanner;
                flag_visible = false;
                for (int j = 0; j < e->shipdesigns_num; ++j) {
                    if (e->orbit[pli].ships[j] > 0) {
                        flag_visible = true;
                        break;
                    }
                }
                by_scanner = false;
                if ((!flag_visible) && e->have_adv_scanner) {
                    for (int pli2 = 0; pli2 < g->galaxy_stars; ++pli2) {
                        const planet_t *p2 = &(g->planet[pli2]);
                        if ((p2->owner == i) && (util_math_dist_fast(p->x, p->y, p2->x, p2->y) <= game_num_adv_scan_range)) {
                            flag_visible = true;
                            break;
                        }
                    }
                    by_scanner = true;
                }
                /*c4ca*/
                if (flag_visible) {
                    bool first, flag_colony_ship, was_explored;
                    int best_colonize, best_colonyship = 0;
                    first = BOOLVEC_IS_CLEAR(p->explored, PLAYER_NUM);
                    if ((p->special == PLANET_SPECIAL_ARTIFACTS) && (!by_scanner) && first) {
                        /* FIXME BUG only 1 artifact landing per turn */
                        *planetptr = pli;
                        *playerptr = i;
                    }
                    flag_colony_ship = false;
                    best_colonize = 200;
                    for (int j = 0; j < e->shipdesigns_num; ++j) {
                        const shipdesign_t *sd = &(g->srd[i].design[j]);
                        int can_colonize;
                        can_colonize = 200;
                        for (int k = 0; k < SPECIAL_SLOT_NUM; ++k) {
                            ship_special_t s;
                            s = sd->special[k];
                            if ((s >= SHIP_SPECIAL_STANDARD_COLONY_BASE) && (s <= SHIP_SPECIAL_RADIATED_COLONY_BASE)) {
                                can_colonize = PLANET_TYPE_MINIMAL - (s - SHIP_SPECIAL_STANDARD_COLONY_BASE);
                            }
                        }
                        if ((can_colonize < 200) && (e->orbit[pli].ships[j] > 0)) {
                            flag_colony_ship = true;
                            if (can_colonize < best_colonize) {
                                best_colonize = can_colonize;
                                best_colonyship = j;
                            }
                            if (e->race == RACE_SILICOID) {
                                best_colonize = PLANET_TYPE_RADIATED;
                            }
                        }
                    }
                    if (0
                      || (best_colonize == 200)
                      || (p->owner != PLAYER_NONE)
                      || (p->type == PLANET_TYPE_NOT_HABITABLE)
                      || (p->type < best_colonize)
                    ) {
                        best_colonize = 0;
                    }
                    was_explored = BOOLVEC_IS1(p->explored, i);
                    BOOLVEC_SET1(p->explored, i);
                    if (IS_HUMAN(g, i)) {
                        if ((best_colonize != 0) || (!was_explored)) {
                            game_update_visibility(g);  /* from explore_draw_cb */
                            if ((p->type < best_colonize) || (best_colonize == 0)) {
                                flag_colony_ship = false;
                            } else {
                                flag_colony_ship = true;
                            }
                            if (ui_explore(g, i, pli, by_scanner, flag_colony_ship) && flag_colony_ship) {
                                p->owner = i;
                                p->pop = 2;
                                --e->orbit[pli].ships[best_colonyship];
                            }
                        }
                    } else {
                        if (best_colonize != 0) {
                            p->owner = i;
                            p->pop = 2;
                            --e->orbit[pli].ships[best_colonyship];
                        }
                    }
                }
            }
        }
    }
}

static void game_turn_bomb_damage(struct game_s *g, uint8_t pli, player_id_t attacker, int *popdmgptr, int *factdmgptr, int *biodmgptr)
{
    planet_t *p = &(g->planet[pli]);
    const empiretechorbit_t *ea = &(g->eto[attacker]);
    const empiretechorbit_t *ed = &(g->eto[p->owner]);
    uint32_t tbl[WEAPON_NUM];
    uint8_t pshield = p->shield, antidote = ed->antidote;
    int totaldmg = 0, totalbio = 0, maxcomp = 0, complevel;
    memset(tbl, 0, sizeof(tbl));
    for (int i = 0; i < ea->shipdesigns_num; ++i) {
        const shipdesign_t *sd = &(g->srd[attacker].design[i]);
        if (!game_fix_orbital_comp || (ea->orbit[pli].ships[i] != 0)) {
            SETMAX(maxcomp, sd->comp);
        }
        for (int j = 0; j < (game_fix_orbital_weap_4 ? WEAPON_SLOT_NUM : (WEAPON_SLOT_NUM - 1)); ++j) {
            weapon_t wpnt = sd->wpnt[j];
            uint32_t v;
            v = ea->orbit[pli].ships[i] * sd->wpnn[j];
            if (v != 0) {
                int ns;
                ns = tbl_shiptech_weap[wpnt].numshots;
                if (ns == -1) {
                    ns = 30;
                }
                v *= ns;
                tbl[wpnt] += v;
            }
        }
    }
    complevel = (maxcomp - 1) * 2 + 6;
    SETRANGE(complevel, 1, 20);
    for (int i = 0; i < (game_fix_orbital_weap_any ? WEAPON_NUM : WEAPON_CRYSTAL_RAY); ++i) {
        uint32_t vcur = tbl[i];
        if (vcur != 0) {
            const struct shiptech_weap_s *w = &(tbl_shiptech_weap[i]);
            uint32_t v;
            int dmgmin, dmgmax;
            SETMIN(vcur, game_num_max_bomb_dmg);
            if (vcur < 10) {
                v = 1;
            } else {
                v = vcur / 10;
                vcur = 10;
            }
            dmgmin = w->damagemin;
            dmgmax = w->damagemax;
            if (dmgmin == dmgmax) {
                if (w->is_bio) {
                    for (uint32_t n = 0; n < vcur; ++n) {
                        if (rnd_1_n(20, &g->seed) <= complevel) {
                            int dmg;
                            dmg = rnd_0_nm1(dmgmin + 1, &g->seed) - antidote;
                            SETMAX(dmg, 0);
                            totalbio += v * dmg;
                            SETMIN(totalbio, game_num_max_bio_dmg);
                        }
                    }
                } else {
                    if (!game_fix_orbital_torpedo == (w->misstype == 0)) {
                        dmgmax /= 2;
                    }
                    dmgmax *= w->nummiss;
                    if (dmgmax > pshield) {
                        for (uint32_t n = 0; n < vcur; ++n) {
                            if (rnd_1_n(20, &g->seed) <= complevel) {
                                int dmg;
                                dmg = dmgmax - pshield;
                                totaldmg += v * dmg;
                            }
                        }
                    }
                }
            } else {
                /*dd3d*/
                if (!w->is_bomb) {
                    dmgmin /= 2;
                    dmgmax /= 2;
                }
                if (dmgmax > pshield) {
                    int dmgrange;
                    dmgrange = dmgmax - dmgmin + 1;
                    dmgmin = dmgmin - 1 - pshield;
                    vcur = (complevel * vcur * v) / 20;
                    for (uint32_t n = 0; n < vcur; ++n) {
                        int dmg;
                        dmg = rnd_1_n(dmgrange, &g->seed) + dmgmin;
                        if (dmg > 0) {
                            totaldmg += dmg;
                        }
                    }
                }
            }
        }
    }
    /*de00*/
    {
        int v, h, hr, hb;
        h = game_num_fact_hp / 5;
        hr = h * 2;
        hb = game_num_fact_hp - h;
        v = totaldmg / (rnd_1_n(hr, &g->seed) + hb);
        SETMIN(v, p->factories);
        *factdmgptr = v;
    }
    {
        int v, h, hr, hb;
        h = game_num_pop_hp / 5;
        hr = h * 2;
        hb = game_num_pop_hp - h;
        v = totaldmg / (rnd_1_n(hr, &g->seed) + hb);
        v += totalbio;
        SETMIN(v, p->pop);
        *popdmgptr = v;
    }
    {
        int v;
        v = p->max_pop3 - totalbio;
        SETMAX(v, 10);
        p->max_pop3 = v;    /* BUG reduced before y/n */
        *biodmgptr = totalbio;
    }
}

static void game_turn_bomb(struct game_s *g)
{
    for (int pli = 0; pli < g->galaxy_stars; ++pli) {
        planet_t *p = &(g->planet[pli]);
        player_id_t owner;
        owner = p->owner;
        if (owner == PLAYER_NONE) {
            continue;
        }
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            empiretechorbit_t *ea = &(g->eto[i]);
            bool flag_treaty;
            int v4;
            if (i == owner) {
                continue;
            }
            v4 = 0;
            for (int j = 0; j < ea->shipdesigns_num; ++j) {
                if (ea->orbit[pli].ships[j] > 0) {
                    v4 += 2;
                }
            }
            flag_treaty = false;
            if ((ea->treaty[owner] == TREATY_ALLIANCE) || ((ea->treaty[owner] == TREATY_NONAGGRESSION) && IS_AI(g, i))) {
                flag_treaty = true;
            }
            if ((v4 > 0) && (!flag_treaty)) {
                int pop_inbound, popdmg, factdmg, biodmg;
                bool flag_do_bomb, flag_play_music;
                pop_inbound = 0;
                for (int j = 0; j < g->transport_num; ++j) {
                    transport_t *r = &(g->transport[j]);
                    if ((r->owner == i) && (r->dest == pli)) {
                        pop_inbound += r->pop;
                    }
                }
                /*cf52*/
                flag_play_music = true;
                game_turn_bomb_damage(g, pli, i, &popdmg, &factdmg, &biodmg);
                if ((popdmg == 0) && (factdmg == 0)) {
                    flag_do_bomb = false;
                } else if (IS_HUMAN(g, i)) {
                    flag_do_bomb = ui_bomb_ask(g, i, pli, pop_inbound);
                    flag_play_music = false;
                } else {
                    flag_do_bomb = game_ai->bomb(g, i, pli, pop_inbound);
                }
                /*d004*/
                if (flag_do_bomb && ((popdmg > 0) || (factdmg > 0))) { /* FIXME biodmg? */
                    p->pop -= popdmg;
                    p->factories -= factdmg;
                    SUBSAT0(p->rebels, popdmg / 2 + 1);
                    if (p->pop == 0) {
                        game_planet_destroy(g, pli, i);
                    }
                    if (IS_HUMAN(g, i) || IS_HUMAN(g, owner)) {
                        ui_bomb_show(g, i, owner, pli, popdmg, factdmg, flag_play_music);
                    }
                    if ((p->pop == 0) && IS_HUMAN(g, i)) {
                        int dv;
                        if (ea->treaty[owner] < TREATY_WAR) {
                            game_diplo_start_war(g, owner, i);
                            dv = 13;
                        } else {
                            dv = 10;
                        }
                        game_diplo_act(g, -50 - rnd_1_n(50, &g->seed), i, owner, dv, pli, 0);
                    } else {
                        /*d118*/
                        game_diplo_battle_finish(g, owner, i, popdmg, 0, biodmg, 0, pli);
                    }
                }
            }
        }
    }
}

static int game_turn_transport_shoot(struct game_s *g, uint8_t planet_i, player_id_t rowner, uint8_t speed, player_id_t attacker, int bases, weapon_t basewpnt)
{
    const planet_t *p = &(g->planet[planet_i]);
    const empiretechorbit_t *ea = &(g->eto[attacker]);
    const empiretechorbit_t *ed = &(g->eto[rowner]);
    int totaldmg = 0, complevel, killed;
    uint8_t bestcomp = 0, bestarmor = 0;
    uint32_t tbl[WEAPON_NUM];
    memset(tbl, 0, sizeof(tbl));
    tbl[basewpnt] = bases * 24;
    for (int i = 0; i < ea->shipdesigns_num; ++i) {
        const shipdesign_t *sd = &(g->srd[attacker].design[i]);
        uint8_t comp;
        comp = sd->comp;
        if (comp > speed) { /* FIXME BUG ? */
            bestcomp = comp;
        }
        for (int j = 0; j < WEAPON_SLOT_NUM; ++j) {
            weapon_t wpnt = sd->wpnt[j];
            uint32_t v;
            v = ea->orbit[planet_i].ships[i] * sd->wpnn[j];
            if (v != 0) {
                const struct shiptech_weap_s *w;
                int ns;
                w = &(tbl_shiptech_weap[wpnt]);
                ns = w->numshots;
                if (ns == -1) {
                    ns = 4;
                }
                v *= ns;
                if (w->nummiss != 0) {
                    v *= w->nummiss;
                }
                SETMIN(v, game_num_max_trans_dmg);
                if (w->is_bomb || w->is_bio) {
                    v = 0;
                }
                if (w->misstype != 0) {
                    v /= 2;
                }
                tbl[wpnt] += (v * 2) / speed;
            }
        }
    }
    if (bases > 0) {
        bestcomp = game_get_best_comp(g, rowner, ed->tech.percent[TECH_FIELD_COMPUTER]);  /* FIXME should be ea ? */
    }
    complevel = (bestcomp - speed) * 2 + 12;
    SETRANGE(complevel, 1, 20);
    for (int i = 0; i < (game_fix_orbital_weap_any ? WEAPON_NUM : WEAPON_CRYSTAL_RAY); ++i) {
        uint32_t vcur = tbl[i];
        if (vcur != 0) {
            const struct shiptech_weap_s *w = &(tbl_shiptech_weap[i]);
            int dmgmin, dmgmax;
            dmgmin = w->damagemin;
            dmgmax = w->damagemax;
            if (dmgmin == dmgmax) {
                for (uint32_t n = 0; n < vcur; ++n) {
                    if (rnd_1_n(20, &g->seed) <= complevel) {
                        totaldmg += dmgmax;
                    }
                }
            } else {
                /*7c47d*/
                int dmgrange;
                dmgrange = dmgmax - dmgmin + 1;
                --dmgmin;
                vcur = (complevel * vcur) / 20;
                for (uint32_t n = 0; n < vcur; ++n) {
                    int dmg;
                    dmg = rnd_1_n(dmgrange, &g->seed) + dmgmin;
                    if (dmg > 0) {
                        totaldmg += dmg;
                    }
                }
            }
        }
    }
    /*7c521*/
    {
        const uint8_t *rc = &(g->srd[rowner].researchcompleted[TECH_FIELD_CONSTRUCTION][0]);
        for (int i = 0; i < ed->tech.completed[TECH_FIELD_CONSTRUCTION]; ++i) {
            uint8_t tier;
            tech_group_t group;
            group = game_tech_get_group(g->gaux, TECH_FIELD_CONSTRUCTION, rc[i]);
            tier = game_tech_get_tier(g->gaux, TECH_FIELD_CONSTRUCTION, rc[i]);
            if (group == TECH_GROUP_ARMOR) {
                bestarmor = tier;
            }
        }
    }
    killed = totaldmg / ((bestarmor + 1) * 15);
    SETMIN(killed, 1000);
    for (monster_id_t i = MONSTER_CRYSTAL; i <= MONSTER_AMOEBA; ++i) {
        monster_t *m;
        m = (i == MONSTER_CRYSTAL) ? &(g->evn.crystal) : &(g->evn.amoeba);
        if (m->exists && (m->killer == PLAYER_NONE) && (m->x == p->x) && (m->y == p->y)){
            killed = 1000;
        }
    }
    return killed;
}

static void game_turn_transport(struct game_s *g)
{
    char buf[0x80];
    for (int pli = 0; pli < g->galaxy_stars; ++pli) {
        planet_t *p = &(g->planet[pli]);
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            p->inbound[i] = 0;
            p->total_inbound[i] = 0;
        }
    }
    for (int i = 0; i < g->transport_num; ++i) {
        transport_t *r = &(g->transport[i]);
        planet_t *p;
        uint8_t dest;
        dest = r->dest;
        p = &(g->planet[dest]);
        if ((r->x == p->x) && (r->y == p->y)) {
            int pop2, pop3;
            player_id_t owner;
            owner = r->owner;
            pop2 = pop3 = r->pop;
            for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                treaty_t t;
                if (j == owner) {
                    continue;
                }
                t = g->eto[owner].treaty[j];
                if ((j == p->owner) || (t == TREATY_NONE) || (t >= TREATY_WAR)) {
                    empiretechorbit_t *e;
                    e = &(g->eto[j]);
                    if (j == p->owner) {
                        pop3 -= game_turn_transport_shoot(g, dest, owner, r->speed, j, p->missile_bases, e->base_weapon);
                    } else {
                        /*e102*/
                        bool any_ships;
                        any_ships = false;
                        for (int k = 0; k < e->shipdesigns_num; ++k) {
                            if (e->orbit[dest].ships[k] > 0) {
                                any_ships = true;
                                break;
                            }
                        }
                        if (any_ships) {
                            pop3 -= game_turn_transport_shoot(g, dest, owner, r->speed, j, 0, WEAPON_NONE);
                        }
                    }
                }
            }
            if (g->evn.have_guardian && (dest == g->evn.planet_orion_i)) {
                pop3 = 0;
            }
            for (monster_id_t j = MONSTER_CRYSTAL; j <= MONSTER_AMOEBA; ++j) {
                monster_t *m;
                m = (j == MONSTER_CRYSTAL) ? &(g->evn.crystal) : &(g->evn.amoeba);
                if (m->exists && /*(m->killer == PLAYER_NONE) &&*/ (m->x == p->x) && (m->y == p->y)) { /* FIXME dead monster kills transports ? */
                    pop3 = 0;
                }
            }
            SETMAX(pop3, 0);
            if (g->eto[owner].have_combat_transporter && (!g->eto[p->owner].have_sub_space_int)) {
                int n;
                n = pop2 - pop3;
                for (int j = 0; j < n; ++j) {
                    if (!rnd_0_nm1(4, &g->seed)) {
                        ++pop3;
                    } else if (!rnd_0_nm1(2, &g->seed)) {
                        ++pop3;
                    }
                }
            }
            /*e2a4*/
            if (pop3 <= 0) {
                if (IS_HUMAN(g, owner) || IS_HUMAN(g, p->owner)) {
                    const char *s;
                    if ((g->gaux->local_players == 1) && IS_HUMAN(g, owner)) {
                        s = game_str_sb_your;
                    } else {
                        s = game_str_tbl_race[g->eto[owner].race];
                    }
                    sprintf(buf, "%s %s %s %s", s, game_str_sm_traad1, g->planet[dest].name, game_str_sm_traad2);
                    ui_turn_msg(g, IS_HUMAN(g, owner) ? owner : p->owner, buf);
                }
            } else if (p->owner == PLAYER_NONE) {
                /*e36d*/
                if (IS_HUMAN(g, owner)) {
                    sprintf(buf, "%s %s %s", game_str_sm_trbdb1, g->planet[dest].name, game_str_sm_trbdb2);
                    ui_turn_msg(g, owner, buf);
                }
            } else {
                /*e3fe*/
                if ((p->owner == PLAYER_NONE) || (p->pop == 0)) {
                    if (IS_HUMAN(g, owner)) {
                        ui_landing(g, owner, dest);
                    }
                    p->pop = pop3;
                    p->bc_to_refit = 0;
                    p->pop_oper_fact = 1;
                    if (dest == g->evn.planet_orion_i) {
                        g->evn.have_orion_conquer = owner + 1;
                    }
                    p->owner = owner;
                } else if (p->owner == owner) {
                    if (p->unrest == PLANET_UNREST_REBELLION) {
                        ADDSATT(p->inbound[owner], pop3, game_num_max_inbound);
                        p->total_inbound[owner] += pop3;
                    } else {
                        ADDSATT(p->pop, pop3, p->max_pop3);
                    }
                } else {
                    /*e5a6*/
                    if (g->eto[owner].treaty[p->owner] != TREATY_ALLIANCE) {
                        ADDSATT(p->inbound[owner], pop3, game_num_max_inbound);
                        p->total_inbound[owner] += pop2;
                    }
                }
            }
            /*e639*/
            util_table_remove_item_any_order(i, g->transport, sizeof(transport_t), g->transport_num);
            --g->transport_num;
            --i;
        }
    }
}

static void game_turn_coup(struct game_s *g)
{
    uint8_t tbl_planets[PLAYER_NUM];
    uint8_t tbl_rebelplanets[PLAYER_NUM];
    memset(tbl_planets, 0, sizeof(tbl_planets));
    memset(tbl_rebelplanets, 0, sizeof(tbl_rebelplanets));
    g->evn.coup = PLAYER_NONE;
    for (int pli = 0; pli < g->galaxy_stars; ++pli) {
        planet_t *p = &(g->planet[pli]);
        if (p->owner != PLAYER_NONE) {
            ++tbl_planets[p->owner];
            if (p->unrest == PLANET_UNREST_REBELLION) {
                ++tbl_rebelplanets[p->owner];
            }
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if ((tbl_rebelplanets[i] >= ((tbl_planets[i] + 1) / 2)) && IS_ALIVE(g, i)) {
            empiretechorbit_t *e = &(g->eto[i]);
            e->trait2 = rnd_0_nm1(TRAIT2_NUM, &g->seed);
            e->trait1 = rnd_0_nm1(TRAIT1_NUM, &g->seed);
            /* WASBUG MOO1 reads an unitialized variable at BP-2 and check for it to be 0 or 1.
               It is the return address to the caller which is neither of the values, leading to the code below. */
            for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                int16_t rel;
                rel = e->relation2[j];
                e->relation1[j] = rel;
                if (i != j) {
                    g->eto[j].relation1[i] = rel;
                }
            }
            game_diplo_break_treaty(g, i, PLAYER_0); /* FIXME multiplayer */
            game_diplo_break_trade(g, i, PLAYER_0); /* FIXME multiplayer */
            if (g->evn.coup == PLAYER_NONE) {
                g->evn.coup = i;
            }
            for (int pli = 0; pli < g->galaxy_stars; ++pli) {
                planet_t *p = &(g->planet[pli]);
                if (p->owner == i) {
                    p->unrest = PLANET_UNREST_NORMAL;
                    p->rebels = 0;
                }
            }
        }
    }
}

static bool game_turn_check_end(struct game_s *g, struct game_end_s *ge)
{
    if (g->end == GAME_END_LOST_EXILE) {
        ge->type = GAME_END_LOST_EXILE;
        ge->name = g->emperor_names[PLAYER_0]; /* FIXME multiplayer */
        return true;
    }
    if (g->end == GAME_END_WON_GOOD) {
        ge->type = GAME_END_WON_GOOD;
        ge->name = g->emperor_names[PLAYER_0]; /* FIXME multiplayer */
        ge->race = g->eto[PLAYER_0].race; /* FIXME multiplayer */
        return true;
    }
    {
        player_id_t pi1 = PLAYER_NONE, pi2 = PLAYER_NONE, pih = PLAYER_NONE;
        bool human_alive = false;
        uint8_t num_planets[PLAYER_NUM];
        uint8_t good_planets[PLAYER_NUM];
        memset(num_planets, 0, sizeof(num_planets));
        memset(good_planets, 0, sizeof(num_planets));
        for (int pli = 0; pli < g->galaxy_stars; ++pli) {
            const planet_t *p = &(g->planet[pli]);
            if (p->owner != PLAYER_NONE) {
                ++num_planets[p->owner];
                if (p->unrest != PLANET_UNREST_REBELLION) {
                    ++good_planets[p->owner];
                }
            }
        }
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            if (num_planets[i] > 0) {
                if (pi1 == PLAYER_NONE) {
                    pi1 = i;
                } else if (pi2 == PLAYER_NONE) {
                    pi2 = i;
                }
                if (IS_HUMAN(g, i)) {
                    pih = i;
                    if (good_planets[i] > 0) {
                        human_alive = true;
                    }
                }
            }
        }
        if (pi2 == PLAYER_NONE) {
            ge->type = GAME_END_WON_TYRANT;
            ge->name = g->emperor_names[pi1];
            ge->race = g->eto[pi1].race;
            return true;
        }
        if (!human_alive) {
            struct news_s ns;
            int killer = g->gaux->human_killer;
            if (killer >= g->players) {
                killer = (pi1 != PLAYER_NONE) ? pi1 : 1;
            }
            ns.type = GAME_NEWS_GENOCIDE;
            ns.race = g->eto[PLAYER_0].race;    /* FIXME multiplayer */
            ns.subtype = 3;
            ns.num1 = 0;
            ui_news_start();
            ui_news(g, &ns);
            ge->type = GAME_END_LOST_FUNERAL;
            ge->race = g->eto[killer].banner;
            ge->banner_dead = pih;
            return true;
        }
    }
    return false;
}

static void game_turn_update_have_met(struct game_s *g)
{
    game_update_empire_contact(g);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        if (!IS_HUMAN(g, i)) {
            continue;
        }
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            if ((i != j) && (!e->have_met[j]) && BOOLVEC_IS1(e->within_frange, j)) {
                e->have_met[j] = 1;
                e->treaty[j] = TREATY_NONE;
                g->eto[j].treaty[i] = TREATY_NONE;
            }
        }
    }
}

static void game_turn_audiences(struct game_s *g)
{
    for (player_id_t ph = PLAYER_0; ph < PLAYER_NUM; ++ph) {
        empiretechorbit_t *e = &(g->eto[ph]);
        if (!IS_HUMAN(g, ph)) {
            continue;
        }
        g->evn.newtech[ph].num = 0;
        for (player_id_t pa = PLAYER_0; pa < PLAYER_NUM; ++pa) {
            if (IS_AI(g, pa) && (e->diplo_type[pa] != 0)) {
                game_audience(g, ph, pa);
                e->diplo_type[pa] = 0;
            }
        }
        ui_newtech(g, ph);
    }
}

static void game_turn_contact_broken(struct game_s *g, player_id_t pi, const BOOLVEC_PTRPARAMI(bv))
{
    empiretechorbit_t *e = &(g->eto[pi]);
    if (!IS_ALIVE(g, pi)) {
        return;
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if ((i != pi) && IS_ALIVE(g, i) && BOOLVEC_IS0(e->within_frange, i) && BOOLVEC_IS1(bv, i)) {
            empiretechorbit_t *e2 = &(g->eto[i]);
            e->mood_trade[i] = 0;
            e->trade_bc[i] = 0;
            e->trade_percent[i] = 0;
            e->spymode[i] = SPYMODE_HIDE;
            e2->mood_trade[pi] = 0;
            e2->trade_bc[pi] = 0;
            e2->trade_percent[pi] = 0;
            e2->spymode[pi] = SPYMODE_HIDE;
            if (IS_HUMAN(g, pi)) {
                char buf[0x80];
                /* TODO ui_sound_music_stop(); */
                sprintf(buf, "%s %s %s", game_str_tr_cont1, game_str_tbl_race[e2->race], game_str_tr_cont2);
                ui_turn_msg(g, pi, buf);
            }
        }
    }
}

static void game_turn_update_seen(struct game_s *g)
{
    game_update_visibility(g);
    for (uint8_t i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
            if (BOOLVEC_IS1(p->within_srange, pi)) {
                g->seen[pi][i].owner = p->owner;
                g->seen[pi][i].pop = p->pop;
                g->seen[pi][i].bases = p->missile_bases;
                g->seen[pi][i].factories = p->factories;
            } else if ((p->owner != PLAYER_NONE) && BOOLVEC_IS1(g->eto[pi].within_frange, p->owner)) {
                g->seen[pi][i].owner = p->owner;
            }
        }
    }
}

static void game_turn_show_newships(struct game_s *g)
{
    /*temp_turn_hmm3 = 0;*/
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        if (IS_HUMAN(g, pi)) {
            bool any_new;
            any_new = false;
            for (int si = 0; si < NUM_SHIPDESIGNS; ++si) {
                if (g->evn.new_ships[pi][si] != 0) {
                    any_new = true;
                    break;
                }
            }
            if (any_new) {
                ui_newships(g, pi);
            }
        }
    }
}

static void game_turn_finished_slider(struct game_s *g)
{
    for (uint8_t pli = 0; pli < g->galaxy_stars; ++pli) {
        planet_t *p = &(g->planet[pli]);
        player_id_t pi;
        empiretechorbit_t *e;
        pi = p->owner;
        if ((pi == PLAYER_NONE) || IS_AI(g, pi)) {
            BOOLVEC_CLEAR(p->finished, FINISHED_NUM);
            continue;
        }
        e = &(g->eto[pi]);
        if (BOOLVEC_IS1(p->finished, FINISHED_SOILATMOS)) {
            int v;
            v = game_planet_get_w1(g, pli);
            if (p->factories >= (p->pop * e->colonist_oper_factories)) {
                p->slider[PLANET_SLIDER_TECH] += p->slider[PLANET_SLIDER_ECO] - v;
            } else {
                p->slider[PLANET_SLIDER_IND] += p->slider[PLANET_SLIDER_ECO] - v;
            }
            p->slider[PLANET_SLIDER_ECO] = v;
            game_add_planet_to_build_finished(g, pli, pi, FINISHED_SOILATMOS);
        }
        if (1
          && (p->owner == pi)
          && (p->pop >= p->max_pop3)
          && (p->unrest == PLANET_UNREST_NORMAL)
        ) {
            bool flag_pending_ecoproj;
            int v;
            v = (p->prod_after_maint * p->slider[PLANET_SLIDER_ECO]) / 100;
            flag_pending_ecoproj = false;
            if (v > 0) {
                if (0
                  || (e->have_atmos_terra && (p->growth == PLANET_GROWTH_HOSTILE))
                  || (e->have_soil_enrich && (p->growth == PLANET_GROWTH_NORMAL))
                  || (e->have_adv_soil_enrich && ((p->growth == PLANET_GROWTH_NORMAL) || (p->growth == PLANET_GROWTH_FERTILE)))
                  || ((p->max_pop3 - p->max_pop2) < e->have_terraform_n)
                ) {
                    flag_pending_ecoproj = true;
                }
            }
            if (!flag_pending_ecoproj) {
                int w, fact, waste, prod;
                fact = p->factories;
                SETMIN(fact, p->pop * e->colonist_oper_factories);
                waste = (e->race == RACE_SILICOID) ? 0 : (((fact * e->ind_waste_scale) / 10 + p->waste) / e->have_eco_restoration_n);
                prod = p->prod_after_maint;
                if (prod == 0) {
                    prod = 1000;
                }
                w = ((waste * 100) + prod - 1) / prod;
                SETRANGE(w, 0, 100);
                SUBSAT0(v, waste);
                if (0
                  || ((game_get_pop_growth_for_eco(g, pli, v) / 10) > 0)
                  || ((p->slider[PLANET_SLIDER_ECO] != 0) && (e->race == RACE_SILICOID))
                ) {
                    if ((p->pop > p->pop_prev) || (p->slider[PLANET_SLIDER_ECO] > (w + game_num_eco_slider_slack))) {
                        if (p->factories >= (p->pop * e->colonist_oper_factories)) {
                            p->slider[PLANET_SLIDER_TECH] += p->slider[PLANET_SLIDER_ECO] - w;
                        } else {
                            p->slider[PLANET_SLIDER_IND] += p->slider[PLANET_SLIDER_ECO] - w;
                        }
                        p->slider[PLANET_SLIDER_ECO] = w;
                        game_add_planet_to_build_finished(g, pli, pi, FINISHED_POPMAX);
                    }
                }
            }
        }
        if (BOOLVEC_IS1(p->finished, FINISHED_SHIELD)) {
            p->slider[PLANET_SLIDER_TECH] += p->slider[PLANET_SLIDER_DEF];
            p->slider[PLANET_SLIDER_DEF] = 0;
            game_add_planet_to_build_finished(g, pli, pi, FINISHED_SHIELD);
        }
        if (BOOLVEC_IS1(p->finished, FINISHED_STARGATE)) {
            /* WASBUG MOO1 has DEF, not SHIP */
            p->slider[PLANET_SLIDER_TECH] += p->slider[PLANET_SLIDER_SHIP];
            p->slider[PLANET_SLIDER_SHIP] = 0;
            game_add_planet_to_build_finished(g, pli, pi, FINISHED_STARGATE);
        }
    }
}

static void game_turn_claim(struct game_s *g)
{
    for (uint8_t pli = 0; pli < g->galaxy_stars; ++pli) {
        planet_t *p = &(g->planet[pli]);
        player_id_t pi;
        pi = p->owner;
        p->claim = pi;
        if (pi == PLAYER_NONE) {
            for (pi = PLAYER_0; pi < g->players; ++pi) {
                empiretechorbit_t *e = &(g->eto[pi]);
                for (int i = 0; i < e->shipdesigns_num; ++i) {
                    if (e->orbit[pli].ships[i] != 0) {
                        p->claim = pi;
                        break;
                    }
                }
            }
        }
    }
}

static void game_turn_update_final_war(struct game_s *g)
{
    if (g->end != GAME_END_FINAL_WAR) {
        return;
    }
    /* FIXME multiplayer */
    game_tech_final_war_share(g);
    for (player_id_t pi1 = PLAYER_0; pi1 < g->players; ++pi1) {
        empiretechorbit_t *e1 = &(g->eto[pi1]);
        for (player_id_t pi2 = pi1 + 1; pi2 < g->players; ++pi2) {
            empiretechorbit_t *e2 = &(g->eto[pi2]);
            if (IS_AI(g, pi1) && IS_AI(g, pi2)) {
                e1->treaty[pi2] = TREATY_ALLIANCE;
                e2->treaty[pi1] = TREATY_ALLIANCE;
                e1->relation1[pi2] = 100;
                e2->relation1[pi1] = 100;
            } else {
                e1->treaty[pi2] = TREATY_FINAL_WAR;
                e2->treaty[pi1] = TREATY_FINAL_WAR;
                e1->relation1[pi2] = -100;
                e2->relation1[pi1] = -100;
            }
        }
    }
}

/* -------------------------------------------------------------------------- */

struct game_end_s game_turn_process(struct game_s *g)
{
    struct game_end_s game_end;
    BOOLVEC_TBL_DECLARE(old_contact, PLAYER_NUM, PLAYER_NUM);
    uint8_t old_focus[PLAYER_NUM];
    int num_alive = 0, num_colony = 0;
    uint8_t artifact_planet = PLANET_NONE;
    player_id_t artifact_player = PLAYER_NONE;
    game_end.type = GAME_END_NONE;
    game_turn_limit_ships(g);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        BOOLVEC_TBL_COPY1(old_contact, g->eto[i].within_frange, i, PLAYER_NUM);
        old_focus[i] = g->planet_focus_i[i];
    }
    if ((g->year > 40) && (!rnd_0_nm1(30, &g->seed)) && (copyprot_status == 0)) {
        copyprot_status = 1;
    }
    ui_turn_pre(g);
    game_turn_countdown_ceasefire(g);
    /* temp_turn_hmm3 = 0; */
    game_turn_update_mood_blunder(g);
    game_update_have_reserve_fuel(g);
    game_ai->turn_p1(g);
    game_ai->turn_p2(g);
    game_update_have_reserve_fuel(g);
    game_ai->turn_p3(g);
    game_turn_init_z_finished(g);
    game_turn_send_transport(g);
    game_update_production(g);
    game_turn_build_eco(g);
    game_update_total_research(g);
    game_update_have_reserve_fuel(g);
    game_turn_update_trade(g);
    game_spy_build(g);
    game_update_production(g);
    game_turn_diplo_adjust(g);
    game_turn_build_def(g);
    game_turn_build_ship(g);
    game_turn_reserve(g);
    game_turn_build_ind(g);
    game_turn_move_ships(g);
    game_turn_limit_ships(g);
    game_remove_empty_fleets(g);
    game_spy_report(g);
    game_battle_handle_all(g);
    /*game_turn_unrest_hmm1(g);*/
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        g->evn.newtech[i].num = 0;
    }
    {
        struct spy_turn_s st[1];
        game_spy_turn(g, st);
        game_spy_esp_human(g, st);
        game_spy_sab_human(g);
    }
    /*ui_newtech(); FIXME redundant;*/
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        g->evn.newtech[i].num = 0;
    }
    /*935f*/
    game_tech_research(g);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if (IS_HUMAN(g, i)) {
            ui_newtech(g, i);
        }
    }
    /* FIXME useless? game_battle should have taken care of this */
    if (g->evn.have_guardian) {
        uint8_t o = g->evn.planet_orion_i;
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            shipcount_t *s = &(g->eto[i].orbit[o].ships[0]);
            memset(s, 0, sizeof(*s));
        }
    }
    game_turn_explore(g, &artifact_planet, &artifact_player);
    game_turn_bomb(g);
    game_turn_transport(g);
    game_turn_ground(g);
    game_turn_coup(g);
    if (game_turn_check_end(g, &game_end)) {
        return game_end;
    }
    game_event_new(g);
    if (game_event_run(g, &game_end)) {
        return game_end;
    }
    if (artifact_planet != PLANET_NONE) {
        game_tech_get_artifact_loot(g, artifact_planet, artifact_player);
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if (IS_HUMAN(g, i)) {
            ui_newtech(g, i);
        }
        g->evn.newtech[i].num = 0;
    }
    game_turn_update_have_met(g);
    game_ai->turn_diplo_p1(g);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        if (g->planet[i].owner != PLAYER_NONE) {
            ++num_colony;
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if (IS_ALIVE(g, i)) {
            ++num_alive;
        }
    }
    if (1
      && (game_num_council_years != 0)
      && (((g->year % game_num_council_years) == 0) || (!g->election_held))
      && (num_alive > 2)
      && (((g->galaxy_stars * 2) / 3) <= num_colony)
      && (g->end == GAME_END_NONE)
    ) {
        game_election(g);
        g->election_held = true;
    }
    if (game_turn_check_end(g, &game_end)) {
        return game_end;
    }
    game_ai->turn_diplo_p2(g);
    game_turn_audiences(g);
    if (game_turn_check_end(g, &game_end)) {
        return game_end;
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        /* TODO set flag in g->eto[i] to delay to player turn? */
        game_turn_contact_broken(g, i, BOOLVEC_TBL_PTRPARAMM(old_contact, i));
    }
    game_fleet_unrefuel(g);
    game_update_production(g);
    if (copyprot_status == 1) {
        ui_copyprotection_check(g);
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        game_update_eco_on_waste(g, i, false);
    }
    game_turn_update_seen(g);
    game_diplo_mood_relax(g);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        g->planet_focus_i[i] = old_focus[i];
    }
    ++g->year;
    if (copyprot_status == 1) {
        ui_copyprotection_lose(g, &game_end);
        return game_end;
    }
    game_turn_show_newships(g);
    /* TODO autosave */
    game_update_tech_util(g);
    /* if (temp_turn_hmm3 != 0) { ui_palette_fadeout_a_f_1(); ui_draw_finish_mode = 2; }*/
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        for (tech_field_t f = 0; f < TECH_FIELD_NUM; ++f) {
            e->tech.percent[f] = game_tech_get_field_percent(g, i, f);
        }
    }
    game_turn_finished_slider(g);
    game_turn_claim(g);
    game_update_within_range(g);
    game_update_visibility(g);
    game_turn_update_final_war(g);
    game_remove_empty_fleets(g);
    game_planet_update_home(g);
    return game_end;
}
